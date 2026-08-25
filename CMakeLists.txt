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

cmake_minimum_required(VERSION 3.25)

project(eunha VERSION 0.1.0 LANGUAGES C)

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Eunha requires Clang")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build profile" FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
  message(FATAL_ERROR "CMAKE_BUILD_TYPE must be Debug or Release")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE EUNHA_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(eunha ${EUNHA_SOURCES})

target_compile_options(eunha PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Walloca
  -Warray-bounds
  -Wcast-align
  -Wcast-qual
  -Wcomma
  -Wconversion
  -Wdouble-promotion
  -Wformat=2
  -Wimplicit-fallthrough
  -Wmissing-prototypes
  -Wnull-dereference
  -Wpointer-arith
  -Wshadow
  -Wsign-conversion
  -Wstrict-prototypes
  -Wswitch-enum
  -Wundef
  -Wunreachable-code
  -Wvla
  -Wwrite-strings
  "$<$<CONFIG:Debug>:-Og;-Werror;-fsanitize=address,undefined;-fno-omit-frame-pointer>"
  "$<$<CONFIG:Release>:-UNDEBUG;-O2;-fstack-protector-strong;-fPIE>"
)

target_compile_definitions(eunha PRIVATE
  "$<$<CONFIG:Release>:_FORTIFY_SOURCE=3>"
)

target_link_options(eunha PRIVATE
  "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
  "$<$<CONFIG:Release>:-pie;-Wl,-z,relro;-Wl,-z,now>"
)

install(TARGETS eunha RUNTIME DESTINATION bin)
