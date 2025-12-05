# MBASIC 5.21 C++ Interpreter Architecture

## Overview

This document describes the architecture for a C++ implementation of an MBASIC 5.21 interpreter, based on the Python reference implementation in `~/src/mbasic`.

## Project Structure

```
mbasicc/
├── CMakeLists.txt              # Build configuration
├── include/
│   └── mbasic/
│       ├── tokens.hpp          # Token types and Token class
│       ├── lexer.hpp           # Lexer class
│       ├── ast.hpp             # AST node definitions
│       ├── parser.hpp          # Parser class
│       ├── runtime.hpp         # Runtime state management
│       ├── interpreter.hpp     # Interpreter class
│       ├── builtins.hpp        # Built-in functions
│       ├── value.hpp           # Value type (variant for BASIC values)
│       ├── error.hpp           # Error handling
│       ├── interactive.hpp     # Interactive REPL mode
│       └── file_io.hpp         # File I/O operations
├── src/
│   ├── main.cpp                # Entry point
│   ├── tokens.cpp
│   ├── lexer.cpp
│   ├── ast.cpp
│   ├── parser.cpp
│   ├── runtime.cpp
│   ├── interpreter.cpp
│   ├── builtins.cpp
│   ├── value.cpp
│   ├── error.cpp
│   ├── interactive.cpp
│   └── file_io.cpp
└── tests/
    ├── test_lexer.cpp
    ├── test_parser.cpp
    ├── test_interpreter.cpp
    └── ...
```

## Core Components

### 1. Value System (`value.hpp`)

MBASIC has four data types. We use `std::variant` for type-safe value representation:

```cpp
#include <variant>
#include <string>
#include <cstdint>

namespace mbasic {

// MBASIC variable types
enum class VarType {
    INTEGER,    // 16-bit signed integer (% suffix)
    SINGLE,     // Single-precision float (! suffix, default)
    DOUBLE,     // Double-precision float (# suffix)
    STRING      // String ($ suffix)
};

// Runtime value - can hold any MBASIC type
using Value = std::variant<
    int16_t,        // INTEGER
    float,          // SINGLE
    double,         // DOUBLE
    std::string     // STRING
>;

// Helper functions
VarType get_type(const Value& v);
std::string value_to_string(const Value& v);
double value_to_number(const Value& v);
bool value_to_bool(const Value& v);  // 0 = false, non-zero = true

// Type coercion (MBASIC rules)
Value coerce_to(const Value& v, VarType target);

} // namespace mbasic
```

### 2. Token System (`tokens.hpp`)

```cpp
#include <string>
#include <optional>
#include <unordered_map>

namespace mbasic {

enum class TokenType {
    // Literals
    NUMBER,
    STRING,

    // Identifiers
    IDENTIFIER,

    // Keywords - Program Control
    AUTO, CONT, DELETE, EDIT, FILES, LIST, LLIST, LOAD,
    MERGE, NEW, RENUM, RUN, SAVE,

    // Keywords - File Operations
    AS, CLOSE, FIELD, GET, INPUT, KILL, LINE_INPUT,
    LSET, NAME, OPEN, OUTPUT, PUT, RESET, RSET,

    // Keywords - Control Flow
    ALL, CALL, CHAIN, ELSE, END, FOR, GOSUB, GOTO,
    IF, NEXT, ON, RESUME, RETURN, STEP, STOP,
    SYSTEM, THEN, TO, WHILE, WEND,

    // Keywords - Data/Arrays
    BASE, CLEAR, COMMON, DATA, DEF, DEFINT, DEFSNG,
    DEFDBL, DEFSTR, DIM, ERASE, FN, LET, OPTION,
    READ, RESTORE,

    // Keywords - I/O
    PRINT, LPRINT, WRITE, USING,

    // Keywords - Other
    ERROR, ERR, ERL, FRE, HELP, OUT, POKE,
    RANDOMIZE, REM, REMARK, SWAP, TRON, TROFF, WAIT, WIDTH,

    // Operators - Arithmetic
    PLUS, MINUS, MULTIPLY, DIVIDE, POWER,
    BACKSLASH,  // Integer division
    MOD,

    // Operators - Relational
    EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN,
    LESS_EQUAL, GREATER_EQUAL,

    // Operators - Logical/Bitwise
    NOT, AND, OR, XOR, EQV, IMP,

    // Built-in Functions - Numeric
    ABS, ATN, CDBL, CINT, COS, CSNG, CVD, CVI, CVS,
    EXP, FIX, INT, LOG, RND, SGN, SIN, SQR, TAN,

    // Built-in Functions - String
    ASC, CHR, HEX, INKEY, INPUT_FUNC, INSTR, LEFT, LEN,
    MID, MKD, MKI, MKS, OCT, RIGHT, SPACE, STR,
    STRING_FUNC, VAL,

    // Built-in Functions - Other
    EOF_FUNC, INP, LOC, LOF, PEEK, POS, SPC, TAB, USR, VARPTR,

    // Delimiters
    LPAREN, RPAREN, COMMA, SEMICOLON, COLON, HASH,
    AMPERSAND,

    // Special
    NEWLINE, LINE_NUMBER, END_OF_FILE, QUESTION, APOSTROPHE
};

struct Token {
    TokenType type;
    std::string value;           // Normalized value (lowercase)
    int line;
    int column;
    std::string original_case;   // Original case for identifiers

    Token(TokenType t, std::string v, int l, int c);
};

// Keyword lookup table
extern const std::unordered_map<std::string, TokenType> KEYWORDS;

} // namespace mbasic
```

### 3. Lexer (`lexer.hpp`)

```cpp
#include <vector>
#include <string>
#include <stdexcept>
#include "tokens.hpp"

namespace mbasic {

class LexerError : public std::runtime_error {
public:
    int line;
    int column;
    LexerError(const std::string& msg, int l, int c);
};

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;

    char current() const;
    char peek(int offset = 1) const;
    char advance();
    void skip_whitespace();

    Token read_number();
    Token read_string();
    Token read_identifier();
    Token read_line_number();
    std::string read_comment();
};

// Convenience function
std::vector<Token> tokenize(const std::string& source);

} // namespace mbasic
```

### 4. AST Nodes (`ast.hpp`)

Using a modern C++ approach with `std::variant` for node types:

```cpp
#include <vector>
#include <optional>
#include <memory>
#include <variant>
#include "tokens.hpp"
#include "value.hpp"

namespace mbasic {

// Forward declarations
struct NumberNode;
struct StringNode;
struct VariableNode;
struct BinaryOpNode;
struct UnaryOpNode;
struct FunctionCallNode;
struct ArrayAccessNode;

// Expression node variant
using ExpressionNode = std::variant<
    std::unique_ptr<NumberNode>,
    std::unique_ptr<StringNode>,
    std::unique_ptr<VariableNode>,
    std::unique_ptr<BinaryOpNode>,
    std::unique_ptr<UnaryOpNode>,
    std::unique_ptr<FunctionCallNode>,
    std::unique_ptr<ArrayAccessNode>
>;

// Expression nodes
struct NumberNode {
    double value;
    int line, column;
};

struct StringNode {
    std::string value;
    int line, column;
};

struct VariableNode {
    std::string name;       // Normalized (lowercase with suffix)
    std::string original;   // Original case
    VarType type;
    int line, column;
};

struct BinaryOpNode {
    TokenType op;
    ExpressionNode left;
    ExpressionNode right;
    int line, column;
};

struct UnaryOpNode {
    TokenType op;
    ExpressionNode operand;
    int line, column;
};

struct FunctionCallNode {
    std::string name;
    std::vector<ExpressionNode> args;
    int line, column;
};

struct ArrayAccessNode {
    std::string name;
    std::vector<ExpressionNode> indices;
    VarType type;
    int line, column;
};

// Forward declarations for statement nodes
struct PrintStatementNode;
struct LetStatementNode;
struct IfStatementNode;
struct ForStatementNode;
struct NextStatementNode;
struct WhileStatementNode;
struct WendStatementNode;
struct GotoStatementNode;
struct GosubStatementNode;
struct ReturnStatementNode;
struct InputStatementNode;
struct DataStatementNode;
struct ReadStatementNode;
struct RestoreStatementNode;
struct DimStatementNode;
struct DefFnStatementNode;
struct DefTypeStatementNode;
struct EndStatementNode;
struct StopStatementNode;
struct RemStatementNode;
struct OnGotoStatementNode;
struct OnGosubStatementNode;
struct OpenStatementNode;
struct CloseStatementNode;
struct PrintFileStatementNode;
struct InputFileStatementNode;
struct FieldStatementNode;
struct GetStatementNode;
struct PutStatementNode;
struct LsetStatementNode;
struct RsetStatementNode;
struct SwapStatementNode;
struct EraseStatementNode;
struct ClearStatementNode;
struct RandomizeStatementNode;
struct OptionBaseStatementNode;
struct TronStatementNode;
struct TroffStatementNode;
struct WidthStatementNode;
struct ErrorStatementNode;
struct OnErrorStatementNode;
struct ResumeStatementNode;
struct PokeStatementNode;
struct ChainStatementNode;
struct CommonStatementNode;
struct MidAssignStatementNode;
// ... more as needed

// Statement node variant
using StatementNode = std::variant<
    std::unique_ptr<PrintStatementNode>,
    std::unique_ptr<LetStatementNode>,
    std::unique_ptr<IfStatementNode>,
    std::unique_ptr<ForStatementNode>,
    std::unique_ptr<NextStatementNode>,
    std::unique_ptr<WhileStatementNode>,
    std::unique_ptr<WendStatementNode>,
    std::unique_ptr<GotoStatementNode>,
    std::unique_ptr<GosubStatementNode>,
    std::unique_ptr<ReturnStatementNode>,
    std::unique_ptr<InputStatementNode>,
    std::unique_ptr<DataStatementNode>,
    std::unique_ptr<ReadStatementNode>,
    std::unique_ptr<RestoreStatementNode>,
    std::unique_ptr<DimStatementNode>,
    std::unique_ptr<DefFnStatementNode>,
    std::unique_ptr<DefTypeStatementNode>,
    std::unique_ptr<EndStatementNode>,
    std::unique_ptr<StopStatementNode>,
    std::unique_ptr<RemStatementNode>,
    std::unique_ptr<OnGotoStatementNode>,
    std::unique_ptr<OnGosubStatementNode>,
    std::unique_ptr<OpenStatementNode>,
    std::unique_ptr<CloseStatementNode>,
    std::unique_ptr<PrintFileStatementNode>,
    std::unique_ptr<InputFileStatementNode>,
    std::unique_ptr<FieldStatementNode>,
    std::unique_ptr<GetStatementNode>,
    std::unique_ptr<PutStatementNode>,
    std::unique_ptr<LsetStatementNode>,
    std::unique_ptr<RsetStatementNode>,
    std::unique_ptr<SwapStatementNode>,
    std::unique_ptr<EraseStatementNode>,
    std::unique_ptr<ClearStatementNode>,
    std::unique_ptr<RandomizeStatementNode>,
    std::unique_ptr<OptionBaseStatementNode>,
    std::unique_ptr<TronStatementNode>,
    std::unique_ptr<TroffStatementNode>,
    std::unique_ptr<WidthStatementNode>,
    std::unique_ptr<ErrorStatementNode>,
    std::unique_ptr<OnErrorStatementNode>,
    std::unique_ptr<ResumeStatementNode>,
    std::unique_ptr<PokeStatementNode>,
    std::unique_ptr<ChainStatementNode>,
    std::unique_ptr<CommonStatementNode>,
    std::unique_ptr<MidAssignStatementNode>
    // ... more as needed
>;

// Statement node definitions
struct PrintStatementNode {
    std::vector<ExpressionNode> expressions;
    std::vector<char> separators;  // ';' or ',' or '\0' for newline
    std::optional<ExpressionNode> file_number;
    int line, column;
    int char_start, char_end;
};

struct LetStatementNode {
    VariableNode variable;
    ExpressionNode expression;
    int line, column;
    int char_start, char_end;
};

struct IfStatementNode {
    ExpressionNode condition;
    std::vector<StatementNode> then_statements;
    std::optional<int> then_line_number;
    std::vector<StatementNode> else_statements;
    std::optional<int> else_line_number;
    int line, column;
    int char_start, char_end;
};

struct ForStatementNode {
    VariableNode variable;
    ExpressionNode start_expr;
    ExpressionNode end_expr;
    std::optional<ExpressionNode> step_expr;
    int line, column;
    int char_start, char_end;
};

struct NextStatementNode {
    std::vector<VariableNode> variables;
    int line, column;
    int char_start, char_end;
};

struct WhileStatementNode {
    ExpressionNode condition;
    int line, column;
    int char_start, char_end;
};

struct WendStatementNode {
    int line, column;
    int char_start, char_end;
};

struct GotoStatementNode {
    int target_line;
    int line, column;
    int char_start, char_end;
};

struct GosubStatementNode {
    int target_line;
    int line, column;
    int char_start, char_end;
};

struct ReturnStatementNode {
    std::optional<int> target_line;  // RETURN can have optional line number
    int line, column;
    int char_start, char_end;
};

struct InputStatementNode {
    std::optional<ExpressionNode> prompt;
    std::vector<VariableNode> variables;
    std::optional<ExpressionNode> file_number;
    bool suppress_question = false;
    int line, column;
    int char_start, char_end;
};

struct DataStatementNode {
    std::vector<Value> values;
    int line, column;
    int char_start, char_end;
};

struct ReadStatementNode {
    std::vector<VariableNode> variables;
    int line, column;
    int char_start, char_end;
};

struct RestoreStatementNode {
    std::optional<int> target_line;
    int line, column;
    int char_start, char_end;
};

struct DimStatementNode {
    struct ArrayDecl {
        std::string name;
        std::vector<ExpressionNode> dimensions;
        VarType type;
    };
    std::vector<ArrayDecl> arrays;
    int line, column;
    int char_start, char_end;
};

struct DefFnStatementNode {
    std::string name;
    std::vector<std::string> params;
    ExpressionNode body;
    int line, column;
    int char_start, char_end;
};

struct DefTypeStatementNode {
    VarType type;
    std::vector<std::pair<char, char>> ranges;  // e.g., ('A', 'Z')
    int line, column;
    int char_start, char_end;
};

struct EndStatementNode {
    int line, column;
    int char_start, char_end;
};

struct StopStatementNode {
    int line, column;
    int char_start, char_end;
};

struct RemStatementNode {
    std::string comment;
    int line, column;
    int char_start, char_end;
};

// ... more statement nodes as needed

// Line and Program nodes
struct LineNode {
    int line_number;
    std::vector<StatementNode> statements;
};

struct ProgramNode {
    std::vector<LineNode> lines;
    std::unordered_map<char, VarType> def_type_map;
};

} // namespace mbasic
```

### 5. Parser (`parser.hpp`)

Two-pass recursive descent parser:

```cpp
#include <vector>
#include <memory>
#include <unordered_map>
#include "tokens.hpp"
#include "ast.hpp"

namespace mbasic {

class ParseError : public std::runtime_error {
public:
    int line;
    int column;
    ParseError(const std::string& msg, int l, int c);
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    ProgramNode parse();

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    // Type defaults from DEF statements
    std::unordered_map<char, VarType> def_type_map_;

    // Two-pass parsing
    void collect_def_types();

    // Token access
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token expect(TokenType type, const std::string& msg);

    // Parse methods
    LineNode parse_line();
    StatementNode parse_statement();

    // Statement parsers
    StatementNode parse_print();
    StatementNode parse_let();
    StatementNode parse_if();
    StatementNode parse_for();
    StatementNode parse_next();
    StatementNode parse_while();
    StatementNode parse_wend();
    StatementNode parse_goto();
    StatementNode parse_gosub();
    StatementNode parse_return();
    StatementNode parse_input();
    StatementNode parse_data();
    StatementNode parse_read();
    StatementNode parse_restore();
    StatementNode parse_dim();
    StatementNode parse_def_fn();
    StatementNode parse_def_type();
    StatementNode parse_on();
    StatementNode parse_open();
    StatementNode parse_close();
    // ... more as needed

    // Expression parsing (precedence climbing)
    ExpressionNode parse_expression();
    ExpressionNode parse_or_expr();
    ExpressionNode parse_and_expr();
    ExpressionNode parse_not_expr();
    ExpressionNode parse_comparison();
    ExpressionNode parse_additive();
    ExpressionNode parse_multiplicative();
    ExpressionNode parse_power();
    ExpressionNode parse_unary();
    ExpressionNode parse_primary();
    ExpressionNode parse_function_call();
    ExpressionNode parse_variable_or_array();

    // Type resolution
    VarType resolve_variable_type(const std::string& name);
};

} // namespace mbasic
```

### 6. Program Counter (`pc.hpp`)

```cpp
#include <optional>

namespace mbasic {

// Reason execution stopped
enum class StopReason {
    RUNNING,       // Still executing
    END,           // END statement
    STOP,          // STOP statement
    BREAKPOINT,    // Hit breakpoint
    ERROR,         // Runtime error
    INPUT,         // Waiting for input
    BREAK          // User break (Ctrl+C)
};

// Program counter - tracks execution position
struct PC {
    int line = 0;
    int stmt_offset = 0;
    StopReason stop_reason = StopReason::RUNNING;

    bool is_running() const { return stop_reason == StopReason::RUNNING; }
    bool is_halted() const { return !is_running(); }

    static PC running_at(int line, int stmt) {
        return PC{line, stmt, StopReason::RUNNING};
    }

    static PC halted() {
        return PC{0, 0, StopReason::END};
    }

    bool operator==(const PC& other) const {
        return line == other.line && stmt_offset == other.stmt_offset;
    }
};

} // namespace mbasic
```

### 7. Runtime (`runtime.hpp`)

Manages execution state:

```cpp
#include <unordered_map>
#include <vector>
#include <stack>
#include <optional>
#include <fstream>
#include "value.hpp"
#include "ast.hpp"
#include "pc.hpp"

namespace mbasic {

// FOR loop state
struct ForLoopState {
    PC resume_pc;
    double end_value;
    double step_value;
};

// Execution stack entry
struct StackEntry {
    enum class Type { GOSUB, WHILE };
    Type type;
    PC return_pc;  // For GOSUB
    PC loop_pc;    // For WHILE
};

// Statement lookup table
class StatementTable {
public:
    void add(PC pc, StatementNode* stmt);
    StatementNode* get(const PC& pc) const;
    PC first_pc() const;
    PC next_pc(const PC& current) const;
    PC find_line(int line_num) const;

private:
    std::map<std::pair<int,int>, StatementNode*> table_;
};

class Runtime {
public:
    explicit Runtime(ProgramNode* program);

    void setup();
    void reset();

    // Variable access
    Value get_variable(const std::string& name);
    void set_variable(const std::string& name, const Value& value);

    // Array access
    Value get_array(const std::string& name, const std::vector<int>& indices);
    void set_array(const std::string& name, const std::vector<int>& indices, const Value& value);
    void dim_array(const std::string& name, const std::vector<int>& dimensions, VarType type);
    void erase_array(const std::string& name);

    // Execution control
    PC pc;
    std::optional<PC> next_pc;  // Set by GOTO/GOSUB, cleared after use
    StatementTable statement_table;

    // Control flow stacks
    std::vector<StackEntry> execution_stack;
    std::unordered_map<std::string, ForLoopState> for_loop_states;

    // DATA/READ
    std::vector<Value> data_items;
    size_t data_pointer = 0;

    // User-defined functions
    std::unordered_map<std::string, DefFnStatementNode*> user_functions;

    // File I/O
    std::unordered_map<int, std::fstream> files;
    std::unordered_map<int, std::string> field_buffers;

    // Error handling
    std::optional<int> error_handler_line;
    bool error_handler_is_gosub = false;

    // State
    int array_base = 0;  // OPTION BASE
    bool trace_on = false;
    double rnd_last = 0.5;
    std::set<PC> breakpoints;
    bool break_requested = false;

    // DEF type map
    std::unordered_map<char, VarType> def_type_map;

    // Line text for error messages
    std::unordered_map<int, std::string> line_text_map;

private:
    ProgramNode* program_;
    std::unordered_map<std::string, Value> variables_;

    struct ArrayData {
        std::vector<int> dimensions;
        std::vector<Value> data;
        VarType type;
    };
    std::unordered_map<std::string, ArrayData> arrays_;

    VarType resolve_type(const std::string& name);
    size_t array_index(const ArrayData& arr, const std::vector<int>& indices);
};

} // namespace mbasic
```

### 8. Interpreter (`interpreter.hpp`)

Tick-based execution for UI integration:

```cpp
#include <functional>
#include <optional>
#include "runtime.hpp"
#include "builtins.hpp"

namespace mbasic {

// Error information
struct ErrorInfo {
    int error_code;
    PC pc;
    std::string message;
};

// I/O Handler interface
class IOHandler {
public:
    virtual ~IOHandler() = default;
    virtual void print(const std::string& text) = 0;
    virtual std::string input() = 0;
    virtual std::optional<char> inkey() = 0;  // Non-blocking
};

// Console I/O implementation
class ConsoleIOHandler : public IOHandler {
public:
    void print(const std::string& text) override;
    std::string input() override;
    std::optional<char> inkey() override;
};

// Interpreter state
struct InterpreterState {
    // Input handling
    std::optional<std::string> input_prompt;
    std::vector<VariableNode> input_variables;
    std::vector<std::string> input_buffer;
    std::optional<int> input_file_number;

    // Debugging
    bool skip_next_breakpoint = false;
    bool pause_requested = false;

    // Error handling
    std::optional<ErrorInfo> error_info;

    // Performance
    size_t statements_executed = 0;
    double execution_time_ms = 0.0;
};

class Interpreter {
public:
    Interpreter(Runtime& runtime, IOHandler* io = nullptr);

    // Run entire program
    void run();

    // Tick-based execution (one statement at a time)
    bool tick();  // Returns true if still running

    // Control
    void pause();
    void resume();
    void stop();

    // Input handling
    void provide_input(const std::string& input);

    // State access
    Runtime& runtime() { return runtime_; }
    const InterpreterState& state() const { return state_; }

private:
    Runtime& runtime_;
    std::unique_ptr<IOHandler> io_owned_;
    IOHandler* io_;
    BuiltinFunctions builtins_;
    InterpreterState state_;

    // Statement execution
    void execute_statement(StatementNode& stmt);
    void execute_print(PrintStatementNode& stmt);
    void execute_let(LetStatementNode& stmt);
    void execute_if(IfStatementNode& stmt);
    void execute_for(ForStatementNode& stmt);
    void execute_next(NextStatementNode& stmt);
    void execute_while(WhileStatementNode& stmt);
    void execute_wend(WendStatementNode& stmt);
    void execute_goto(GotoStatementNode& stmt);
    void execute_gosub(GosubStatementNode& stmt);
    void execute_return(ReturnStatementNode& stmt);
    void execute_input(InputStatementNode& stmt);
    void execute_read(ReadStatementNode& stmt);
    void execute_restore(RestoreStatementNode& stmt);
    void execute_dim(DimStatementNode& stmt);
    void execute_on_goto(OnGotoStatementNode& stmt);
    void execute_on_gosub(OnGosubStatementNode& stmt);
    void execute_open(OpenStatementNode& stmt);
    void execute_close(CloseStatementNode& stmt);
    // ... more as needed

    // Expression evaluation
    Value evaluate(const ExpressionNode& expr);
    Value evaluate_binary(const BinaryOpNode& node);
    Value evaluate_unary(const UnaryOpNode& node);
    Value evaluate_function(const FunctionCallNode& node);

    // Error handling
    void raise_error(int code, const std::string& msg);
};

} // namespace mbasic
```

### 9. Built-in Functions (`builtins.hpp`)

```cpp
#include <functional>
#include <unordered_map>
#include "value.hpp"

namespace mbasic {

class Runtime;  // Forward declaration

class BuiltinFunctions {
public:
    explicit BuiltinFunctions(Runtime& runtime);

    // Call a built-in function
    Value call(const std::string& name, const std::vector<Value>& args);

private:
    Runtime& runtime_;

    // Math functions
    Value fn_abs(const std::vector<Value>& args);
    Value fn_atn(const std::vector<Value>& args);
    Value fn_cos(const std::vector<Value>& args);
    Value fn_exp(const std::vector<Value>& args);
    Value fn_fix(const std::vector<Value>& args);
    Value fn_int(const std::vector<Value>& args);
    Value fn_log(const std::vector<Value>& args);
    Value fn_rnd(const std::vector<Value>& args);
    Value fn_sgn(const std::vector<Value>& args);
    Value fn_sin(const std::vector<Value>& args);
    Value fn_sqr(const std::vector<Value>& args);
    Value fn_tan(const std::vector<Value>& args);

    // Type conversion
    Value fn_cint(const std::vector<Value>& args);
    Value fn_csng(const std::vector<Value>& args);
    Value fn_cdbl(const std::vector<Value>& args);
    Value fn_cvi(const std::vector<Value>& args);
    Value fn_cvs(const std::vector<Value>& args);
    Value fn_cvd(const std::vector<Value>& args);
    Value fn_mki(const std::vector<Value>& args);
    Value fn_mks(const std::vector<Value>& args);
    Value fn_mkd(const std::vector<Value>& args);

    // String functions
    Value fn_asc(const std::vector<Value>& args);
    Value fn_chr(const std::vector<Value>& args);
    Value fn_hex(const std::vector<Value>& args);
    Value fn_instr(const std::vector<Value>& args);
    Value fn_left(const std::vector<Value>& args);
    Value fn_len(const std::vector<Value>& args);
    Value fn_mid(const std::vector<Value>& args);
    Value fn_oct(const std::vector<Value>& args);
    Value fn_right(const std::vector<Value>& args);
    Value fn_space(const std::vector<Value>& args);
    Value fn_str(const std::vector<Value>& args);
    Value fn_string(const std::vector<Value>& args);
    Value fn_val(const std::vector<Value>& args);

    // I/O functions
    Value fn_eof(const std::vector<Value>& args);
    Value fn_inkey(const std::vector<Value>& args);
    Value fn_input(const std::vector<Value>& args);
    Value fn_loc(const std::vector<Value>& args);
    Value fn_lof(const std::vector<Value>& args);
    Value fn_pos(const std::vector<Value>& args);

    // Special
    Value fn_fre(const std::vector<Value>& args);
    Value fn_peek(const std::vector<Value>& args);
    Value fn_tab(const std::vector<Value>& args);
    Value fn_spc(const std::vector<Value>& args);
    Value fn_usr(const std::vector<Value>& args);
    Value fn_varptr(const std::vector<Value>& args);

    // Function dispatch table
    using FnPtr = Value (BuiltinFunctions::*)(const std::vector<Value>&);
    std::unordered_map<std::string, FnPtr> functions_;
};

// Special marker types for TAB and SPC
struct TabMarker { int column; };
struct SpcMarker { int count; };

// PRINT USING formatter
class UsingFormatter {
public:
    explicit UsingFormatter(const std::string& format);
    std::string format(const std::vector<Value>& values);

private:
    std::string format_string_;
    // ... format field parsing
};

} // namespace mbasic
```

### 10. Interactive Mode (`interactive.hpp`)

```cpp
#include <string>
#include <memory>
#include "ast.hpp"
#include "runtime.hpp"
#include "interpreter.hpp"

namespace mbasic {

class InteractiveMode {
public:
    InteractiveMode();

    // Main REPL loop
    void run();

    // Process a single line of input
    void process_line(const std::string& line);

private:
    // Program storage
    std::map<int, LineNode> program_lines_;
    std::unique_ptr<ProgramNode> current_program_;

    // DEF type map
    std::unordered_map<char, VarType> def_type_map_;

    // Runtime for immediate mode
    std::unique_ptr<Runtime> immediate_runtime_;
    std::unique_ptr<Interpreter> immediate_interpreter_;

    // Runtime for RUN (preserved for CONT)
    std::unique_ptr<Runtime> program_runtime_;
    std::unique_ptr<Interpreter> program_interpreter_;

    // Current file
    std::string current_file_;

    // AUTO mode state
    bool auto_mode_ = false;
    int auto_line_ = 10;
    int auto_step_ = 10;

    // Commands
    void cmd_run(const std::string& args);
    void cmd_list(const std::string& args);
    void cmd_new();
    void cmd_load(const std::string& filename);
    void cmd_save(const std::string& filename);
    void cmd_delete(const std::string& range);
    void cmd_renum(const std::string& args);
    void cmd_auto(const std::string& args);
    void cmd_edit(int line_num);
    void cmd_cont();
    void cmd_files(const std::string& pattern);
    void cmd_help(const std::string& topic);

    // Helper methods
    void insert_line(int line_num, const std::string& text);
    void delete_line(int line_num);
    void rebuild_program();
    void execute_immediate(const std::string& statement);
};

} // namespace mbasic
```

## Build System (CMakeLists.txt)

```cmake
cmake_minimum_required(VERSION 3.16)
project(mbasic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main library
add_library(mbasic_lib
    src/tokens.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/runtime.cpp
    src/interpreter.cpp
    src/builtins.cpp
    src/value.cpp
    src/error.cpp
    src/interactive.cpp
    src/file_io.cpp
)

target_include_directories(mbasic_lib PUBLIC include)

# Main executable
add_executable(mbasic src/main.cpp)
target_link_libraries(mbasic mbasic_lib)

# Tests (using Catch2 or Google Test)
enable_testing()
add_subdirectory(tests)
```

## Implementation Phases

### Phase 1: Core Infrastructure
1. Value system with type coercion
2. Token definitions and keyword table
3. Lexer with full token support
4. Basic error handling

### Phase 2: Parser
1. Two-pass architecture
2. Expression parsing with operator precedence
3. Core statements: PRINT, LET, IF, FOR/NEXT, WHILE/WEND, GOTO, GOSUB/RETURN
4. DATA/READ/RESTORE

### Phase 3: Interpreter Core
1. Runtime state management
2. Variable and array storage
3. PC-based execution
4. Core statement execution
5. Expression evaluation

### Phase 4: Built-in Functions
1. Math functions (SIN, COS, SQR, etc.)
2. String functions (LEFT$, MID$, RIGHT$, etc.)
3. Type conversion (CINT, CSNG, VAL, STR$, etc.)
4. Special functions (TAB, SPC, RND)

### Phase 5: File I/O
1. Sequential file operations (OPEN, CLOSE, PRINT#, INPUT#)
2. Random file operations (FIELD, GET, PUT, LSET, RSET)
3. Binary conversion (MKI$, CVI, etc.)

### Phase 6: Interactive Mode
1. REPL loop
2. Line editing and storage
3. Commands: RUN, LIST, NEW, LOAD, SAVE
4. AUTO and EDIT modes
5. CONT for resuming after STOP

### Phase 7: Advanced Features
1. Error handling (ON ERROR, RESUME)
2. CHAIN and COMMON
3. TRON/TROFF debugging
4. Remaining statements

### Phase 8: Testing and Compatibility
1. Unit tests for each component
2. Integration tests with .bas programs
3. Compatibility testing against Python implementation

## Design Decisions

### 1. Use of `std::variant` for AST
- Type-safe alternative to inheritance hierarchies
- Compile-time exhaustiveness checking with `std::visit`
- Memory efficient (no vtable overhead)

### 2. Smart Pointers in AST
- `std::unique_ptr` for ownership semantics
- Prevents memory leaks
- Clear ownership model

### 3. Tick-based Execution
- Allows UI integration without threading
- Enables breakpoints and stepping
- Supports async input handling

### 4. Two-pass Parser
- First pass collects DEF type statements
- Second pass parses with type information available
- Matches Python implementation behavior

### 5. Case-insensitive Keywords
- Keywords normalized to lowercase internally
- Original case preserved for display
- Variable names preserve user's case preference

## Error Codes

Standard MBASIC error codes (from reference manual):
- 1: NEXT without FOR
- 2: Syntax error
- 3: RETURN without GOSUB
- 4: Out of DATA
- 5: Illegal function call
- 6: Overflow
- 7: Out of memory
- 8: Undefined line number
- 9: Subscript out of range
- 10: Duplicate definition
- 11: Division by zero
- 12: Illegal direct
- 13: Type mismatch
- 14: Out of string space
- 15: String too long
- 16: String formula too complex
- 17: Can't continue
- 18: Undefined user function
- 50-69: File I/O errors

## Testing Strategy

1. **Unit Tests**: Test each component in isolation
2. **Integration Tests**: Run .bas programs from test suite
3. **Comparison Tests**: Compare output with Python implementation
4. **Fuzz Testing**: Random input to find edge cases

## Dependencies

- C++17 compiler (GCC 8+, Clang 7+, MSVC 2017+)
- CMake 3.16+
- Optional: Catch2 or Google Test for testing
- Optional: readline for better REPL experience
