#include "mbasic/tokens.hpp"

namespace mbasic {

// Static keyword table
static const std::unordered_map<std::string, TokenType> keywords = {
    // Program control
    {"auto", TokenType::AUTO},
    {"cont", TokenType::CONT},
    {"delete", TokenType::DELETE},
    {"edit", TokenType::EDIT},
    {"files", TokenType::FILES},
    {"list", TokenType::LIST},
    {"llist", TokenType::LLIST},
    {"load", TokenType::LOAD},
    {"merge", TokenType::MERGE},
    {"new", TokenType::NEW},
    {"renum", TokenType::RENUM},
    {"run", TokenType::RUN},
    {"save", TokenType::SAVE},

    // File operations
    {"as", TokenType::AS},
    {"close", TokenType::CLOSE},
    {"field", TokenType::FIELD},
    {"get", TokenType::GET},
    {"input", TokenType::INPUT},
    {"kill", TokenType::KILL},
    {"line", TokenType::LINE_INPUT},
    {"lset", TokenType::LSET},
    {"name", TokenType::NAME},
    {"open", TokenType::OPEN},
    {"output", TokenType::OUTPUT},
    {"put", TokenType::PUT},
    {"reset", TokenType::RESET},
    {"rset", TokenType::RSET},
    {"append", TokenType::APPEND},

    // Control flow
    {"all", TokenType::ALL},
    {"call", TokenType::CALL},
    {"chain", TokenType::CHAIN},
    {"else", TokenType::ELSE},
    {"end", TokenType::END},
    {"for", TokenType::FOR},
    {"gosub", TokenType::GOSUB},
    {"goto", TokenType::GOTO},
    {"if", TokenType::IF},
    {"next", TokenType::NEXT},
    {"on", TokenType::ON},
    {"resume", TokenType::RESUME},
    {"return", TokenType::RETURN},
    {"step", TokenType::STEP},
    {"stop", TokenType::STOP},
    {"system", TokenType::SYSTEM},
    {"cls", TokenType::CLS},
    {"then", TokenType::THEN},
    {"to", TokenType::TO},
    {"while", TokenType::WHILE},
    {"wend", TokenType::WEND},

    // Data/Arrays
    {"base", TokenType::BASE},
    {"clear", TokenType::CLEAR},
    {"common", TokenType::COMMON},
    {"data", TokenType::DATA},
    {"def", TokenType::DEF},
    {"defint", TokenType::DEFINT},
    {"defsng", TokenType::DEFSNG},
    {"defdbl", TokenType::DEFDBL},
    {"defstr", TokenType::DEFSTR},
    {"dim", TokenType::DIM},
    {"erase", TokenType::ERASE},
    {"fn", TokenType::FN},
    {"let", TokenType::LET},
    {"option", TokenType::OPTION},
    {"read", TokenType::READ},
    {"restore", TokenType::RESTORE},

    // I/O
    {"print", TokenType::PRINT},
    {"lprint", TokenType::LPRINT},
    {"write", TokenType::WRITE},
    {"using", TokenType::USING},

    // Other
    {"error", TokenType::ERROR},
    {"err", TokenType::ERR},
    {"erl", TokenType::ERL},
    {"fre", TokenType::FRE},
    {"help", TokenType::HELP},
    {"out", TokenType::OUT},
    {"poke", TokenType::POKE},
    {"randomize", TokenType::RANDOMIZE},
    {"rem", TokenType::REM},
    {"remark", TokenType::REMARK},
    {"swap", TokenType::SWAP},
    {"tron", TokenType::TRON},
    {"troff", TokenType::TROFF},
    {"wait", TokenType::WAIT},
    {"width", TokenType::WIDTH},

    // Operators
    {"mod", TokenType::MOD},
    {"not", TokenType::NOT},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"xor", TokenType::XOR},
    {"eqv", TokenType::EQV},
    {"imp", TokenType::IMP},

    // Numeric functions
    {"abs", TokenType::ABS},
    {"atn", TokenType::ATN},
    {"cdbl", TokenType::CDBL},
    {"cint", TokenType::CINT},
    {"cos", TokenType::COS},
    {"csng", TokenType::CSNG},
    {"cvd", TokenType::CVD},
    {"cvi", TokenType::CVI},
    {"cvs", TokenType::CVS},
    {"exp", TokenType::EXP},
    {"fix", TokenType::FIX},
    {"int", TokenType::INT},
    {"log", TokenType::LOG},
    {"rnd", TokenType::RND},
    {"sgn", TokenType::SGN},
    {"sin", TokenType::SIN},
    {"sqr", TokenType::SQR},
    {"tan", TokenType::TAN},

    // String functions (with $ suffix)
    {"asc", TokenType::ASC},
    {"chr$", TokenType::CHR},
    {"hex$", TokenType::HEX},
    {"inkey$", TokenType::INKEY},
    {"input$", TokenType::INPUT_FUNC},
    {"instr", TokenType::INSTR},
    {"left$", TokenType::LEFT},
    {"len", TokenType::LEN},
    {"mid$", TokenType::MID},
    {"mkd$", TokenType::MKD},
    {"mki$", TokenType::MKI},
    {"mks$", TokenType::MKS},
    {"oct$", TokenType::OCT},
    {"right$", TokenType::RIGHT},
    {"space$", TokenType::SPACE},
    {"str$", TokenType::STR},
    {"string$", TokenType::STRING_FUNC},
    {"val", TokenType::VAL},

    // Other functions
    {"date$", TokenType::DATE_FUNC},
    {"eof", TokenType::EOF_FUNC},
    {"environ$", TokenType::ENVIRON_FUNC},
    {"error$", TokenType::ERROR_FUNC},
    {"inp", TokenType::INP},
    {"loc", TokenType::LOC},
    {"lof", TokenType::LOF},
    {"lpos", TokenType::LPOS},
    {"peek", TokenType::PEEK},
    {"pos", TokenType::POS},
    {"spc", TokenType::SPC},
    {"tab", TokenType::TAB},
    {"time$", TokenType::TIME_FUNC},
    {"timer", TokenType::TIMER},
    {"usr", TokenType::USR},
    {"varptr", TokenType::VARPTR},
};

const std::unordered_map<std::string, TokenType>& get_keywords() {
    return keywords;
}

bool is_keyword(const std::string& s) {
    return keywords.find(s) != keywords.end();
}

TokenType keyword_type(const std::string& s) {
    auto it = keywords.find(s);
    return (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
}

std::string token_type_name(TokenType t) {
    switch (t) {
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::AUTO: return "AUTO";
        case TokenType::CONT: return "CONT";
        case TokenType::DELETE: return "DELETE";
        case TokenType::EDIT: return "EDIT";
        case TokenType::FILES: return "FILES";
        case TokenType::LIST: return "LIST";
        case TokenType::LLIST: return "LLIST";
        case TokenType::LOAD: return "LOAD";
        case TokenType::MERGE: return "MERGE";
        case TokenType::NEW: return "NEW";
        case TokenType::RENUM: return "RENUM";
        case TokenType::RUN: return "RUN";
        case TokenType::SAVE: return "SAVE";
        case TokenType::AS: return "AS";
        case TokenType::CLOSE: return "CLOSE";
        case TokenType::FIELD: return "FIELD";
        case TokenType::GET: return "GET";
        case TokenType::INPUT: return "INPUT";
        case TokenType::KILL: return "KILL";
        case TokenType::LINE_INPUT: return "LINE";
        case TokenType::LSET: return "LSET";
        case TokenType::NAME: return "NAME";
        case TokenType::OPEN: return "OPEN";
        case TokenType::OUTPUT: return "OUTPUT";
        case TokenType::PUT: return "PUT";
        case TokenType::RESET: return "RESET";
        case TokenType::RSET: return "RSET";
        case TokenType::APPEND: return "APPEND";
        case TokenType::ALL: return "ALL";
        case TokenType::CALL: return "CALL";
        case TokenType::CHAIN: return "CHAIN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::END: return "END";
        case TokenType::FOR: return "FOR";
        case TokenType::GOSUB: return "GOSUB";
        case TokenType::GOTO: return "GOTO";
        case TokenType::IF: return "IF";
        case TokenType::NEXT: return "NEXT";
        case TokenType::ON: return "ON";
        case TokenType::RESUME: return "RESUME";
        case TokenType::RETURN: return "RETURN";
        case TokenType::STEP: return "STEP";
        case TokenType::STOP: return "STOP";
        case TokenType::SYSTEM: return "SYSTEM";
        case TokenType::CLS: return "CLS";
        case TokenType::THEN: return "THEN";
        case TokenType::TO: return "TO";
        case TokenType::WHILE: return "WHILE";
        case TokenType::WEND: return "WEND";
        case TokenType::BASE: return "BASE";
        case TokenType::CLEAR: return "CLEAR";
        case TokenType::COMMON: return "COMMON";
        case TokenType::DATA: return "DATA";
        case TokenType::DEF: return "DEF";
        case TokenType::DEFINT: return "DEFINT";
        case TokenType::DEFSNG: return "DEFSNG";
        case TokenType::DEFDBL: return "DEFDBL";
        case TokenType::DEFSTR: return "DEFSTR";
        case TokenType::DIM: return "DIM";
        case TokenType::ERASE: return "ERASE";
        case TokenType::FN: return "FN";
        case TokenType::LET: return "LET";
        case TokenType::OPTION: return "OPTION";
        case TokenType::READ: return "READ";
        case TokenType::RESTORE: return "RESTORE";
        case TokenType::PRINT: return "PRINT";
        case TokenType::LPRINT: return "LPRINT";
        case TokenType::WRITE: return "WRITE";
        case TokenType::USING: return "USING";
        case TokenType::ERROR: return "ERROR";
        case TokenType::ERR: return "ERR";
        case TokenType::ERL: return "ERL";
        case TokenType::FRE: return "FRE";
        case TokenType::HELP: return "HELP";
        case TokenType::OUT: return "OUT";
        case TokenType::POKE: return "POKE";
        case TokenType::RANDOMIZE: return "RANDOMIZE";
        case TokenType::REM: return "REM";
        case TokenType::REMARK: return "REMARK";
        case TokenType::SWAP: return "SWAP";
        case TokenType::TRON: return "TRON";
        case TokenType::TROFF: return "TROFF";
        case TokenType::WAIT: return "WAIT";
        case TokenType::WIDTH: return "WIDTH";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::POWER: return "POWER";
        case TokenType::BACKSLASH: return "BACKSLASH";
        case TokenType::MOD: return "MOD";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS_THAN: return "LESS_THAN";
        case TokenType::GREATER_THAN: return "GREATER_THAN";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::NOT: return "NOT";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::XOR: return "XOR";
        case TokenType::EQV: return "EQV";
        case TokenType::IMP: return "IMP";
        case TokenType::ABS: return "ABS";
        case TokenType::ATN: return "ATN";
        case TokenType::CDBL: return "CDBL";
        case TokenType::CINT: return "CINT";
        case TokenType::COS: return "COS";
        case TokenType::CSNG: return "CSNG";
        case TokenType::CVD: return "CVD";
        case TokenType::CVI: return "CVI";
        case TokenType::CVS: return "CVS";
        case TokenType::EXP: return "EXP";
        case TokenType::FIX: return "FIX";
        case TokenType::INT: return "INT";
        case TokenType::LOG: return "LOG";
        case TokenType::RND: return "RND";
        case TokenType::SGN: return "SGN";
        case TokenType::SIN: return "SIN";
        case TokenType::SQR: return "SQR";
        case TokenType::TAN: return "TAN";
        case TokenType::ASC: return "ASC";
        case TokenType::CHR: return "CHR$";
        case TokenType::HEX: return "HEX$";
        case TokenType::INKEY: return "INKEY$";
        case TokenType::INPUT_FUNC: return "INPUT$";
        case TokenType::INSTR: return "INSTR";
        case TokenType::LEFT: return "LEFT$";
        case TokenType::LEN: return "LEN";
        case TokenType::MID: return "MID$";
        case TokenType::MKD: return "MKD$";
        case TokenType::MKI: return "MKI$";
        case TokenType::MKS: return "MKS$";
        case TokenType::OCT: return "OCT$";
        case TokenType::RIGHT: return "RIGHT$";
        case TokenType::SPACE: return "SPACE$";
        case TokenType::STR: return "STR$";
        case TokenType::STRING_FUNC: return "STRING$";
        case TokenType::VAL: return "VAL";
        case TokenType::DATE_FUNC: return "DATE$";
        case TokenType::EOF_FUNC: return "EOF";
        case TokenType::ENVIRON_FUNC: return "ENVIRON$";
        case TokenType::ERROR_FUNC: return "ERROR$";
        case TokenType::INP: return "INP";
        case TokenType::LOC: return "LOC";
        case TokenType::LOF: return "LOF";
        case TokenType::LPOS: return "LPOS";
        case TokenType::PEEK: return "PEEK";
        case TokenType::POS: return "POS";
        case TokenType::SPC: return "SPC";
        case TokenType::TAB: return "TAB";
        case TokenType::TIME_FUNC: return "TIME$";
        case TokenType::TIMER: return "TIMER";
        case TokenType::USR: return "USR";
        case TokenType::VARPTR: return "VARPTR";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COLON: return "COLON";
        case TokenType::HASH: return "HASH";
        case TokenType::AMPERSAND: return "AMPERSAND";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::LINE_NUMBER: return "LINE_NUMBER";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::QUESTION: return "QUESTION";
        case TokenType::APOSTROPHE: return "APOSTROPHE";
        default: return "UNKNOWN";
    }
}

} // namespace mbasic
