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

#ifndef EUNHA_STRING_H
#define EUNHA_STRING_H

#include <stddef.h>
#include <eunha/arena.h>
#include <eunha/core.h>

/**
 * struct str8 - Non-owning immutable byte-string view.
 * @data: First byte, or NULL when len is zero.
 * @len: Number of bytes in the view, excluding any trailing null byte.
 */
struct str8 {
    const u8 *data;
    size_t len;
};

/**
 * struct str8_list_node - One string view in a linked string list.
 * @data: Non-owning string view stored by this node.
 * @next: Next node, or NULL for the final node.
 */
struct str8_list_node {
    struct str8 str;
    struct str8_list_node *next;
};

/**
 * struct str8_list - Ordered collection of string views.
 * @head: First node, or NULL when the list is empty.
 * @tail: Final node, or NULL when the list is empty.
 * @total_len: Sum of the lengths of all stored string views.
 */
struct str8_list {
    struct str8_list_node *head;
    struct str8_list_node *tail;
    size_t total_len;
};

/** Creates a non-owning string view from a string literal. */
#define str8_lit(value) str8((const u8 *)(value), sizeof(value) - 1)

/** Iterates a caller-owned byte cursor over the string's contents. */
#define for_each_char(pos, str) \
    for ((pos) = (str)->data;   \
         (pos) != NULL && (pos) < (str)->data + (str)->len; ++(pos))

/** Creates a non-owning UTF-8 view over len bytes. */
struct str8 str8(const u8 *data, size_t len);

/** Creates a non-owning UTF-8 view over a C string, or a zero view for NULL. */
struct str8 str8_cstring(const char *cstr);

/** Returns the string length in bytes, excluding the null terminator. */
size_t str8_len(struct str8 str);

/**
 * Returns whether first and second contain the same bytes. Comparison is
 * bytewise and case-sensitive; two empty strings are equal.
 */
bool str8_equal(struct str8 first, struct str8 second);

/**
 * Returns a view with leading and trailing ASCII whitespace removed. The
 * string bytes are not copied, and the returned view may be empty.
 */
struct str8 str8_trim(struct str8 str);

/**
 * Copies a string and a trailing null byte into the arena. Aborts if the
 * required size cannot be represented or allocated.
 */
struct str8 str8_copy(struct arena *arena, struct str8 source);

/**
 * Returns a new arena-owned concatenation of first and second. The inputs
 * remain unchanged. Aborts if the required size cannot be represented or
 * allocated.
 */
struct str8 str8_cat(struct arena *arena, struct str8 first,
                     struct str8 second);

/**
 * Returns whether haystack contains needle. Matching is bytewise and
 * case-sensitive; an empty needle always matches.
 */
bool str8_contains(struct str8 haystack, struct str8 needle);

/**
 * Returns whether string has prefix. Matching is bytewise and case-sensitive;
 * an empty prefix always matches.
 */
bool str8_has_prefix(struct str8 str, struct str8 prefix);

/**
 * Splits string at each occurrence of the delimiter byte. The returned list
 * nodes are arena-owned, but their string views reference the original bytes.
 * Empty fields are preserved. Aborts if node storage cannot be allocated.
 */
struct str8_list str8_split(struct arena *arena, struct str8 str, u8 delimiter);

/**
 * Appends str to list by allocating a list node from arena. The string bytes
 * are not copied, so they must remain valid for as long as the list is used.
 * Aborts if the total length overflows or node storage cannot be allocated.
 */
void str8_list_push(struct arena *arena, struct str8_list *list,
                    struct str8 str);

/**
 * Prepends str to list by allocating a list node from arena. The string bytes
 * are not copied, so they must remain valid for as long as the list is used.
 * Aborts if the total length overflows or node storage cannot be allocated.
 */
void str8_list_pushfront(struct arena *arena, struct str8_list *list,
                         struct str8 str);

/**
 * Copies the strings in list into one contiguous, null-terminated arena
 * allocation and returns a view over it. The list and its strings remain
 * unchanged. Aborts if the required size cannot be represented or allocated.
 */
struct str8 str8_list_join(struct arena *arena, const struct str8_list *list);

#endif
