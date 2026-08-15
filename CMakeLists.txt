cmake_minimum_required(VERSION 3.25)

project(eunha
  VERSION 0.1.0
  LANGUAGES C
)

add_executable(eunha src/main.c)

target_compile_features(eunha PRIVATE c_std_17)
target_compile_options(eunha PRIVATE
  $<$<COMPILE_LANG_AND_ID:C,Clang>:
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wcast-align
    -Wformat=2
    -Wundef
    -Wunreachable-code
    -Wwrite-strings
  >
)

install(TARGETS eunha)
