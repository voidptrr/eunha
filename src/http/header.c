/*
 * MIT License
 *
 * Copyright (c) 2026 Tommaso Bruno
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "datastruct/vector.h"
#include "http/header.h"

#define HEADERS_DEFAULT_CAPACITY 8

struct header_entry {
    uint64_t hash;
    struct string name;
    struct vector values;
    bool occupied;
};

/* Returns the ASCII lowercase form used by hashing and equality. */
static uint8_t ascii_lower(uint8_t byte) {
    if (byte >= 'A' && byte <= 'Z') {
        return (uint8_t)(byte + ('a' - 'A'));
    }

    return byte;
}

/* Hashes field names with ASCII case ignored. */
static uint64_t header_name_hash(const char* name, size_t length) {
    uint64_t hash = UINT64_C(14695981039346656037);

    for (size_t index = 0; index < length; index += 1) {
        hash ^= ascii_lower((uint8_t)name[index]);
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

/* Compares an owned field name with another byte range ignoring ASCII case. */
static bool header_name_equals(
    const struct string* stored, const char* name, size_t length) {
    if (stored->length != length) {
        return false;
    }

    for (size_t index = 0; index < length; index += 1) {
        if (ascii_lower((uint8_t)stored->data[index]) !=
            ascii_lower((uint8_t)name[index])) {
            return false;
        }
    }

    return true;
}

/* Returns whether every field-name byte belongs to the HTTP token grammar. */
static bool header_name_is_valid(const struct string* name) {
    for (size_t index = 0; index < name->length; index += 1) {
        uint8_t byte = (uint8_t)name->data[index];

        if ((byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'Z') ||
            (byte >= 'a' && byte <= 'z')) {
            continue;
        }

        /* These punctuation bytes complete HTTP's permitted token alphabet. */
        switch (byte) {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
            break;
        default:
            return false;
        }
    }

    return name->length > 0;
}

/* Rejects control bytes that are not permitted inside a field value. */
static bool header_value_is_valid(const struct string* value) {
    for (size_t index = 0; index < value->length; index += 1) {
        uint8_t byte = (uint8_t)value->data[index];

        /* HTAB is allowed, but other C0 controls and DEL are not. */
        if ((byte < 0x20 && byte != '\t') || byte == 0x7F) {
            return false;
        }
    }

    return true;
}

/* Releases the name and every value owned by one occupied entry. */
static void header_entry_deinit(struct header_entry* entry) {
    for (size_t index = 0; index < vector_len(&entry->values); index += 1) {
        struct string* value = vector_get(&entry->values, index);
        string_deinit(value);
    }

    vector_deinit(&entry->values);
    string_deinit(&entry->name);
    *entry = (struct header_entry){0};
}

/* Finds either the matching entry or the first empty probe position. */
static size_t headers_find_index(const struct headers* headers,
    const char* name, size_t length, uint64_t hash, bool* found) {
    size_t index = (size_t)(hash % headers->capacity);

    for (;;) {
        const struct header_entry* entry = &headers->entries[index];
        if (!entry->occupied) {
            *found = false;
            return index;
        }

        if (entry->hash == hash &&
            header_name_equals(&entry->name, name, length)) {
            *found = true;
            return index;
        }

        index = (index + 1) % headers->capacity;
    }
}

/* Doubles the table and moves every occupied entry into its new probe chain. */
static enum eunha_result headers_grow(struct headers* headers) {
    if (headers->capacity > SIZE_MAX / 2) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    size_t next_capacity = headers->capacity * 2;
    if (next_capacity > SIZE_MAX / sizeof(struct header_entry)) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    struct header_entry* next_entries =
        malloc(next_capacity * sizeof(struct header_entry));
    if (next_entries == NULL) {
        return EUNHA_ERROR;
    }

    memset(next_entries, 0, next_capacity * sizeof(struct header_entry));

    for (size_t old_index = 0; old_index < headers->capacity; old_index += 1) {
        struct header_entry entry = headers->entries[old_index];
        if (!entry.occupied) {
            continue;
        }

        size_t next_index = (size_t)(entry.hash % next_capacity);
        while (next_entries[next_index].occupied) {
            next_index = (next_index + 1) % next_capacity;
        }

        next_entries[next_index] = entry;
    }

    free(headers->entries);
    headers->entries = next_entries;
    headers->capacity = next_capacity;
    return EUNHA_OK;
}

/* Appends one owned value and empties the caller's string after success. */
static enum eunha_result header_entry_add_value(
    struct header_entry* entry, struct string* value) {
    if (vector_append(&entry->values, value) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    *value = (struct string){0};
    return EUNHA_OK;
}

/* Inserts a new name or appends a value to its existing entry. */
static enum eunha_result headers_insert(
    struct headers* headers, struct string* name, struct string* value) {
    uint64_t hash = header_name_hash(name->data, name->length);
    bool found = false;
    size_t index =
        headers_find_index(headers, name->data, name->length, hash, &found);

    if (found) {
        return header_entry_add_value(&headers->entries[index], value);
    }

    size_t load_limit = headers->capacity - (headers->capacity / 4);
    if (headers->length + 1 > load_limit) {
        if (headers_grow(headers) == EUNHA_ERROR) {
            return EUNHA_ERROR;
        }

        index =
            headers_find_index(headers, name->data, name->length, hash, &found);
        assert(!found);
    }

    struct header_entry entry = {
        .hash = hash,
        .occupied = true,
    };
    if (vector_init(&entry.values, sizeof(struct string)) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    entry.name = *name;
    *name = (struct string){0};

    if (header_entry_add_value(&entry, value) == EUNHA_ERROR) {
        header_entry_deinit(&entry);
        return EUNHA_ERROR;
    }

    headers->entries[index] = entry;
    headers->length += 1;
    return EUNHA_OK;
}

/* Returns one case-insensitive entry, or NULL when the name is absent. */
static const struct header_entry* headers_find(
    const struct headers* headers, const char* name) {
    size_t length = strlen(name);
    uint64_t hash = header_name_hash(name, length);
    bool found = false;
    size_t index = headers_find_index(headers, name, length, hash, &found);
    return found ? &headers->entries[index] : NULL;
}

/* Initializes an empty header collection. */
enum eunha_result headers_init(struct headers* headers) {
    assert(headers != NULL);

    headers->entries =
        malloc(HEADERS_DEFAULT_CAPACITY * sizeof(struct header_entry));
    if (headers->entries == NULL) {
        return EUNHA_ERROR;
    }

    memset(headers->entries, 0,
        HEADERS_DEFAULT_CAPACITY * sizeof(struct header_entry));
    headers->length = 0;
    headers->capacity = HEADERS_DEFAULT_CAPACITY;
    return EUNHA_OK;
}

/* Parses and stores one owned name/value field. */
enum eunha_result headers_parse_line(
    struct headers* headers, const struct string* line) {
    assert(headers != NULL);
    assert(line != NULL);

    struct string_split field;
    if (string_split_once(line, ':', &field) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    /* A field needs a colon, a non-empty token name, and a safe raw value. */
    if (field.length != 2 || !header_name_is_valid(&field.values[0]) ||
        !header_value_is_valid(&field.values[1])) {
        string_split_deinit(&field);
        errno = EINVAL;
        return EUNHA_ERROR;
    }

    /* Optional whitespace is syntax around the value, not part of its data. */
    string_trim(&field.values[1]);
    if (headers_insert(headers, &field.values[0], &field.values[1]) ==
        EUNHA_ERROR) {
        string_split_deinit(&field);
        return EUNHA_ERROR;
    }

    string_split_deinit(&field);
    return EUNHA_OK;
}

/* Returns the first value for a case-insensitive field name. */
const struct string* headers_get(
    const struct headers* headers, const char* name) {
    assert(headers != NULL);
    assert(name != NULL);

    const struct header_entry* entry = headers_find(headers, name);
    if (entry == NULL) {
        return NULL;
    }

    const struct string* values = entry->values.data;
    return &values[0];
}

/* Returns the number of values grouped under one field name. */
size_t headers_value_count(const struct headers* headers, const char* name) {
    assert(headers != NULL);
    assert(name != NULL);

    const struct header_entry* entry = headers_find(headers, name);
    return entry == NULL ? 0 : vector_len(&entry->values);
}

/* Returns one grouped value by arrival index. */
const struct string* headers_get_at(
    const struct headers* headers, const char* name, size_t index) {
    assert(headers != NULL);
    assert(name != NULL);

    const struct header_entry* entry = headers_find(headers, name);
    if (entry == NULL || index >= vector_len(&entry->values)) {
        return NULL;
    }

    const struct string* values = entry->values.data;
    return &values[index];
}

/* Releases every occupied entry before releasing the table. */
void headers_deinit(struct headers* headers) {
    assert(headers != NULL);

    for (size_t index = 0; index < headers->capacity; index += 1) {
        if (headers->entries[index].occupied) {
            header_entry_deinit(&headers->entries[index]);
        }
    }

    free(headers->entries);
    headers->entries = NULL;
    headers->length = 0;
    headers->capacity = 0;
}
