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
BUILD_DIR ?= build
prefix ?= /usr/local
bindir ?= $(prefix)/bin
RESOLVED_CC := $(shell command -v $(CC) 2>/dev/null || printf '%s' '$(CC)')
CC_BASENAME := $(notdir $(CC))
ESCAPED_RESOLVED_CC := $(subst /,\/,$(RESOLVED_CC))

TARGET := $(BUILD_DIR)/eunha
SOURCES := main.c config.c utils.c net/server.c http/parser.c http/header.c http/request.c datastruct/string.c datastruct/vector.c
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wcast-align -Wformat=2 -Wundef -Wwrite-strings
CFLAGS += -std=c17 $(WARNINGS)
CPPFLAGS += -Iinclude

.PHONY: all install test clean compile_commands

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)

test:
	$(MAKE) -C tests PROJECT_ROOT=$(CURDIR) BUILD_DIR=$(abspath $(BUILD_DIR)) CC="$(CC)" CFLAGS="$(CFLAGS)"

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(bindir)/eunha

compile_commands:
	$(MAKE) clean
	mkdir -p $(BUILD_DIR)
	bear --config bear.yaml --output $(BUILD_DIR)/compile_commands.json -- $(MAKE) CC=$(RESOLVED_CC) all test
	sed -i 's/"$(CC_BASENAME)"/"$(ESCAPED_RESOLVED_CC)"/' $(BUILD_DIR)/compile_commands.json

clean:
	rm -rf $(BUILD_DIR)
