# MBASICC - MBASIC 5.21 C++ Interpreter

A modern C++ implementation of Microsoft BASIC-80 version 5.21.

**Repository:** https://github.com/jwoehr/mbasic_cc

[![CI](https://github.com/jwoehr/mbasic_cc/actions/workflows/ci.yml/badge.svg)](https://github.com/jwoehr/mbasic_cc/actions/workflows/ci.yml)

## Installation

### From Package (Recommended)

**Debian/Ubuntu:**
```bash
# Download the latest .deb from releases
sudo dpkg -i mbasicc_X.Y.Z_amd64.deb
sudo apt-get install -f  # Install dependencies if needed
```

**Fedora/RHEL/CentOS:**
```bash
# Download the latest .rpm from releases
sudo rpm -i mbasicc-X.Y.Z-1.*.rpm
```

### From Pre-built Binary

```bash
# Download and extract the binary tarball
tar xzf mbasicc-X.Y.Z-linux-amd64.tar.gz
sudo cp mbasicc-X.Y.Z-linux-amd64/mbasicc /usr/local/bin/
```

### Building from Source

**Requirements:**
- C++17 compiler (g++ 7+ or clang++ 5+)
- libedit development files
- make

**Debian/Ubuntu:**
```bash
sudo apt-get install build-essential libedit-dev
git clone https://github.com/jwoehr/mbasic_cc.git
cd mbasic_cc
make
sudo cp mbasicc /usr/local/bin/
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ make libedit-devel
git clone https://github.com/jwoehr/mbasic_cc.git
cd mbasic_cc
make
sudo cp mbasicc /usr/local/bin/
```

**macOS:**
```bash
# libedit is included in macOS
git clone https://github.com/jwoehr/mbasic_cc.git
cd mbasic_cc
make
sudo cp mbasicc /usr/local/bin/
```

### Building Without editline

If libedit is not available on your system, edit `src/readline.cpp`:
1. Comment out the "EDITLINE IMPLEMENTATION" section
2. Uncomment the "FALLBACK IMPLEMENTATION" section
3. Remove `-ledit` from `LDFLAGS` in the Makefile

This provides basic line input without history or line editing features.

---

## Usage

```bash
# Run a BASIC program
mbasicc program.bas

# Interactive REPL
mbasicc

# Parse only (show AST)
mbasicc --parse program.bas

# Tokenize only
mbasicc --tokenize program.bas
```

## Interactive Commands

| Command | Description |
|---------|-------------|
| `NEW` | Clear program from memory |
| `RUN` | Execute program |
| `LIST [n[-m]]` | List program lines |
| `LOAD "file"` | Load program from file |
| `SAVE "file"` | Save program to file |
| `FILES [pattern]` | List files (default: *.bas) |
| `AUTO [start[,step]]` | Auto line numbering mode |
| `EDIT line` | Edit a program line |
| `DELETE line[-line]` | Delete program lines |
| `RENUM [new[,old[,inc]]]` | Renumber program lines |
| `CONT` | Continue after STOP/BREAK |
| `TRON` / `TROFF` | Enable/disable trace mode |
| `SYSTEM` / `QUIT` | Exit interpreter |

---

## Example Program

```basic
10 REM Hello World in MBASIC
20 PRINT "Hello, World!"
30 FOR I = 1 TO 5
40   PRINT "Count: "; I
50 NEXT I
60 END
```

Save as `hello.bas` and run:
```bash
mbasicc hello.bas
```

---

## Implementation Status

This implementation covers the vast majority of MBASIC 5.21 features:

### Fully Implemented
- All program flow control (FOR/NEXT, WHILE/WEND, GOSUB/RETURN, GOTO, IF/THEN/ELSE)
- All data types (INTEGER %, SINGLE !, DOUBLE #, STRING $)
- All arithmetic and logical operators including MOD, \, EQV, IMP
- All standard math functions (SIN, COS, TAN, ATN, LOG, EXP, SQR, etc.)
- All string functions (LEFT$, RIGHT$, MID$, INSTR, CHR$, ASC, etc.)
- Array handling (DIM, ERASE, multi-dimensional arrays, OPTION BASE)
- User-defined functions (DEF FN)
- File I/O (sequential and random access)
- Error handling (ON ERROR GOTO, RESUME, ERR, ERL)
- PRINT USING formatted output
- DATA/READ/RESTORE
- CHAIN and COMMON for program chaining
- Interactive REPL with line editing and history

### Not Implemented (Hardware-Specific)
- Direct memory access (PEEK, POKE, VARPTR)
- Port I/O (INP, OUT, WAIT)
- Machine code calls (USR, CALL, DEF USR)
- Cassette tape I/O (CLOAD, CSAVE)

These hardware-specific features are implemented as stubs that return 0 or do nothing.

---

## Project Structure

```
mbasic_cc/
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

---

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run the test suite
5. Submit a pull request

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## References

- BASIC-80 Reference Manual v5.21 (AA-P226A-TV)
- [MBASIC Wikipedia](https://en.wikipedia.org/wiki/MBASIC)

---

# MBASIC 5.21 Feature Reference

## Commands (REPL/Direct Mode)

### General Commands
| Command | Status | Description |
|---------|--------|-------------|
| `AUTO [line][,increment]` | Implemented | Auto-generate line numbers |
| `CONT` | Implemented | Continue execution after STOP/BREAK |
| `DELETE [line][-line]` | Implemented | Delete program lines |
| `EDIT line` | Implemented | Enter edit mode |
| `LIST [line][-[line]]` | Implemented | List program lines |
| `LLIST [line][-[line]]` | Implemented | List to line printer |
| `NEW` | Implemented | Clear memory and all variables |
| `RENUM [[new][,[old][,inc]]]` | Implemented | Renumber program lines |
| `RUN [line]` | Implemented | Execute program |
| `RUN filename[,R]` | Implemented | Load and run from disk |
| `TRON` | Implemented | Enable trace mode |
| `TROFF` | Implemented | Disable trace mode |

### File Management Commands
| Command | Status | Description |
|---------|--------|-------------|
| `FILES [pattern]` | Implemented | List disk files |
| `KILL filename` | Implemented | Delete disk file |
| `LOAD filename[,R]` | Implemented | Load program from disk |
| `MERGE filename` | Implemented | Merge program from disk |
| `NAME old AS new` | Implemented | Rename disk file |
| `RESET` | Implemented | Close all files |
| `SAVE filename[,A][,P]` | Implemented | Save program to disk |
| `SYSTEM` | Implemented | Exit interpreter |

---

## Statements

### Program Flow Control
| Statement | Status | Description |
|-----------|--------|-------------|
| `CHAIN [MERGE] file[,...]` | Implemented | Chain programs |
| `END` | Implemented | End program execution |
| `FOR var=start TO end [STEP n]` | Implemented | Begin loop |
| `GOSUB line` | Implemented | Call subroutine |
| `GOTO line` | Implemented | Unconditional branch |
| `IF...THEN...ELSE` | Implemented | Conditional execution |
| `NEXT [var[,var...]]` | Implemented | End loop |
| `ON expr GOTO line-list` | Implemented | Computed GOTO |
| `ON expr GOSUB line-list` | Implemented | Computed GOSUB |
| `ON ERROR GOTO line` | Implemented | Enable error trapping |
| `RESUME [0\|NEXT\|line]` | Implemented | Continue after error |
| `RETURN` | Implemented | Return from subroutine |
| `STOP` | Implemented | Halt execution |
| `WHILE...WEND` | Implemented | Loop while true |

### Data and Variables
| Statement | Status | Description |
|-----------|--------|-------------|
| `CLEAR` | Implemented | Clear variables |
| `COMMON variable-list` | Implemented | Pass vars to CHAINed program |
| `DATA constant-list` | Implemented | Store data for READ |
| `DEF FN name[(params)]=expr` | Implemented | Define user function |
| `DEFDBL letter-range` | Implemented | Declare double precision |
| `DEFINT letter-range` | Implemented | Declare integer |
| `DEFSNG letter-range` | Implemented | Declare single precision |
| `DEFSTR letter-range` | Implemented | Declare string |
| `DIM array(subscripts)` | Implemented | Dimension arrays |
| `ERASE array-list` | Implemented | Remove arrays |
| `LET variable=expression` | Implemented | Assign value |
| `OPTION BASE n` | Implemented | Set array base (0 or 1) |
| `READ variable-list` | Implemented | Read from DATA |
| `RESTORE [line]` | Implemented | Reset DATA pointer |
| `SWAP variable,variable` | Implemented | Exchange values |

### Input/Output
| Statement | Status | Description |
|-----------|--------|-------------|
| `CLS` | Implemented | Clear screen |
| `INPUT [;]["prompt";]var-list` | Implemented | Get user input |
| `LINE INPUT [;]["prompt";]str$` | Implemented | Input entire line |
| `PRINT [expr-list]` | Implemented | Output to terminal |
| `PRINT USING fmt;expr-list` | Implemented | Formatted output |
| `LPRINT [expr-list]` | Implemented | Output to line printer |
| `WRITE [expr-list]` | Implemented | Output with delimiters |

### File I/O
| Statement | Status | Description |
|-----------|--------|-------------|
| `CLOSE [[#]file-list]` | Implemented | Close files |
| `FIELD [#]file,width AS str...` | Implemented | Allocate random buffer |
| `GET [#]file[,record]` | Implemented | Read random record |
| `INPUT# file,var-list` | Implemented | Input from file |
| `LINE INPUT# file,str$` | Implemented | Input line from file |
| `LSET string=expression` | Implemented | Left-justify in field |
| `OPEN mode,[#]file,name[,len]` | Implemented | Open file |
| `PRINT# file,expr-list` | Implemented | Output to file |
| `PUT [#]file[,record]` | Implemented | Write random record |
| `RSET string=expression` | Implemented | Right-justify in field |
| `WRITE# file,expr-list` | Implemented | Write with delimiters |

---

## Functions

### Mathematical Functions
| Function | Description |
|----------|-------------|
| `ABS(x)` | Absolute value |
| `ATN(x)` | Arctangent (radians) |
| `COS(x)` | Cosine (radians) |
| `EXP(x)` | e^x |
| `FIX(x)` | Truncate to integer |
| `INT(x)` | Largest integer <= x |
| `LOG(x)` | Natural logarithm |
| `RND[(x)]` | Random number 0 to 1 |
| `SGN(x)` | Sign (-1, 0, 1) |
| `SIN(x)` | Sine (radians) |
| `SQR(x)` | Square root |
| `TAN(x)` | Tangent (radians) |

### Type Conversion Functions
| Function | Description |
|----------|-------------|
| `CDBL(x)` | Convert to double |
| `CINT(x)` | Convert to integer |
| `CSNG(x)` | Convert to single |
| `CVD(str)` | 8-byte string to double |
| `CVI(str)` | 2-byte string to integer |
| `CVS(str)` | 4-byte string to single |
| `MKD$(dbl)` | Double to 8-byte string |
| `MKI$(int)` | Integer to 2-byte string |
| `MKS$(sng)` | Single to 4-byte string |

### String Functions
| Function | Description |
|----------|-------------|
| `ASC(str)` | ASCII code of first char |
| `CHR$(code)` | Character from ASCII code |
| `HEX$(n)` | Hexadecimal string |
| `INSTR([start,]s1,s2)` | Find substring position |
| `LEFT$(str,n)` | Leftmost n characters |
| `LEN(str)` | Length of string |
| `MID$(str,start[,len])` | Substring |
| `OCT$(n)` | Octal string |
| `RIGHT$(str,n)` | Rightmost n characters |
| `SPACE$(n)` | String of n spaces |
| `SPC(n)` | Print n spaces |
| `STR$(n)` | Convert number to string |
| `STRING$(n,char)` | Repeated character string |
| `TAB(column)` | Tab to column |
| `VAL(str)` | Convert string to number |

### System Functions
| Function | Description |
|----------|-------------|
| `DATE$` | Current date (MM-DD-YYYY) |
| `EOF(file)` | Test for end of file |
| `ENVIRON$(name)` | Environment variable value |
| `ERL` | Line number of last error |
| `ERR` | Error code of last error |
| `FRE(x)` | Free memory (stub) |
| `INKEY$` | Read char without waiting |
| `INPUT$(n[,#file])` | Read n characters |
| `LOC(file)` | Current record position |
| `LOF(file)` | File length |
| `LPOS(x)` | Line printer position |
| `POS(x)` | Current cursor column |
| `TIME$` | Current time (HH:MM:SS) |
| `TIMER` | Seconds since midnight |

---

## Operators

| Operator | Description | Precedence |
|----------|-------------|------------|
| `^` | Exponentiation | Highest |
| `-` (unary) | Negation | |
| `*`, `/` | Multiply, Divide | |
| `\` | Integer division | |
| `MOD` | Modulus | |
| `+`, `-` | Add, Subtract | |
| `=`, `<>`, `<`, `>`, `<=`, `>=` | Comparison | |
| `NOT` | Bitwise NOT | |
| `AND` | Bitwise AND | |
| `OR` | Bitwise OR | |
| `XOR` | Bitwise XOR | |
| `EQV` | Equivalence | |
| `IMP` | Implication | Lowest |

---

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 1 | NF | NEXT without FOR |
| 2 | SN | Syntax error |
| 3 | RG | RETURN without GOSUB |
| 4 | OD | Out of DATA |
| 5 | FC | Illegal function call |
| 6 | OV | Overflow |
| 7 | OM | Out of memory |
| 8 | UL | Undefined line |
| 9 | BS | Subscript out of range |
| 10 | DD | Redimensioned array |
| 11 | /0 | Division by zero |
| 12 | ID | Illegal direct |
| 13 | TM | Type mismatch |
| 15 | LS | String too long |
| 17 | CN | Can't continue |
| 18 | UF | Undefined user function |
| 19 | NR | No RESUME |
| 20 | RE | RESUME without error |
| 52 | BN | Bad file number |
| 53 | FF | File not found |
| 54 | BM | Bad file mode |
| 55 | AO | File already open |
| 57 | IO | Disk I/O error |
| 62 | IE | Input past end |
| 64 | BF | Bad file name |
