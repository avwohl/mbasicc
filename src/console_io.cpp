// MBASICC - MBASIC 5.21 C++ Interpreter
// https://github.com/avwohl/mbasicc
//
// ConsoleIO Implementation - Standard console I/O using std::cin/std::cout

#include "mbasic/io_handler.hpp"
#include <iostream>

namespace mbasic {

void ConsoleIO::print(const std::string& text) {
    std::cout << text;
    // Update column position
    for (char c : text) {
        if (c == '\n') {
            column_ = 0;
        } else if (c == '\t') {
            column_ = ((column_ / 14) + 1) * 14;  // Tab zones (14 columns)
        } else {
            column_++;
        }
    }
    std::cout.flush();
}

std::string ConsoleIO::input(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    std::string line;
    std::getline(std::cin, line);
    column_ = 0;
    return line;
}

std::optional<char> ConsoleIO::inkey() {
    // Non-blocking input is platform-specific
    // On POSIX systems, this would require termios manipulation
    // On Windows, _kbhit() and _getch()
    // For portability, return nothing by default
    // Custom IOHandler implementations can provide platform-specific versions
    return std::nullopt;
}

} // namespace mbasic
