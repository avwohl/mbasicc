#pragma once

#include <string>
#include <unordered_map>

namespace mbasic {

enum class TokenType {
    // Literals
    NUMBER,
    STRING,

    // Identifiers
    IDENTIFIER,

    // Keywords - Program Control
    AUTO,
    CONT,
    DELETE,
    EDIT,
    FILES,
    LIST,
    LLIST,
    LOAD,
    MERGE,
    NEW,
    RENUM,
    RUN,
    SAVE,

    // Keywords - File Operations
    AS,
    CLOSE,
    FIELD,
    GET,
    INPUT,
    KILL,
    LINE_INPUT,
    LSET,
    NAME,
    OPEN,
    OUTPUT,
    PUT,
    RESET,
    RSET,
    APPEND,

    // Keywords - Control Flow
    ALL,
    CALL,
    CHAIN,
    ELSE,
    END,
    FOR,
    GOSUB,
    GOTO,
    IF,
    NEXT,
    ON,
    RESUME,
    RETURN,
    STEP,
    STOP,
    SYSTEM,
    CLS,
    THEN,
    TO,
    WHILE,
    WEND,

    // Keywords - Data/Arrays
    BASE,
    CLEAR,
    COMMON,
    DATA,
    DEF,
    DEFINT,
    DEFSNG,
    DEFDBL,
    DEFSTR,
    DIM,
    ERASE,
    FN,
    LET,
    OPTION,
    READ,
    RESTORE,

    // Keywords - I/O
    PRINT,
    LPRINT,
    WRITE,
    USING,

    // Keywords - Other
    ERROR,
    ERR,
    ERL,
    FRE,
    HELP,
    OUT,
    POKE,
    RANDOMIZE,
    REM,
    REMARK,
    SWAP,
    TRON,
    TROFF,
    WAIT,
    WIDTH,

    // Operators - Arithmetic
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    POWER,
    BACKSLASH,  // Integer division
    MOD,

    // Operators - Relational
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,

    // Operators - Logical/Bitwise
    NOT,
    AND,
    OR,
    XOR,
    EQV,
    IMP,

    // Built-in Functions - Numeric
    ABS,
    ATN,
    CDBL,
    CINT,
    COS,
    CSNG,
    CVD,
    CVI,
    CVS,
    EXP,
    FIX,
    INT,
    LOG,
    RND,
    SGN,
    SIN,
    SQR,
    TAN,

    // Built-in Functions - String
    ASC,
    CHR,
    HEX,
    INKEY,
    INPUT_FUNC,
    INSTR,
    LEFT,
    LEN,
    MID,
    MKD,
    MKI,
    MKS,
    OCT,
    RIGHT,
    SPACE,
    STR,
    STRING_FUNC,
    VAL,

    // Built-in Functions - Other
    DATE_FUNC,
    EOF_FUNC,
    ENVIRON_FUNC,
    ERROR_FUNC,
    INP,
    LOC,
    LOF,
    LPOS,
    PEEK,
    POS,
    SPC,
    TAB,
    TIME_FUNC,
    TIMER,
    USR,
    VARPTR,

    // Delimiters
    LPAREN,
    RPAREN,
    COMMA,
    SEMICOLON,
    COLON,
    HASH,
    AMPERSAND,

    // Special
    NEWLINE,
    LINE_NUMBER,
    END_OF_FILE,
    QUESTION,
    APOSTROPHE
};

struct Token {
    TokenType type;
    std::string value;       // Normalized value (lowercase for identifiers/keywords)
    int line;
    int column;
    std::string original_case;  // Original case for identifiers

    Token() : type(TokenType::END_OF_FILE), line(0), column(0) {}

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), column(c) {}

    Token(TokenType t, std::string v, int l, int c, std::string orig)
        : type(t), value(std::move(v)), line(l), column(c), original_case(std::move(orig)) {}
};

// Keyword lookup table (lowercase -> TokenType)
const std::unordered_map<std::string, TokenType>& get_keywords();

// Check if a string is a keyword
bool is_keyword(const std::string& s);

// Get TokenType for a keyword (returns IDENTIFIER if not found)
TokenType keyword_type(const std::string& s);

// Token type to string (for debugging)
std::string token_type_name(TokenType t);

} // namespace mbasic
