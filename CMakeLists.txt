cmake_minimum_required(VERSION 3.16)
project(mbasic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Main library
add_library(mbasic_lib
    src/value.cpp
    src/tokens.cpp
    src/lexer.cpp
    src/error.cpp
)

target_include_directories(mbasic_lib PUBLIC include)

# Main executable
add_executable(mbasic src/main.cpp)
target_link_libraries(mbasic mbasic_lib)

# Check for readline
find_library(READLINE_LIBRARY readline)
if(READLINE_LIBRARY)
    target_compile_definitions(mbasic PRIVATE HAVE_READLINE)
    target_link_libraries(mbasic ${READLINE_LIBRARY})
endif()

# Tests
enable_testing()

# Simple test runner for now
add_executable(test_lexer tests/test_lexer.cpp)
target_link_libraries(test_lexer mbasic_lib)
add_test(NAME lexer_tests COMMAND test_lexer)
