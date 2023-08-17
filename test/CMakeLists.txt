
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(functional.o functional.cpp)
add_executable(any.o any.cpp)
add_executable(concepts.o concepts.cpp)
add_executable(unique_ptr.o unique_ptr.cpp)
add_executable(shared_ptr.o shared_ptr.cpp)
