#include "mbasic/interpreter.hpp"
#include "mbasic/lexer.hpp"
#include "mbasic/parser.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <limits>

namespace mbasic {

// Helper for floating-point comparison with tolerance
// Uses relative epsilon for large values, absolute epsilon for small values
static bool float_equal(double a, double b) {
    // Handle exact equality (including infinities)
    if (a == b) return true;

    double diff = std::fabs(a - b);
    double larger = std::fmax(std::fabs(a), std::fabs(b));

    // Use relative tolerance scaled by the larger magnitude
    // Single-precision epsilon is about 1.19e-7, we use a slightly larger tolerance
    // to account for float-to-double conversion artifacts
    constexpr double rel_epsilon = 1e-6;
    constexpr double abs_epsilon = 1e-9;

    return diff <= std::fmax(abs_epsilon, larger * rel_epsilon);
}

// ============================================================================
// Interpreter
// ============================================================================

Interpreter::Interpreter(Runtime& runtime, IOHandler* io)
    : runtime_(runtime), io_(io)
{
    if (!io_) {
        io_owned_ = std::make_unique<ConsoleIO>();
        io_ = io_owned_.get();
    }

    // Seed random number generator
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

void Interpreter::run() {
    while (tick()) {
        // Continue execution
    }
}

bool Interpreter::tick() {
    // Check if halted
    if (!runtime_.pc.is_running()) {
        return false;
    }

    // Check for pause
    if (state_.pause_requested) {
        runtime_.pc.reason = StopReason::STOP;
        return false;
    }

    // Check for break
    if (runtime_.break_requested) {
        runtime_.break_requested = false;
        runtime_.pc.reason = StopReason::BREAK;
        return false;
    }

    // Check for breakpoint
    if (runtime_.breakpoints.count(runtime_.pc) && !state_.skip_next_breakpoint) {
        runtime_.pc.reason = StopReason::BREAKPOINT;
        state_.skip_next_breakpoint = true;
        return false;
    }
    state_.skip_next_breakpoint = false;

    // Get current statement
    Stmt* stmt = runtime_.statements.get(runtime_.pc);
    if (!stmt) {
        runtime_.pc = PC::halted();
        return false;
    }

    // Trace output
    if (runtime_.trace_on) {
        io_->print("[" + std::to_string(runtime_.pc.line) + "]\n");
    }

    // Execute statement
    try {
        execute(*stmt);
        state_.statements_executed++;
    } catch (const RuntimeError& e) {
        // Handle error
        if (runtime_.error_handler_line) {
            // Set ERR and ERL
            runtime_.set_variable("err%", int16_t(e.error_code));
            runtime_.set_variable("erl%", int16_t(runtime_.pc.line));

            // Save error PC for RESUME/RESUME NEXT
            runtime_.error_pc = runtime_.pc;

            // Jump to error handler
            if (runtime_.error_handler_is_gosub) {
                StackEntry entry;
                entry.type = StackEntry::Type::GOSUB;
                entry.return_pc = runtime_.statements.next(runtime_.pc);
                runtime_.exec_stack.push_back(entry);
            }
            runtime_.next_pc = runtime_.statements.find_line(*runtime_.error_handler_line);
        } else {
            state_.error = {e.error_code, runtime_.pc, e.what()};
            runtime_.pc.reason = StopReason::ERROR;
            return false;
        }
    }

    // Advance PC
    advance_pc();

    return runtime_.pc.is_running();
}

void Interpreter::advance_pc() {
    if (runtime_.next_pc) {
        runtime_.pc = *runtime_.next_pc;
        runtime_.next_pc = std::nullopt;
    } else if (runtime_.pc.is_running()) {
        runtime_.pc = runtime_.statements.next(runtime_.pc);
    }
}

void Interpreter::jump_to(int line) {
    PC target = runtime_.statements.find_line(line);
    if (!runtime_.statements.valid(target)) {
        raise_error(ErrorCode::UNDEFINED_LINE, "Undefined line number: " + std::to_string(line));
    }
    runtime_.next_pc = target;
}

void Interpreter::stop() {
    runtime_.pc = PC::halted();
}

void Interpreter::raise_error(int code, const std::string& msg) {
    // Store error info for ERL/ERR functions
    runtime_.last_error_code = code;
    runtime_.last_error_line = runtime_.pc.line;
    throw RuntimeError(code, msg, runtime_.pc.line);
}

// ============================================================================
// Statement Execution
// ============================================================================

void Interpreter::execute(Stmt& stmt) {
    std::visit([this](auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<PrintStmt>>) exec_print(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<PrintUsingStmt>>) exec_print_using(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<LprintStmt>>) exec_lprint(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<LprintUsingStmt>>) exec_lprint_using(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<InputStmt>>) exec_input(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<LineInputStmt>>) exec_line_input(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<LetStmt>>) exec_let(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<IfStmt>>) exec_if(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ForStmt>>) exec_for(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<NextStmt>>) exec_next(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<WhileStmt>>) exec_while(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<WendStmt>>) exec_wend(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<GotoStmt>>) exec_goto(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<GosubStmt>>) exec_gosub(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ReturnStmt>>) exec_return(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<OnGotoStmt>>) exec_on_goto(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<OnGosubStmt>>) exec_on_gosub(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<DataStmt>>) exec_data(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ReadStmt>>) exec_read(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<RestoreStmt>>) exec_restore(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<DimStmt>>) exec_dim(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<DefFnStmt>>) exec_def_fn(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<DefTypeStmt>>) exec_def_type(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<EndStmt>>) exec_end(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ClsStmt>>) exec_cls(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<StopStmt>>) exec_stop(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<RemStmt>>) exec_rem(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<SwapStmt>>) exec_swap(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<EraseStmt>>) exec_erase(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ClearStmt>>) exec_clear(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<OptionBaseStmt>>) exec_option_base(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<RandomizeStmt>>) exec_randomize(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<TronStmt>>) exec_tron(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<TroffStmt>>) exec_troff(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<WidthStmt>>) exec_width(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<PokeStmt>>) exec_poke(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ErrorStmt>>) exec_error(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<OnErrorStmt>>) exec_on_error(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ResumeStmt>>) exec_resume(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<OpenStmt>>) exec_open(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<CloseStmt>>) exec_close(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<FieldStmt>>) exec_field(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<GetStmt>>) exec_get(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<PutStmt>>) exec_put(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<LsetStmt>>) exec_lset(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<RsetStmt>>) exec_rset(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<WriteStmt>>) exec_write(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<ChainStmt>>) exec_chain(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<CommonStmt>>) exec_common(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<MidAssignStmt>>) exec_mid_assign(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<CallStmt>>) exec_call(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<OutStmt>>) exec_out(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<WaitStmt>>) exec_wait(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<KillStmt>>) exec_kill(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<NameStmt>>) exec_name(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<MergeStmt>>) exec_merge(*s);
        else if constexpr (std::is_same_v<T, std::unique_ptr<RunStmt>>) exec_run(*s);
    }, stmt);
}

void Interpreter::exec_print(PrintStmt& s) {
    std::string output;

    for (size_t i = 0; i < s.expressions.size(); ++i) {
        Value val = eval(s.expressions[i]);

        // Check for TAB/SPC markers
        if (is_numeric(val)) {
            // Could be TAB or SPC result
            output += to_string(val);
        } else {
            output += std::get<std::string>(val);
        }

        // Handle separator
        if (i < s.separators.size()) {
            char sep = s.separators[i];
            if (sep == ',') {
                // Tab to next zone (14 columns)
                int col = io_->get_column() + static_cast<int>(output.length());
                int next_zone = ((col / 14) + 1) * 14;
                while (col < next_zone) {
                    output += ' ';
                    col++;
                }
            } else if (sep == ';') {
                // No spacing
            } else if (sep == ' ') {
                // Implicit separator - add a space
                output += ' ';
            } else if (sep == '\0') {
                // Newline
                output += '\n';
            }
        }
    }

    // If no expressions or last separator indicates newline
    if (s.expressions.empty() ||
        (s.separators.size() == s.expressions.size() && s.separators.back() == '\0')) {
        if (output.empty() || output.back() != '\n') {
            output += '\n';
        }
    }

    // Output to file or console
    if (s.file_number) {
        int filenum = static_cast<int>(to_number(eval(*s.file_number)));
        auto it = runtime_.files.find(filenum);
        if (it == runtime_.files.end() || !it->second.is_open()) {
            raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
        }
        it->second << output;
        it->second.flush();
    } else {
        io_->print(output);
    }
}

void Interpreter::exec_print_using(PrintUsingStmt& s) {
    std::string format = std::get<std::string>(eval(s.format_string));
    std::string output;

    size_t expr_idx = 0;
    size_t fmt_pos = 0;

    while (fmt_pos < format.size() && expr_idx < s.expressions.size()) {
        char c = format[fmt_pos];

        // Check for numeric format specifiers
        if (c == '#' || c == '+' || c == '-' || c == '$' || c == '*' || c == '.') {
            // Parse numeric format field
            size_t field_start = fmt_pos;
            bool has_sign = false;
            bool leading_sign = false;
            bool trailing_sign = false;
            bool dollar_sign = false;
            [[maybe_unused]] bool floating_dollar = false;  // TODO: implement floating dollar format
            bool asterisk_fill = false;
            int decimal_pos = -1;
            int digit_count = 0;
            int decimal_digits = 0;
            bool exponential = false;

            // Check for leading sign or dollar
            if (format[fmt_pos] == '+') {
                leading_sign = true;
                has_sign = true;
                fmt_pos++;
            } else if (fmt_pos + 1 < format.size() && format[fmt_pos] == '$' && format[fmt_pos + 1] == '$') {
                floating_dollar = true;
                dollar_sign = true;
                fmt_pos += 2;
            } else if (format[fmt_pos] == '$') {
                // Single $ prefix (fixed position)
                dollar_sign = true;
                fmt_pos++;
            } else if (fmt_pos + 1 < format.size() && format[fmt_pos] == '*' && format[fmt_pos + 1] == '*') {
                asterisk_fill = true;
                fmt_pos += 2;
                // Check for $
                if (fmt_pos < format.size() && format[fmt_pos] == '$') {
                    dollar_sign = true;
                    fmt_pos++;
                }
            }

            // Count # before decimal, including embedded commas for thousands separator
            bool has_comma = false;
            while (fmt_pos < format.size() && (format[fmt_pos] == '#' || format[fmt_pos] == ',')) {
                if (format[fmt_pos] == '#') {
                    digit_count++;
                } else if (format[fmt_pos] == ',') {
                    has_comma = true;  // Mark that we want thousands separators
                }
                fmt_pos++;
            }

            // Check for decimal point
            if (fmt_pos < format.size() && format[fmt_pos] == '.') {
                decimal_pos = digit_count;
                fmt_pos++;
                // Count # after decimal
                while (fmt_pos < format.size() && format[fmt_pos] == '#') {
                    decimal_digits++;
                    fmt_pos++;
                }
            }

            // Check for trailing sign or minus
            if (fmt_pos < format.size() && format[fmt_pos] == '-') {
                trailing_sign = true;
                has_sign = true;
                fmt_pos++;
            } else if (fmt_pos < format.size() && format[fmt_pos] == '+') {
                trailing_sign = true;
                has_sign = true;
                fmt_pos++;
            }

            // Check for exponential
            if (fmt_pos + 3 < format.size() &&
                format[fmt_pos] == '^' && format[fmt_pos+1] == '^' &&
                format[fmt_pos+2] == '^' && format[fmt_pos+3] == '^') {
                exponential = true;
                fmt_pos += 4;
            }

            // Only format if we found any format characters
            if (digit_count > 0 || decimal_digits > 0 || exponential) {
                Value val = eval(s.expressions[expr_idx++]);
                double num = to_number(val);

                int total_width = digit_count + decimal_digits + (decimal_pos >= 0 ? 1 : 0);
                if (has_sign) total_width++;
                if (dollar_sign) total_width++;

                std::ostringstream oss;
                if (exponential) {
                    oss << std::scientific << std::setprecision(decimal_digits > 0 ? decimal_digits : 2) << num;
                } else if (decimal_pos >= 0) {
                    oss << std::fixed << std::setprecision(decimal_digits) << num;
                } else {
                    oss << std::fixed << std::setprecision(0) << num;
                }

                std::string numstr = oss.str();

                // Add sign prefix/suffix
                char sign_char = (num < 0) ? '-' : (has_sign ? '+' : ' ');
                if (num < 0 && numstr[0] == '-') {
                    numstr = numstr.substr(1);  // Remove the negative sign, we'll add it ourselves
                }

                // Add thousands separators if comma format specified
                if (has_comma && !exponential) {
                    size_t dot_pos = numstr.find('.');
                    std::string int_part = (dot_pos != std::string::npos) ? numstr.substr(0, dot_pos) : numstr;
                    std::string dec_part = (dot_pos != std::string::npos) ? numstr.substr(dot_pos) : "";
                    std::string new_int;
                    int count = 0;
                    for (int i = static_cast<int>(int_part.size()) - 1; i >= 0; --i) {
                        if (count > 0 && count % 3 == 0 && std::isdigit(int_part[i])) {
                            new_int = "," + new_int;
                        }
                        new_int = int_part[i] + new_int;
                        if (std::isdigit(int_part[i])) count++;
                    }
                    numstr = new_int + dec_part;
                }

                // Pad to width
                while (static_cast<int>(numstr.size()) < total_width - (dollar_sign ? 1 : 0) - (has_sign ? 1 : 0)) {
                    numstr = (asterisk_fill ? "*" : " ") + numstr;
                }

                // Build final string
                std::string result;
                if (leading_sign) result += sign_char;
                if (dollar_sign) result += '$';
                result += numstr;
                if (trailing_sign) result += sign_char;

                output += result;
            } else {
                // No valid format, output literal
                output += format.substr(field_start, fmt_pos - field_start);
            }
        }
        // String format: ! (first char), \  \ (fixed width), & (variable)
        else if (c == '!') {
            // First character only
            Value val = eval(s.expressions[expr_idx++]);
            std::string str = std::get<std::string>(val);
            output += str.empty() ? " " : str.substr(0, 1);
            fmt_pos++;
        }
        else if (c == '&') {
            // Variable length string
            Value val = eval(s.expressions[expr_idx++]);
            output += std::get<std::string>(val);
            fmt_pos++;
        }
        else if (c == '\\') {
            // Fixed width string field (backslash spaces backslash)
            size_t end_pos = format.find('\\', fmt_pos + 1);
            if (end_pos != std::string::npos) {
                int width = static_cast<int>(end_pos - fmt_pos + 1);  // Including both backslashes
                Value val = eval(s.expressions[expr_idx++]);
                std::string str = std::get<std::string>(val);
                if (static_cast<int>(str.size()) < width) {
                    str += std::string(width - str.size(), ' ');
                } else {
                    str = str.substr(0, width);
                }
                output += str;
                fmt_pos = end_pos + 1;
            } else {
                output += c;
                fmt_pos++;
            }
        }
        else if (c == '_') {
            // Literal next character
            fmt_pos++;
            if (fmt_pos < format.size()) {
                output += format[fmt_pos++];
            }
        }
        else {
            // Literal character
            output += c;
            fmt_pos++;
        }
    }

    // Add remaining format (literal text)
    while (fmt_pos < format.size()) {
        output += format[fmt_pos++];
    }

    output += '\n';

    // Output to file or console
    if (s.file_number) {
        int filenum = static_cast<int>(to_number(eval(*s.file_number)));
        auto it = runtime_.files.find(filenum);
        if (it == runtime_.files.end() || !it->second.is_open()) {
            raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
        }
        it->second << output;
        it->second.flush();
    } else {
        io_->print(output);
    }
}

void Interpreter::exec_lprint(LprintStmt& s) {
    // Same as PRINT for now
    for (size_t i = 0; i < s.expressions.size(); ++i) {
        io_->print(to_string(eval(s.expressions[i])));
        if (i < s.separators.size() && s.separators[i] == ',') {
            io_->print("\t");
        }
    }
    io_->print("\n");
}

void Interpreter::exec_lprint_using(LprintUsingStmt& s) {
    // Same formatting as PRINT USING, output to printer (console for now)
    std::string format = std::get<std::string>(eval(s.format_string));
    std::string output;

    size_t expr_idx = 0;
    size_t fmt_pos = 0;

    while (fmt_pos < format.size() && expr_idx < s.expressions.size()) {
        char c = format[fmt_pos];

        // Check for numeric format specifiers
        if (c == '#' || c == '+' || c == '-' || c == '$' || c == '*' || c == '.') {
            // Parse numeric format field
            size_t field_start = fmt_pos;
            bool has_sign = false;
            bool leading_sign = false;
            bool trailing_sign = false;
            bool dollar_sign = false;
            [[maybe_unused]] bool floating_dollar = false;  // TODO: implement floating dollar format
            bool asterisk_fill = false;
            int decimal_pos = -1;
            int digit_count = 0;
            int decimal_digits = 0;
            bool exponential = false;

            // Check for leading sign or dollar
            if (format[fmt_pos] == '+') {
                leading_sign = true;
                has_sign = true;
                fmt_pos++;
            } else if (fmt_pos + 1 < format.size() && format[fmt_pos] == '$' && format[fmt_pos + 1] == '$') {
                floating_dollar = true;
                dollar_sign = true;
                fmt_pos += 2;
            } else if (format[fmt_pos] == '$') {
                dollar_sign = true;
                fmt_pos++;
            } else if (fmt_pos + 1 < format.size() && format[fmt_pos] == '*' && format[fmt_pos + 1] == '*') {
                asterisk_fill = true;
                fmt_pos += 2;
                if (fmt_pos < format.size() && format[fmt_pos] == '$') {
                    dollar_sign = true;
                    fmt_pos++;
                }
            }

            // Count # before decimal
            bool has_comma = false;
            while (fmt_pos < format.size() && (format[fmt_pos] == '#' || format[fmt_pos] == ',')) {
                if (format[fmt_pos] == '#') {
                    digit_count++;
                } else if (format[fmt_pos] == ',') {
                    has_comma = true;
                }
                fmt_pos++;
            }

            // Check for decimal point
            if (fmt_pos < format.size() && format[fmt_pos] == '.') {
                decimal_pos = digit_count;
                fmt_pos++;
                while (fmt_pos < format.size() && format[fmt_pos] == '#') {
                    decimal_digits++;
                    fmt_pos++;
                }
            }

            // Check for trailing sign
            if (fmt_pos < format.size() && (format[fmt_pos] == '-' || format[fmt_pos] == '+')) {
                trailing_sign = true;
                has_sign = true;
                fmt_pos++;
            }

            // Check for exponential
            if (fmt_pos + 3 < format.size() &&
                format[fmt_pos] == '^' && format[fmt_pos+1] == '^' &&
                format[fmt_pos+2] == '^' && format[fmt_pos+3] == '^') {
                exponential = true;
                fmt_pos += 4;
            }

            if (digit_count > 0 || decimal_digits > 0 || exponential) {
                Value val = eval(s.expressions[expr_idx++]);
                double num = to_number(val);

                int total_width = digit_count + decimal_digits + (decimal_pos >= 0 ? 1 : 0);
                if (has_sign) total_width++;
                if (dollar_sign) total_width++;

                std::ostringstream oss;
                if (exponential) {
                    oss << std::scientific << std::setprecision(decimal_digits > 0 ? decimal_digits : 2) << num;
                } else if (decimal_pos >= 0) {
                    oss << std::fixed << std::setprecision(decimal_digits) << num;
                } else {
                    oss << std::fixed << std::setprecision(0) << num;
                }

                std::string numstr = oss.str();
                char sign_char = (num < 0) ? '-' : (has_sign ? '+' : ' ');
                if (num < 0 && numstr[0] == '-') {
                    numstr = numstr.substr(1);
                }

                if (has_comma && !exponential) {
                    size_t dot_pos = numstr.find('.');
                    std::string int_part = (dot_pos != std::string::npos) ? numstr.substr(0, dot_pos) : numstr;
                    std::string dec_part = (dot_pos != std::string::npos) ? numstr.substr(dot_pos) : "";
                    std::string new_int;
                    int count = 0;
                    for (int i = static_cast<int>(int_part.size()) - 1; i >= 0; --i) {
                        if (count > 0 && count % 3 == 0 && std::isdigit(int_part[i])) {
                            new_int = "," + new_int;
                        }
                        new_int = int_part[i] + new_int;
                        if (std::isdigit(int_part[i])) count++;
                    }
                    numstr = new_int + dec_part;
                }

                while (static_cast<int>(numstr.size()) < total_width - (dollar_sign ? 1 : 0) - (has_sign ? 1 : 0)) {
                    numstr = (asterisk_fill ? "*" : " ") + numstr;
                }

                std::string result;
                if (leading_sign) result += sign_char;
                if (dollar_sign) result += '$';
                result += numstr;
                if (trailing_sign) result += sign_char;

                output += result;
            } else {
                output += format.substr(field_start, fmt_pos - field_start);
            }
        }
        else if (c == '!') {
            Value val = eval(s.expressions[expr_idx++]);
            std::string str = std::get<std::string>(val);
            output += str.empty() ? " " : str.substr(0, 1);
            fmt_pos++;
        }
        else if (c == '&') {
            Value val = eval(s.expressions[expr_idx++]);
            output += std::get<std::string>(val);
            fmt_pos++;
        }
        else if (c == '\\') {
            size_t end_pos = format.find('\\', fmt_pos + 1);
            if (end_pos != std::string::npos) {
                int width = static_cast<int>(end_pos - fmt_pos + 1);
                Value val = eval(s.expressions[expr_idx++]);
                std::string str = std::get<std::string>(val);
                if (static_cast<int>(str.size()) < width) {
                    str += std::string(width - str.size(), ' ');
                } else {
                    str = str.substr(0, width);
                }
                output += str;
                fmt_pos = end_pos + 1;
            } else {
                output += c;
                fmt_pos++;
            }
        }
        else if (c == '_') {
            fmt_pos++;
            if (fmt_pos < format.size()) {
                output += format[fmt_pos++];
            }
        }
        else {
            output += c;
            fmt_pos++;
        }
    }

    while (fmt_pos < format.size()) {
        output += format[fmt_pos++];
    }

    output += '\n';
    io_->print(output);
}

void Interpreter::exec_input(InputStmt& s) {
    std::string line;

    // Check if reading from file
    if (s.file_number) {
        int filenum = static_cast<int>(to_number(eval(*s.file_number)));
        auto it = runtime_.files.find(filenum);
        if (it == runtime_.files.end() || !it->second.is_open()) {
            raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
        }
        if (!std::getline(it->second, line)) {
            raise_error(ErrorCode::INPUT_PAST_END, "Input past end of file");
        }
    } else {
        // Console input
        std::string prompt;
        if (s.prompt) {
            prompt = std::get<std::string>(eval(*s.prompt));
        }
        if (!s.suppress_question) {
            prompt += "? ";
        }
        line = io_->input(prompt);
    }

    // Parse input values
    std::vector<std::string> values;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        if (start != std::string::npos) {
            values.push_back(item.substr(start, end - start + 1));
        } else {
            values.push_back("");
        }
    }

    // Assign to variables
    for (size_t i = 0; i < s.variables.size() && i < values.size(); ++i) {
        const auto& var = s.variables[i];

        // Get the type from the lvalue
        VarType type = std::visit([](const auto& v) {
            return v.type;
        }, var);

        Value val;
        if (type == VarType::STRING) {
            val = values[i];
        } else {
            try {
                val = std::stod(values[i]);
            } catch (...) {
                val = 0.0;
            }
        }

        set_lvalue(var, coerce_to(val, type));
    }
}

void Interpreter::exec_line_input(LineInputStmt& s) {
    std::string line;

    // Check if reading from file
    if (s.file_number) {
        int filenum = static_cast<int>(to_number(eval(*s.file_number)));
        auto it = runtime_.files.find(filenum);
        if (it == runtime_.files.end() || !it->second.is_open()) {
            raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
        }
        if (!std::getline(it->second, line)) {
            raise_error(ErrorCode::INPUT_PAST_END, "Input past end of file");
        }
    } else {
        // Console input
        std::string prompt;
        if (s.prompt) {
            prompt = std::get<std::string>(eval(*s.prompt));
        }
        line = io_->input(prompt);
    }

    runtime_.set_variable(s.variable.name, line);
}

void Interpreter::exec_let(LetStmt& s) {
    Value val = eval(s.expression);
    set_lvalue(s.target, val);
}

void Interpreter::exec_if(IfStmt& s) {
    Value cond = eval(s.condition);

    if (to_bool(cond)) {
        // THEN branch
        if (s.then_line) {
            jump_to(*s.then_line);
        } else if (!s.then_stmts.empty()) {
            // Execute inline statements
            for (auto& stmt : s.then_stmts) {
                execute(stmt);
                if (!runtime_.pc.is_running()) return;
            }
        }
    } else {
        // ELSE branch
        if (s.else_line) {
            jump_to(*s.else_line);
        } else if (!s.else_stmts.empty()) {
            for (auto& stmt : s.else_stmts) {
                execute(stmt);
                if (!runtime_.pc.is_running()) return;
            }
        }
    }
}

void Interpreter::exec_for(ForStmt& s) {
    // Evaluate start, end, step
    double start_val = to_number(eval(s.start_expr));
    double end_val = to_number(eval(s.end_expr));
    double step_val = s.step_expr ? to_number(eval(*s.step_expr)) : 1.0;

    // Set loop variable
    runtime_.set_variable(s.variable.name, start_val);

    // Save loop state
    ForLoopState state;
    state.resume_pc = runtime_.pc;  // Will be updated to point after FOR
    state.end_value = end_val;
    state.step_value = step_val;
    runtime_.for_states[s.variable.name] = state;

    // Check if loop should execute at all
    if ((step_val > 0 && start_val > end_val) ||
        (step_val < 0 && start_val < end_val)) {
        // Skip to matching NEXT
        PC scan = runtime_.pc;
        int depth = 1;
        std::string for_var_name = s.variable.name;
        while (depth > 0) {
            scan = runtime_.statements.next(scan);
            if (!runtime_.statements.valid(scan)) {
                raise_error(ErrorCode::FOR_WITHOUT_NEXT, "FOR without NEXT");
            }
            Stmt* stmt = runtime_.statements.get(scan);
            std::visit([&](auto& ptr) {
                using T = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<T, std::unique_ptr<ForStmt>>) {
                    depth++;
                } else if constexpr (std::is_same_v<T, std::unique_ptr<NextStmt>>) {
                    auto& next_stmt = *ptr;
                    if (next_stmt.variables.empty()) {
                        depth--;  // Bare NEXT, closes innermost FOR
                    } else {
                        for (const auto& v : next_stmt.variables) {
                            if (v.name == for_var_name) {
                                depth--;
                                break;
                            }
                        }
                    }
                }
            }, *stmt);
        }
        // Jump past the NEXT
        runtime_.next_pc = runtime_.statements.next(scan);
        // Remove the FOR state since we never entered
        runtime_.for_states.erase(s.variable.name);
    }
}

void Interpreter::exec_next(NextStmt& s) {
    // Get variable name(s)
    std::vector<std::string> var_names;
    if (s.variables.empty()) {
        // NEXT without variable - use most recent FOR
        if (runtime_.for_states.empty()) {
            raise_error(ErrorCode::NEXT_WITHOUT_FOR, "NEXT without FOR");
        }
        // Find most recently added FOR (we don't track order, so just pick one)
        var_names.push_back(runtime_.for_states.begin()->first);
    } else {
        for (const auto& v : s.variables) {
            var_names.push_back(v.name);
        }
    }

    for (const auto& var_name : var_names) {
        auto it = runtime_.for_states.find(var_name);
        if (it == runtime_.for_states.end()) {
            raise_error(ErrorCode::NEXT_WITHOUT_FOR, "NEXT without FOR: " + var_name);
        }

        ForLoopState& state = it->second;

        // Increment loop variable
        double current = to_number(runtime_.get_variable(var_name));
        current += state.step_value;
        runtime_.set_variable(var_name, current);

        // Check termination
        bool done = false;
        if (state.step_value > 0) {
            done = current > state.end_value;
        } else {
            done = current < state.end_value;
        }

        if (done) {
            // Loop finished - remove state and continue
            runtime_.for_states.erase(it);
        } else {
            // Continue loop - jump back to statement after FOR
            runtime_.next_pc = runtime_.statements.next(state.resume_pc);
        }
    }
}

void Interpreter::exec_while(WhileStmt& s) {
    Value cond = eval(s.condition);

    if (to_bool(cond)) {
        // Push WHILE marker
        StackEntry entry;
        entry.type = StackEntry::Type::WHILE;
        entry.loop_pc = runtime_.pc;
        runtime_.exec_stack.push_back(entry);
    } else {
        // Skip to WEND
        // For now, we need to scan forward to find WEND
        // This is a simplified approach - proper implementation would track nesting
        PC scan = runtime_.pc;
        int depth = 1;
        while (depth > 0) {
            scan = runtime_.statements.next(scan);
            if (!runtime_.statements.valid(scan)) {
                raise_error(ErrorCode::WHILE_WITHOUT_WEND, "WHILE without WEND");
            }
            Stmt* stmt = runtime_.statements.get(scan);
            if (std::get_if<std::unique_ptr<WhileStmt>>(stmt)) {
                depth++;
            } else if (std::get_if<std::unique_ptr<WendStmt>>(stmt)) {
                depth--;
            }
        }
        runtime_.next_pc = runtime_.statements.next(scan);
    }
}

void Interpreter::exec_wend([[maybe_unused]] WendStmt& s) {
    // Find matching WHILE on stack
    for (auto it = runtime_.exec_stack.rbegin(); it != runtime_.exec_stack.rend(); ++it) {
        if (it->type == StackEntry::Type::WHILE) {
            // Jump back to WHILE to re-check condition
            runtime_.next_pc = it->loop_pc;
            runtime_.exec_stack.erase(std::next(it).base());
            return;
        }
    }

    raise_error(ErrorCode::WEND_WITHOUT_WHILE, "WEND without WHILE");
}

void Interpreter::exec_goto(GotoStmt& s) {
    jump_to(s.target_line);
}

void Interpreter::exec_gosub(GosubStmt& s) {
    StackEntry entry;
    entry.type = StackEntry::Type::GOSUB;
    entry.return_pc = runtime_.statements.next(runtime_.pc);
    runtime_.exec_stack.push_back(entry);

    jump_to(s.target_line);
}

void Interpreter::exec_return(ReturnStmt& s) {
    // Find GOSUB on stack
    for (auto it = runtime_.exec_stack.rbegin(); it != runtime_.exec_stack.rend(); ++it) {
        if (it->type == StackEntry::Type::GOSUB) {
            if (s.target_line) {
                runtime_.next_pc = runtime_.statements.find_line(*s.target_line);
            } else {
                runtime_.next_pc = it->return_pc;
            }
            runtime_.exec_stack.erase(std::next(it).base());
            return;
        }
    }

    raise_error(ErrorCode::RETURN_WITHOUT_GOSUB, "RETURN without GOSUB");
}

void Interpreter::exec_on_goto(OnGotoStmt& s) {
    int idx = static_cast<int>(to_number(eval(s.selector)));
    if (idx >= 1 && idx <= static_cast<int>(s.targets.size())) {
        jump_to(s.targets[idx - 1]);
    }
    // If out of range, continue to next statement
}

void Interpreter::exec_on_gosub(OnGosubStmt& s) {
    int idx = static_cast<int>(to_number(eval(s.selector)));
    if (idx >= 1 && idx <= static_cast<int>(s.targets.size())) {
        StackEntry entry;
        entry.type = StackEntry::Type::GOSUB;
        entry.return_pc = runtime_.statements.next(runtime_.pc);
        runtime_.exec_stack.push_back(entry);
        jump_to(s.targets[idx - 1]);
    }
}

void Interpreter::exec_data([[maybe_unused]] DataStmt& s) {
    // DATA is not allowed in direct mode
    if (runtime_.direct_mode) {
        raise_error(ErrorCode::ILLEGAL_DIRECT, "Illegal direct");
    }
    // DATA statements are processed at load time
}

void Interpreter::exec_read(ReadStmt& s) {
    for (const auto& var : s.variables) {
        Value val = runtime_.read_data();
        set_lvalue(var, val);
    }
}

void Interpreter::exec_restore(RestoreStmt& s) {
    runtime_.restore_data(s.target_line);
}

void Interpreter::exec_dim(DimStmt& s) {
    for (const auto& decl : s.arrays) {
        std::vector<int> dims;
        for (const auto& dim_expr : decl.dimensions) {
            dims.push_back(static_cast<int>(to_number(eval(dim_expr))));
        }
        runtime_.dim_array(decl.name, dims, decl.type);
    }
}

void Interpreter::exec_def_fn([[maybe_unused]] DefFnStmt& s) {
    // DEF FN is not allowed in direct mode
    if (runtime_.direct_mode) {
        raise_error(ErrorCode::ILLEGAL_DIRECT, "Illegal direct");
    }
    // DEF FN statements are processed at load time
}

void Interpreter::exec_def_type([[maybe_unused]] DefTypeStmt& s) {
    // DEF type statements are processed at parse time
}

void Interpreter::exec_end([[maybe_unused]] EndStmt& s) {
    // Check if we're in an error handler without RESUME
    if (runtime_.error_pc) {
        raise_error(ErrorCode::NO_RESUME, "No RESUME");
    }
    runtime_.pc = PC::halted(StopReason::END);
}

void Interpreter::exec_cls([[maybe_unused]] ClsStmt& s) {
    // Clear screen using ANSI escape sequence
    io_->print("\033[2J\033[H");
}

void Interpreter::exec_stop([[maybe_unused]] StopStmt& s) {
    runtime_.pc.reason = StopReason::STOP;
}

void Interpreter::exec_rem([[maybe_unused]] RemStmt& s) {
    // Comments - nothing to do
}

void Interpreter::exec_swap(SwapStmt& s) {
    Value v1 = get_lvalue(s.var1);
    Value v2 = get_lvalue(s.var2);
    set_lvalue(s.var1, v2);
    set_lvalue(s.var2, v1);
}

void Interpreter::exec_erase(EraseStmt& s) {
    for (const auto& name : s.arrays) {
        runtime_.erase_array(name);
    }
}

void Interpreter::exec_clear([[maybe_unused]] ClearStmt& s) {
    runtime_.reset();
}

void Interpreter::exec_option_base(OptionBaseStmt& s) {
    runtime_.array_base = s.base;
}

void Interpreter::exec_randomize(RandomizeStmt& s) {
    if (s.seed) {
        int seed = static_cast<int>(to_number(eval(*s.seed)));
        std::srand(seed);
    } else {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }
}

void Interpreter::exec_tron([[maybe_unused]] TronStmt& s) {
    runtime_.trace_on = true;
}

void Interpreter::exec_troff([[maybe_unused]] TroffStmt& s) {
    runtime_.trace_on = false;
}

void Interpreter::exec_width(WidthStmt& s) {
    int w = static_cast<int>(to_number(eval(s.width)));
    io_->set_width(w);
}

void Interpreter::exec_poke([[maybe_unused]] PokeStmt& s) {
    // POKE is not supported in this implementation
}

void Interpreter::exec_error(ErrorStmt& s) {
    int code = static_cast<int>(to_number(eval(s.error_code)));
    raise_error(code, error_message(code));
}

void Interpreter::exec_on_error(OnErrorStmt& s) {
    runtime_.error_handler_line = s.target_line;
    runtime_.error_handler_is_gosub = s.is_gosub;
}

void Interpreter::exec_resume(ResumeStmt& s) {
    // RESUME after error
    runtime_.set_variable("err%", int16_t(0));

    if (!runtime_.error_pc) {
        raise_error(ErrorCode::RESUME_WITHOUT_ERROR, "RESUME without error");
    }

    if (s.resume_type == ResumeStmt::Type::NEXT) {
        // Continue to next statement after the one that caused the error
        runtime_.next_pc = runtime_.statements.next(*runtime_.error_pc);
    } else if (s.target_line) {
        // RESUME line_number - go to specific line
        jump_to(*s.target_line);
    } else {
        // RESUME (implicit) - retry the statement that caused the error
        runtime_.next_pc = *runtime_.error_pc;
    }

    // Clear error state
    runtime_.error_pc = std::nullopt;
}

void Interpreter::exec_open(OpenStmt& s) {
    // File I/O - simplified implementation
    std::string filename = std::get<std::string>(eval(s.filename));
    int filenum = static_cast<int>(to_number(eval(s.file_number)));

    // Validate filename
    if (filename.empty()) {
        raise_error(ErrorCode::BAD_FILE_NAME, "Bad file name");
    }

    // Validate file number (MBASIC allows 1-15)
    if (filenum < 1 || filenum > 15) {
        raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
    }

    // Check if too many files are open
    int open_count = 0;
    for (const auto& [num, file] : runtime_.files) {
        if (file.is_open()) open_count++;
    }
    if (open_count >= 15 && !runtime_.files[filenum].is_open()) {
        raise_error(ErrorCode::TOO_MANY_FILES, "Too many files");
    }

    std::ios_base::openmode mode = std::ios::in;  // Default
    switch (s.mode) {
        case FileMode::INPUT:
            mode = std::ios::in;
            break;
        case FileMode::OUTPUT:
            mode = std::ios::out | std::ios::trunc;
            break;
        case FileMode::APPEND:
            mode = std::ios::out | std::ios::app;
            break;
        case FileMode::RANDOM:
            mode = std::ios::in | std::ios::out | std::ios::binary;
            break;
    }

    runtime_.files[filenum].open(filename, mode);
    if (!runtime_.files[filenum]) {
        // For random mode, try creating the file if it doesn't exist
        if (s.mode == FileMode::RANDOM) {
            runtime_.files[filenum].clear();
            runtime_.files[filenum].open(filename, std::ios::out | std::ios::binary);
            runtime_.files[filenum].close();
            runtime_.files[filenum].open(filename, mode);
        }
        if (!runtime_.files[filenum]) {
            raise_error(ErrorCode::FILE_NOT_FOUND, "Cannot open file: " + filename);
        }
    }
}

void Interpreter::exec_close(CloseStmt& s) {
    if (s.file_numbers.empty()) {
        // Close all files
        for (auto& [num, file] : runtime_.files) {
            if (file.is_open()) file.close();
        }
        runtime_.files.clear();
    } else {
        for (const auto& expr : s.file_numbers) {
            int num = static_cast<int>(to_number(eval(expr)));
            if (runtime_.files.count(num) && runtime_.files[num].is_open()) {
                runtime_.files[num].close();
            }
            runtime_.files.erase(num);
        }
    }
}

void Interpreter::exec_field(FieldStmt& s) {
    // FIELD for random access files
    int filenum = static_cast<int>(to_number(eval(s.file_number)));

    auto it = runtime_.files.find(filenum);
    if (it == runtime_.files.end() || !it->second.is_open()) {
        raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
    }

    // Create/reset field buffer for this file
    auto& buf = runtime_.field_buffers[filenum];
    buf.fields.clear();

    int offset = 0;
    for (const auto& fld : s.fields) {
        int width = static_cast<int>(to_number(eval(fld.width)));
        std::string var_name = fld.variable.name;

        // Store field mapping
        buf.fields[var_name] = {offset, width};
        offset += width;
    }

    // Initialize buffer to appropriate size (filled with spaces)
    buf.buffer.assign(offset, ' ');
    buf.current_record = 0;
}

void Interpreter::exec_get(GetStmt& s) {
    // GET for random access files
    int filenum = static_cast<int>(to_number(eval(s.file_number)));

    auto it = runtime_.files.find(filenum);
    if (it == runtime_.files.end() || !it->second.is_open()) {
        raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
    }

    auto buf_it = runtime_.field_buffers.find(filenum);
    if (buf_it == runtime_.field_buffers.end() || buf_it->second.buffer.empty()) {
        raise_error(ErrorCode::BAD_FILE_MODE, "No FIELD defined for file");
    }

    auto& buf = buf_it->second;
    size_t rec_len = buf.buffer.size();

    // Determine record number
    int rec;
    if (s.record_number) {
        rec = static_cast<int>(to_number(eval(*s.record_number)));
        if (rec < 1) raise_error(ErrorCode::BAD_RECORD_NUMBER, "Bad record number");
    } else {
        rec = buf.current_record + 1;
    }

    // Seek to record position (records are 1-based)
    it->second.seekg((rec - 1) * rec_len, std::ios::beg);
    if (it->second.fail() && !it->second.eof()) {
        it->second.clear();
        raise_error(ErrorCode::DISK_IO_ERROR, "Disk I/O error");
    }

    // Read the record into field buffer
    it->second.read(buf.buffer.data(), rec_len);
    if (it->second.bad()) {
        it->second.clear();
        raise_error(ErrorCode::DISK_IO_ERROR, "Disk I/O error");
    }
    size_t bytes_read = it->second.gcount();
    it->second.clear();  // Clear EOF flag if set

    // Pad with spaces if we read past EOF
    for (size_t i = bytes_read; i < rec_len; ++i) {
        buf.buffer[i] = ' ';
    }

    buf.current_record = rec;

    // Update field variables from buffer
    for (const auto& [var_name, pos] : buf.fields) {
        auto [offset, width] = pos;
        std::string value(buf.buffer.begin() + offset, buf.buffer.begin() + offset + width);
        runtime_.set_variable(var_name, value);
    }
}

void Interpreter::exec_put(PutStmt& s) {
    // PUT for random access files
    int filenum = static_cast<int>(to_number(eval(s.file_number)));

    auto it = runtime_.files.find(filenum);
    if (it == runtime_.files.end() || !it->second.is_open()) {
        raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
    }

    auto buf_it = runtime_.field_buffers.find(filenum);
    if (buf_it == runtime_.field_buffers.end() || buf_it->second.buffer.empty()) {
        raise_error(ErrorCode::BAD_FILE_MODE, "No FIELD defined for file");
    }

    auto& buf = buf_it->second;
    size_t rec_len = buf.buffer.size();

    // Determine record number
    int rec;
    if (s.record_number) {
        rec = static_cast<int>(to_number(eval(*s.record_number)));
        if (rec < 1) raise_error(ErrorCode::BAD_RECORD_NUMBER, "Bad record number");
    } else {
        rec = buf.current_record + 1;
    }

    // Seek to record position (records are 1-based)
    it->second.seekp((rec - 1) * rec_len, std::ios::beg);
    if (it->second.fail()) {
        it->second.clear();
        raise_error(ErrorCode::DISK_IO_ERROR, "Disk I/O error");
    }

    // Write the record
    it->second.write(buf.buffer.data(), rec_len);
    it->second.flush();
    if (it->second.fail()) {
        it->second.clear();
        raise_error(ErrorCode::DISK_IO_ERROR, "Disk I/O error");
    }

    buf.current_record = rec;
}

void Interpreter::exec_lset(LsetStmt& s) {
    std::string val = std::get<std::string>(eval(s.value));
    std::string var_name = s.variable.name;

    // Find which file buffer this variable belongs to
    for (auto& [filenum, buf] : runtime_.field_buffers) {
        auto field_it = buf.fields.find(var_name);
        if (field_it != buf.fields.end()) {
            auto [offset, width] = field_it->second;

            // Left-justify: pad on right if too short, truncate if too long
            if (static_cast<int>(val.size()) < width) {
                val += std::string(width - val.size(), ' ');
            } else if (static_cast<int>(val.size()) > width) {
                val = val.substr(0, width);
            }

            // Update buffer
            for (int i = 0; i < width; ++i) {
                buf.buffer[offset + i] = val[i];
            }

            // Also update variable with padded value
            runtime_.set_variable(var_name, val);
            return;
        }
    }

    // Not a field variable - just do normal assignment
    runtime_.set_variable(var_name, val);
}

void Interpreter::exec_rset(RsetStmt& s) {
    std::string val = std::get<std::string>(eval(s.value));
    std::string var_name = s.variable.name;

    // Find which file buffer this variable belongs to
    for (auto& [filenum, buf] : runtime_.field_buffers) {
        auto field_it = buf.fields.find(var_name);
        if (field_it != buf.fields.end()) {
            auto [offset, width] = field_it->second;

            // Right-justify: pad on left if too short, truncate from left if too long
            if (static_cast<int>(val.size()) < width) {
                val = std::string(width - val.size(), ' ') + val;
            } else if (static_cast<int>(val.size()) > width) {
                val = val.substr(val.size() - width);
            }

            // Update buffer
            for (int i = 0; i < width; ++i) {
                buf.buffer[offset + i] = val[i];
            }

            // Also update variable with padded value
            runtime_.set_variable(var_name, val);
            return;
        }
    }

    // Not a field variable - just do normal assignment
    runtime_.set_variable(var_name, val);
}

void Interpreter::exec_write(WriteStmt& s) {
    // WRITE with proper formatting
    std::string output;
    for (size_t i = 0; i < s.expressions.size(); ++i) {
        if (i > 0) output += ",";
        Value val = eval(s.expressions[i]);
        if (is_string(val)) {
            output += "\"" + std::get<std::string>(val) + "\"";
        } else {
            output += to_string(val);
        }
    }
    output += "\n";

    // Output to file or console
    if (s.file_number) {
        int filenum = static_cast<int>(to_number(eval(*s.file_number)));
        auto it = runtime_.files.find(filenum);
        if (it == runtime_.files.end() || !it->second.is_open()) {
            raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
        }
        it->second << output;
        it->second.flush();
    } else {
        io_->print(output);
    }
}

void Interpreter::exec_chain(ChainStmt& s) {
    // CHAIN - load and run another program
    InterpreterState::ChainRequest req;
    req.filename = std::get<std::string>(eval(s.filename));
    if (s.line_number) {
        req.line_number = static_cast<int>(to_number(eval(*s.line_number)));
    }
    req.all = s.all;
    req.merge = s.merge;

    // Set the chain request - the REPL will handle it
    state_.chain_request = req;

    // Stop execution - REPL will load new program and continue
    runtime_.pc.reason = StopReason::END;
}

void Interpreter::exec_common(CommonStmt& s) {
    // COMMON - declare shared variables for CHAIN
    // Add variable names to common_vars list (order matters)
    for (const auto& var_name : s.variables) {
        // Only add if not already in list
        auto it = std::find(runtime_.common_vars.begin(), runtime_.common_vars.end(), var_name);
        if (it == runtime_.common_vars.end()) {
            runtime_.common_vars.push_back(var_name);
        }
    }
}

void Interpreter::exec_mid_assign(MidAssignStmt& s) {
    std::string current = std::get<std::string>(runtime_.get_variable(s.variable.name));
    std::string replacement = std::get<std::string>(eval(s.replacement));

    int start = static_cast<int>(to_number(eval(s.start))) - 1;  // 1-based
    int length = s.length ? static_cast<int>(to_number(eval(*s.length)))
                          : static_cast<int>(replacement.length());

    if (start >= 0 && start < static_cast<int>(current.length())) {
        length = std::min(length, static_cast<int>(current.length() - start));
        length = std::min(length, static_cast<int>(replacement.length()));
        current.replace(start, length, replacement.substr(0, length));
    }

    runtime_.set_variable(s.variable.name, current);
}

void Interpreter::exec_call([[maybe_unused]] CallStmt& s) {
    // CALL - not implemented
}

void Interpreter::exec_out([[maybe_unused]] OutStmt& s) {
    // OUT - hardware I/O, not implemented
}

void Interpreter::exec_wait([[maybe_unused]] WaitStmt& s) {
    // WAIT - hardware I/O, not implemented
}

void Interpreter::exec_kill(KillStmt& s) {
    // KILL - delete a file
    std::string filename = std::get<std::string>(eval(s.filename));
    if (std::remove(filename.c_str()) != 0) {
        raise_error(ErrorCode::FILE_NOT_FOUND, "Cannot delete file: " + filename);
    }
}

void Interpreter::exec_name(NameStmt& s) {
    // NAME old AS new - rename a file
    std::string old_name = std::get<std::string>(eval(s.old_name));
    std::string new_name = std::get<std::string>(eval(s.new_name));
    if (std::rename(old_name.c_str(), new_name.c_str()) != 0) {
        raise_error(ErrorCode::FILE_NOT_FOUND, "Cannot rename file: " + old_name);
    }
}

void Interpreter::exec_merge(MergeStmt& s) {
    // MERGE - load and merge a program file at runtime
    std::string filename = std::get<std::string>(eval(s.filename));

    // Read the file
    std::ifstream file(filename);
    if (!file) {
        raise_error(ErrorCode::FILE_NOT_FOUND, "Cannot open file: " + filename);
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    // Parse the file
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        Program merged_program = parser.parse();

        // Merge the program into the statement table
        runtime_.statements.merge(merged_program);

    } catch (const LexerError& e) {
        raise_error(ErrorCode::SYNTAX_ERROR, e.what());
    } catch (const ParseError& e) {
        raise_error(ErrorCode::SYNTAX_ERROR, e.what());
    }
}

void Interpreter::exec_run(RunStmt& s) {
    // RUN - load and run a program or restart current program

    if (s.filename.has_value()) {
        // RUN "filename" - set run request and stop (REPL will handle loading)
        InterpreterState::RunRequest req;
        req.filename = std::get<std::string>(eval(*s.filename));
        req.start_line = s.start_line;
        req.keep_variables = s.keep_variables;

        state_.run_request = req;
        runtime_.pc.reason = StopReason::END;
    } else if (s.start_line.has_value()) {
        // RUN line_number - restart at specific line (keep program)
        PC start = runtime_.statements.find_line(*s.start_line);
        if (!runtime_.statements.valid(start)) {
            raise_error(ErrorCode::UNDEFINED_LINE, "Undefined line number: " + std::to_string(*s.start_line));
            return;
        }
        runtime_.reset();
        runtime_.next_pc = start;
    } else {
        // RUN with no arguments - restart from beginning
        runtime_.reset();
        runtime_.next_pc = runtime_.statements.first();
    }
}

// ============================================================================
// LValue helpers
// ============================================================================

Value Interpreter::get_lvalue(const std::variant<VariableExpr, ArrayAccessExpr>& lv) {
    return std::visit([this](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, VariableExpr>) {
            return runtime_.get_variable(v.name);
        } else {
            std::vector<int> indices;
            for (const auto& idx : v.indices) {
                indices.push_back(static_cast<int>(to_number(eval(idx))));
            }
            return runtime_.get_array(v.name, indices);
        }
    }, lv);
}

void Interpreter::set_lvalue(const std::variant<VariableExpr, ArrayAccessExpr>& lv, const Value& val) {
    std::visit([this, &val](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, VariableExpr>) {
            runtime_.set_variable(v.name, coerce_to(val, v.type));
        } else {
            std::vector<int> indices;
            for (const auto& idx : v.indices) {
                indices.push_back(static_cast<int>(to_number(eval(idx))));
            }
            runtime_.set_array(v.name, indices, val);
        }
    }, lv);
}

// ============================================================================
// Expression Evaluation
// ============================================================================

Value Interpreter::eval(const Expr& expr) {
    return std::visit([this](const auto& e) -> Value {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<NumberExpr>>) {
            return e->value;
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<StringExpr>>) {
            return e->value;
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<VariableExpr>>) {
            return runtime_.get_variable(e->name);
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<BinaryExpr>>) {
            return eval_binary(*e);
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<UnaryExpr>>) {
            return eval_unary(*e);
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<FunctionCallExpr>>) {
            return eval_function(*e);
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<ArrayAccessExpr>>) {
            std::vector<int> indices;
            for (const auto& idx : e->indices) {
                indices.push_back(static_cast<int>(to_number(eval(idx))));
            }
            return runtime_.get_array(e->name, indices);
        }
        else {
            return 0.0;
        }
    }, expr);
}

Value Interpreter::eval_binary(const BinaryExpr& e) {
    // String concatenation
    if (e.op == TokenType::PLUS || e.op == TokenType::AMPERSAND) {
        Value left = eval(e.left);
        Value right = eval(e.right);
        if (is_string(left) || is_string(right)) {
            std::string l = is_string(left) ? std::get<std::string>(left) : "";
            std::string r = is_string(right) ? std::get<std::string>(right) : "";
            std::string result = l + r;
            if (result.size() > 255) {
                raise_error(ErrorCode::STRING_TOO_LONG, "String too long");
            }
            return result;
        }
    }

    // Numeric operations
    double left = to_number(eval(e.left));
    double right = to_number(eval(e.right));

    switch (e.op) {
        case TokenType::PLUS: return left + right;
        case TokenType::MINUS: return left - right;
        case TokenType::MULTIPLY: return left * right;
        case TokenType::DIVIDE:
            if (right == 0) raise_error(ErrorCode::DIVISION_BY_ZERO, "Division by zero");
            return left / right;
        case TokenType::BACKSLASH: {  // Integer division
            if (right == 0) raise_error(ErrorCode::DIVISION_BY_ZERO, "Division by zero");
            return static_cast<double>(static_cast<int>(left) / static_cast<int>(right));
        }
        case TokenType::MOD: {
            if (right == 0) raise_error(ErrorCode::DIVISION_BY_ZERO, "Division by zero");
            return static_cast<double>(static_cast<int>(left) % static_cast<int>(right));
        }
        case TokenType::POWER: return std::pow(left, right);

        // Comparison - use float_equal for numeric equality to handle float/double precision
        case TokenType::EQUAL:
            if (is_string(eval(e.left))) {
                return (std::get<std::string>(eval(e.left)) == std::get<std::string>(eval(e.right))) ? -1.0 : 0.0;
            }
            return float_equal(left, right) ? -1.0 : 0.0;
        case TokenType::NOT_EQUAL:
            return !float_equal(left, right) ? -1.0 : 0.0;
        case TokenType::LESS_THAN:
            return (left < right && !float_equal(left, right)) ? -1.0 : 0.0;
        case TokenType::GREATER_THAN:
            return (left > right && !float_equal(left, right)) ? -1.0 : 0.0;
        case TokenType::LESS_EQUAL:
            return (left < right || float_equal(left, right)) ? -1.0 : 0.0;
        case TokenType::GREATER_EQUAL:
            return (left > right || float_equal(left, right)) ? -1.0 : 0.0;

        // Logical (bitwise in BASIC)
        case TokenType::AND:
            return static_cast<double>(static_cast<int16_t>(left) & static_cast<int16_t>(right));
        case TokenType::OR:
            return static_cast<double>(static_cast<int16_t>(left) | static_cast<int16_t>(right));
        case TokenType::XOR:
            return static_cast<double>(static_cast<int16_t>(left) ^ static_cast<int16_t>(right));
        case TokenType::EQV:
            return static_cast<double>(~(static_cast<int16_t>(left) ^ static_cast<int16_t>(right)));
        case TokenType::IMP:
            return static_cast<double>((~static_cast<int16_t>(left)) | static_cast<int16_t>(right));

        default:
            raise_error(ErrorCode::INTERNAL_ERROR, "Internal error: unknown operator");
            return 0.0;
    }
}

Value Interpreter::eval_unary(const UnaryExpr& e) {
    Value operand = eval(e.operand);

    switch (e.op) {
        case TokenType::MINUS:
            return -to_number(operand);
        case TokenType::NOT:
            return static_cast<double>(~static_cast<int16_t>(to_number(operand)));
        case TokenType::PLUS:
            return to_number(operand);  // Unary plus is a no-op
        default:
            raise_error(ErrorCode::INTERNAL_ERROR, "Internal error: unknown unary operator");
            return operand;
    }
}

Value Interpreter::eval_function(const FunctionCallExpr& e) {
    // Evaluate arguments
    std::vector<Value> args;
    for (const auto& arg : e.args) {
        args.push_back(eval(arg));
    }

    // Check for user-defined function
    if (e.name.substr(0, 2) == "fn") {
        return eval_user_function(e.name, args);
    }

    // Built-in functions
    const std::string& name = e.name;

    if (name == "abs") return builtin_abs(args);
    if (name == "atn") return builtin_atn(args);
    if (name == "cos") return builtin_cos(args);
    if (name == "exp") return builtin_exp(args);
    if (name == "fix") return builtin_fix(args);
    if (name == "int") return builtin_int(args);
    if (name == "log") return builtin_log(args);
    if (name == "rnd") return builtin_rnd(args);
    if (name == "sgn") return builtin_sgn(args);
    if (name == "sin") return builtin_sin(args);
    if (name == "sqr") return builtin_sqr(args);
    if (name == "tan") return builtin_tan(args);
    if (name == "cint") return builtin_cint(args);
    if (name == "csng") return builtin_csng(args);
    if (name == "cdbl") return builtin_cdbl(args);
    if (name == "asc") return builtin_asc(args);
    if (name == "chr$") return builtin_chr(args);
    if (name == "hex$") return builtin_hex(args);
    if (name == "oct$") return builtin_oct(args);
    if (name == "left$") return builtin_left(args);
    if (name == "right$") return builtin_right(args);
    if (name == "mid$") return builtin_mid(args);
    if (name == "len") return builtin_len(args);
    if (name == "str$") return builtin_str(args);
    if (name == "val") return builtin_val(args);
    if (name == "space$") return builtin_space(args);
    if (name == "string$") return builtin_string(args);
    if (name == "instr") return builtin_instr(args);
    if (name == "tab") return builtin_tab(args);
    if (name == "spc") return builtin_spc(args);
    if (name == "fre") return builtin_fre(args);
    if (name == "pos") return builtin_pos(args);
    if (name == "peek") return builtin_peek(args);
    if (name == "inp") return builtin_inp(args);
    if (name == "eof") return builtin_eof(args);
    if (name == "lof") return builtin_lof(args);
    if (name == "loc") return builtin_loc(args);
    if (name == "cvi") return builtin_cvi(args);
    if (name == "cvs") return builtin_cvs(args);
    if (name == "cvd") return builtin_cvd(args);
    if (name == "mki$") return builtin_mki(args);
    if (name == "mks$") return builtin_mks(args);
    if (name == "mkd$") return builtin_mkd(args);
    if (name == "inkey$") return builtin_inkey(args);
    if (name == "input$") return builtin_input_func(args);
    if (name == "lpos") return builtin_lpos(args);
    if (name == "erl") return builtin_erl(args);
    if (name == "err") return builtin_err(args);
    if (name == "timer") return builtin_timer(args);
    if (name == "date$") return builtin_date(args);
    if (name == "time$") return builtin_time(args);
    if (name == "environ$") return builtin_environ(args);
    if (name == "error$") return builtin_error_str(args);

    raise_error(ErrorCode::UNDEFINED_USER_FUNCTION, "Unknown function: " + name);
    return 0.0;
}

Value Interpreter::eval_user_function(const std::string& name, const std::vector<Value>& args) {
    auto it = runtime_.user_functions.find(name);
    if (it == runtime_.user_functions.end()) {
        raise_error(ErrorCode::UNDEFINED_USER_FUNCTION, "Undefined function: " + name);
    }

    DefFnStmt* fn = it->second;

    // Check argument count
    if (args.size() != fn->params.size()) {
        raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "Wrong number of arguments");
    }

    // Save current values of parameters
    std::vector<std::pair<std::string, Value>> saved;
    for (const auto& param : fn->params) {
        if (runtime_.has_variable(param)) {
            saved.push_back({param, runtime_.get_variable(param)});
        }
    }

    // Set parameters to argument values
    for (size_t i = 0; i < args.size(); ++i) {
        runtime_.set_variable(fn->params[i], args[i]);
    }

    // Evaluate function body
    Value result = eval(fn->body);

    // Restore saved values
    for (const auto& [name, val] : saved) {
        runtime_.set_variable(name, val);
    }

    return result;
}

// ============================================================================
// Built-in Functions
// ============================================================================

Value Interpreter::builtin_abs(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "ABS requires argument");
    return std::abs(to_number(args[0]));
}

Value Interpreter::builtin_atn(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "ATN requires argument");
    return std::atan(to_number(args[0]));
}

Value Interpreter::builtin_cos(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "COS requires argument");
    return std::cos(to_number(args[0]));
}

Value Interpreter::builtin_exp(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "EXP requires argument");
    return std::exp(to_number(args[0]));
}

Value Interpreter::builtin_fix(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "FIX requires argument");
    double val = to_number(args[0]);
    return (val >= 0) ? std::floor(val) : std::ceil(val);
}

Value Interpreter::builtin_int(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "INT requires argument");
    return std::floor(to_number(args[0]));
}

Value Interpreter::builtin_log(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "LOG requires argument");
    double val = to_number(args[0]);
    if (val <= 0) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "LOG of non-positive number");
    return std::log(val);
}

Value Interpreter::builtin_rnd(const std::vector<Value>& args) {
    int arg = args.empty() ? 1 : static_cast<int>(to_number(args[0]));
    if (arg == 0) {
        return runtime_.rnd_last;
    } else if (arg < 0) {
        std::srand(static_cast<unsigned>(arg));
    }
    runtime_.rnd_last = static_cast<double>(std::rand()) / RAND_MAX;
    return runtime_.rnd_last;
}

Value Interpreter::builtin_sgn(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "SGN requires argument");
    double val = to_number(args[0]);
    if (val > 0) return 1.0;
    if (val < 0) return -1.0;
    return 0.0;
}

Value Interpreter::builtin_sin(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "SIN requires argument");
    return std::sin(to_number(args[0]));
}

Value Interpreter::builtin_sqr(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "SQR requires argument");
    double val = to_number(args[0]);
    if (val < 0) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "SQR of negative number");
    return std::sqrt(val);
}

Value Interpreter::builtin_tan(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "TAN requires argument");
    return std::tan(to_number(args[0]));
}

Value Interpreter::builtin_cint(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CINT requires argument");
    return static_cast<double>(to_integer(args[0]));
}

Value Interpreter::builtin_csng(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CSNG requires argument");
    return static_cast<float>(to_number(args[0]));
}

Value Interpreter::builtin_cdbl(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CDBL requires argument");
    return to_number(args[0]);
}

Value Interpreter::builtin_asc(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "ASC requires argument");
    std::string s = std::get<std::string>(args[0]);
    if (s.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "ASC of empty string");
    return static_cast<double>(static_cast<unsigned char>(s[0]));
}

Value Interpreter::builtin_chr(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CHR$ requires argument");
    int code = static_cast<int>(to_number(args[0]));
    if (code < 0 || code > 255) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CHR$ out of range");
    return std::string(1, static_cast<char>(code));
}

Value Interpreter::builtin_hex(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "HEX$ requires argument");
    int val = static_cast<int>(to_number(args[0]));
    std::stringstream ss;
    ss << std::hex << std::uppercase << val;
    return ss.str();
}

Value Interpreter::builtin_oct(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "OCT$ requires argument");
    int val = static_cast<int>(to_number(args[0]));
    std::stringstream ss;
    ss << std::oct << val;
    return ss.str();
}

Value Interpreter::builtin_left(const std::vector<Value>& args) {
    if (args.size() < 2) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "LEFT$ requires 2 arguments");
    std::string s = std::get<std::string>(args[0]);
    int n = static_cast<int>(to_number(args[1]));
    if (n < 0) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "LEFT$ negative count");
    return s.substr(0, std::min(static_cast<size_t>(n), s.length()));
}

Value Interpreter::builtin_right(const std::vector<Value>& args) {
    if (args.size() < 2) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "RIGHT$ requires 2 arguments");
    std::string s = std::get<std::string>(args[0]);
    int n = static_cast<int>(to_number(args[1]));
    if (n < 0) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "RIGHT$ negative count");
    if (static_cast<size_t>(n) >= s.length()) return s;
    return s.substr(s.length() - n);
}

Value Interpreter::builtin_mid(const std::vector<Value>& args) {
    if (args.size() < 2) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "MID$ requires at least 2 arguments");
    std::string s = std::get<std::string>(args[0]);
    int start = static_cast<int>(to_number(args[1])) - 1;  // 1-based
    if (start < 0) start = 0;
    if (static_cast<size_t>(start) >= s.length()) return std::string{};

    size_t len = (args.size() > 2) ? static_cast<size_t>(to_number(args[2])) : s.length();
    return s.substr(start, len);
}

Value Interpreter::builtin_len(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "LEN requires argument");
    return static_cast<double>(std::get<std::string>(args[0]).length());
}

Value Interpreter::builtin_str(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "STR$ requires argument");
    return to_string(args[0]);
}

Value Interpreter::builtin_val(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "VAL requires argument");
    std::string s = std::get<std::string>(args[0]);
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;
    }
}

Value Interpreter::builtin_space(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "SPACE$ requires argument");
    int n = static_cast<int>(to_number(args[0]));
    if (n < 0) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "SPACE$ negative count");
    if (n > 255) raise_error(ErrorCode::STRING_TOO_LONG, "String too long");
    return std::string(n, ' ');
}

Value Interpreter::builtin_string(const std::vector<Value>& args) {
    if (args.size() < 2) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "STRING$ requires 2 arguments");
    int n = static_cast<int>(to_number(args[0]));
    if (n < 0) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "STRING$ negative count");
    if (n > 255) raise_error(ErrorCode::STRING_TOO_LONG, "String too long");

    char c;
    if (is_string(args[1])) {
        std::string s = std::get<std::string>(args[1]);
        c = s.empty() ? ' ' : s[0];
    } else {
        c = static_cast<char>(static_cast<int>(to_number(args[1])));
    }

    return std::string(n, c);
}

Value Interpreter::builtin_instr(const std::vector<Value>& args) {
    if (args.size() < 2) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "INSTR requires at least 2 arguments");

    int start = 0;
    std::string haystack, needle;

    if (args.size() >= 3 && is_numeric(args[0])) {
        start = static_cast<int>(to_number(args[0])) - 1;  // 1-based
        haystack = std::get<std::string>(args[1]);
        needle = std::get<std::string>(args[2]);
    } else {
        haystack = std::get<std::string>(args[0]);
        needle = std::get<std::string>(args[1]);
    }

    if (start < 0) start = 0;
    if (static_cast<size_t>(start) >= haystack.length()) return 0.0;
    if (needle.empty()) return static_cast<double>(start + 1);

    size_t pos = haystack.find(needle, start);
    return (pos == std::string::npos) ? 0.0 : static_cast<double>(pos + 1);
}

Value Interpreter::builtin_tab(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "TAB requires argument");
    int col = static_cast<int>(to_number(args[0])) - 1;  // 1-based
    int current = io_->get_column();
    if (col > current) {
        return std::string(col - current, ' ');
    }
    return std::string{};
}

Value Interpreter::builtin_spc(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "SPC requires argument");
    int n = static_cast<int>(to_number(args[0]));
    if (n < 0) n = 0;
    return std::string(n, ' ');
}

Value Interpreter::builtin_fre([[maybe_unused]] const std::vector<Value>& args) {
    // Return a large number indicating "lots of free memory"
    return 32767.0;
}

Value Interpreter::builtin_pos([[maybe_unused]] const std::vector<Value>& args) {
    return static_cast<double>(io_->get_column() + 1);  // 1-based
}

Value Interpreter::builtin_peek([[maybe_unused]] const std::vector<Value>& args) {
    // PEEK not implemented - return 0
    return 0.0;
}

Value Interpreter::builtin_inp([[maybe_unused]] const std::vector<Value>& args) {
    // INP not implemented - return 0
    return 0.0;
}

Value Interpreter::builtin_eof(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "EOF requires argument");
    int filenum = static_cast<int>(to_number(args[0]));
    auto it = runtime_.files.find(filenum);
    if (it == runtime_.files.end() || !it->second.is_open()) {
        raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
    }
    // Check if at end of file
    if (it->second.eof()) return -1.0;  // True in BASIC
    // Peek to check
    int c = it->second.peek();
    return (c == EOF) ? -1.0 : 0.0;
}

Value Interpreter::builtin_lof(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "LOF requires argument");
    int filenum = static_cast<int>(to_number(args[0]));
    auto it = runtime_.files.find(filenum);
    if (it == runtime_.files.end() || !it->second.is_open()) {
        raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
    }
    // Get file size
    auto cur = it->second.tellg();
    it->second.seekg(0, std::ios::end);
    auto size = it->second.tellg();
    it->second.seekg(cur);
    return static_cast<double>(size);
}

Value Interpreter::builtin_loc(const std::vector<Value>& args) {
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "LOC requires argument");
    int filenum = static_cast<int>(to_number(args[0]));
    auto it = runtime_.files.find(filenum);
    if (it == runtime_.files.end() || !it->second.is_open()) {
        raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
    }
    // Return current position (1-based record number for random, byte/128 for sequential)
    auto pos = it->second.tellg();
    return static_cast<double>(pos / 128 + 1);
}

Value Interpreter::builtin_cvi(const std::vector<Value>& args) {
    // Convert 2-byte string to integer
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CVI requires argument");
    std::string s = std::get<std::string>(args[0]);
    if (s.size() < 2) s.resize(2, '\0');
    int16_t val;
    std::memcpy(&val, s.data(), sizeof(int16_t));
    return static_cast<double>(val);
}

Value Interpreter::builtin_cvs(const std::vector<Value>& args) {
    // Convert 4-byte string to single precision float
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CVS requires argument");
    std::string s = std::get<std::string>(args[0]);
    if (s.size() < 4) s.resize(4, '\0');
    float val;
    std::memcpy(&val, s.data(), sizeof(float));
    return static_cast<double>(val);
}

Value Interpreter::builtin_cvd(const std::vector<Value>& args) {
    // Convert 8-byte string to double precision float
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "CVD requires argument");
    std::string s = std::get<std::string>(args[0]);
    if (s.size() < 8) s.resize(8, '\0');
    double val;
    std::memcpy(&val, s.data(), sizeof(double));
    return val;
}

Value Interpreter::builtin_mki(const std::vector<Value>& args) {
    // Convert integer to 2-byte string
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "MKI$ requires argument");
    int16_t val = static_cast<int16_t>(to_number(args[0]));
    std::string result(2, '\0');
    std::memcpy(&result[0], &val, sizeof(int16_t));
    return result;
}

Value Interpreter::builtin_mks(const std::vector<Value>& args) {
    // Convert single to 4-byte string
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "MKS$ requires argument");
    float val = static_cast<float>(to_number(args[0]));
    std::string result(4, '\0');
    std::memcpy(&result[0], &val, sizeof(float));
    return result;
}

Value Interpreter::builtin_mkd(const std::vector<Value>& args) {
    // Convert double to 8-byte string
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "MKD$ requires argument");
    double val = to_number(args[0]);
    std::string result(8, '\0');
    std::memcpy(&result[0], &val, sizeof(double));
    return result;
}

Value Interpreter::builtin_inkey([[maybe_unused]] const std::vector<Value>& args) {
    // Non-blocking keyboard input
    auto key = io_->inkey();
    if (key) {
        return std::string(1, *key);
    }
    return std::string{};
}

Value Interpreter::builtin_input_func(const std::vector<Value>& args) {
    // INPUT$(n[,#filenum]) - read n characters
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "INPUT$ requires argument");
    int n = static_cast<int>(to_number(args[0]));
    if (n < 0) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "INPUT$ negative count");

    std::string result;
    if (args.size() > 1) {
        // Read from file
        int filenum = static_cast<int>(to_number(args[1]));
        auto it = runtime_.files.find(filenum);
        if (it == runtime_.files.end() || !it->second.is_open()) {
            raise_error(ErrorCode::BAD_FILE_NUMBER, "Bad file number");
        }
        result.resize(n);
        it->second.read(&result[0], n);
        result.resize(it->second.gcount());
    } else {
        // Read from console - blocking
        for (int i = 0; i < n; ++i) {
            char c = std::cin.get();
            if (std::cin.eof()) break;
            result += c;
        }
    }
    return result;
}

Value Interpreter::builtin_lpos([[maybe_unused]] const std::vector<Value>& args) {
    // Line printer position - just return 0 (no printer support)
    return 0.0;
}

Value Interpreter::builtin_erl([[maybe_unused]] const std::vector<Value>& args) {
    // ERL - return line number where last error occurred
    return static_cast<double>(runtime_.last_error_line);
}

Value Interpreter::builtin_err([[maybe_unused]] const std::vector<Value>& args) {
    // ERR - return last error code
    return static_cast<double>(runtime_.last_error_code);
}

Value Interpreter::builtin_timer([[maybe_unused]] const std::vector<Value>& args) {
    // TIMER - return seconds since midnight
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    return static_cast<double>(tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec);
}

Value Interpreter::builtin_date([[maybe_unused]] const std::vector<Value>& args) {
    // DATE$ - return current date as string MM-DD-YYYY
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buf[32];  // Extra space to avoid truncation warnings
    std::snprintf(buf, sizeof(buf), "%02d-%02d-%04d",
                  tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900);
    return std::string(buf);
}

Value Interpreter::builtin_time([[maybe_unused]] const std::vector<Value>& args) {
    // TIME$ - return current time as string HH:MM:SS
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                  tm->tm_hour, tm->tm_min, tm->tm_sec);
    return std::string(buf);
}

Value Interpreter::builtin_environ(const std::vector<Value>& args) {
    // ENVIRON$(name) - get environment variable
    if (args.empty()) raise_error(ErrorCode::ILLEGAL_FUNCTION_CALL, "ENVIRON$ requires argument");
    std::string name = std::get<std::string>(args[0]);
    const char* val = std::getenv(name.c_str());
    return val ? std::string(val) : std::string{};
}

Value Interpreter::builtin_error_str(const std::vector<Value>& args) {
    // ERROR$(code) - return error message for error code
    int code;
    if (args.empty()) {
        code = runtime_.last_error_code;
    } else {
        code = static_cast<int>(to_number(args[0]));
    }
    return error_message(code);
}

void Interpreter::provide_input(const std::string& input) {
    state_.input_buffer.push_back(input);
}

} // namespace mbasic
