# MBASICC - MBASIC 5.21 C++ Interpreter

A modern C++ implementation of Microsoft BASIC-80 version 5.21.

**Repository:** https://github.com/avwohl/mbasicc

[![CI](https://github.com/avwohl/mbasicc/actions/workflows/ci.yml/badge.svg)](https://github.com/avwohl/mbasicc/actions/workflows/ci.yml)

## Installation

Download a .deb, .rpm or Linux binary tarball from the
[releases](https://github.com/avwohl/mbasicc/releases) page, or build from source:

```bash
sudo apt-get install build-essential libedit-dev
git clone https://github.com/avwohl/mbasicc.git
cd mbasicc
make
sudo cp mbasicc /usr/local/bin/
```

Source builds need a C++17 compiler, libedit and make. Fedora, macOS, builds
without editline, and the FreeDOS/DJGPP build are in
[docs/installation.md](docs/installation.md).

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

The REPL commands (NEW, RUN, LIST, LOAD, SAVE, RENUM and the rest) are listed in
[docs/reference.md](docs/reference.md#interactive-commands).

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

mbasicc covers the vast majority of MBASIC 5.21: flow control, all four data
types, math and string functions, arrays, DEF FN, sequential and random file
I/O, error trapping, PRINT USING, CHAIN/COMMON, and an interactive REPL.
Hardware-specific features (PEEK/POKE, INP/OUT, USR/CALL, cassette I/O) are
stubs. The full list is in [docs/reference.md](docs/reference.md).

## Documentation

- [docs/installation.md](docs/installation.md) - packages, source builds, no-editline build, FreeDOS/DJGPP
- [docs/reference.md](docs/reference.md) - implementation status, interactive commands, and every MBASIC 5.21 command, statement, function, operator and error code
- [docs/development.md](docs/development.md) - project structure, building `libmbasic.a`, running tests
- [ARCHITECTURE.md](ARCHITECTURE.md) - interpreter design
- [CHANGELOG.md](CHANGELOG.md) - release history
- `man mbasicc` - the man page (`man/mbasicc.1`)

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

## Related Projects

- [80un](https://github.com/avwohl/80un) - Unpacker for the CP/M archive and compression formats LBR, ARC, squeeze, crunch, and CrLZH.
- [cpmdroid](https://github.com/avwohl/cpmdroid) - Z80/CP/M emulator for Android phones and tablets. It emulates the RomWBW HBIOS interface and a VT100 terminal.
- [cpmemu](https://github.com/avwohl/cpmemu) - Z80/CP/M emulator for Linux and Windows, with Z80 and 8080 CPU cores. It translates the BDOS and BIOS calls of CP/M 2.2 programs to the host file system.
- [ioscpm](https://github.com/avwohl/ioscpm) - Z80/CP/M emulator for iOS and macOS. It emulates the RomWBW HBIOS interface and runs CP/M 2.2 and CP/M 3.
- [learn-ada-z80](https://github.com/avwohl/learn-ada-z80) - Collection of more than 90 Ada example programs for uada80, the Ada compiler for the Z80 processor and CP/M.
- [mbasic](https://github.com/avwohl/mbasic) - Python interpreter for MBASIC 5.21, the Microsoft BASIC-80 for CP/M. Two compiler backends compile the programs to CP/M .COM files or to JavaScript.
- [mbasic2025](https://github.com/avwohl/mbasic2025) - Reconstruction of the lost source code of MBASIC 5.21, the Microsoft BASIC-80 for CP/M. The MACRO-80 source code assembles to a binary that matches mbasic.com byte for byte.
- [mbasicc_web](https://github.com/avwohl/mbasicc_web) - Web browser interpreter for MBASIC 5.21, the Microsoft BASIC-80 for CP/M. Emscripten compiles the mbasicc interpreter to WebAssembly.
- [mpm2](https://github.com/avwohl/mpm2) - Z80 emulator for MP/M II, the multi-user CP/M operating system. Users connect over SSH, and SFTP clients transfer files.
- [romwbw_emu](https://github.com/avwohl/romwbw_emu) - Hardware-level Z80/CP/M emulator for Linux and macOS. It emulates the RomWBW HBIOS interface and switches banks in 512 KB of ROM and 512 KB of RAM.
- [scelbal](https://github.com/avwohl/scelbal) - Floating-point BASIC interpreter for the 8080 processor and CP/M. A translator converts the original 8008 source code to 8080 source code.
- [uada80](https://github.com/avwohl/uada80) - Ada compiler for the Z80 processor and CP/M 2.2. It compiles a subset of Ada 2012 to CP/M .COM files.
- [uc80](https://github.com/avwohl/uc80) - C compiler for the Z80 processor and CP/M. It optimizes for small code size.
- [ucow](https://github.com/avwohl/ucow) - Cowgol compiler for the Z80 processor and CP/M. It runs on Linux in Python.
- [um80_and_friends](https://github.com/avwohl/um80_and_friends) - Linux toolchain that is compatible with Microsoft MACRO-80. It has an assembler, a linker, a librarian, and a disassembler.
- [upeepz80](https://github.com/avwohl/upeepz80) - Peephole optimizer for Z80 compilers that write lowercase Z80 assembly language. It shortens jumps to jr, builds djnz loops, and removes dead stores.
- [uplm80](https://github.com/avwohl/uplm80) - PL/M-80 compiler for the Z80 processor and CP/M. It writes Intel 8080 and Zilog Z80 assembly language.
- [z80cpmw](https://github.com/avwohl/z80cpmw) - Z80/CP/M emulator for Windows. It emulates the RomWBW HBIOS interface and boots CP/M from disk images.
