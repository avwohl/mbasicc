#include "mbasic/runtime.hpp"
#include <algorithm>
#include <cmath>

namespace mbasic {

// ============================================================================
// StatementTable
// ============================================================================

const std::string StatementTable::empty_string_;

void StatementTable::build(Program& program) {
    table_.clear();
    line_first_stmt_.clear();
    line_numbers_.clear();
    line_text_.clear();

    for (auto& line : program.lines) {
        int line_num = line.line_number;
        line_numbers_.push_back(line_num);
        line_first_stmt_[line_num] = 0;
        line_text_[line_num] = line.source_text;

        for (size_t i = 0; i < line.statements.size(); ++i) {
            table_[{line_num, static_cast<int>(i)}] = &line.statements[i];
        }
    }

    std::sort(line_numbers_.begin(), line_numbers_.end());
}

void StatementTable::merge(Program& program) {
    // Merge lines from another program
    // Existing line numbers are replaced, new ones are added
    for (auto& line : program.lines) {
        int line_num = line.line_number;

        // First, remove any existing statements for this line
        auto it = table_.begin();
        while (it != table_.end()) {
            if (it->first.first == line_num) {
                it = table_.erase(it);
            } else {
                ++it;
            }
        }

        // Store the line in our persistent storage
        auto stored_line = std::make_unique<Line>(std::move(line));

        // Add the new statements
        line_first_stmt_[line_num] = 0;
        line_text_[line_num] = stored_line->source_text;

        for (size_t i = 0; i < stored_line->statements.size(); ++i) {
            table_[{line_num, static_cast<int>(i)}] = &stored_line->statements[i];
        }

        // Check if this line number is already in line_numbers_
        auto pos = std::lower_bound(line_numbers_.begin(), line_numbers_.end(), line_num);
        if (pos == line_numbers_.end() || *pos != line_num) {
            // Insert new line number in sorted order
            line_numbers_.insert(pos, line_num);
        }

        // Keep the line alive
        merged_lines_.push_back(std::move(stored_line));
    }
}

Stmt* StatementTable::get(const PC& pc) {
    auto it = table_.find({pc.line, pc.stmt});
    return (it != table_.end()) ? it->second : nullptr;
}

PC StatementTable::first() const {
    if (line_numbers_.empty()) {
        return PC::halted();
    }
    return PC::running_at(line_numbers_.front(), 0);
}

PC StatementTable::next(const PC& current) const {
    // Try next statement on same line
    auto it = table_.find({current.line, current.stmt + 1});
    if (it != table_.end()) {
        return PC::running_at(current.line, current.stmt + 1);
    }

    // Find next line
    auto line_it = std::upper_bound(line_numbers_.begin(), line_numbers_.end(), current.line);
    if (line_it == line_numbers_.end()) {
        return PC::halted();
    }

    return PC::running_at(*line_it, 0);
}

PC StatementTable::find_line(int line_num) const {
    auto it = line_first_stmt_.find(line_num);
    if (it == line_first_stmt_.end()) {
        return PC::halted(StopReason::ERROR);
    }
    return PC::running_at(line_num, it->second);
}

bool StatementTable::valid(const PC& pc) const {
    return table_.find({pc.line, pc.stmt}) != table_.end();
}

const std::string& StatementTable::line_text(int line_num) const {
    auto it = line_text_.find(line_num);
    return (it != line_text_.end()) ? it->second : empty_string_;
}

// ============================================================================
// Runtime
// ============================================================================

Runtime::Runtime() {
    // Initialize default types (all SINGLE)
    for (char c = 'a'; c <= 'z'; ++c) {
        def_type_map[c] = VarType::SINGLE;
    }

    // Initialize system variables
    variables_["err%"] = int16_t{0};
    variables_["erl%"] = int16_t{0};
}

void Runtime::load(Program& program) {
    // Copy DEF type map
    def_type_map = program.def_type_map;

    // Build statement table
    statements.build(program);

    // Collect DATA values
    collect_data(program);

    // Collect user functions
    user_functions.clear();
    for (auto& line : program.lines) {
        for (auto& stmt : line.statements) {
            if (auto* def = std::get_if<std::unique_ptr<DefFnStmt>>(&stmt)) {
                user_functions[(*def)->name] = def->get();
            }
        }
    }

    // Set PC to first statement
    pc = statements.first();
}

void Runtime::reset() {
    // Clear variables (except system)
    Value err = variables_["err%"];
    Value erl = variables_["erl%"];
    variables_.clear();
    variables_["err%"] = err;
    variables_["erl%"] = erl;

    // Clear arrays
    arrays_.clear();

    // Reset execution state
    pc = statements.first();
    next_pc = std::nullopt;
    exec_stack.clear();
    for_states.clear();

    // Reset DATA
    data_ptr = 0;

    // Reset other state
    array_base = 0;
    trace_on = false;
    break_requested = false;
    error_handler_line = std::nullopt;
    error_handler_is_gosub = false;

    // Close files
    for (auto& [num, file] : files) {
        if (file.is_open()) file.close();
    }
    files.clear();
    field_buffers.clear();
}

void Runtime::clear() {
    reset();
    data_values.clear();
    data_line_map.clear();
    user_functions.clear();
    breakpoints.clear();
}

// ============================================================================
// Variable Access
// ============================================================================

Value Runtime::get_variable(const std::string& name) {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return it->second;
    }
    // Return default value for type
    return default_for_type(resolve_type(name));
}

void Runtime::set_variable(const std::string& name, const Value& value) {
    VarType target_type = resolve_type(name);
    variables_[name] = coerce_to(value, target_type);
}

bool Runtime::has_variable(const std::string& name) const {
    return variables_.find(name) != variables_.end();
}

// ============================================================================
// Array Access
// ============================================================================

Value Runtime::get_array(const std::string& name, const std::vector<int>& indices) {
    auto it = arrays_.find(name);
    if (it == arrays_.end()) {
        // Auto-dimension array with default size (10 per dimension)
        std::vector<int> dims(indices.size(), 10);
        dim_array(name, dims, resolve_type(name));
        it = arrays_.find(name);
    }

    const auto& arr = it->second;
    size_t idx = array_index(arr, indices);
    return arr.data[idx];
}

void Runtime::set_array(const std::string& name, const std::vector<int>& indices, const Value& value) {
    auto it = arrays_.find(name);
    if (it == arrays_.end()) {
        // Auto-dimension array
        std::vector<int> dims(indices.size(), 10);
        dim_array(name, dims, resolve_type(name));
        it = arrays_.find(name);
    }

    auto& arr = it->second;
    size_t idx = array_index(arr, indices);
    arr.data[idx] = coerce_to(value, arr.type);
}

void Runtime::dim_array(const std::string& name, const std::vector<int>& dimensions, VarType type) {
    if (arrays_.find(name) != arrays_.end()) {
        throw RuntimeError(ErrorCode::DUPLICATE_DEFINITION,
                          "Array already dimensioned: " + name);
    }

    ArrayData arr;
    arr.dimensions = dimensions;
    arr.type = type;

    // Calculate total size
    size_t total = 1;
    for (int dim : dimensions) {
        total *= (dim + 1 - array_base);  // Account for OPTION BASE
    }

    // Initialize with default values
    arr.data.resize(total, default_for_type(type));
    arrays_[name] = std::move(arr);
}

void Runtime::erase_array(const std::string& name) {
    arrays_.erase(name);
}

bool Runtime::has_array(const std::string& name) const {
    return arrays_.find(name) != arrays_.end();
}

size_t Runtime::array_index(const ArrayData& arr, const std::vector<int>& indices) const {
    if (indices.size() != arr.dimensions.size()) {
        throw RuntimeError(ErrorCode::SUBSCRIPT_OUT_OF_RANGE,
                          "Wrong number of subscripts");
    }

    size_t idx = 0;
    size_t multiplier = 1;

    for (size_t i = indices.size(); i > 0; --i) {
        int index = indices[i - 1] - array_base;
        int dim = arr.dimensions[i - 1] + 1 - array_base;

        if (index < 0 || index >= dim) {
            throw RuntimeError(ErrorCode::SUBSCRIPT_OUT_OF_RANGE,
                              "Subscript out of range");
        }

        idx += index * multiplier;
        multiplier *= dim;
    }

    return idx;
}

// ============================================================================
// DATA/READ
// ============================================================================

void Runtime::collect_data(Program& program) {
    data_values.clear();
    data_line_map.clear();

    for (const auto& line : program.lines) {
        for (const auto& stmt : line.statements) {
            if (auto* data = std::get_if<std::unique_ptr<DataStmt>>(&stmt)) {
                size_t start_idx = data_values.size();
                data_line_map[line.line_number] = start_idx;

                for (const auto& val : (*data)->values) {
                    data_values.push_back(val);
                }
            }
        }
    }

    data_ptr = 0;
}

Value Runtime::read_data() {
    if (data_ptr >= data_values.size()) {
        throw RuntimeError(ErrorCode::OUT_OF_DATA, "Out of DATA");
    }
    return data_values[data_ptr++];
}

void Runtime::restore_data(std::optional<int> line) {
    if (!line) {
        data_ptr = 0;
    } else {
        auto it = data_line_map.find(*line);
        if (it != data_line_map.end()) {
            data_ptr = it->second;
        } else {
            // Find first DATA at or after the specified line
            for (const auto& [ln, idx] : data_line_map) {
                if (ln >= *line) {
                    data_ptr = idx;
                    return;
                }
            }
            // No DATA found at or after line, restore to end
            data_ptr = data_values.size();
        }
    }
}

// ============================================================================
// Helpers
// ============================================================================

VarType Runtime::resolve_type(const std::string& name) const {
    if (!name.empty()) {
        char last = name.back();
        switch (last) {
            case '%': return VarType::INTEGER;
            case '!': return VarType::SINGLE;
            case '#': return VarType::DOUBLE;
            case '$': return VarType::STRING;
        }
    }

    if (!name.empty() && std::isalpha(name[0])) {
        char first = std::tolower(name[0]);
        auto it = def_type_map.find(first);
        if (it != def_type_map.end()) {
            return it->second;
        }
    }

    return VarType::SINGLE;
}

Value Runtime::default_for_type(VarType type) {
    switch (type) {
        case VarType::INTEGER: return int16_t{0};
        case VarType::SINGLE: return float{0.0f};
        case VarType::DOUBLE: return double{0.0};
        case VarType::STRING: return std::string{};
    }
    return float{0.0f};
}

} // namespace mbasic
