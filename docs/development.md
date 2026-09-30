# Development

Source layout, building the portable library, and running the tests. Back to the [README](../README.md).

## Project Structure

```
mbasicc/
├── include/mbasic/      # Header files
│   ├── ast.hpp          # Abstract Syntax Tree definitions
│   ├── error.hpp        # Error codes and messages
│   ├── file_handler.hpp # File I/O abstraction (for WASM portability)
│   ├── interpreter.hpp  # Interpreter class
│   ├── io_handler.hpp   # Console I/O abstraction (for WASM portability)
│   ├── lexer.hpp        # Lexical analyzer
│   ├── parser.hpp       # Parser
│   ├── readline.hpp     # Line editing wrapper
│   ├── runtime.hpp      # Runtime state
│   ├── tokens.hpp       # Token definitions
│   └── value.hpp        # Value types
├── src/                 # Implementation files
│   ├── ast.cpp
│   ├── console_io.cpp   # Console I/O implementation (std::cin/std::cout)
│   ├── error.cpp
│   ├── file_handler.cpp # File I/O implementation (std::fstream)
│   ├── interpreter.cpp
│   ├── lexer.cpp
│   ├── main.cpp
│   ├── parser.cpp
│   ├── readline.cpp     # editline wrapper (portable)
│   ├── runtime.cpp
│   ├── tokens.cpp
│   └── value.cpp
├── man/                 # Documentation
│   └── mbasicc.1        # Man page
├── tests/               # Test files
├── .github/workflows/   # CI/CD configuration
│   ├── ci.yml           # Continuous integration
│   └── release.yml      # Release builds
├── Makefile
├── README.md
├── CHANGELOG.md
└── LICENSE
```

### Building the Library

For WebAssembly or embedding in other projects, you can build just the portable core:

```bash
make lib          # Creates libmbasic.a
```

This includes the lexer, parser, AST, runtime, and interpreter - but not the I/O implementations.
Provide your own `IOHandler` implementation for custom platforms.

---

## Running Tests

The test suite from the Python reference implementation can be used:

```bash
# Run all tests
for f in ~/src/mbasic/basic/dev/tests_with_results/test_*.bas; do
    echo "=== $(basename $f) ==="
    mbasicc "$f"
done
```

Or run individual tests:
```bash
mbasicc ~/src/mbasic/basic/dev/tests_with_results/test_simple.bas
mbasicc ~/src/mbasic/basic/dev/tests_with_results/test_math_functions.bas
mbasicc ~/src/mbasic/basic/dev/tests_with_results/test_string_functions.bas
```
