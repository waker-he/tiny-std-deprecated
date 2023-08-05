cmake_minimum_required(VERSION 3.25.1)
project(tiny-stl CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(test)



file(GLOB_RECURSE ALL_CXX_SOURCE_FILES test/*.cpp src/*.hpp)

set(CLANG_FORMAT_BIN clang-format)
set(CLANG_FORMAT_STYLE "file")

add_custom_target(
  clang-format
  COMMAND ${CLANG_FORMAT_BIN}
  -i
  -style=${CLANG_FORMAT_STYLE}
  ${ALL_CXX_SOURCE_FILES}
)

add_custom_target(
    clang-tidy
    COMMAND /usr/bin/clang-tidy
    -p ${CMAKE_BINARY_DIR}
    ${ALL_CXX_SOURCE_FILES}
)