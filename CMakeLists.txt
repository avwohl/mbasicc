cmake_minimum_required(VERSION 3.16)
project(mbasic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Portable core (no platform I/O - this is the subset usable for WASM builds).
# Built as libmbasic.a, the same library "make lib" produces.
add_library(mbasic_core
    src/value.cpp
    src/tokens.cpp
    src/lexer.cpp
    src/error.cpp
    src/ast.cpp
    src/parser.cpp
    src/runtime.cpp
    src/interpreter.cpp
)
set_target_properties(mbasic_core PROPERTIES OUTPUT_NAME mbasic)
target_include_directories(mbasic_core PUBLIC include)

# Platform-specific I/O implementations, on top of the portable core
add_library(mbasic_lib
    src/console_io.cpp
    src/file_handler.cpp
    src/readline.cpp
)
target_link_libraries(mbasic_lib PUBLIC mbasic_core)

# src/readline.cpp includes <editline/readline.h> and needs libedit (-ledit).
# Without it, the file compiles a std::getline fallback keyed off MBASIC_NO_EDITLINE.
find_library(EDITLINE_LIBRARY edit)
if(EDITLINE_LIBRARY)
    target_link_libraries(mbasic_lib PUBLIC ${EDITLINE_LIBRARY})
else()
    target_compile_definitions(mbasic_lib PRIVATE MBASIC_NO_EDITLINE)
endif()

# Main executable (named mbasicc, as in the Makefile, the man page and README)
add_executable(mbasicc src/main.cpp)
target_link_libraries(mbasicc PRIVATE mbasic_lib)

# Tests
enable_testing()

# Simple test runner for now
add_executable(test_lexer tests/test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE mbasic_lib)
add_test(NAME lexer_tests COMMAND test_lexer)
