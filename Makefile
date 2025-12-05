# MBASIC 5.21 C++ Interpreter Makefile
# https://github.com/avwohl/mbasicc

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDES := -Iinclude

# Library source files (portable core - can be used for WASM builds)
LIB_CORE_SRCS := src/value.cpp src/tokens.cpp src/lexer.cpp src/error.cpp \
                 src/ast.cpp src/parser.cpp src/runtime.cpp src/interpreter.cpp
LIB_CORE_OBJS := $(LIB_CORE_SRCS:.cpp=.o)

# I/O implementation files (platform-specific)
LIB_IO_SRCS := src/console_io.cpp src/file_handler.cpp src/readline.cpp
LIB_IO_OBJS := $(LIB_IO_SRCS:.cpp=.o)

# All library objects
LIB_OBJS := $(LIB_CORE_OBJS) $(LIB_IO_OBJS)

MAIN_SRC := src/main.cpp
TEST_SRC := tests/test_lexer.cpp

# Installation directories
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1

# Targets
.PHONY: all clean test lib install uninstall

all: mbasicc

LDFLAGS := -ledit

# Build the executable
mbasicc: $(LIB_OBJS) $(MAIN_SRC:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build static library (for linking with other projects)
lib: libmbasic.a

libmbasic.a: $(LIB_CORE_OBJS)
	ar rcs $@ $^

test_lexer: $(LIB_OBJS) $(TEST_SRC:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Object file compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Run tests
test: test_lexer
	./test_lexer

clean:
	rm -f $(LIB_OBJS) src/main.o tests/test_lexer.o mbasicc test_lexer libmbasic.a

# Install binary and man page
install: mbasicc
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 mbasicc $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 man/mbasicc.1 $(DESTDIR)$(MANDIR)/

# Uninstall
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/mbasicc
	rm -f $(DESTDIR)$(MANDIR)/mbasicc.1

# Dependencies
src/value.o: include/mbasic/value.hpp
src/tokens.o: include/mbasic/tokens.hpp
src/lexer.o: include/mbasic/lexer.hpp include/mbasic/tokens.hpp include/mbasic/error.hpp
src/error.o: include/mbasic/error.hpp
src/ast.o: include/mbasic/ast.hpp include/mbasic/value.hpp include/mbasic/tokens.hpp
src/parser.o: include/mbasic/parser.hpp include/mbasic/ast.hpp include/mbasic/lexer.hpp include/mbasic/error.hpp
src/runtime.o: include/mbasic/runtime.hpp include/mbasic/value.hpp include/mbasic/ast.hpp include/mbasic/error.hpp
src/interpreter.o: include/mbasic/interpreter.hpp include/mbasic/runtime.hpp include/mbasic/ast.hpp include/mbasic/value.hpp include/mbasic/io_handler.hpp
src/console_io.o: include/mbasic/io_handler.hpp
src/file_handler.o: include/mbasic/file_handler.hpp
src/readline.o: include/mbasic/readline.hpp
src/main.o: include/mbasic/lexer.hpp include/mbasic/parser.hpp include/mbasic/error.hpp include/mbasic/interpreter.hpp include/mbasic/runtime.hpp include/mbasic/readline.hpp
tests/test_lexer.o: include/mbasic/lexer.hpp include/mbasic/error.hpp
