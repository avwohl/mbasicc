#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "tokens.hpp"
#include "ast.hpp"
#include "error.hpp"

namespace mbasic {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parse entire program
    Program parse();

    // Parse a single line (for immediate mode)
    std::vector<Stmt> parse_immediate(const std::vector<Token>& tokens);

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    // DEF type map (populated in first pass)
    std::unordered_map<char, VarType> def_type_map_;

    // Token access
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    Token advance();
    bool at_end() const;
    bool check(TokenType type) const;
    bool check_any(std::initializer_list<TokenType> types) const;
    bool match(TokenType type);
    bool match_any(std::initializer_list<TokenType> types);
    Token expect(TokenType type, const std::string& msg);

    // Two-pass parsing
    void collect_def_types();

    // Program structure
    Line parse_line();
    Stmt parse_statement();

    // Statement parsers
    Stmt parse_print();
    Stmt parse_lprint();
    Stmt parse_input();
    Stmt parse_line_input();
    Stmt parse_let();
    Stmt parse_if();
    Stmt parse_for();
    Stmt parse_next();
    Stmt parse_while();
    Stmt parse_wend();
    Stmt parse_goto();
    Stmt parse_gosub();
    Stmt parse_return();
    Stmt parse_on();
    Stmt parse_data();
    Stmt parse_read();
    Stmt parse_restore();
    Stmt parse_dim();
    Stmt parse_def();
    Stmt parse_deftype(VarType type);
    Stmt parse_end();
    Stmt parse_stop();
    Stmt parse_cls();
    Stmt parse_rem();
    Stmt parse_swap();
    Stmt parse_erase();
    Stmt parse_clear();
    Stmt parse_option();
    Stmt parse_randomize();
    Stmt parse_tron();
    Stmt parse_troff();
    Stmt parse_width();
    Stmt parse_poke();
    Stmt parse_error();
    Stmt parse_resume();
    Stmt parse_open();
    Stmt parse_close();
    Stmt parse_reset();
    Stmt parse_field();
    Stmt parse_get();
    Stmt parse_put();
    Stmt parse_lset();
    Stmt parse_rset();
    Stmt parse_write();
    Stmt parse_chain();
    Stmt parse_common();
    Stmt parse_call();
    Stmt parse_out();
    Stmt parse_wait();
    Stmt parse_kill();
    Stmt parse_name();
    Stmt parse_merge();
    Stmt parse_run();

    // Expression parsing (precedence climbing)
    Expr parse_expression();
    Expr parse_imp_expr();
    Expr parse_eqv_expr();
    Expr parse_xor_expr();
    Expr parse_or_expr();
    Expr parse_and_expr();
    Expr parse_not_expr();
    Expr parse_comparison();
    Expr parse_additive();
    Expr parse_mod_expr();
    Expr parse_int_div_expr();
    Expr parse_multiplicative();
    Expr parse_power();
    Expr parse_unary();
    Expr parse_primary();

    // Helpers
    VarType resolve_type(const std::string& name);
    VarType type_from_name(const std::string& name);
    std::pair<VariableExpr, bool> parse_variable();  // Returns (var, is_array)
    std::variant<VariableExpr, ArrayAccessExpr> parse_lvalue();
    std::vector<Expr> parse_expression_list();
    std::vector<Expr> parse_subscripts();

    // Check if token starts an expression
    bool is_expression_start() const;

    // Check if token is a statement keyword
    bool is_statement_keyword() const;

    // Skip to end of line (for error recovery)
    void skip_to_eol();
};

// Convenience function
Program parse(const std::string& source);

} // namespace mbasic
