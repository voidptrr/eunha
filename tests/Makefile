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

WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wcast-align -Wformat=2 -Wundef -Wwrite-strings
CFLAGS ?= -std=c17 $(WARNINGS)
CPPFLAGS += -I$(PROJECT_ROOT)

TEST_TARGETS := $(BUILD_DIR)/test_vector $(BUILD_DIR)/test_parser_core $(BUILD_DIR)/test_parser_start_line $(BUILD_DIR)/test_parser_header $(BUILD_DIR)/test_parser_body

.PHONY: test

test: $(TEST_TARGETS)
	$(BUILD_DIR)/test_vector
	$(BUILD_DIR)/test_parser_core
	$(BUILD_DIR)/test_parser_start_line
	$(BUILD_DIR)/test_parser_header
	$(BUILD_DIR)/test_parser_body

$(BUILD_DIR)/test_vector: test_vector.c $(PROJECT_ROOT)/datastruct/vector.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_parser_core: http/parser/test_core.c $(PROJECT_ROOT)/http/parser.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_parser_start_line: http/parser/test_start_line.c $(PROJECT_ROOT)/http/parser.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_parser_header: http/parser/test_header.c $(PROJECT_ROOT)/http/parser.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_parser_body: http/parser/test_body.c $(PROJECT_ROOT)/http/parser.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
