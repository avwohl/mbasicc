#include "mbasic/lexer.hpp"
#include "mbasic/error.hpp"
#include <cctype>
#include <cstdlib>
#include <algorithm>

namespace mbasic {

Lexer::Lexer(const std::string& source) : source_(source) {}

char Lexer::current() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::peek(int offset) const {
    size_t p = pos_ + offset;
    if (p >= source_.size()) return '\0';
    return source_[p];
}

char Lexer::advance() {
    if (pos_ >= source_.size()) return '\0';
    char c = source_[pos_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::at_end() const {
    return pos_ >= source_.size();
}

void Lexer::skip_whitespace() {
    while (!at_end() && (current() == ' ' || current() == '\t')) {
        advance();
    }
}

std::string Lexer::to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

Token Lexer::read_number() {
    int start_line = line_;
    int start_col = column_;
    std::string num_str;

    // Check for octal/hex prefix
    if (current() == '&') {
        num_str += advance();
        char next = std::toupper(current());

        if (next == 'H') {
            // Hexadecimal
            num_str += advance();
            while (!at_end() && std::isxdigit(current())) {
                num_str += advance();
            }
            // Parse hex value
            long value = 0;
            if (num_str.size() > 2) {
                value = std::strtol(num_str.c_str() + 2, nullptr, 16);
            }
            return Token(TokenType::NUMBER, std::to_string(value), start_line, start_col);
        } else if (next == 'O') {
            // Octal with &O prefix
            num_str += advance();
            while (!at_end() && current() >= '0' && current() <= '7') {
                num_str += advance();
            }
            long value = 0;
            if (num_str.size() > 2) {
                value = std::strtol(num_str.c_str() + 2, nullptr, 8);
            }
            return Token(TokenType::NUMBER, std::to_string(value), start_line, start_col);
        } else if (current() >= '0' && current() <= '7') {
            // Octal with just & prefix
            while (!at_end() && current() >= '0' && current() <= '7') {
                num_str += advance();
            }
            long value = 0;
            if (num_str.size() > 1) {
                value = std::strtol(num_str.c_str() + 1, nullptr, 8);
            }
            return Token(TokenType::NUMBER, std::to_string(value), start_line, start_col);
        }
    }

    // Check for leading decimal point (.5 syntax)
    if (current() == '.' && std::isdigit(peek())) {
        num_str += advance();  // Consume '.'
        while (!at_end() && std::isdigit(current())) {
            num_str += advance();
        }
    } else {
        // Read digits before decimal point
        while (!at_end() && std::isdigit(current())) {
            num_str += advance();
        }

        // Check for decimal point
        if (current() == '.') {
            char next = peek();
            // Allow trailing dot or dot followed by digits
            if (next == '\0' || std::isdigit(next) || !std::isalnum(next)) {
                num_str += advance();  // Consume '.'
                while (!at_end() && std::isdigit(current())) {
                    num_str += advance();
                }
            }
        }
    }

    // Check for scientific notation (E or D)
    if (!at_end() && (std::toupper(current()) == 'E' || std::toupper(current()) == 'D')) {
        num_str += advance();
        // Optional sign
        if (current() == '+' || current() == '-') {
            num_str += advance();
        }
        // Exponent digits
        if (at_end() || !std::isdigit(current())) {
            throw LexerError("Invalid number format: " + num_str, start_line, start_col);
        }
        while (!at_end() && std::isdigit(current())) {
            num_str += advance();
        }
    }

    // Check for type suffix (! # %)
    if (!at_end() && (current() == '!' || current() == '#' || current() == '%')) {
        advance();  // Skip suffix, we don't store it
    }

    return Token(TokenType::NUMBER, num_str, start_line, start_col);
}

Token Lexer::read_string() {
    int start_line = line_;
    int start_col = column_;

    advance();  // Skip opening quote
    std::string str_val;

    while (!at_end() && current() != '"') {
        if (current() == '\n') {
            throw LexerError("Unterminated string", line_, column_);
        }
        str_val += advance();
    }

    if (at_end()) {
        throw LexerError("Unterminated string", start_line, start_col);
    }

    advance();  // Skip closing quote
    return Token(TokenType::STRING, str_val, start_line, start_col);
}

Token Lexer::read_identifier() {
    int start_line = line_;
    int start_col = column_;
    std::string ident;

    // First character must be a letter
    if (!at_end() && std::isalpha(current())) {
        ident += advance();
    } else {
        throw LexerError("Invalid identifier", start_line, start_col);
    }

    // Subsequent characters can be letters, digits, or periods
    while (!at_end()) {
        char c = current();
        if (std::isalnum(c) || c == '.') {
            ident += advance();
        } else if (c == '$' || c == '%' || c == '!' || c == '#') {
            // Type suffix - terminates identifier
            ident += advance();
            break;
        } else {
            break;
        }
    }

    std::string ident_lower = to_lower(ident);

    // Check if it's a keyword
    if (is_keyword(ident_lower)) {
        Token tok(keyword_type(ident_lower), ident_lower, start_line, start_col);
        tok.original_case = ident;
        return tok;
    }

    // Special case: File I/O keywords followed by # (e.g., PRINT#1)
    if (ident_lower.size() > 1 && ident_lower.back() == '#') {
        std::string without_hash = ident_lower.substr(0, ident_lower.size() - 1);
        if (without_hash == "print" || without_hash == "lprint" ||
            without_hash == "input" || without_hash == "write" ||
            without_hash == "field" || without_hash == "get" ||
            without_hash == "put" || without_hash == "close") {
            // Put the # back to be tokenized separately
            pos_--;
            column_--;
            Token tok(keyword_type(without_hash), without_hash, start_line, start_col);
            tok.original_case = ident.substr(0, ident.size() - 1);
            return tok;
        }
    }

    // Regular identifier
    Token tok(TokenType::IDENTIFIER, ident_lower, start_line, start_col);
    tok.original_case = ident;
    return tok;
}

Token Lexer::read_line_number() {
    int start_line = line_;
    int start_col = column_;
    std::string num_str;

    while (!at_end() && std::isdigit(current())) {
        num_str += advance();
    }

    long line_num = std::strtol(num_str.c_str(), nullptr, 10);
    if (line_num > 65529) {
        throw LexerError("Line number " + num_str + " exceeds maximum of 65529",
                         start_line, start_col);
    }

    return Token(TokenType::LINE_NUMBER, num_str, start_line, start_col);
}

std::string Lexer::read_comment() {
    std::string comment;
    while (!at_end() && current() != '\n') {
        comment += advance();
    }
    // Trim leading/trailing whitespace
    size_t start = comment.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = comment.find_last_not_of(" \t");
    return comment.substr(start, end - start + 1);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    bool at_line_start = true;

    while (!at_end()) {
        skip_whitespace();
        if (at_end()) break;

        int start_line = line_;
        int start_col = column_;
        char c = current();

        // Line number at start of line
        if (at_line_start && std::isdigit(c)) {
            tokens.push_back(read_line_number());
            at_line_start = false;
            continue;
        }

        // Newline
        if (c == '\n') {
            tokens.push_back(Token(TokenType::NEWLINE, "\n", start_line, start_col));
            advance();
            if (!at_end() && current() == '\r') advance();
            at_line_start = true;
            continue;
        }

        if (c == '\r') {
            tokens.push_back(Token(TokenType::NEWLINE, "\r", start_line, start_col));
            advance();
            if (!at_end() && current() == '\n') advance();
            at_line_start = true;
            continue;
        }

        // Apostrophe comment
        if (c == '\'') {
            advance();
            std::string comment = read_comment();
            tokens.push_back(Token(TokenType::APOSTROPHE, comment, start_line, start_col));
            continue;
        }

        // Numbers (including &H hex, &O octal, .5 leading decimal)
        if (std::isdigit(c) ||
            (c == '&' && (std::toupper(peek()) == 'H' ||
                         std::toupper(peek()) == 'O' ||
                         std::isdigit(peek()))) ||
            (c == '.' && std::isdigit(peek()))) {
            tokens.push_back(read_number());
            at_line_start = false;
            continue;
        }

        // Strings
        if (c == '"') {
            tokens.push_back(read_string());
            at_line_start = false;
            continue;
        }

        // Identifiers and keywords
        if (std::isalpha(c)) {
            Token tok = read_identifier();
            // Special handling for REM/REMARK - read rest as comment
            if (tok.type == TokenType::REM || tok.type == TokenType::REMARK) {
                std::string comment = read_comment();
                tok.value = comment;
            }
            tokens.push_back(tok);
            at_line_start = false;
            continue;
        }

        // Operators and delimiters
        at_line_start = false;

        switch (c) {
            case '+':
                tokens.push_back(Token(TokenType::PLUS, "+", start_line, start_col));
                advance();
                break;
            case '-':
                tokens.push_back(Token(TokenType::MINUS, "-", start_line, start_col));
                advance();
                break;
            case '*':
                tokens.push_back(Token(TokenType::MULTIPLY, "*", start_line, start_col));
                advance();
                break;
            case '/':
                tokens.push_back(Token(TokenType::DIVIDE, "/", start_line, start_col));
                advance();
                break;
            case '^':
                tokens.push_back(Token(TokenType::POWER, "^", start_line, start_col));
                advance();
                break;
            case '\\':
                tokens.push_back(Token(TokenType::BACKSLASH, "\\", start_line, start_col));
                advance();
                break;
            case '=':
                tokens.push_back(Token(TokenType::EQUAL, "=", start_line, start_col));
                advance();
                break;
            case '<':
                advance();
                if (current() == '>') {
                    tokens.push_back(Token(TokenType::NOT_EQUAL, "<>", start_line, start_col));
                    advance();
                } else if (current() == '=') {
                    tokens.push_back(Token(TokenType::LESS_EQUAL, "<=", start_line, start_col));
                    advance();
                } else {
                    tokens.push_back(Token(TokenType::LESS_THAN, "<", start_line, start_col));
                }
                break;
            case '>':
                advance();
                if (current() == '<') {
                    tokens.push_back(Token(TokenType::NOT_EQUAL, "><", start_line, start_col));
                    advance();
                } else if (current() == '=') {
                    tokens.push_back(Token(TokenType::GREATER_EQUAL, ">=", start_line, start_col));
                    advance();
                } else {
                    tokens.push_back(Token(TokenType::GREATER_THAN, ">", start_line, start_col));
                }
                break;
            case '(':
                tokens.push_back(Token(TokenType::LPAREN, "(", start_line, start_col));
                advance();
                break;
            case ')':
                tokens.push_back(Token(TokenType::RPAREN, ")", start_line, start_col));
                advance();
                break;
            case ',':
                tokens.push_back(Token(TokenType::COMMA, ",", start_line, start_col));
                advance();
                break;
            case ';':
                tokens.push_back(Token(TokenType::SEMICOLON, ";", start_line, start_col));
                advance();
                break;
            case ':':
                tokens.push_back(Token(TokenType::COLON, ":", start_line, start_col));
                advance();
                break;
            case '?':
                tokens.push_back(Token(TokenType::QUESTION, "?", start_line, start_col));
                advance();
                break;
            case '#':
                tokens.push_back(Token(TokenType::HASH, "#", start_line, start_col));
                advance();
                break;
            case '&':
                // Standalone & (not hex/octal prefix)
                tokens.push_back(Token(TokenType::AMPERSAND, "&", start_line, start_col));
                advance();
                break;
            default:
                // Skip control characters
                if (static_cast<unsigned char>(c) < 32 && c != '\t') {
                    advance();
                    continue;
                }
                throw LexerError(std::string("Unexpected character: '") + c + "'",
                                 start_line, start_col);
        }
    }

    // Add EOF token
    tokens.push_back(Token(TokenType::END_OF_FILE, "", line_, column_));
    return tokens;
}

std::vector<Token> tokenize(const std::string& source) {
    Lexer lexer(source);
    return lexer.tokenize();
}

} // namespace mbasic
