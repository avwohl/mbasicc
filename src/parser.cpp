#include "mbasic/parser.hpp"
#include "mbasic/lexer.hpp"
#include <algorithm>
#include <cstdlib>

namespace mbasic {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {
    // Initialize default types (all SINGLE)
    for (char c = 'a'; c <= 'z'; ++c) {
        def_type_map_[c] = VarType::SINGLE;
    }
}

// Token access helpers
const Token& Parser::current() const {
    if (pos_ >= tokens_.size()) {
        static Token eof(TokenType::END_OF_FILE, "", 0, 0);
        return eof;
    }
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {
    size_t p = pos_ + offset;
    if (p >= tokens_.size()) {
        static Token eof(TokenType::END_OF_FILE, "", 0, 0);
        return eof;
    }
    return tokens_[p];
}

Token Parser::advance() {
    if (pos_ < tokens_.size()) {
        return tokens_[pos_++];
    }
    return Token(TokenType::END_OF_FILE, "", 0, 0);
}

bool Parser::at_end() const {
    return pos_ >= tokens_.size() || current().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::check_any(std::initializer_list<TokenType> types) const {
    for (auto t : types) {
        if (check(t)) return true;
    }
    return false;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match_any(std::initializer_list<TokenType> types) {
    for (auto t : types) {
        if (match(t)) return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const std::string& msg) {
    if (check(type)) {
        return advance();
    }
    throw ParseError(msg, current().line, current().column);
}

void Parser::skip_to_eol() {
    while (!at_end() && !check(TokenType::NEWLINE) && !check(TokenType::COLON)) {
        advance();
    }
}

// First pass: collect DEF type statements
void Parser::collect_def_types() {
    size_t saved_pos = pos_;
    pos_ = 0;

    while (!at_end()) {
        // Skip to line numbers
        if (check(TokenType::LINE_NUMBER)) {
            advance();
        }

        // Look for DEFINT, DEFSNG, DEFDBL, DEFSTR
        if (check_any({TokenType::DEFINT, TokenType::DEFSNG, TokenType::DEFDBL, TokenType::DEFSTR})) {
            VarType type;
            switch (current().type) {
                case TokenType::DEFINT: type = VarType::INTEGER; break;
                case TokenType::DEFSNG: type = VarType::SINGLE; break;
                case TokenType::DEFDBL: type = VarType::DOUBLE; break;
                case TokenType::DEFSTR: type = VarType::STRING; break;
                default: type = VarType::SINGLE;
            }
            advance();

            // Parse letter ranges: A-Z, X, Y-Z, etc.
            do {
                if (check(TokenType::IDENTIFIER)) {
                    std::string id = current().value;
                    char start_letter = std::tolower(id[0]);
                    advance();

                    if (match(TokenType::MINUS)) {
                        // Range: A-Z
                        if (check(TokenType::IDENTIFIER)) {
                            char end_letter = std::tolower(current().value[0]);
                            advance();
                            for (char c = start_letter; c <= end_letter; ++c) {
                                def_type_map_[c] = type;
                            }
                        }
                    } else {
                        // Single letter
                        def_type_map_[start_letter] = type;
                    }
                } else {
                    break;
                }
            } while (match(TokenType::COMMA));
        }

        // Skip to next line
        while (!at_end() && !check(TokenType::NEWLINE)) {
            advance();
        }
        if (check(TokenType::NEWLINE)) {
            advance();
        }
    }

    pos_ = saved_pos;
}

VarType Parser::resolve_type(const std::string& name) {
    // Check for explicit type suffix
    if (!name.empty()) {
        char last = name.back();
        switch (last) {
            case '%': return VarType::INTEGER;
            case '!': return VarType::SINGLE;
            case '#': return VarType::DOUBLE;
            case '$': return VarType::STRING;
        }
    }

    // Use DEF type based on first letter
    if (!name.empty() && std::isalpha(name[0])) {
        char first = std::tolower(name[0]);
        auto it = def_type_map_.find(first);
        if (it != def_type_map_.end()) {
            return it->second;
        }
    }

    return VarType::SINGLE;  // Default
}

VarType Parser::type_from_name(const std::string& name) {
    return resolve_type(name);
}

Program Parser::parse() {
    // First pass: collect DEF type statements
    collect_def_types();

    Program program;
    program.def_type_map = def_type_map_;

    while (!at_end()) {
        // Skip empty lines
        while (match(TokenType::NEWLINE)) {}
        if (at_end()) break;

        try {
            Line line = parse_line();
            program.lines.push_back(std::move(line));
        } catch (const ParseError& e) {
            // Error recovery: skip to next line
            skip_to_eol();
            while (match(TokenType::NEWLINE)) {}
            throw;  // Re-throw for now
        }
    }

    return program;
}

Line Parser::parse_line() {
    Line line;

    // Expect line number
    if (!check(TokenType::LINE_NUMBER)) {
        throw ParseError("Expected line number", current().line, current().column);
    }

    line.line_number = std::stoi(current().value);
    advance();

    // Parse statements separated by colons
    do {
        // Skip empty statements (multiple colons)
        while (match(TokenType::COLON)) {}

        if (check(TokenType::NEWLINE) || at_end()) break;

        Stmt stmt = parse_statement();
        line.statements.push_back(std::move(stmt));

    } while (match(TokenType::COLON));

    // Handle trailing comment (apostrophe starts inline comment)
    if (check(TokenType::APOSTROPHE)) {
        advance();  // Skip the APOSTROPHE token (comment text is in its value)
    }

    // Expect end of line
    if (!at_end() && !check(TokenType::NEWLINE)) {
        throw ParseError("Expected end of line", current().line, current().column);
    }
    match(TokenType::NEWLINE);

    return line;
}

bool Parser::is_expression_start() const {
    TokenType t = current().type;
    return t == TokenType::NUMBER ||
           t == TokenType::STRING ||
           t == TokenType::IDENTIFIER ||
           t == TokenType::LPAREN ||
           t == TokenType::MINUS ||
           t == TokenType::PLUS ||
           t == TokenType::NOT ||
           // Functions
           t == TokenType::ABS || t == TokenType::ATN || t == TokenType::COS ||
           t == TokenType::EXP || t == TokenType::FIX || t == TokenType::INT ||
           t == TokenType::LOG || t == TokenType::RND || t == TokenType::SGN ||
           t == TokenType::SIN || t == TokenType::SQR || t == TokenType::TAN ||
           t == TokenType::CINT || t == TokenType::CSNG || t == TokenType::CDBL ||
           t == TokenType::ASC || t == TokenType::CHR || t == TokenType::HEX ||
           t == TokenType::LEFT || t == TokenType::LEN || t == TokenType::MID ||
           t == TokenType::OCT || t == TokenType::RIGHT || t == TokenType::SPACE ||
           t == TokenType::STR || t == TokenType::STRING_FUNC || t == TokenType::VAL ||
           t == TokenType::INSTR || t == TokenType::INKEY || t == TokenType::INPUT_FUNC ||
           t == TokenType::EOF_FUNC || t == TokenType::LOC || t == TokenType::LOF ||
           t == TokenType::PEEK || t == TokenType::POS || t == TokenType::TAB ||
           t == TokenType::SPC || t == TokenType::FRE || t == TokenType::USR ||
           t == TokenType::VARPTR || t == TokenType::CVI || t == TokenType::CVS ||
           t == TokenType::CVD || t == TokenType::MKI || t == TokenType::MKS ||
           t == TokenType::MKD || t == TokenType::ERR || t == TokenType::ERL ||
           t == TokenType::FN || t == TokenType::LPOS ||
           t == TokenType::TIMER || t == TokenType::DATE_FUNC ||
           t == TokenType::TIME_FUNC || t == TokenType::ENVIRON_FUNC ||
           t == TokenType::ERROR_FUNC;
}

Stmt Parser::parse_statement() {
    int start_line = current().line;
    int start_col = current().column;

    TokenType t = current().type;

    // Handle ? as PRINT shorthand
    if (t == TokenType::QUESTION) {
        advance();
        return parse_print();
    }

    switch (t) {
        case TokenType::PRINT: advance(); return parse_print();
        case TokenType::LPRINT: advance(); return parse_lprint();
        case TokenType::INPUT: advance(); return parse_input();
        case TokenType::LINE_INPUT: advance(); return parse_line_input();
        case TokenType::LET: advance(); return parse_let();
        case TokenType::IF: advance(); return parse_if();
        case TokenType::FOR: advance(); return parse_for();
        case TokenType::NEXT: advance(); return parse_next();
        case TokenType::WHILE: advance(); return parse_while();
        case TokenType::WEND: advance(); return parse_wend();
        case TokenType::GOTO: advance(); return parse_goto();
        case TokenType::GOSUB: advance(); return parse_gosub();
        case TokenType::RETURN: advance(); return parse_return();
        case TokenType::ON: advance(); return parse_on();
        case TokenType::DATA: advance(); return parse_data();
        case TokenType::READ: advance(); return parse_read();
        case TokenType::RESTORE: advance(); return parse_restore();
        case TokenType::DIM: advance(); return parse_dim();
        case TokenType::DEF: advance(); return parse_def();
        case TokenType::DEFINT: advance(); return parse_deftype(VarType::INTEGER);
        case TokenType::DEFSNG: advance(); return parse_deftype(VarType::SINGLE);
        case TokenType::DEFDBL: advance(); return parse_deftype(VarType::DOUBLE);
        case TokenType::DEFSTR: advance(); return parse_deftype(VarType::STRING);
        case TokenType::END: advance(); return parse_end();
        case TokenType::SYSTEM: advance(); return parse_end();  // SYSTEM = END
        case TokenType::STOP: advance(); return parse_stop();
        case TokenType::CLS: advance(); return parse_cls();
        case TokenType::REM:
        case TokenType::REMARK:
        case TokenType::APOSTROPHE:
            return parse_rem();
        case TokenType::SWAP: advance(); return parse_swap();
        case TokenType::ERASE: advance(); return parse_erase();
        case TokenType::CLEAR: advance(); return parse_clear();
        case TokenType::OPTION: advance(); return parse_option();
        case TokenType::RANDOMIZE: advance(); return parse_randomize();
        case TokenType::TRON: advance(); return parse_tron();
        case TokenType::TROFF: advance(); return parse_troff();
        case TokenType::WIDTH: advance(); return parse_width();
        case TokenType::POKE: advance(); return parse_poke();
        case TokenType::ERROR: advance(); return parse_error();
        case TokenType::RESUME: advance(); return parse_resume();
        case TokenType::OPEN: advance(); return parse_open();
        case TokenType::CLOSE: advance(); return parse_close();
        case TokenType::RESET: advance(); return parse_reset();
        case TokenType::FIELD: advance(); return parse_field();
        case TokenType::GET: advance(); return parse_get();
        case TokenType::PUT: advance(); return parse_put();
        case TokenType::LSET: advance(); return parse_lset();
        case TokenType::RSET: advance(); return parse_rset();
        case TokenType::WRITE: advance(); return parse_write();
        case TokenType::CHAIN: advance(); return parse_chain();
        case TokenType::COMMON: advance(); return parse_common();
        case TokenType::CALL: advance(); return parse_call();
        case TokenType::OUT: advance(); return parse_out();
        case TokenType::WAIT: advance(); return parse_wait();
        case TokenType::KILL: advance(); return parse_kill();
        case TokenType::NAME: advance(); return parse_name();
        case TokenType::MERGE: advance(); return parse_merge();
        case TokenType::RUN: advance(); return parse_run();

        case TokenType::MID:
            // MID$ assignment: MID$(var$, pos, len) = value
            {
                advance();  // Skip MID$
                expect(TokenType::LPAREN, "Expected '(' after MID$");

                auto [var, is_array] = parse_variable();
                expect(TokenType::COMMA, "Expected ',' after variable");
                Expr start = parse_expression();
                std::optional<Expr> length;
                if (match(TokenType::COMMA)) {
                    length = parse_expression();
                }
                expect(TokenType::RPAREN, "Expected ')' after MID$ arguments");
                expect(TokenType::EQUAL, "Expected '=' for MID$ assignment");
                Expr replacement = parse_expression();

                auto stmt = std::make_unique<MidAssignStmt>();
                stmt->line = start_line;
                stmt->column = start_col;
                stmt->variable = std::move(var);
                stmt->start = std::move(start);
                stmt->length = std::move(length);
                stmt->replacement = std::move(replacement);
                return Stmt{std::move(stmt)};
            }

        case TokenType::IDENTIFIER:
            return parse_let();

        default:
            throw ParseError("Unexpected token: " + token_type_name(t),
                           current().line, current().column);
    }
}

// Parse variable, returns (VariableExpr, is_array_access)
std::pair<VariableExpr, bool> Parser::parse_variable() {
    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected variable name", current().line, current().column);
    }

    Token tok = advance();
    VarType type = resolve_type(tok.value);
    bool is_array = check(TokenType::LPAREN);

    return {VariableExpr(tok.value, tok.original_case, type, tok.line, tok.column), is_array};
}

std::variant<VariableExpr, ArrayAccessExpr> Parser::parse_lvalue() {
    auto [var, is_array] = parse_variable();

    if (is_array) {
        // Array access
        expect(TokenType::LPAREN, "Expected '('");
        std::vector<Expr> indices;
        do {
            indices.push_back(parse_expression());
        } while (match(TokenType::COMMA));
        expect(TokenType::RPAREN, "Expected ')'");

        return ArrayAccessExpr(var.name, var.original, std::move(indices), var.type, var.line, var.column);
    }

    return var;
}

std::vector<Expr> Parser::parse_expression_list() {
    std::vector<Expr> exprs;
    do {
        exprs.push_back(parse_expression());
    } while (match(TokenType::COMMA));
    return exprs;
}

std::vector<Expr> Parser::parse_subscripts() {
    std::vector<Expr> indices;
    expect(TokenType::LPAREN, "Expected '('");
    do {
        indices.push_back(parse_expression());
    } while (match(TokenType::COMMA));
    expect(TokenType::RPAREN, "Expected ')'");
    return indices;
}

// ============================================================================
// Statement Parsers
// ============================================================================

Stmt Parser::parse_print() {
    auto stmt = std::make_unique<PrintStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // Check for file number: PRINT #n, ...
    if (match(TokenType::HASH)) {
        stmt->file_number = parse_expression();
        expect(TokenType::COMMA, "Expected ',' after file number");
    }

    // Check for USING
    if (match(TokenType::USING)) {
        auto using_stmt = std::make_unique<PrintUsingStmt>();
        using_stmt->line = stmt->line;
        using_stmt->column = stmt->column;
        using_stmt->file_number = std::move(stmt->file_number);
        using_stmt->format_string = parse_expression();
        expect(TokenType::SEMICOLON, "Expected ';' after format string");

        while (is_expression_start()) {
            using_stmt->expressions.push_back(parse_expression());
            if (!match_any({TokenType::SEMICOLON, TokenType::COMMA})) break;
        }

        return Stmt{std::move(using_stmt)};
    }

    // Parse print list
    // Handle leading separator (PRINT , "text" to tab to next zone)
    while (check(TokenType::COMMA) || check(TokenType::SEMICOLON)) {
        // Empty expression (just a separator)
        stmt->expressions.push_back(make_expr<StringExpr>("", current().line, current().column));
        if (match(TokenType::COMMA)) {
            stmt->separators.push_back(',');
        } else if (match(TokenType::SEMICOLON)) {
            stmt->separators.push_back(';');
        }
    }

    while (is_expression_start() || check(TokenType::TAB) || check(TokenType::SPC)) {
        stmt->expressions.push_back(parse_expression());

        if (match(TokenType::SEMICOLON)) {
            stmt->separators.push_back(';');
            // Handle multiple consecutive semicolons
            while (check(TokenType::SEMICOLON)) {
                stmt->expressions.push_back(make_expr<StringExpr>("", current().line, current().column));
                advance();
                stmt->separators.push_back(';');
            }
        } else if (match(TokenType::COMMA)) {
            stmt->separators.push_back(',');
            // Handle multiple consecutive commas (e.g., PRINT "A",,"B")
            while (check(TokenType::COMMA)) {
                stmt->expressions.push_back(make_expr<StringExpr>("", current().line, current().column));
                advance();
                stmt->separators.push_back(',');
            }
        } else if (is_expression_start() || check(TokenType::TAB) || check(TokenType::SPC)) {
            // No separator, but another expression follows - treat as implicit space
            stmt->separators.push_back(' ');
        } else {
            break;
        }
    }

    // Add final separator (newline if none)
    if (stmt->separators.size() < stmt->expressions.size()) {
        stmt->separators.push_back('\0');  // Indicates newline
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_lprint() {
    int line = current().line;
    int col = current().column;

    // Check for USING
    if (match(TokenType::USING)) {
        auto using_stmt = std::make_unique<LprintUsingStmt>();
        using_stmt->line = line;
        using_stmt->column = col;
        using_stmt->format_string = parse_expression();
        expect(TokenType::SEMICOLON, "Expected ';' after format string");

        while (is_expression_start()) {
            using_stmt->expressions.push_back(parse_expression());
            if (!match_any({TokenType::SEMICOLON, TokenType::COMMA})) break;
        }

        return Stmt{std::move(using_stmt)};
    }

    auto stmt = std::make_unique<LprintStmt>();
    stmt->line = line;
    stmt->column = col;

    while (is_expression_start()) {
        stmt->expressions.push_back(parse_expression());

        if (match(TokenType::SEMICOLON)) {
            stmt->separators.push_back(';');
        } else if (match(TokenType::COMMA)) {
            stmt->separators.push_back(',');
        } else {
            break;
        }
    }

    if (stmt->separators.size() < stmt->expressions.size()) {
        stmt->separators.push_back('\0');
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_input() {
    auto stmt = std::make_unique<InputStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // Check for INPUT; (suppress question mark)
    if (match(TokenType::SEMICOLON)) {
        stmt->suppress_question = true;
    }

    // Check for file number: INPUT #n, ...
    if (match(TokenType::HASH)) {
        stmt->file_number = parse_expression();
        expect(TokenType::COMMA, "Expected ',' after file number");
    }

    // Check for prompt string
    if (check(TokenType::STRING)) {
        stmt->prompt = parse_expression();
        if (match(TokenType::SEMICOLON) || match(TokenType::COMMA)) {
            // Continue to variables
        }
    }

    // Parse variable list (can include array elements like A$(1))
    do {
        auto lvalue = parse_lvalue();
        stmt->variables.push_back(std::move(lvalue));
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_line_input() {
    // LINE INPUT already consumed; expect INPUT
    if (check(TokenType::INPUT)) {
        advance();
    }

    auto stmt = std::make_unique<LineInputStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // Check for file number
    if (match(TokenType::HASH)) {
        stmt->file_number = parse_expression();
        expect(TokenType::COMMA, "Expected ',' after file number");
    }

    // Check for prompt
    if (check(TokenType::STRING)) {
        stmt->prompt = parse_expression();
        match_any({TokenType::SEMICOLON, TokenType::COMMA});
    }

    // Parse variable
    auto [var, is_array] = parse_variable();
    stmt->variable = std::move(var);

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_let() {
    auto stmt = std::make_unique<LetStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->target = parse_lvalue();
    expect(TokenType::EQUAL, "Expected '=' in assignment");
    stmt->expression = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_if() {
    auto stmt = std::make_unique<IfStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->condition = parse_expression();

    // THEN is optional but expected
    if (!match(TokenType::THEN)) {
        if (!match(TokenType::GOTO)) {
            throw ParseError("Expected THEN or GOTO after IF condition", current().line, current().column);
        }
        // IF ... GOTO line_number [ELSE ...]
        if (check(TokenType::NUMBER)) {
            stmt->then_line = std::stoi(current().value);
            advance();
        } else {
            throw ParseError("Expected line number after GOTO", current().line, current().column);
        }
        // Check for ELSE after IF...GOTO line_number
        if (match(TokenType::ELSE)) {
            if (check(TokenType::NUMBER)) {
                stmt->else_line = std::stoi(current().value);
                advance();
            } else {
                while (!at_end() && !check(TokenType::NEWLINE)) {
                    stmt->else_stmts.push_back(parse_statement());
                    if (!match(TokenType::COLON)) break;
                }
            }
        }
        return Stmt{std::move(stmt)};
    }

    // Check for THEN line_number
    if (check(TokenType::NUMBER)) {
        stmt->then_line = std::stoi(current().value);
        advance();
        // Skip optional colon before ELSE (e.g., THEN 100 :ELSE 200)
        // But only if ELSE follows - otherwise let line parser handle the colon
        if (check(TokenType::COLON) && peek(1).type == TokenType::ELSE) {
            advance();  // consume the colon
        }
    } else {
        // Parse THEN statements
        while (!at_end() && !check(TokenType::ELSE) && !check(TokenType::NEWLINE)) {
            stmt->then_stmts.push_back(parse_statement());
            if (!match(TokenType::COLON)) break;
        }
    }

    // Check for ELSE
    if (match(TokenType::ELSE)) {
        if (check(TokenType::NUMBER)) {
            stmt->else_line = std::stoi(current().value);
            advance();
        } else {
            while (!at_end() && !check(TokenType::NEWLINE)) {
                stmt->else_stmts.push_back(parse_statement());
                if (!match(TokenType::COLON)) break;
            }
        }
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_for() {
    auto stmt = std::make_unique<ForStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    auto [var, is_array] = parse_variable();
    stmt->variable = std::move(var);

    expect(TokenType::EQUAL, "Expected '=' in FOR statement");
    stmt->start_expr = parse_expression();

    expect(TokenType::TO, "Expected TO in FOR statement");
    stmt->end_expr = parse_expression();

    if (match(TokenType::STEP)) {
        stmt->step_expr = parse_expression();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_next() {
    auto stmt = std::make_unique<NextStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // NEXT can have no variable, one variable, or comma-separated variables
    if (check(TokenType::IDENTIFIER)) {
        do {
            auto [var, is_array] = parse_variable();
            stmt->variables.push_back(std::move(var));
        } while (match(TokenType::COMMA));
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_while() {
    auto stmt = std::make_unique<WhileStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    stmt->condition = parse_expression();
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_wend() {
    auto stmt = std::make_unique<WendStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_goto() {
    auto stmt = std::make_unique<GotoStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (!check(TokenType::NUMBER)) {
        throw ParseError("Expected line number after GOTO", current().line, current().column);
    }
    stmt->target_line = std::stoi(current().value);
    advance();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_gosub() {
    auto stmt = std::make_unique<GosubStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (!check(TokenType::NUMBER)) {
        throw ParseError("Expected line number after GOSUB", current().line, current().column);
    }
    stmt->target_line = std::stoi(current().value);
    advance();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_return() {
    auto stmt = std::make_unique<ReturnStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (check(TokenType::NUMBER)) {
        stmt->target_line = std::stoi(current().value);
        advance();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_on() {
    int line = current().line;
    int col = current().column;

    // Check for ON ERROR
    if (match(TokenType::ERROR)) {
        auto stmt = std::make_unique<OnErrorStmt>();
        stmt->line = line;
        stmt->column = col;

        if (match(TokenType::GOTO)) {
            stmt->is_gosub = false;
        } else if (match(TokenType::GOSUB)) {
            stmt->is_gosub = true;
        } else {
            throw ParseError("Expected GOTO or GOSUB after ON ERROR", current().line, current().column);
        }

        if (check(TokenType::NUMBER)) {
            int target = std::stoi(current().value);
            advance();
            if (target == 0) {
                stmt->target_line = std::nullopt;  // Disable error handling
            } else {
                stmt->target_line = target;
            }
        } else {
            throw ParseError("Expected line number after ON ERROR GOTO/GOSUB", current().line, current().column);
        }

        return Stmt{std::move(stmt)};
    }

    // ON expr GOTO/GOSUB line1, line2, ...
    Expr selector = parse_expression();

    bool is_gosub = false;
    if (match(TokenType::GOTO)) {
        is_gosub = false;
    } else if (match(TokenType::GOSUB)) {
        is_gosub = true;
    } else {
        throw ParseError("Expected GOTO or GOSUB after ON expression", current().line, current().column);
    }

    std::vector<int> targets;
    do {
        if (!check(TokenType::NUMBER)) {
            throw ParseError("Expected line number", current().line, current().column);
        }
        targets.push_back(std::stoi(current().value));
        advance();
    } while (match(TokenType::COMMA));

    if (is_gosub) {
        auto stmt = std::make_unique<OnGosubStmt>();
        stmt->line = line;
        stmt->column = col;
        stmt->selector = std::move(selector);
        stmt->targets = std::move(targets);
        return Stmt{std::move(stmt)};
    } else {
        auto stmt = std::make_unique<OnGotoStmt>();
        stmt->line = line;
        stmt->column = col;
        stmt->selector = std::move(selector);
        stmt->targets = std::move(targets);
        return Stmt{std::move(stmt)};
    }
}

Stmt Parser::parse_data() {
    auto stmt = std::make_unique<DataStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    do {
        if (check(TokenType::STRING)) {
            stmt->values.push_back(current().value);
            advance();
        } else if (check(TokenType::NUMBER)) {
            stmt->values.push_back(std::stod(current().value));
            advance();
        } else if (check(TokenType::MINUS)) {
            advance();
            if (check(TokenType::NUMBER)) {
                stmt->values.push_back(-std::stod(current().value));
                advance();
            } else {
                throw ParseError("Expected number after minus in DATA", current().line, current().column);
            }
        } else if (check(TokenType::IDENTIFIER)) {
            // Unquoted string
            stmt->values.push_back(current().original_case);
            advance();
        } else if (!check(TokenType::COMMA) && !check(TokenType::NEWLINE) &&
                   !check(TokenType::COLON) && !check(TokenType::END_OF_FILE)) {
            // Keywords used as unquoted strings in DATA (e.g., DATA PRINT,CLOSE)
            // Convert the keyword token to its string representation
            stmt->values.push_back(token_type_name(current().type));
            advance();
        } else {
            break;
        }
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_read() {
    auto stmt = std::make_unique<ReadStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    do {
        stmt->variables.push_back(parse_lvalue());
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_restore() {
    auto stmt = std::make_unique<RestoreStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (check(TokenType::NUMBER)) {
        stmt->target_line = std::stoi(current().value);
        advance();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_dim() {
    auto stmt = std::make_unique<DimStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    do {
        DimStmt::ArrayDecl decl;

        if (!check(TokenType::IDENTIFIER)) {
            throw ParseError("Expected array name", current().line, current().column);
        }

        Token tok = advance();
        decl.name = tok.value;
        decl.original = tok.original_case;
        decl.type = resolve_type(tok.value);

        decl.dimensions = parse_subscripts();

        stmt->arrays.push_back(std::move(decl));
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_def() {
    auto stmt = std::make_unique<DefFnStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // Handle both "DEF FN A" and "DEF FNA" forms
    if (match(TokenType::FN)) {
        // Standard form: DEF FN A(X) = ...
        if (!check(TokenType::IDENTIFIER)) {
            throw ParseError("Expected function name after FN", current().line, current().column);
        }
        stmt->name = "fn" + current().value;
        advance();
    } else if (check(TokenType::IDENTIFIER)) {
        // Classic form: DEF FNA(X) = ... where FNA is a single identifier
        std::string name = current().value;
        if (name.length() >= 2 && name.substr(0, 2) == "fn") {
            stmt->name = name;  // Already has "fn" prefix
            advance();
        } else {
            throw ParseError("Expected FN or FN-prefixed name after DEF", current().line, current().column);
        }
    } else {
        throw ParseError("Expected FN after DEF", current().line, current().column);
    }

    // Parameters
    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            do {
                if (!check(TokenType::IDENTIFIER)) {
                    throw ParseError("Expected parameter name", current().line, current().column);
                }
                stmt->params.push_back(current().value);
                advance();
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "Expected ')' after parameters");
    }

    expect(TokenType::EQUAL, "Expected '=' in DEF FN");
    stmt->body = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_deftype(VarType type) {
    auto stmt = std::make_unique<DefTypeStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    stmt->type = type;

    do {
        if (!check(TokenType::IDENTIFIER)) {
            throw ParseError("Expected letter or letter range", current().line, current().column);
        }

        char start_letter = std::tolower(current().value[0]);
        advance();

        if (match(TokenType::MINUS)) {
            if (!check(TokenType::IDENTIFIER)) {
                throw ParseError("Expected letter after '-'", current().line, current().column);
            }
            char end_letter = std::tolower(current().value[0]);
            advance();
            stmt->ranges.push_back({start_letter, end_letter});
        } else {
            stmt->ranges.push_back({start_letter, start_letter});
        }
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_end() {
    auto stmt = std::make_unique<EndStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_stop() {
    auto stmt = std::make_unique<StopStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_cls() {
    auto stmt = std::make_unique<ClsStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_rem() {
    auto stmt = std::make_unique<RemStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // REM token contains the comment text
    if (check_any({TokenType::REM, TokenType::REMARK, TokenType::APOSTROPHE})) {
        stmt->comment = current().value;
        advance();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_swap() {
    auto stmt = std::make_unique<SwapStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->var1 = parse_lvalue();
    expect(TokenType::COMMA, "Expected ',' in SWAP");
    stmt->var2 = parse_lvalue();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_erase() {
    auto stmt = std::make_unique<EraseStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    do {
        if (!check(TokenType::IDENTIFIER)) {
            throw ParseError("Expected array name", current().line, current().column);
        }
        stmt->arrays.push_back(current().value);
        advance();
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_clear() {
    auto stmt = std::make_unique<ClearStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (is_expression_start()) {
        stmt->string_space = parse_expression();
        if (match(TokenType::COMMA)) {
            stmt->stack_space = parse_expression();
        }
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_option() {
    auto stmt = std::make_unique<OptionBaseStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    expect(TokenType::BASE, "Expected BASE after OPTION");

    if (!check(TokenType::NUMBER)) {
        throw ParseError("Expected 0 or 1 after OPTION BASE", current().line, current().column);
    }

    int base = std::stoi(current().value);
    if (base != 0 && base != 1) {
        throw ParseError("OPTION BASE must be 0 or 1", current().line, current().column);
    }
    stmt->base = base;
    advance();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_randomize() {
    auto stmt = std::make_unique<RandomizeStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (is_expression_start()) {
        stmt->seed = parse_expression();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_tron() {
    auto stmt = std::make_unique<TronStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_troff() {
    auto stmt = std::make_unique<TroffStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_width() {
    auto stmt = std::make_unique<WidthStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // WIDTH #n, w or WIDTH w
    if (match(TokenType::HASH)) {
        stmt->file_number = parse_expression();
        expect(TokenType::COMMA, "Expected ',' after file number");
    }

    stmt->width = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_poke() {
    auto stmt = std::make_unique<PokeStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->address = parse_expression();
    expect(TokenType::COMMA, "Expected ',' in POKE");
    stmt->value = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_error() {
    auto stmt = std::make_unique<ErrorStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    stmt->error_code = parse_expression();
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_resume() {
    auto stmt = std::make_unique<ResumeStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (match(TokenType::NEXT)) {
        stmt->resume_type = ResumeStmt::Type::NEXT;
    } else if (check(TokenType::NUMBER)) {
        stmt->resume_type = ResumeStmt::Type::LINE;
        stmt->target_line = std::stoi(current().value);
        advance();
    } else {
        stmt->resume_type = ResumeStmt::Type::IMPLICIT;
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_open() {
    auto stmt = std::make_unique<OpenStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // Check for classic MBASIC syntax: OPEN "mode", #n, "filename" [,reclen]
    // Or modern syntax: OPEN filename FOR mode AS #n [LEN = reclen]

    Expr first_expr = parse_expression();

    if (match(TokenType::COMMA)) {
        // Classic syntax: OPEN "mode", #n, "filename"
        // first_expr is the mode string
        auto* str_expr = std::get_if<std::unique_ptr<StringExpr>>(&first_expr);
        if (!str_expr) {
            throw ParseError("Expected string for file mode", current().line, current().column);
        }
        std::string mode_str = (*str_expr)->value;

        if (mode_str == "I" || mode_str == "i") {
            stmt->mode = FileMode::INPUT;
        } else if (mode_str == "O" || mode_str == "o") {
            stmt->mode = FileMode::OUTPUT;
        } else if (mode_str == "A" || mode_str == "a") {
            stmt->mode = FileMode::APPEND;
        } else if (mode_str == "R" || mode_str == "r") {
            stmt->mode = FileMode::RANDOM;
        } else {
            throw ParseError("Invalid file mode: " + mode_str, current().line, current().column);
        }

        match(TokenType::HASH);  // Optional #
        stmt->file_number = parse_expression();
        expect(TokenType::COMMA, "Expected ',' before filename");
        stmt->filename = parse_expression();

        // Optional record length
        if (match(TokenType::COMMA)) {
            stmt->record_length = parse_expression();
        }
    } else if (check(TokenType::FOR)) {
        // Modern syntax: OPEN filename FOR mode AS #n [LEN = reclen]
        stmt->filename = std::move(first_expr);

        expect(TokenType::FOR, "Expected FOR in OPEN");
        if (match(TokenType::INPUT)) {
            stmt->mode = FileMode::INPUT;
        } else if (match(TokenType::OUTPUT)) {
            stmt->mode = FileMode::OUTPUT;
        } else if (match(TokenType::APPEND)) {
            stmt->mode = FileMode::APPEND;
        } else if (check(TokenType::IDENTIFIER) && (current().value == "random" || current().value == "RANDOM")) {
            advance();
            stmt->mode = FileMode::RANDOM;
        } else {
            throw ParseError("Expected INPUT, OUTPUT, APPEND, or RANDOM", current().line, current().column);
        }

        expect(TokenType::AS, "Expected AS in OPEN");
        match(TokenType::HASH);  // Optional #
        stmt->file_number = parse_expression();

        // Optional LEN = reclen for random files
        if (match(TokenType::LEN)) {
            expect(TokenType::EQUAL, "Expected '=' after LEN");
            stmt->record_length = parse_expression();
        }
    } else {
        throw ParseError("Expected ',' or FOR in OPEN statement", current().line, current().column);
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_close() {
    auto stmt = std::make_unique<CloseStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // CLOSE with no args closes all files
    while (match(TokenType::HASH) || is_expression_start()) {
        if (tokens_[pos_ - 1].type != TokenType::HASH && is_expression_start()) {
            // Expression without preceding #
        }
        stmt->file_numbers.push_back(parse_expression());
        if (!match(TokenType::COMMA)) break;
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_reset() {
    // RESET closes all files - equivalent to CLOSE with no args
    auto stmt = std::make_unique<CloseStmt>();
    stmt->line = current().line;
    stmt->column = current().column;
    // Empty file_numbers means close all
    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_field() {
    auto stmt = std::make_unique<FieldStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    match(TokenType::HASH);
    stmt->file_number = parse_expression();
    expect(TokenType::COMMA, "Expected ',' after file number");

    do {
        FieldStmt::FieldVar fv;
        fv.width = parse_expression();
        expect(TokenType::AS, "Expected AS in FIELD");
        auto [var, is_array] = parse_variable();
        fv.variable = std::move(var);
        stmt->fields.push_back(std::move(fv));
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_get() {
    auto stmt = std::make_unique<GetStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    match(TokenType::HASH);
    stmt->file_number = parse_expression();

    if (match(TokenType::COMMA)) {
        stmt->record_number = parse_expression();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_put() {
    auto stmt = std::make_unique<PutStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    match(TokenType::HASH);
    stmt->file_number = parse_expression();

    if (match(TokenType::COMMA)) {
        stmt->record_number = parse_expression();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_lset() {
    auto stmt = std::make_unique<LsetStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    auto [var, is_array] = parse_variable();
    stmt->variable = std::move(var);
    expect(TokenType::EQUAL, "Expected '=' in LSET");
    stmt->value = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_rset() {
    auto stmt = std::make_unique<RsetStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    auto [var, is_array] = parse_variable();
    stmt->variable = std::move(var);
    expect(TokenType::EQUAL, "Expected '=' in RSET");
    stmt->value = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_write() {
    auto stmt = std::make_unique<WriteStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (match(TokenType::HASH)) {
        stmt->file_number = parse_expression();
        expect(TokenType::COMMA, "Expected ',' after file number");
    }

    if (is_expression_start()) {
        stmt->expressions = parse_expression_list();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_chain() {
    auto stmt = std::make_unique<ChainStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    if (match(TokenType::MERGE)) {
        stmt->merge = true;
    }

    stmt->filename = parse_expression();

    if (match(TokenType::COMMA)) {
        if (is_expression_start()) {
            stmt->line_number = parse_expression();
        }

        if (match(TokenType::COMMA)) {
            if (match(TokenType::ALL)) {
                stmt->all = true;
            } else if (match(TokenType::DELETE)) {
                stmt->delete_lines = true;
            }
        }
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_common() {
    auto stmt = std::make_unique<CommonStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    do {
        if (!check(TokenType::IDENTIFIER)) {
            throw ParseError("Expected variable name", current().line, current().column);
        }
        stmt->variables.push_back(current().value);
        advance();
    } while (match(TokenType::COMMA));

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_call() {
    auto stmt = std::make_unique<CallStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->address = parse_expression();

    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            stmt->args = parse_expression_list();
        }
        expect(TokenType::RPAREN, "Expected ')' after CALL arguments");
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_out() {
    auto stmt = std::make_unique<OutStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->port = parse_expression();
    expect(TokenType::COMMA, "Expected ',' in OUT");
    stmt->value = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_wait() {
    auto stmt = std::make_unique<WaitStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->port = parse_expression();
    expect(TokenType::COMMA, "Expected ',' in WAIT");
    stmt->and_mask = parse_expression();

    if (match(TokenType::COMMA)) {
        stmt->xor_mask = parse_expression();
    }

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_kill() {
    auto stmt = std::make_unique<KillStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->filename = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_name() {
    auto stmt = std::make_unique<NameStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->old_name = parse_expression();
    expect(TokenType::AS, "Expected AS in NAME statement");
    stmt->new_name = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_merge() {
    auto stmt = std::make_unique<MergeStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    stmt->filename = parse_expression();

    return Stmt{std::move(stmt)};
}

Stmt Parser::parse_run() {
    auto stmt = std::make_unique<RunStmt>();
    stmt->line = current().line;
    stmt->column = current().column;

    // RUN has several forms:
    // RUN                  - run current program from start
    // RUN 100              - run current program from line 100
    // RUN "filename"       - load and run program
    // RUN "filename",R     - load and run, keep variables
    // RUN "filename",100   - load and run from line 100 (not standard but supported)

    // Check if we have a filename (string expression) or line number
    if (check(TokenType::STRING)) {
        stmt->filename = parse_expression();

        // Check for ,R (keep variables) or ,line_number
        if (match(TokenType::COMMA)) {
            if (check(TokenType::IDENTIFIER) && current().value == "r") {
                advance();
                stmt->keep_variables = true;
            } else if (check(TokenType::NUMBER)) {
                stmt->start_line = static_cast<int>(std::stod(current().value));
                advance();
            }
        }
    } else if (check(TokenType::NUMBER)) {
        // RUN line_number
        stmt->start_line = static_cast<int>(std::stod(current().value));
        advance();
    }
    // Otherwise, RUN with no arguments - run from start

    return Stmt{std::move(stmt)};
}

// ============================================================================
// Expression Parsing (Operator Precedence)
// ============================================================================

// Precedence (lowest to highest):
// IMP < EQV < XOR < OR < AND < NOT < comparison < + - < MOD < \ < * / < ^ < unary

Expr Parser::parse_expression() {
    return parse_imp_expr();
}

Expr Parser::parse_imp_expr() {
    Expr left = parse_eqv_expr();

    while (match(TokenType::IMP)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_eqv_expr();
        left = make_expr<BinaryExpr>(TokenType::IMP, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_eqv_expr() {
    Expr left = parse_xor_expr();

    while (match(TokenType::EQV)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_xor_expr();
        left = make_expr<BinaryExpr>(TokenType::EQV, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_xor_expr() {
    Expr left = parse_or_expr();

    while (match(TokenType::XOR)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_or_expr();
        left = make_expr<BinaryExpr>(TokenType::XOR, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_or_expr() {
    Expr left = parse_and_expr();

    while (match(TokenType::OR)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_and_expr();
        left = make_expr<BinaryExpr>(TokenType::OR, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_and_expr() {
    Expr left = parse_not_expr();

    while (match(TokenType::AND)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_not_expr();
        left = make_expr<BinaryExpr>(TokenType::AND, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_not_expr() {
    if (match(TokenType::NOT)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr operand = parse_not_expr();
        return make_expr<UnaryExpr>(TokenType::NOT, std::move(operand), line, col);
    }

    return parse_comparison();
}

Expr Parser::parse_comparison() {
    Expr left = parse_additive();

    while (check_any({TokenType::EQUAL, TokenType::NOT_EQUAL, TokenType::LESS_THAN,
                      TokenType::GREATER_THAN, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
        Token op = advance();
        Expr right = parse_additive();
        left = make_expr<BinaryExpr>(op.type, std::move(left), std::move(right), op.line, op.column);
    }

    return left;
}

Expr Parser::parse_additive() {
    Expr left = parse_mod_expr();

    while (check_any({TokenType::PLUS, TokenType::MINUS})) {
        Token op = advance();
        Expr right = parse_mod_expr();
        left = make_expr<BinaryExpr>(op.type, std::move(left), std::move(right), op.line, op.column);
    }

    return left;
}

Expr Parser::parse_mod_expr() {
    Expr left = parse_int_div_expr();

    while (match(TokenType::MOD)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_int_div_expr();
        left = make_expr<BinaryExpr>(TokenType::MOD, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_int_div_expr() {
    Expr left = parse_multiplicative();

    while (match(TokenType::BACKSLASH)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_multiplicative();
        left = make_expr<BinaryExpr>(TokenType::BACKSLASH, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_multiplicative() {
    Expr left = parse_unary();

    while (check_any({TokenType::MULTIPLY, TokenType::DIVIDE})) {
        Token op = advance();
        Expr right = parse_unary();
        left = make_expr<BinaryExpr>(op.type, std::move(left), std::move(right), op.line, op.column);
    }

    return left;
}

Expr Parser::parse_power() {
    // Power base is a primary (number, variable, parenthesized expr)
    Expr left = parse_primary();

    // Power is right-associative and has highest precedence (before unary minus)
    if (match(TokenType::POWER)) {
        int line = tokens_[pos_ - 1].line;
        int col = tokens_[pos_ - 1].column;
        Expr right = parse_power();  // Right-associative
        return make_expr<BinaryExpr>(TokenType::POWER, std::move(left), std::move(right), line, col);
    }

    return left;
}

Expr Parser::parse_unary() {
    // Unary minus/plus has lower precedence than exponentiation
    // So -2^2 = -(2^2) = -4, not (-2)^2 = 4
    if (check_any({TokenType::MINUS, TokenType::PLUS})) {
        Token op = advance();
        Expr operand = parse_unary();  // Allow chained unary operators like --x
        if (op.type == TokenType::MINUS) {
            return make_expr<UnaryExpr>(TokenType::MINUS, std::move(operand), op.line, op.column);
        }
        return operand;  // Unary + is a no-op
    }

    return parse_power();  // Try power expression (which handles primary)
}

Expr Parser::parse_primary() {
    int line = current().line;
    int col = current().column;

    // Number literal
    if (check(TokenType::NUMBER)) {
        double value = std::stod(current().value);
        advance();
        return make_expr<NumberExpr>(value, line, col);
    }

    // String literal
    if (check(TokenType::STRING)) {
        std::string value = current().value;
        advance();
        return make_expr<StringExpr>(std::move(value), line, col);
    }

    // Parenthesized expression
    if (match(TokenType::LPAREN)) {
        Expr expr = parse_expression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    // ERR and ERL special variables
    if (match(TokenType::ERR)) {
        return make_expr<VariableExpr>("err%", "ERR%", VarType::INTEGER, line, col);
    }
    if (match(TokenType::ERL)) {
        return make_expr<VariableExpr>("erl%", "ERL%", VarType::INTEGER, line, col);
    }

    // FN user-defined function call
    if (match(TokenType::FN)) {
        if (!check(TokenType::IDENTIFIER)) {
            throw ParseError("Expected function name after FN", current().line, current().column);
        }
        std::string name = "fn" + current().value;
        advance();

        std::vector<Expr> args;
        if (match(TokenType::LPAREN)) {
            if (!check(TokenType::RPAREN)) {
                args = parse_expression_list();
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
        }

        return make_expr<FunctionCallExpr>(std::move(name), std::move(args), line, col);
    }

    // Built-in functions
    static const std::unordered_set<TokenType> functions = {
        TokenType::ABS, TokenType::ATN, TokenType::COS, TokenType::EXP,
        TokenType::FIX, TokenType::INT, TokenType::LOG, TokenType::RND,
        TokenType::SGN, TokenType::SIN, TokenType::SQR, TokenType::TAN,
        TokenType::CINT, TokenType::CSNG, TokenType::CDBL,
        TokenType::CVI, TokenType::CVS, TokenType::CVD,
        TokenType::MKI, TokenType::MKS, TokenType::MKD,
        TokenType::ASC, TokenType::CHR, TokenType::HEX, TokenType::OCT,
        TokenType::LEFT, TokenType::RIGHT, TokenType::MID,
        TokenType::LEN, TokenType::STR, TokenType::VAL,
        TokenType::SPACE, TokenType::STRING_FUNC, TokenType::INSTR,
        TokenType::INKEY, TokenType::INPUT_FUNC,
        TokenType::EOF_FUNC, TokenType::LOC, TokenType::LOF,
        TokenType::PEEK, TokenType::POS, TokenType::FRE,
        TokenType::TAB, TokenType::SPC, TokenType::USR, TokenType::VARPTR,
        TokenType::INP, TokenType::LPOS,
        TokenType::TIMER, TokenType::DATE_FUNC, TokenType::TIME_FUNC,
        TokenType::ENVIRON_FUNC, TokenType::ERROR_FUNC
    };

    if (functions.count(current().type)) {
        std::string name = current().value;
        advance();

        std::vector<Expr> args;
        if (match(TokenType::LPAREN)) {
            if (!check(TokenType::RPAREN)) {
                args = parse_expression_list();
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
        }

        return make_expr<FunctionCallExpr>(std::move(name), std::move(args), line, col);
    }

    // Variable or array access
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current().value;
        std::string original = current().original_case;
        VarType type = resolve_type(name);
        advance();

        // Check for user-defined function call (FNA, FNB, etc.)
        // These are identifiers that start with "fn" and have at least one more character
        if (name.length() > 2 && name.substr(0, 2) == "fn" && check(TokenType::LPAREN)) {
            advance();  // consume (
            std::vector<Expr> args;
            if (!check(TokenType::RPAREN)) {
                args = parse_expression_list();
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
            return make_expr<FunctionCallExpr>(std::move(name), std::move(args), line, col);
        }

        // Check for array subscript
        if (match(TokenType::LPAREN)) {
            std::vector<Expr> indices;
            if (!check(TokenType::RPAREN)) {
                indices = parse_expression_list();
            }
            expect(TokenType::RPAREN, "Expected ')' after subscripts");
            return make_expr<ArrayAccessExpr>(std::move(name), std::move(original),
                                               std::move(indices), type, line, col);
        }

        return make_expr<VariableExpr>(std::move(name), std::move(original), type, line, col);
    }

    throw ParseError("Missing operand",
                     current().line, current().column);
}

// Convenience function
Program parse(const std::string& source) {
    std::vector<Token> tokens = tokenize(source);
    Parser parser(std::move(tokens));
    return parser.parse();
}

} // namespace mbasic
