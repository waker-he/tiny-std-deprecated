
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(functional.o functional.cpp)
add_executable(any.o any.cpp)
add_executable(concepts.o concepts.cpp)
add_executable(smart_pointers.o smart_pointers.cpp)
