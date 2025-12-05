#pragma once

#include <stdexcept>
#include <string>

namespace mbasic {

// Base class for all MBASIC errors
class MBasicError : public std::runtime_error {
public:
    int line;
    int column;

    MBasicError(const std::string& msg, int l = 0, int c = 0)
        : std::runtime_error(msg), line(l), column(c) {}
};

// Lexer errors
class LexerError : public MBasicError {
public:
    LexerError(const std::string& msg, int l, int c)
        : MBasicError("Lexer error at " + std::to_string(l) + ":" +
                      std::to_string(c) + ": " + msg, l, c) {}
};

// Parser errors
class ParseError : public MBasicError {
public:
    ParseError(const std::string& msg, int l, int c)
        : MBasicError("Syntax error at " + std::to_string(l) + ":" +
                      std::to_string(c) + ": " + msg, l, c) {}
};

// Runtime errors with MBASIC error codes
class RuntimeError : public MBasicError {
public:
    int error_code;

    RuntimeError(int code, const std::string& msg, int l = 0)
        : MBasicError(msg, l, 0), error_code(code) {}
};

// Standard MBASIC error codes
namespace ErrorCode {
    constexpr int NEXT_WITHOUT_FOR = 1;
    constexpr int SYNTAX_ERROR = 2;
    constexpr int RETURN_WITHOUT_GOSUB = 3;
    constexpr int OUT_OF_DATA = 4;
    constexpr int ILLEGAL_FUNCTION_CALL = 5;
    constexpr int OVERFLOW_ERROR = 6;
    constexpr int OUT_OF_MEMORY = 7;
    constexpr int UNDEFINED_LINE = 8;
    constexpr int SUBSCRIPT_OUT_OF_RANGE = 9;
    constexpr int DUPLICATE_DEFINITION = 10;
    constexpr int DIVISION_BY_ZERO = 11;
    constexpr int ILLEGAL_DIRECT = 12;
    constexpr int TYPE_MISMATCH = 13;
    constexpr int OUT_OF_STRING_SPACE = 14;
    constexpr int STRING_TOO_LONG = 15;
    constexpr int STRING_FORMULA_TOO_COMPLEX = 16;
    constexpr int CANT_CONTINUE = 17;
    constexpr int UNDEFINED_USER_FUNCTION = 18;
    constexpr int NO_RESUME = 19;
    constexpr int RESUME_WITHOUT_ERROR = 20;
    constexpr int MISSING_OPERAND = 22;
    constexpr int LINE_BUFFER_OVERFLOW = 23;
    constexpr int FOR_WITHOUT_NEXT = 26;
    constexpr int WHILE_WITHOUT_WEND = 29;
    constexpr int WEND_WITHOUT_WHILE = 30;
    // File I/O errors 50-69
    constexpr int FILE_NOT_FOUND = 53;
    constexpr int FILE_ALREADY_OPEN = 55;
    constexpr int DISK_FULL = 61;
    constexpr int INPUT_PAST_END = 62;
    constexpr int BAD_FILE_NUMBER = 52;
    constexpr int BAD_FILE_MODE = 54;
    constexpr int BAD_RECORD_NUMBER = 63;
    constexpr int BAD_FILE_NAME = 64;
    constexpr int DIRECT_STATEMENT_IN_FILE = 66;
    constexpr int TOO_MANY_FILES = 67;
    constexpr int FIELD_OVERFLOW = 50;
    constexpr int INTERNAL_ERROR = 51;
    constexpr int DISK_IO_ERROR = 57;
    constexpr int FILE_ALREADY_EXISTS = 58;
}

// Get error message for error code
inline std::string error_message(int code) {
    switch (code) {
        case ErrorCode::NEXT_WITHOUT_FOR: return "NEXT without FOR";
        case ErrorCode::SYNTAX_ERROR: return "Syntax error";
        case ErrorCode::RETURN_WITHOUT_GOSUB: return "RETURN without GOSUB";
        case ErrorCode::OUT_OF_DATA: return "Out of DATA";
        case ErrorCode::ILLEGAL_FUNCTION_CALL: return "Illegal function call";
        case ErrorCode::OVERFLOW_ERROR: return "Overflow";
        case ErrorCode::OUT_OF_MEMORY: return "Out of memory";
        case ErrorCode::UNDEFINED_LINE: return "Undefined line number";
        case ErrorCode::SUBSCRIPT_OUT_OF_RANGE: return "Subscript out of range";
        case ErrorCode::DUPLICATE_DEFINITION: return "Duplicate definition";
        case ErrorCode::DIVISION_BY_ZERO: return "Division by zero";
        case ErrorCode::ILLEGAL_DIRECT: return "Illegal direct";
        case ErrorCode::TYPE_MISMATCH: return "Type mismatch";
        case ErrorCode::OUT_OF_STRING_SPACE: return "Out of string space";
        case ErrorCode::STRING_TOO_LONG: return "String too long";
        case ErrorCode::STRING_FORMULA_TOO_COMPLEX: return "String formula too complex";
        case ErrorCode::CANT_CONTINUE: return "Can't continue";
        case ErrorCode::UNDEFINED_USER_FUNCTION: return "Undefined user function";
        case ErrorCode::NO_RESUME: return "No RESUME";
        case ErrorCode::RESUME_WITHOUT_ERROR: return "RESUME without error";
        case ErrorCode::MISSING_OPERAND: return "Missing operand";
        case ErrorCode::LINE_BUFFER_OVERFLOW: return "Line buffer overflow";
        case ErrorCode::FOR_WITHOUT_NEXT: return "FOR without NEXT";
        case ErrorCode::WHILE_WITHOUT_WEND: return "WHILE without WEND";
        case ErrorCode::WEND_WITHOUT_WHILE: return "WEND without WHILE";
        case ErrorCode::FILE_NOT_FOUND: return "File not found";
        case ErrorCode::FILE_ALREADY_OPEN: return "File already open";
        case ErrorCode::DISK_FULL: return "Disk full";
        case ErrorCode::INPUT_PAST_END: return "Input past end";
        case ErrorCode::BAD_FILE_NUMBER: return "Bad file number";
        case ErrorCode::BAD_FILE_MODE: return "Bad file mode";
        case ErrorCode::BAD_RECORD_NUMBER: return "Bad record number";
        case ErrorCode::BAD_FILE_NAME: return "Bad file name";
        case ErrorCode::DIRECT_STATEMENT_IN_FILE: return "Direct statement in file";
        case ErrorCode::TOO_MANY_FILES: return "Too many files";
        case ErrorCode::FIELD_OVERFLOW: return "Field overflow";
        case ErrorCode::INTERNAL_ERROR: return "Internal error";
        case ErrorCode::DISK_IO_ERROR: return "Disk I/O error";
        case ErrorCode::FILE_ALREADY_EXISTS: return "File already exists";
        default: return "Unknown error";
    }
}

} // namespace mbasic
