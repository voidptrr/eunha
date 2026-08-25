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

NAME := eunha
CC := gcc
warning_flags := \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Walloc-zero \
	-Walloca \
	-Warray-bounds=2 \
	-Wcast-qual \
	-Wconversion \
	-Wformat=2 \
	-Wformat-overflow=2 \
	-Wformat-truncation=2 \
	-Wmissing-prototypes \
	-Wnull-dereference \
	-Wshadow \
	-Wsign-conversion \
	-Wstrict-prototypes \
	-Wstringop-overflow=4 \
	-Wundef \
	-Wvla \
	-Wwrite-strings

profile ?= debug

ifeq ($(profile),debug)
profile_cflags := -Og -g3 -Werror -fanalyzer \
	-fsanitize=address,undefined \
	-fno-omit-frame-pointer
profile_ldflags := -fsanitize=address,undefined
else ifeq ($(profile),release)
profile_cppflags := -D_FORTIFY_SOURCE=3
profile_cflags := -O2 -fstack-protector-strong -fPIE
profile_ldflags := -pie -Wl,-z,relro -Wl,-z,now
else
$(error unknown build profile '$(profile)')
endif

CPPFLAGS := $(profile_cppflags)
CFLAGS := -std=c17 $(warning_flags) $(profile_cflags)
LDFLAGS := $(profile_ldflags)

prefix ?= /usr/local
bindir := $(prefix)/bin
builddir := build/$(profile)
target := $(builddir)/$(NAME)

sources := $(sort $(shell find src -type f -name '*.c'))
objects := $(patsubst src/%.c,$(builddir)/%.o,$(sources))
dependencies := $(objects:.o=.d)

.PHONY: all build clean compile_commands debug install release

all: debug

debug:
	$(MAKE) profile=debug build

release:
	$(MAKE) profile=release build

build: $(target)

$(target): $(objects)
	$(CC) $(LDFLAGS) $(objects) -o $@

$(builddir)/%.o: src/%.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

compile_commands:
	bear --config bear.yaml -- $(MAKE) --always-make profile=debug build

install: release
	install -d $(bindir)
	install -m 755 build/release/$(NAME) $(bindir)/$(NAME)

clean:
	$(RM) -r build compile_commands.json

-include $(dependencies)
