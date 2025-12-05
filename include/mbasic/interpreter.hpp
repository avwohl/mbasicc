#pragma once
// MBASICC - MBASIC 5.21 C++ Interpreter
// https://github.com/jwoehr/mbasic_cc

#include <functional>
#include <optional>
#include <memory>
#include "io_handler.hpp"
#include "runtime.hpp"
#include "ast.hpp"

namespace mbasic {

// ============================================================================
// Interpreter State
// ============================================================================

struct InterpreterState {
    // Input handling
    std::optional<std::string> input_prompt;
    std::vector<std::string> pending_vars;  // Variables waiting for input
    std::vector<std::string> input_buffer;  // Buffered input values
    std::optional<int> input_file;          // Reading from file

    // Error info
    struct ErrorInfo {
        int code;
        PC pc;
        std::string message;
    };
    std::optional<ErrorInfo> error;

    // Stats
    size_t statements_executed = 0;

    // Pause control
    bool pause_requested = false;
    bool skip_next_breakpoint = false;

    // CHAIN request
    struct ChainRequest {
        std::string filename;
        std::optional<int> line_number;
        bool all = false;      // Keep all variables
        bool merge = false;    // Merge program
    };
    std::optional<ChainRequest> chain_request;

    // RUN request (load and run new program)
    struct RunRequest {
        std::string filename;
        std::optional<int> start_line;
        bool keep_variables = false;
    };
    std::optional<RunRequest> run_request;
};

// ============================================================================
// Interpreter
// ============================================================================

class Interpreter {
public:
    Interpreter(Runtime& runtime, IOHandler* io = nullptr);

    // Run entire program
    void run();

    // Execute one statement (tick)
    bool tick();  // Returns true if still running

    // Control
    void pause() { state_.pause_requested = true; }
    void resume() { state_.pause_requested = false; }
    void stop();

    // Input handling
    void provide_input(const std::string& input);

    // Accessors
    Runtime& runtime() { return runtime_; }
    const Runtime& runtime() const { return runtime_; }
    const InterpreterState& state() const { return state_; }
    IOHandler& io() { return *io_; }

private:
    Runtime& runtime_;
    std::unique_ptr<IOHandler> io_owned_;
    IOHandler* io_;
    InterpreterState state_;

    // Statement execution
    void execute(Stmt& stmt);

    void exec_print(PrintStmt& s);
    void exec_print_using(PrintUsingStmt& s);
    void exec_lprint(LprintStmt& s);
    void exec_lprint_using(LprintUsingStmt& s);
    void exec_input(InputStmt& s);
    void exec_line_input(LineInputStmt& s);
    void exec_let(LetStmt& s);
    void exec_if(IfStmt& s);
    void exec_for(ForStmt& s);
    void exec_next(NextStmt& s);
    void exec_while(WhileStmt& s);
    void exec_wend(WendStmt& s);
    void exec_goto(GotoStmt& s);
    void exec_gosub(GosubStmt& s);
    void exec_return(ReturnStmt& s);
    void exec_on_goto(OnGotoStmt& s);
    void exec_on_gosub(OnGosubStmt& s);
    void exec_data(DataStmt& s);
    void exec_read(ReadStmt& s);
    void exec_restore(RestoreStmt& s);
    void exec_dim(DimStmt& s);
    void exec_def_fn(DefFnStmt& s);
    void exec_def_type(DefTypeStmt& s);
    void exec_end(EndStmt& s);
    void exec_cls(ClsStmt& s);
    void exec_stop(StopStmt& s);
    void exec_rem(RemStmt& s);
    void exec_swap(SwapStmt& s);
    void exec_erase(EraseStmt& s);
    void exec_clear(ClearStmt& s);
    void exec_option_base(OptionBaseStmt& s);
    void exec_randomize(RandomizeStmt& s);
    void exec_tron(TronStmt& s);
    void exec_troff(TroffStmt& s);
    void exec_width(WidthStmt& s);
    void exec_poke(PokeStmt& s);
    void exec_error(ErrorStmt& s);
    void exec_on_error(OnErrorStmt& s);
    void exec_resume(ResumeStmt& s);
    void exec_open(OpenStmt& s);
    void exec_close(CloseStmt& s);
    void exec_field(FieldStmt& s);
    void exec_get(GetStmt& s);
    void exec_put(PutStmt& s);
    void exec_lset(LsetStmt& s);
    void exec_rset(RsetStmt& s);
    void exec_write(WriteStmt& s);
    void exec_chain(ChainStmt& s);
    void exec_common(CommonStmt& s);
    void exec_mid_assign(MidAssignStmt& s);
    void exec_call(CallStmt& s);
    void exec_out(OutStmt& s);
    void exec_wait(WaitStmt& s);
    void exec_kill(KillStmt& s);
    void exec_name(NameStmt& s);
    void exec_merge(MergeStmt& s);
    void exec_run(RunStmt& s);

    // Expression evaluation
    Value eval(const Expr& expr);
    Value eval_binary(const BinaryExpr& e);
    Value eval_unary(const UnaryExpr& e);
    Value eval_function(const FunctionCallExpr& e);
    Value eval_user_function(const std::string& name, const std::vector<Value>& args);

    // Built-in functions
    Value builtin_abs(const std::vector<Value>& args);
    Value builtin_atn(const std::vector<Value>& args);
    Value builtin_cos(const std::vector<Value>& args);
    Value builtin_exp(const std::vector<Value>& args);
    Value builtin_fix(const std::vector<Value>& args);
    Value builtin_int(const std::vector<Value>& args);
    Value builtin_log(const std::vector<Value>& args);
    Value builtin_rnd(const std::vector<Value>& args);
    Value builtin_sgn(const std::vector<Value>& args);
    Value builtin_sin(const std::vector<Value>& args);
    Value builtin_sqr(const std::vector<Value>& args);
    Value builtin_tan(const std::vector<Value>& args);
    Value builtin_cint(const std::vector<Value>& args);
    Value builtin_csng(const std::vector<Value>& args);
    Value builtin_cdbl(const std::vector<Value>& args);
    Value builtin_asc(const std::vector<Value>& args);
    Value builtin_chr(const std::vector<Value>& args);
    Value builtin_hex(const std::vector<Value>& args);
    Value builtin_oct(const std::vector<Value>& args);
    Value builtin_left(const std::vector<Value>& args);
    Value builtin_right(const std::vector<Value>& args);
    Value builtin_mid(const std::vector<Value>& args);
    Value builtin_len(const std::vector<Value>& args);
    Value builtin_str(const std::vector<Value>& args);
    Value builtin_val(const std::vector<Value>& args);
    Value builtin_space(const std::vector<Value>& args);
    Value builtin_string(const std::vector<Value>& args);
    Value builtin_instr(const std::vector<Value>& args);
    Value builtin_tab(const std::vector<Value>& args);
    Value builtin_spc(const std::vector<Value>& args);
    Value builtin_fre(const std::vector<Value>& args);
    Value builtin_pos(const std::vector<Value>& args);
    Value builtin_peek(const std::vector<Value>& args);
    Value builtin_inp(const std::vector<Value>& args);
    Value builtin_eof(const std::vector<Value>& args);
    Value builtin_lof(const std::vector<Value>& args);
    Value builtin_loc(const std::vector<Value>& args);
    Value builtin_cvi(const std::vector<Value>& args);
    Value builtin_cvs(const std::vector<Value>& args);
    Value builtin_cvd(const std::vector<Value>& args);
    Value builtin_mki(const std::vector<Value>& args);
    Value builtin_mks(const std::vector<Value>& args);
    Value builtin_mkd(const std::vector<Value>& args);
    Value builtin_inkey(const std::vector<Value>& args);
    Value builtin_input_func(const std::vector<Value>& args);
    Value builtin_lpos(const std::vector<Value>& args);
    Value builtin_erl(const std::vector<Value>& args);
    Value builtin_err(const std::vector<Value>& args);
    Value builtin_timer(const std::vector<Value>& args);
    Value builtin_date(const std::vector<Value>& args);
    Value builtin_time(const std::vector<Value>& args);
    Value builtin_environ(const std::vector<Value>& args);
    Value builtin_error_str(const std::vector<Value>& args);

    // Helpers
    void raise_error(int code, const std::string& msg);
    void advance_pc();
    void jump_to(int line);

    // Get value from lvalue
    Value get_lvalue(const std::variant<VariableExpr, ArrayAccessExpr>& lv);
    void set_lvalue(const std::variant<VariableExpr, ArrayAccessExpr>& lv, const Value& val);
};

} // namespace mbasic
