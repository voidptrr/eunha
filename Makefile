# MIT License
#
# Copyright (c) 2026 Tommaso Bruno
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

.DEFAULT_GOAL := all

CC := clang
AR := ar
ARFLAGS := rcs

MODE ?= release
PREFIX ?= $(if $(prefix),$(prefix),/usr/local)
DESTDIR ?=

COMMON_FLAGS := \
	-std=c23 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Walloca \
	-Warray-bounds \
	-Wcast-align \
	-Wcast-qual \
	-Wcomma \
	-Wconversion \
	-Wdouble-promotion \
	-Wformat=2 \
	-Wimplicit-fallthrough \
	-Wmissing-prototypes \
	-Wnull-dereference \
	-Wpointer-arith \
	-Wshadow \
	-Wsign-conversion \
	-Wstrict-prototypes \
	-Wswitch-enum \
	-Wundef \
	-Wunreachable-code \
	-Wvla \
	-Wwrite-strings

DEBUG_FLAGS := \
	-g \
	-Og \
	-Werror \
	-fsanitize=address,undefined \
	-fno-omit-frame-pointer

RELEASE_FLAGS := \
	-O2 \
	-UNDEBUG \
	-D_FORTIFY_SOURCE=3 \
	-fstack-protector-strong \
	-fPIC

ifeq ($(MODE),debug)
MODE_FLAGS := $(DEBUG_FLAGS)
else ifeq ($(MODE),release)
MODE_FLAGS := $(RELEASE_FLAGS)
else
$(error MODE must be debug or release)
endif

CPPFLAGS += -Iinclude
CFLAGS += $(COMMON_FLAGS) $(MODE_FLAGS)

SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard include/eunha/*.h)
TEST_SOURCES := $(wildcard tests/*_test.c)

BUILD_DIR := build/$(MODE)
LIBRARY := $(BUILD_DIR)/libeunha.a
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/objects/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)
TEST_BINARIES := $(patsubst tests/%.c,build/tests/%,$(TEST_SOURCES))

.PHONY: all debug release tests test check install clean

all: $(LIBRARY)

debug:
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release all

tests:
	$(MAKE) MODE=debug $(TEST_BINARIES)

test: tests
	@set -e; for test_binary in $(TEST_BINARIES); do \
		$$test_binary; \
	done

check: test

install: $(LIBRARY)
	mkdir -p "$(DESTDIR)$(PREFIX)/lib" "$(DESTDIR)$(PREFIX)/include/eunha"
	install -m 0644 "$(LIBRARY)" "$(DESTDIR)$(PREFIX)/lib/libeunha.a"
	install -m 0644 $(HEADERS) "$(DESTDIR)$(PREFIX)/include/eunha"

clean:
	rm -rf build

$(LIBRARY): $(OBJECTS)
	mkdir -p "$(@D)"
	rm -f "$@"
	$(AR) $(ARFLAGS) "$@" $^

$(BUILD_DIR)/objects/%.o: src/%.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c "$<" -o "$@"

build/tests/%: tests/%.c $(LIBRARY)
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) "$<" "$(LIBRARY)" $(LDFLAGS) $(LDLIBS) -o "$@"

-include $(DEPENDENCIES)
