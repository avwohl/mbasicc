# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2024-XX-XX

### Added
- Complete MBASIC 5.21 interpreter implementation in C++17
- Interactive REPL with line editing and command history (via libedit)
- All standard MBASIC statements: FOR/NEXT, WHILE/WEND, IF/THEN/ELSE, GOSUB/RETURN, etc.
- All data types: INTEGER (%), SINGLE (!), DOUBLE (#), STRING ($)
- All arithmetic operators including MOD, \\ (integer division), ^ (exponentiation)
- All logical operators: AND, OR, NOT, XOR, EQV, IMP
- All comparison operators with floating-point tolerance
- All standard math functions: SIN, COS, TAN, ATN, LOG, EXP, SQR, ABS, SGN, INT, FIX, RND
- All string functions: LEFT$, RIGHT$, MID$, INSTR, CHR$, ASC, LEN, STR$, VAL, etc.
- User-defined functions with DEF FN
- Array support with DIM, ERASE, multi-dimensional arrays, OPTION BASE
- Sequential file I/O: OPEN, CLOSE, PRINT#, INPUT#, LINE INPUT#, WRITE#
- Random access file I/O: FIELD, LSET, RSET, GET, PUT
- Binary data conversion: MKI$, MKS$, MKD$, CVI, CVS, CVD
- Error handling: ON ERROR GOTO, RESUME, ERR, ERL
- PRINT USING formatted output with numeric and string formats
- DATA/READ/RESTORE for inline data
- CHAIN and COMMON for program chaining
- REPL commands: NEW, RUN, LIST, LOAD, SAVE, FILES, EDIT, DELETE, RENUM, AUTO, MERGE, etc.
- Trace mode with TRON/TROFF
- System functions: DATE$, TIME$, TIMER, ENVIRON$

### Changed
- Isolated editline/readline dependency to single file (src/readline.cpp) for portability

### Fixed
- Floating-point comparison now uses epsilon tolerance for reliable equality checks
- Compiler warnings eliminated with proper [[maybe_unused]] annotations

### Not Implemented (by design)
- Hardware-specific features: PEEK, POKE, INP, OUT, WAIT, USR, CALL, VARPTR
- Cassette tape I/O: CLOAD, CSAVE
- These are implemented as stubs returning 0 or doing nothing
