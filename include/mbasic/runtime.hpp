#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <stack>
#include <optional>
#include <fstream>
#include <set>
#include <functional>
#include "value.hpp"
#include "ast.hpp"
#include "error.hpp"

namespace mbasic {

// ============================================================================
// Program Counter
// ============================================================================

enum class StopReason {
    RUNNING,       // Still executing
    END,           // END statement
    STOP,          // STOP statement
    BREAKPOINT,    // Hit breakpoint
    ERROR,         // Runtime error
    INPUT,         // Waiting for input
    BREAK          // User break (Ctrl+C)
};

struct PC {
    int line = 0;
    int stmt = 0;
    StopReason reason = StopReason::RUNNING;

    bool is_running() const { return reason == StopReason::RUNNING; }
    bool is_halted() const { return !is_running(); }

    static PC running_at(int l, int s) {
        return PC{l, s, StopReason::RUNNING};
    }

    static PC halted(StopReason r = StopReason::END) {
        return PC{0, 0, r};
    }

    bool operator==(const PC& other) const {
        return line == other.line && stmt == other.stmt;
    }

    bool operator<(const PC& other) const {
        if (line != other.line) return line < other.line;
        return stmt < other.stmt;
    }
};

// ============================================================================
// FOR Loop State
// ============================================================================

struct ForLoopState {
    PC resume_pc;       // PC to return to after NEXT
    double end_value;   // Loop termination value
    double step_value;  // Step value
};

// ============================================================================
// Execution Stack Entry
// ============================================================================

struct StackEntry {
    enum class Type { GOSUB, WHILE };
    Type type;
    PC return_pc;    // For GOSUB
    PC loop_pc;      // For WHILE (to check condition again)
};

// ============================================================================
// Statement Table
// ============================================================================

class StatementTable {
public:
    void build(Program& program);

    // Merge lines from another program (MERGE command)
    void merge(Program& program);

    // Get statement at PC
    Stmt* get(const PC& pc);

    // Get first PC
    PC first() const;

    // Get next PC (sequential)
    PC next(const PC& current) const;

    // Find PC for line number
    PC find_line(int line_num) const;

    // Check if PC is valid
    bool valid(const PC& pc) const;

    // Get line text for error messages
    const std::string& line_text(int line_num) const;

    // Storage for merged statements (must persist for lifetime of table)
    std::vector<std::unique_ptr<Line>> merged_lines_;

private:
    // Map of (line, stmt) -> statement pointer
    std::map<std::pair<int, int>, Stmt*> table_;

    // Map of line number -> first statement offset
    std::map<int, int> line_first_stmt_;

    // Ordered list of line numbers
    std::vector<int> line_numbers_;

    // Line text for error messages
    std::unordered_map<int, std::string> line_text_;

    // Empty string for missing lines
    static const std::string empty_string_;
};

// ============================================================================
// Runtime
// ============================================================================

class Runtime {
public:
    Runtime();

    // Initialize from program
    void load(Program& program);

    // Reset state (but keep program)
    void reset();

    // Clear everything
    void clear();

    // ========== Variable Access ==========
    Value get_variable(const std::string& name);
    void set_variable(const std::string& name, const Value& value);
    bool has_variable(const std::string& name) const;

    // ========== Array Access ==========
    Value get_array(const std::string& name, const std::vector<int>& indices);
    void set_array(const std::string& name, const std::vector<int>& indices, const Value& value);
    void dim_array(const std::string& name, const std::vector<int>& dimensions, VarType type);
    void erase_array(const std::string& name);
    bool has_array(const std::string& name) const;

    // ========== Execution State ==========
    PC pc;                              // Current program counter
    std::optional<PC> next_pc;          // Jump target (set by GOTO/GOSUB)
    StatementTable statements;          // Statement lookup

    // ========== Control Flow ==========
    std::vector<StackEntry> exec_stack; // GOSUB/WHILE stack
    std::unordered_map<std::string, ForLoopState> for_states;  // FOR loop states

    // ========== DATA/READ ==========
    std::vector<Value> data_values;     // All DATA values
    size_t data_ptr = 0;                // Current READ position
    std::unordered_map<int, size_t> data_line_map;  // Line -> first data index

    // Collect DATA from program
    void collect_data(Program& program);

    // Read next DATA value
    Value read_data();

    // RESTORE to beginning or specific line
    void restore_data(std::optional<int> line = std::nullopt);

    // ========== User Functions ==========
    std::unordered_map<std::string, DefFnStmt*> user_functions;

    // ========== File I/O ==========
    std::unordered_map<int, std::fstream> files;

    // Field buffer for random access files
    struct FieldBuffer {
        std::vector<char> buffer;                                    // The actual data buffer
        std::unordered_map<std::string, std::pair<int, int>> fields; // var_name -> (offset, width)
        int current_record = 0;
    };
    std::unordered_map<int, FieldBuffer> field_buffers;

    // ========== Error Handling ==========
    std::optional<int> error_handler_line;
    bool error_handler_is_gosub = false;
    int last_error_code = 0;      // ERR - last error code
    int last_error_line = 0;      // ERL - line where error occurred
    std::optional<PC> error_pc;   // PC at error (for RESUME/RESUME NEXT)

    // ========== State ==========
    int array_base = 0;         // OPTION BASE (0 or 1)
    bool trace_on = false;      // TRON/TROFF
    double rnd_last = 0.5;      // Last RND value (for seeding)
    std::set<PC> breakpoints;   // Breakpoints
    bool break_requested = false;  // Ctrl+C
    bool direct_mode = false;   // True when executing in immediate/direct mode

    // ========== DEF Types ==========
    std::unordered_map<char, VarType> def_type_map;

    // ========== COMMON Variables ==========
    std::vector<std::string> common_vars;  // Variable names declared in COMMON (order matters)

    // ========== Helpers ==========
    VarType resolve_type(const std::string& name) const;

private:
    // Variable storage
    std::unordered_map<std::string, Value> variables_;

    // Array storage
    struct ArrayData {
        std::vector<int> dimensions;
        std::vector<Value> data;
        VarType type;
    };
    std::unordered_map<std::string, ArrayData> arrays_;

    // Helper to compute flat index
    size_t array_index(const ArrayData& arr, const std::vector<int>& indices) const;

    // Helper to get default value for type
    static Value default_for_type(VarType type);
};

} // namespace mbasic
