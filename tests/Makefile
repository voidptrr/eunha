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

CC ?= gcc
PROJECT_ROOT ?= ..
BUILD_DIR ?= $(PROJECT_ROOT)/build
SOURCE_DIR := $(PROJECT_ROOT)/src

WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wcast-align -Wformat=2 -Wundef -Wwrite-strings
CFLAGS ?= -std=c17 $(WARNINGS)
CPPFLAGS += -I$(PROJECT_ROOT)/include

TEST_TARGETS := $(BUILD_DIR)/test_vector $(BUILD_DIR)/test_string $(BUILD_DIR)/test_utils $(BUILD_DIR)/test_header $(BUILD_DIR)/test_request $(BUILD_DIR)/test_parser

.PHONY: test

test: $(TEST_TARGETS)
	$(BUILD_DIR)/test_vector
	$(BUILD_DIR)/test_string
	$(BUILD_DIR)/test_utils
	$(BUILD_DIR)/test_header
	$(BUILD_DIR)/test_request
	$(BUILD_DIR)/test_parser

$(BUILD_DIR)/test_vector: test_vector.c $(SOURCE_DIR)/datastruct/vector.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_string: test_string.c $(SOURCE_DIR)/datastruct/string.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_utils: test_utils.c $(SOURCE_DIR)/utils.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_header: http/test_header.c $(SOURCE_DIR)/http/header.c $(SOURCE_DIR)/datastruct/string.c $(SOURCE_DIR)/datastruct/vector.c $(SOURCE_DIR)/utils.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_request: http/test_request.c $(SOURCE_DIR)/http/request.c $(SOURCE_DIR)/http/header.c $(SOURCE_DIR)/datastruct/string.c $(SOURCE_DIR)/datastruct/vector.c $(SOURCE_DIR)/utils.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_parser: http/test_parser.c $(SOURCE_DIR)/http/parser.c $(SOURCE_DIR)/http/request.c $(SOURCE_DIR)/http/header.c $(SOURCE_DIR)/datastruct/string.c $(SOURCE_DIR)/datastruct/vector.c $(SOURCE_DIR)/utils.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -Wl,--wrap=malloc -Wl,--wrap=realloc -o $@
