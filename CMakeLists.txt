cmake_minimum_required(VERSION 3.24.0)
set(CMAKE_CXX_STANDARD 17)

project(tpy CXX)

add_subdirectory(src)
add_subdirectory(tools)

target_include_directories(tpy_utility PUBLIC "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}/include")
target_include_directories(tpy_source PUBLIC "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}/include")

target_include_directories(tpy PUBLIC "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}/include")

target_link_libraries(tpy PUBLIC tpy_utility tpy_source)


# Set up the testing rig with catch 2.
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)

add_subdirectory(tests)

target_include_directories(tests PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}/include")
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain tpy_utility tpy_source)