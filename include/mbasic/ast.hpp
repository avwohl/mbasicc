#pragma once

#include <vector>
#include <optional>
#include <memory>
#include <variant>
#include <string>
#include "tokens.hpp"
#include "value.hpp"

namespace mbasic {

// Forward declarations for expression nodes
struct NumberExpr;
struct StringExpr;
struct VariableExpr;
struct BinaryExpr;
struct UnaryExpr;
struct FunctionCallExpr;
struct ArrayAccessExpr;

// Expression node - uses variant for type safety
using Expr = std::variant<
    std::unique_ptr<NumberExpr>,
    std::unique_ptr<StringExpr>,
    std::unique_ptr<VariableExpr>,
    std::unique_ptr<BinaryExpr>,
    std::unique_ptr<UnaryExpr>,
    std::unique_ptr<FunctionCallExpr>,
    std::unique_ptr<ArrayAccessExpr>
>;

// Helper to create expression nodes
template<typename T, typename... Args>
Expr make_expr(Args&&... args) {
    return Expr{std::make_unique<T>(std::forward<Args>(args)...)};
}

// ============================================================================
// Expression Nodes
// ============================================================================

struct NumberExpr {
    double value;
    int line, column;

    NumberExpr(double v, int l, int c) : value(v), line(l), column(c) {}
};

struct StringExpr {
    std::string value;
    int line, column;

    StringExpr(std::string v, int l, int c) : value(std::move(v)), line(l), column(c) {}
};

struct VariableExpr {
    std::string name;       // Normalized name (lowercase with suffix)
    std::string original;   // Original case
    VarType type = VarType::SINGLE;
    int line = 0, column = 0;

    VariableExpr() = default;

    VariableExpr(std::string n, std::string orig, VarType t, int l, int c)
        : name(std::move(n)), original(std::move(orig)), type(t), line(l), column(c) {}
};

struct BinaryExpr {
    TokenType op;
    Expr left;
    Expr right;
    int line, column;

    BinaryExpr(TokenType o, Expr l, Expr r, int ln, int c)
        : op(o), left(std::move(l)), right(std::move(r)), line(ln), column(c) {}
};

struct UnaryExpr {
    TokenType op;
    Expr operand;
    int line, column;

    UnaryExpr(TokenType o, Expr e, int l, int c)
        : op(o), operand(std::move(e)), line(l), column(c) {}
};

struct FunctionCallExpr {
    std::string name;
    std::vector<Expr> args;
    int line, column;

    FunctionCallExpr(std::string n, std::vector<Expr> a, int l, int c)
        : name(std::move(n)), args(std::move(a)), line(l), column(c) {}
};

struct ArrayAccessExpr {
    std::string name;
    std::string original;
    std::vector<Expr> indices;
    VarType type;
    int line, column;

    ArrayAccessExpr(std::string n, std::string orig, std::vector<Expr> idx, VarType t, int l, int c)
        : name(std::move(n)), original(std::move(orig)), indices(std::move(idx)), type(t), line(l), column(c) {}
};

// ============================================================================
// Forward declarations for statement nodes
// ============================================================================

struct PrintStmt;
struct PrintUsingStmt;
struct LprintStmt;
struct LprintUsingStmt;
struct InputStmt;
struct LineInputStmt;
struct LetStmt;
struct IfStmt;
struct ForStmt;
struct NextStmt;
struct WhileStmt;
struct WendStmt;
struct GotoStmt;
struct GosubStmt;
struct ReturnStmt;
struct OnGotoStmt;
struct OnGosubStmt;
struct DataStmt;
struct ReadStmt;
struct RestoreStmt;
struct DimStmt;
struct DefFnStmt;
struct DefTypeStmt;
struct EndStmt;
struct ClsStmt;
struct StopStmt;
struct RemStmt;
struct SwapStmt;
struct EraseStmt;
struct ClearStmt;
struct OptionBaseStmt;
struct RandomizeStmt;
struct TronStmt;
struct TroffStmt;
struct WidthStmt;
struct PokeStmt;
struct ErrorStmt;
struct OnErrorStmt;
struct ResumeStmt;
struct OpenStmt;
struct CloseStmt;
struct FieldStmt;
struct GetStmt;
struct PutStmt;
struct LsetStmt;
struct RsetStmt;
struct WriteStmt;
struct ChainStmt;
struct CommonStmt;
struct MidAssignStmt;
struct CallStmt;
struct OutStmt;
struct WaitStmt;
struct KillStmt;
struct NameStmt;
struct MergeStmt;
struct RunStmt;

// Statement variant
using Stmt = std::variant<
    std::unique_ptr<PrintStmt>,
    std::unique_ptr<PrintUsingStmt>,
    std::unique_ptr<LprintStmt>,
    std::unique_ptr<LprintUsingStmt>,
    std::unique_ptr<InputStmt>,
    std::unique_ptr<LineInputStmt>,
    std::unique_ptr<LetStmt>,
    std::unique_ptr<IfStmt>,
    std::unique_ptr<ForStmt>,
    std::unique_ptr<NextStmt>,
    std::unique_ptr<WhileStmt>,
    std::unique_ptr<WendStmt>,
    std::unique_ptr<GotoStmt>,
    std::unique_ptr<GosubStmt>,
    std::unique_ptr<ReturnStmt>,
    std::unique_ptr<OnGotoStmt>,
    std::unique_ptr<OnGosubStmt>,
    std::unique_ptr<DataStmt>,
    std::unique_ptr<ReadStmt>,
    std::unique_ptr<RestoreStmt>,
    std::unique_ptr<DimStmt>,
    std::unique_ptr<DefFnStmt>,
    std::unique_ptr<DefTypeStmt>,
    std::unique_ptr<EndStmt>,
    std::unique_ptr<ClsStmt>,
    std::unique_ptr<StopStmt>,
    std::unique_ptr<RemStmt>,
    std::unique_ptr<SwapStmt>,
    std::unique_ptr<EraseStmt>,
    std::unique_ptr<ClearStmt>,
    std::unique_ptr<OptionBaseStmt>,
    std::unique_ptr<RandomizeStmt>,
    std::unique_ptr<TronStmt>,
    std::unique_ptr<TroffStmt>,
    std::unique_ptr<WidthStmt>,
    std::unique_ptr<PokeStmt>,
    std::unique_ptr<ErrorStmt>,
    std::unique_ptr<OnErrorStmt>,
    std::unique_ptr<ResumeStmt>,
    std::unique_ptr<OpenStmt>,
    std::unique_ptr<CloseStmt>,
    std::unique_ptr<FieldStmt>,
    std::unique_ptr<GetStmt>,
    std::unique_ptr<PutStmt>,
    std::unique_ptr<LsetStmt>,
    std::unique_ptr<RsetStmt>,
    std::unique_ptr<WriteStmt>,
    std::unique_ptr<ChainStmt>,
    std::unique_ptr<CommonStmt>,
    std::unique_ptr<MidAssignStmt>,
    std::unique_ptr<CallStmt>,
    std::unique_ptr<OutStmt>,
    std::unique_ptr<WaitStmt>,
    std::unique_ptr<KillStmt>,
    std::unique_ptr<NameStmt>,
    std::unique_ptr<MergeStmt>,
    std::unique_ptr<RunStmt>
>;

// Helper to create statement nodes
template<typename T, typename... Args>
Stmt make_stmt(Args&&... args) {
    return Stmt{std::make_unique<T>(std::forward<Args>(args)...)};
}

// ============================================================================
// Statement Nodes
// ============================================================================

// Base info for all statements
struct StmtInfo {
    int line = 0;
    int column = 0;
    int char_start = 0;
    int char_end = 0;
};

struct PrintStmt : StmtInfo {
    std::vector<Expr> expressions;
    std::vector<char> separators;  // ';', ',', or '\0' for newline
    std::optional<Expr> file_number;
};

struct PrintUsingStmt : StmtInfo {
    Expr format_string;
    std::vector<Expr> expressions;
    std::optional<Expr> file_number;
};

struct LprintStmt : StmtInfo {
    std::vector<Expr> expressions;
    std::vector<char> separators;
};

struct LprintUsingStmt : StmtInfo {
    Expr format_string;
    std::vector<Expr> expressions;
};

struct InputStmt : StmtInfo {
    std::optional<Expr> prompt;
    std::vector<std::variant<VariableExpr, ArrayAccessExpr>> variables;
    std::optional<Expr> file_number;
    bool suppress_question = false;
};

struct LineInputStmt : StmtInfo {
    std::optional<Expr> prompt;
    VariableExpr variable;
    std::optional<Expr> file_number;
};

struct LetStmt : StmtInfo {
    std::variant<VariableExpr, ArrayAccessExpr> target;
    Expr expression;
};

struct IfStmt : StmtInfo {
    Expr condition;
    std::vector<Stmt> then_stmts;
    std::optional<int> then_line;  // IF...THEN line_number
    std::vector<Stmt> else_stmts;
    std::optional<int> else_line;
};

struct ForStmt : StmtInfo {
    VariableExpr variable;
    Expr start_expr;
    Expr end_expr;
    std::optional<Expr> step_expr;
};

struct NextStmt : StmtInfo {
    std::vector<VariableExpr> variables;  // Can be empty (NEXT without var)
};

struct WhileStmt : StmtInfo {
    Expr condition;
};

struct WendStmt : StmtInfo {};

struct GotoStmt : StmtInfo {
    int target_line;
};

struct GosubStmt : StmtInfo {
    int target_line;
};

struct ReturnStmt : StmtInfo {
    std::optional<int> target_line;
};

struct OnGotoStmt : StmtInfo {
    Expr selector;
    std::vector<int> targets;
};

struct OnGosubStmt : StmtInfo {
    Expr selector;
    std::vector<int> targets;
};

struct DataStmt : StmtInfo {
    std::vector<Value> values;
};

struct ReadStmt : StmtInfo {
    std::vector<std::variant<VariableExpr, ArrayAccessExpr>> variables;
};

struct RestoreStmt : StmtInfo {
    std::optional<int> target_line;
};

struct DimStmt : StmtInfo {
    struct ArrayDecl {
        std::string name;
        std::string original;
        std::vector<Expr> dimensions;
        VarType type;
    };
    std::vector<ArrayDecl> arrays;
};

struct DefFnStmt : StmtInfo {
    std::string name;
    std::vector<std::string> params;
    Expr body;
};

struct DefTypeStmt : StmtInfo {
    VarType type;  // INTEGER, SINGLE, DOUBLE, STRING
    std::vector<std::pair<char, char>> ranges;  // e.g., ('A', 'Z')
};

struct EndStmt : StmtInfo {};

struct ClsStmt : StmtInfo {};

struct StopStmt : StmtInfo {};

struct RemStmt : StmtInfo {
    std::string comment;
};

struct SwapStmt : StmtInfo {
    std::variant<VariableExpr, ArrayAccessExpr> var1;
    std::variant<VariableExpr, ArrayAccessExpr> var2;
};

struct EraseStmt : StmtInfo {
    std::vector<std::string> arrays;
};

struct ClearStmt : StmtInfo {
    std::optional<Expr> string_space;
    std::optional<Expr> stack_space;
};

struct OptionBaseStmt : StmtInfo {
    int base;  // 0 or 1
};

struct RandomizeStmt : StmtInfo {
    std::optional<Expr> seed;
};

struct TronStmt : StmtInfo {};

struct TroffStmt : StmtInfo {};

struct WidthStmt : StmtInfo {
    Expr width;
    std::optional<Expr> file_number;
};

struct PokeStmt : StmtInfo {
    Expr address;
    Expr value;
};

struct ErrorStmt : StmtInfo {
    Expr error_code;
};

struct OnErrorStmt : StmtInfo {
    std::optional<int> target_line;  // None means ON ERROR GOTO 0 (disable)
    bool is_gosub = false;
};

struct ResumeStmt : StmtInfo {
    enum class Type { NEXT, LINE, IMPLICIT };
    Type resume_type = Type::IMPLICIT;
    std::optional<int> target_line;
};

// File I/O modes
enum class FileMode { INPUT, OUTPUT, APPEND, RANDOM };

struct OpenStmt : StmtInfo {
    Expr filename;
    FileMode mode = FileMode::INPUT;
    Expr file_number;
    std::optional<Expr> record_length;
};

struct CloseStmt : StmtInfo {
    std::vector<Expr> file_numbers;  // Empty means close all
};

struct FieldStmt : StmtInfo {
    Expr file_number;
    struct FieldVar {
        Expr width;
        VariableExpr variable;
    };
    std::vector<FieldVar> fields;
};

struct GetStmt : StmtInfo {
    Expr file_number;
    std::optional<Expr> record_number;
};

struct PutStmt : StmtInfo {
    Expr file_number;
    std::optional<Expr> record_number;
};

struct LsetStmt : StmtInfo {
    VariableExpr variable;
    Expr value;
};

struct RsetStmt : StmtInfo {
    VariableExpr variable;
    Expr value;
};

struct WriteStmt : StmtInfo {
    std::optional<Expr> file_number;
    std::vector<Expr> expressions;
};

struct ChainStmt : StmtInfo {
    Expr filename;
    std::optional<Expr> line_number;
    bool all = false;
    bool merge = false;
    bool delete_lines = false;
};

struct CommonStmt : StmtInfo {
    std::vector<std::string> variables;
};

struct MidAssignStmt : StmtInfo {
    VariableExpr variable;
    Expr start;
    std::optional<Expr> length;
    Expr replacement;
};

struct CallStmt : StmtInfo {
    Expr address;
    std::vector<Expr> args;
};

struct OutStmt : StmtInfo {
    Expr port;
    Expr value;
};

struct WaitStmt : StmtInfo {
    Expr port;
    Expr and_mask;
    std::optional<Expr> xor_mask;
};

struct KillStmt : StmtInfo {
    Expr filename;
};

struct NameStmt : StmtInfo {
    Expr old_name;
    Expr new_name;
};

struct MergeStmt : StmtInfo {
    Expr filename;
};

struct RunStmt : StmtInfo {
    std::optional<Expr> filename;       // RUN "file" - load and run
    std::optional<int> start_line;      // RUN 100 or RUN "file",100 - start at line
    bool keep_variables = false;        // RUN "file",R - keep variables
};

// ============================================================================
// Program Structure
// ============================================================================

struct Line {
    int line_number;
    std::vector<Stmt> statements;
    std::string source_text;  // Original source for error messages
};

struct Program {
    std::vector<Line> lines;
    std::unordered_map<char, VarType> def_type_map;

    // Initialize default types (all SINGLE)
    Program() {
        for (char c = 'a'; c <= 'z'; ++c) {
            def_type_map[c] = VarType::SINGLE;
        }
    }
};

// ============================================================================
// AST Visitor (for traversal)
// ============================================================================

// Get line/column from any expression
inline std::pair<int, int> expr_location(const Expr& e) {
    return std::visit([](const auto& ptr) -> std::pair<int, int> {
        return {ptr->line, ptr->column};
    }, e);
}

// Clone an expression (deep copy)
Expr clone_expr(const Expr& e);

} // namespace mbasic
