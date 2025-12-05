#pragma once

#include <vector>
#include <string>
#include <optional>
#include "tokens.hpp"

namespace mbasic {

class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Tokenize the entire source
    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;

    // Character access
    char current() const;
    char peek(int offset = 1) const;
    char advance();
    bool at_end() const;

    // Whitespace handling
    void skip_whitespace();

    // Token readers
    Token read_number();
    Token read_string();
    Token read_identifier();
    Token read_line_number();
    std::string read_comment();

    // Helper to lowercase a string
    static std::string to_lower(const std::string& s);
};

// Convenience function
std::vector<Token> tokenize(const std::string& source);

} // namespace mbasic
