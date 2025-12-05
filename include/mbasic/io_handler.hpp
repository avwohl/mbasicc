#pragma once
// MBASICC - MBASIC 5.21 C++ Interpreter
// https://github.com/avwohl/mbasicc
//
// I/O Handler Abstraction
// This interface allows the interpreter to be portable across different
// platforms (console, WebAssembly, embedded systems, etc.)

#include <string>
#include <optional>

namespace mbasic {

// ============================================================================
// IOHandler - Abstract interface for console I/O
// ============================================================================
// Implement this interface to provide custom I/O for different platforms.
// The interpreter uses this for all console input/output operations.

class IOHandler {
public:
    virtual ~IOHandler() = default;

    // Output text to the console
    // Should handle newlines and update internal column tracking
    virtual void print(const std::string& text) = 0;

    // Read a line of input from the user
    // @param prompt: Optional prompt to display before reading
    // @return: The input line (without trailing newline)
    virtual std::string input(const std::string& prompt) = 0;

    // Non-blocking key check (for INKEY$ function)
    // @return: A character if one is available, nullopt otherwise
    virtual std::optional<char> inkey() = 0;

    // Get current column position (0-based)
    // Used by POS() function and TAB() spacing
    virtual int get_column() const = 0;

    // Set column position (for internal tracking after TAB)
    virtual void set_column(int col) = 0;

    // Get current print width (default 80)
    // Used by WIDTH statement
    virtual int get_width() const = 0;

    // Set print width
    virtual void set_width(int w) = 0;

    // Clear the screen (CLS command)
    // Default implementation outputs ANSI escape sequence
    virtual void clear_screen() {
        print("\033[2J\033[H");
    }
};

// ============================================================================
// ConsoleIO - Standard console implementation using std::cin/std::cout
// ============================================================================
// This is the default implementation for terminal/console applications.
// For WebAssembly or other platforms, provide a custom IOHandler.

class ConsoleIO : public IOHandler {
public:
    void print(const std::string& text) override;
    std::string input(const std::string& prompt) override;
    std::optional<char> inkey() override;
    int get_column() const override { return column_; }
    void set_column(int col) override { column_ = col; }
    int get_width() const override { return width_; }
    void set_width(int w) override { width_ = w; }

private:
    int column_ = 0;
    int width_ = 80;
};

} // namespace mbasic
