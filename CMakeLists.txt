cmake_minimum_required(VERSION 3.25)

project(eunha
  VERSION 0.1.0
  LANGUAGES C
)

add_executable(eunha src/main.c)

target_compile_features(eunha PRIVATE c_std_17)

install(TARGETS eunha)
