#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include "mbasic/readline.hpp"
#include "mbasic/lexer.hpp"
#include "mbasic/parser.hpp"
#include "mbasic/runtime.hpp"
#include "mbasic/interpreter.hpp"
#include "mbasic/error.hpp"

// Maximum line length (MBASIC limit)
constexpr size_t MAX_LINE_LENGTH = 255;

// Read a line with optional pre-filled text for editing
std::string read_line_prefilled(const char* prompt, const std::string& prefill) {
    return mbasic::readline_getline_prefilled(prompt, prefill);
}

// Read a line with editing support
std::string read_line(const char* prompt = "", bool check_length = false) {
    std::string result = mbasic::readline_getline(prompt);

    if (result == "\x04") {
        return result;  // EOF marker
    }

    // Check for line buffer overflow if requested
    if (check_length && result.size() > MAX_LINE_LENGTH) {
        std::cerr << "?" << mbasic::error_message(mbasic::ErrorCode::LINE_BUFFER_OVERFLOW) << "\n";
        return "";  // Return empty to indicate error
    }

    return result;
}

void print_tokens(const std::vector<mbasic::Token>& tokens) {
    for (const auto& tok : tokens) {
        std::cout << mbasic::token_type_name(tok.type);
        if (!tok.value.empty()) {
            std::cout << "(" << tok.value << ")";
        }
        if (!tok.original_case.empty() && tok.original_case != tok.value) {
            std::cout << "[" << tok.original_case << "]";
        }
        std::cout << " ";
        if (tok.type == mbasic::TokenType::NEWLINE) {
            std::cout << "\n";
        }
    }
    std::cout << std::endl;
}

void print_program(const mbasic::Program& program) {
    std::cout << "Parsed " << program.lines.size() << " lines:\n";
    for (const auto& line : program.lines) {
        std::cout << "  Line " << line.line_number << ": "
                  << line.statements.size() << " statement(s)\n";
    }
}

void run_program(const std::string& source) {
    auto program = mbasic::parse(source);

    auto runtime = std::make_unique<mbasic::Runtime>();
    runtime->load(program);

    auto interp = std::make_unique<mbasic::Interpreter>(*runtime);
    interp->run();

    // Check for runtime errors that weren't handled by ON ERROR
    if (interp->state().error) {
        const auto& err = *interp->state().error;
        std::cerr << "?" << err.message << " in " << err.pc.line << "\n";
        return;
    }

    // Handle RUN requests (RUN "filename")
    while (interp->state().run_request) {
        auto run_req = *interp->state().run_request;

        // Load the new program
        std::string filename = run_req.filename;
        // Add .bas extension if not present
        if (filename.find('.') == std::string::npos) {
            filename += ".bas";
        }

        std::ifstream file(filename);
        if (!file) {
            std::cerr << "?File not found: " << filename << "\n";
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string new_source = buffer.str();

        program = mbasic::parse(new_source);
        runtime = std::make_unique<mbasic::Runtime>();
        runtime->load(program);

        interp = std::make_unique<mbasic::Interpreter>(*runtime);

        // If a start line was specified, jump to it
        if (run_req.start_line) {
            mbasic::PC target = runtime->statements.find_line(*run_req.start_line);
            if (target.line != 0) {
                runtime->pc = target;
            }
        }

        interp->run();

        // Check for runtime errors after RUN
        if (interp->state().error) {
            const auto& err = *interp->state().error;
            std::cerr << "?" << err.message << " in " << err.pc.line << "\n";
            return;
        }
    }
}

// Interactive REPL
class BasicSession {
public:
    std::map<int, std::string> program_lines;
    std::unique_ptr<mbasic::Runtime> runtime;
    std::unique_ptr<mbasic::Interpreter> interpreter;

    std::string build_source() const {
        std::stringstream ss;
        for (const auto& [num, text] : program_lines) {
            ss << text << "\n";
        }
        return ss.str();
    }

    void list(int start = 0, int end = 65535) {
        for (const auto& [num, text] : program_lines) {
            if (num >= start && num <= end) {
                std::cout << text << "\n";
            }
        }
    }

    void new_program() {
        program_lines.clear();
        runtime.reset();
        interpreter.reset();
    }

    bool run() {
        try {
            std::string source = build_source();
            if (source.empty()) {
                return true;
            }

            auto program = mbasic::parse(source);
            runtime = std::make_unique<mbasic::Runtime>();
            runtime->load(program);

            interpreter = std::make_unique<mbasic::Interpreter>(*runtime);
            interpreter->run();

            // Check for runtime errors
            if (interpreter->state().error) {
                const auto& err = *interpreter->state().error;
                std::cerr << "?" << err.message << " in " << err.pc.line << "\n";
                return false;
            }

            // Check for CHAIN request
            while (interpreter->state().chain_request) {
                auto chain_req = *interpreter->state().chain_request;

                // Save variables based on CHAIN options
                std::unordered_map<std::string, mbasic::Value> saved_vars;
                if (chain_req.all) {
                    // Save all variables - need to iterate through runtime
                    // For now, we'll use common_vars as a simplification
                    for (const auto& var_name : runtime->common_vars) {
                        if (runtime->has_variable(var_name)) {
                            saved_vars[var_name] = runtime->get_variable(var_name);
                        }
                    }
                } else {
                    // Save only COMMON variables
                    for (const auto& var_name : runtime->common_vars) {
                        if (runtime->has_variable(var_name)) {
                            saved_vars[var_name] = runtime->get_variable(var_name);
                        }
                    }
                }

                // Load the new program
                std::string filename = chain_req.filename;
                // Add .bas extension if not present
                if (filename.find('.') == std::string::npos) {
                    filename += ".bas";
                }

                if (chain_req.merge) {
                    // Merge: keep existing lines, add/replace from new file
                    std::ifstream file(filename);
                    if (!file) {
                        std::cerr << "?File not found: " << filename << "\n";
                        return false;
                    }
                    std::string line;
                    while (std::getline(file, line)) {
                        size_t i = 0;
                        while (i < line.size() && std::isspace(line[i])) i++;
                        if (i >= line.size() || !std::isdigit(line[i])) continue;
                        int line_num = 0;
                        while (i < line.size() && std::isdigit(line[i])) {
                            line_num = line_num * 10 + (line[i] - '0');
                            i++;
                        }
                        program_lines[line_num] = line;
                    }
                } else {
                    // Replace: clear and load new
                    if (!load(filename)) {
                        return false;
                    }
                }

                // Rebuild and run
                source = build_source();
                if (source.empty()) {
                    return true;
                }

                program = mbasic::parse(source);
                runtime = std::make_unique<mbasic::Runtime>();
                runtime->load(program);

                // Restore saved variables
                for (const auto& [name, value] : saved_vars) {
                    runtime->set_variable(name, value);
                }

                interpreter = std::make_unique<mbasic::Interpreter>(*runtime);

                // If a start line was specified, jump to it
                if (chain_req.line_number) {
                    // Find the PC for that line
                    mbasic::PC target = runtime->statements.find_line(*chain_req.line_number);
                    if (target.line != 0) {
                        runtime->pc = target;
                    }
                }

                interpreter->run();

                // Check for runtime errors after CHAIN
                if (interpreter->state().error) {
                    const auto& err = *interpreter->state().error;
                    std::cerr << "?" << err.message << " in " << err.pc.line << "\n";
                    return false;
                }
            }

            // Check for RUN request (RUN "filename")
            while (interpreter->state().run_request) {
                auto run_req = *interpreter->state().run_request;

                // Load the new program
                std::string filename = run_req.filename;
                // Add .bas extension if not present
                if (filename.find('.') == std::string::npos) {
                    filename += ".bas";
                }

                if (!load(filename)) {
                    return false;
                }

                // Rebuild and run
                source = build_source();
                if (source.empty()) {
                    return true;
                }

                program = mbasic::parse(source);
                runtime = std::make_unique<mbasic::Runtime>();
                runtime->load(program);

                interpreter = std::make_unique<mbasic::Interpreter>(*runtime);

                // If a start line was specified, jump to it
                if (run_req.start_line) {
                    mbasic::PC target = runtime->statements.find_line(*run_req.start_line);
                    if (target.line != 0) {
                        runtime->pc = target;
                    }
                }

                interpreter->run();

                // Check for runtime errors after RUN
                if (interpreter->state().error) {
                    const auto& err = *interpreter->state().error;
                    std::cerr << "?" << err.message << " in " << err.pc.line << "\n";
                    return false;
                }
            }

            return true;
        } catch (const mbasic::ParseError& e) {
            std::cerr << "?" << e.what() << "\n";
            return false;
        } catch (const mbasic::RuntimeError& e) {
            std::cerr << "?" << e.what() << "\n";
            return false;
        }
    }

    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "?File not found: " << filename << "\n";
            return false;
        }

        new_program();

        std::string line;
        while (std::getline(file, line)) {
            // Extract line number
            size_t i = 0;
            while (i < line.size() && std::isspace(line[i])) i++;
            if (i >= line.size()) continue;  // Skip empty lines
            if (!std::isdigit(line[i])) {
                // Non-empty line without line number is a direct statement
                std::cerr << "?" << mbasic::error_message(mbasic::ErrorCode::DIRECT_STATEMENT_IN_FILE) << "\n";
                new_program();  // Clear partially loaded program
                return false;
            }

            int line_num = 0;
            while (i < line.size() && std::isdigit(line[i])) {
                line_num = line_num * 10 + (line[i] - '0');
                i++;
            }
            program_lines[line_num] = line;
        }
        return true;
    }

    bool save(const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "?Cannot write to file: " << filename << "\n";
            return false;
        }

        for (const auto& [num, text] : program_lines) {
            file << text << "\n";
        }
        return true;
    }

    bool cont() {
        // Continue execution from where it stopped
        if (!interpreter || !runtime) {
            std::cerr << "?" << mbasic::error_message(mbasic::ErrorCode::CANT_CONTINUE) << "\n";
            return false;
        }
        if (runtime->pc.is_running()) {
            std::cerr << "?" << mbasic::error_message(mbasic::ErrorCode::CANT_CONTINUE) << "\n";
            return false;
        }
        // Check if stopped (not ended)
        if (runtime->pc.reason == mbasic::StopReason::END) {
            std::cerr << "?" << mbasic::error_message(mbasic::ErrorCode::CANT_CONTINUE) << "\n";
            return false;
        }
        try {
            // Reset to running and continue
            runtime->pc.reason = mbasic::StopReason::RUNNING;
            interpreter->run();

            // Check for runtime errors
            if (interpreter->state().error) {
                const auto& err = *interpreter->state().error;
                std::cerr << "?" << err.message << " in " << err.pc.line << "\n";
                return false;
            }
            return true;
        } catch (const mbasic::RuntimeError& e) {
            std::cerr << "?" << e.what() << "\n";
            return false;
        }
    }

    void delete_lines(int start, int end) {
        // Delete lines in range [start, end]
        auto it = program_lines.lower_bound(start);
        while (it != program_lines.end() && it->first <= end) {
            it = program_lines.erase(it);
        }
    }

    void renum(int new_start = 10, int old_start = 0, int increment = 10) {
        // Build mapping from old line numbers to new line numbers
        std::map<int, int> line_map;
        int new_num = new_start;
        for (const auto& [old_num, text] : program_lines) {
            if (old_num >= old_start) {
                line_map[old_num] = new_num;
                new_num += increment;
            } else {
                line_map[old_num] = old_num;  // Keep lines before old_start unchanged
            }
        }

        // Create new program with renumbered lines
        std::map<int, std::string> new_lines;
        for (const auto& [old_num, text] : program_lines) {
            int target_num = line_map[old_num];

            // Update line number references in the code (GOTO, GOSUB, THEN, etc.)
            std::string new_text = update_line_references(text, line_map, target_num);
            new_lines[target_num] = new_text;
        }
        program_lines = std::move(new_lines);
    }

private:
    std::string update_line_references(const std::string& text,
                                       const std::map<int, int>& line_map,
                                       int new_line_num) {
        std::string result;
        size_t i = 0;

        // First, update the line number at the start
        while (i < text.size() && std::isspace(text[i])) {
            result += text[i++];
        }
        // Skip old line number
        while (i < text.size() && std::isdigit(text[i])) i++;
        // Add new line number
        result += std::to_string(new_line_num);

        // Process the rest of the line, looking for keywords that take line numbers
        bool in_string = false;
        while (i < text.size()) {
            if (text[i] == '"') {
                in_string = !in_string;
                result += text[i++];
                continue;
            }
            if (in_string) {
                result += text[i++];
                continue;
            }

            // Check for keywords followed by line numbers
            bool found_keyword = false;
            for (const char* kw : {"GOTO", "GOSUB", "THEN", "ELSE", "RESTORE", "RESUME", "RUN", "LIST", "DELETE", "RENUM", "ERL"}) {
                size_t kwlen = strlen(kw);
                if (i + kwlen <= text.size()) {
                    std::string word;
                    for (size_t j = 0; j < kwlen; j++) {
                        word += std::toupper(text[i + j]);
                    }
                    if (word == kw) {
                        // Check it's not part of a longer identifier
                        if (i + kwlen >= text.size() || !std::isalnum(text[i + kwlen])) {
                            result += text.substr(i, kwlen);
                            i += kwlen;
                            // Skip whitespace
                            while (i < text.size() && std::isspace(text[i])) {
                                result += text[i++];
                            }
                            // Now look for line number(s)
                            while (i < text.size()) {
                                if (std::isdigit(text[i])) {
                                    // Parse the line number
                                    int line_num = 0;
                                    size_t start = i;
                                    while (i < text.size() && std::isdigit(text[i])) {
                                        line_num = line_num * 10 + (text[i] - '0');
                                        i++;
                                    }
                                    // Map to new number if it exists
                                    auto it = line_map.find(line_num);
                                    if (it != line_map.end()) {
                                        result += std::to_string(it->second);
                                    } else {
                                        result += text.substr(start, i - start);
                                    }
                                    // Check for comma (ON GOTO/GOSUB lists)
                                    while (i < text.size() && std::isspace(text[i])) {
                                        result += text[i++];
                                    }
                                    if (i < text.size() && text[i] == ',') {
                                        result += text[i++];
                                        while (i < text.size() && std::isspace(text[i])) {
                                            result += text[i++];
                                        }
                                        continue;  // Look for next line number
                                    }
                                }
                                break;
                            }
                            found_keyword = true;
                            break;
                        }
                    }
                }
            }
            if (!found_keyword) {
                result += text[i++];
            }
        }
        return result;
    }
};

void run_repl() {
    std::cout << "MBASIC 5.21 Interpreter (C++ Edition)\n";
    std::cout << "Copyright (c) 2025. Type SYSTEM to exit.\n\n";

    BasicSession session;
    std::string line;

    while (true) {
        line = read_line("Ok\n", true);  // Check line length
        if (line == "\x04" || (line.empty() && std::cin.eof())) {
            break;
        }
        if (line.empty()) continue;  // Line was too long, error already printed

        // Trim leading whitespace
        size_t start = 0;
        while (start < line.size() && std::isspace(line[start])) start++;
        if (start >= line.size()) continue;

        // Get first word (uppercase for commands)
        std::string first_word;
        size_t pos = start;
        while (pos < line.size() && !std::isspace(line[pos])) {
            first_word += std::toupper(line[pos]);
            pos++;
        }

        // Skip whitespace after first word
        while (pos < line.size() && std::isspace(line[pos])) pos++;
        std::string rest = line.substr(pos);

        // Check for commands
        if (first_word == "SYSTEM" || first_word == "QUIT" || first_word == "EXIT") {
            break;
        } else if (first_word == "NEW") {
            session.new_program();
        } else if (first_word == "RUN") {
            if (!rest.empty()) {
                // RUN filename[,R] - load and run from disk
                std::string filename = rest;
                bool keep_open = false;
                // Check for ,R option
                size_t comma = filename.find(',');
                if (comma != std::string::npos) {
                    std::string opt = filename.substr(comma + 1);
                    filename = filename.substr(0, comma);
                    // Trim and check for R
                    while (!opt.empty() && std::isspace(opt.front())) opt.erase(0, 1);
                    if (!opt.empty() && (opt[0] == 'R' || opt[0] == 'r')) {
                        keep_open = true;
                    }
                }
                // Strip quotes
                if (!filename.empty() && filename.front() == '"') {
                    filename = filename.substr(1);
                }
                if (!filename.empty() && filename.back() == '"') {
                    filename.pop_back();
                }
                // Load the file
                if (session.load(filename)) {
                    session.run();
                }
                (void)keep_open;  // TODO: keep files open with ,R option
            } else {
                session.run();
            }
        } else if (first_word == "LIST") {
            int start_line = 0, end_line = 65535;
            if (!rest.empty()) {
                // Parse LIST range (e.g., LIST 100-200 or LIST 100)
                size_t dash = rest.find('-');
                if (dash != std::string::npos) {
                    if (dash > 0) start_line = std::stoi(rest.substr(0, dash));
                    if (dash + 1 < rest.size()) end_line = std::stoi(rest.substr(dash + 1));
                } else {
                    start_line = end_line = std::stoi(rest);
                }
            }
            session.list(start_line, end_line);
        } else if (first_word == "LOAD") {
            // Remove quotes if present
            std::string filename = rest;
            if (!filename.empty() && filename.front() == '"') {
                filename = filename.substr(1);
            }
            if (!filename.empty() && filename.back() == '"') {
                filename.pop_back();
            }
            if (session.load(filename)) {
                std::cout << "Ok\n";
            }
        } else if (first_word == "SAVE") {
            std::string filename = rest;
            if (!filename.empty() && filename.front() == '"') {
                filename = filename.substr(1);
            }
            if (!filename.empty() && filename.back() == '"') {
                filename.pop_back();
            }
            if (session.save(filename)) {
                std::cout << "Ok\n";
            }
        } else if (first_word == "FILES") {
            // FILES command - list files matching pattern
            std::string pattern = rest;
            if (!pattern.empty() && pattern.front() == '"') {
                pattern = pattern.substr(1);
            }
            if (!pattern.empty() && pattern.back() == '"') {
                pattern.pop_back();
            }
            if (pattern.empty()) {
                pattern = "*.bas";  // Default pattern
            }
            // Use system ls command with glob
            std::string cmd = "ls -1 " + pattern + " 2>/dev/null";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe)) {
                    std::cout << buffer;
                }
                pclose(pipe);
            }
        } else if (first_word == "AUTO") {
            // AUTO command - automatic line numbering
            int auto_start = 10;
            int auto_step = 10;
            if (!rest.empty()) {
                size_t comma = rest.find(',');
                if (comma != std::string::npos) {
                    auto_start = std::stoi(rest.substr(0, comma));
                    auto_step = std::stoi(rest.substr(comma + 1));
                } else {
                    auto_start = std::stoi(rest);
                }
            }
            // Enter AUTO mode
            int line_num = auto_start;
            while (true) {
                std::string prompt = std::to_string(line_num) + " ";
                std::string auto_line = read_line(prompt.c_str());
                if (auto_line.empty() || auto_line == "\x04") {
                    break;  // Exit AUTO mode on empty line or Ctrl+D
                }
                // Store the line with its number
                session.program_lines[line_num] = std::to_string(line_num) + " " + auto_line;
                line_num += auto_step;
            }
        } else if (first_word == "CONT") {
            // CONT command - continue after STOP or Ctrl+C
            session.cont();
        } else if (first_word == "EDIT") {
            // EDIT command - edit a program line
            if (rest.empty()) {
                std::cerr << "?Syntax error\n";
            } else {
                int line_num = std::stoi(rest);
                auto it = session.program_lines.find(line_num);
                if (it == session.program_lines.end()) {
                    std::cerr << "?Undefined line number\n";
                } else {
                    // Pre-fill readline with the existing line content
                    std::string existing_line = it->second;

                    // Read with pre-filled text - user can edit directly
                    std::string new_line = read_line_prefilled("", existing_line);

                    if (new_line == "\x04") {
                        // Ctrl+D - cancel edit
                    } else if (!new_line.empty()) {
                        // Check if it starts with a line number
                        size_t i = 0;
                        while (i < new_line.size() && std::isspace(new_line[i])) i++;

                        if (i < new_line.size() && std::isdigit(new_line[i])) {
                            int new_num = 0;
                            while (i < new_line.size() && std::isdigit(new_line[i])) {
                                new_num = new_num * 10 + (new_line[i] - '0');
                                i++;
                            }

                            // If line number changed, delete old and add new
                            if (new_num != line_num) {
                                session.program_lines.erase(line_num);
                            }
                            session.program_lines[new_num] = new_line;
                        } else {
                            // No line number - prepend the original
                            session.program_lines[line_num] = std::to_string(line_num) + " " + new_line;
                        }
                    }
                    // Empty line deletes the line (like typing just line number)
                    else {
                        session.program_lines.erase(line_num);
                    }
                }
            }
        } else if (first_word == "DELETE") {
            // DELETE command - delete program lines
            int start_line = 0, end_line = 65535;
            if (!rest.empty()) {
                size_t dash = rest.find('-');
                if (dash != std::string::npos) {
                    if (dash > 0) start_line = std::stoi(rest.substr(0, dash));
                    if (dash + 1 < rest.size()) end_line = std::stoi(rest.substr(dash + 1));
                    else end_line = 65535;  // DELETE 100- means from 100 to end
                } else {
                    start_line = end_line = std::stoi(rest);
                }
            }
            session.delete_lines(start_line, end_line);
        } else if (first_word == "KILL") {
            // KILL command - delete file
            std::string filename = rest;
            if (!filename.empty() && filename.front() == '"') {
                filename = filename.substr(1);
            }
            if (!filename.empty() && filename.back() == '"') {
                filename.pop_back();
            }
            if (std::remove(filename.c_str()) != 0) {
                std::cerr << "?File not found\n";
            }
        } else if (first_word == "NAME") {
            // NAME command - rename file (NAME "old" AS "new")
            size_t as_pos = rest.find(" AS ");
            if (as_pos == std::string::npos) {
                // Try lowercase
                as_pos = rest.find(" as ");
            }
            if (as_pos == std::string::npos) {
                std::cerr << "?Syntax error\n";
            } else {
                std::string old_name = rest.substr(0, as_pos);
                std::string new_name = rest.substr(as_pos + 4);
                // Strip quotes
                auto strip_quotes = [](std::string& s) {
                    while (!s.empty() && (s.front() == '"' || std::isspace(s.front()))) s.erase(0, 1);
                    while (!s.empty() && (s.back() == '"' || std::isspace(s.back()))) s.pop_back();
                };
                strip_quotes(old_name);
                strip_quotes(new_name);
                // Check if target file already exists
                std::ifstream test_file(new_name);
                if (test_file.good()) {
                    test_file.close();
                    std::cerr << "?File already exists\n";
                } else if (std::rename(old_name.c_str(), new_name.c_str()) != 0) {
                    std::cerr << "?File not found\n";
                }
            }
        } else if (first_word == "TRON") {
            // TRON in immediate mode
            if (session.runtime) {
                session.runtime->trace_on = true;
            }
        } else if (first_word == "TROFF") {
            // TROFF in immediate mode
            if (session.runtime) {
                session.runtime->trace_on = false;
            }
        } else if (first_word == "RENUM") {
            // RENUM [[new][,[old][,inc]]] - renumber program lines
            int new_start = 10, old_start = 0, increment = 10;
            if (!rest.empty()) {
                // Parse RENUM arguments: new,old,inc or new,,inc or ,old,inc etc.
                std::vector<std::string> parts;
                std::stringstream ss(rest);
                std::string part;
                while (std::getline(ss, part, ',')) {
                    parts.push_back(part);
                }
                if (parts.size() >= 1 && !parts[0].empty()) {
                    new_start = std::stoi(parts[0]);
                }
                if (parts.size() >= 2 && !parts[1].empty()) {
                    old_start = std::stoi(parts[1]);
                }
                if (parts.size() >= 3 && !parts[2].empty()) {
                    increment = std::stoi(parts[2]);
                }
            }
            session.renum(new_start, old_start, increment);
        } else if (first_word == "RESET") {
            // RESET - close all open files
            if (session.runtime) {
                for (auto& [num, file] : session.runtime->files) {
                    if (file.is_open()) {
                        file.close();
                    }
                }
                session.runtime->files.clear();
                session.runtime->field_buffers.clear();
            }
        } else if (first_word == "MERGE") {
            // MERGE filename - merge program from file
            std::string filename = rest;
            if (!filename.empty() && filename.front() == '"') {
                filename = filename.substr(1);
            }
            if (!filename.empty() && filename.back() == '"') {
                filename.pop_back();
            }
            std::ifstream file(filename);
            if (!file) {
                std::cerr << "?File not found: " << filename << "\n";
            } else {
                std::string merge_line;
                while (std::getline(file, merge_line)) {
                    // Extract line number
                    size_t i = 0;
                    while (i < merge_line.size() && std::isspace(merge_line[i])) i++;
                    if (i >= merge_line.size() || !std::isdigit(merge_line[i])) continue;

                    int line_num = 0;
                    while (i < merge_line.size() && std::isdigit(merge_line[i])) {
                        line_num = line_num * 10 + (merge_line[i] - '0');
                        i++;
                    }
                    // Merge: add/replace line
                    session.program_lines[line_num] = merge_line;
                }
                std::cout << "Ok\n";
            }
        } else if (first_word == "LLIST") {
            // LLIST - list to printer (output to stdout with marker)
            int start_line = 0, end_line = 65535;
            if (!rest.empty()) {
                size_t dash = rest.find('-');
                if (dash != std::string::npos) {
                    if (dash > 0) start_line = std::stoi(rest.substr(0, dash));
                    if (dash + 1 < rest.size()) end_line = std::stoi(rest.substr(dash + 1));
                } else {
                    start_line = end_line = std::stoi(rest);
                }
            }
            // Output to printer (simulated as stdout)
            for (const auto& [num, text] : session.program_lines) {
                if (num >= start_line && num <= end_line) {
                    std::cout << text << "\n";
                }
            }
        } else if (std::isdigit(line[start])) {
            // Line starts with a number - add/replace program line
            int line_num = 0;
            size_t i = start;
            while (i < line.size() && std::isdigit(line[i])) {
                line_num = line_num * 10 + (line[i] - '0');
                i++;
            }

            // Skip whitespace after line number
            while (i < line.size() && std::isspace(line[i])) i++;

            if (i >= line.size()) {
                // Just a line number - delete the line
                session.program_lines.erase(line_num);
            } else {
                // Store the full line
                session.program_lines[line_num] = line;
            }
        } else {
            // Immediate mode - try to execute
            try {
                // Wrap in a temporary program
                std::string temp = "1 " + line + "\n2 END\n";
                auto program = mbasic::parse(temp);
                mbasic::Runtime runtime;
                runtime.load(program);
                runtime.direct_mode = true;  // Mark as direct/immediate mode
                mbasic::Interpreter interp(runtime);
                interp.run();
            } catch (const mbasic::ParseError& e) {
                std::cerr << "?" << e.what() << "\n";
            } catch (const mbasic::LexerError& e) {
                std::cerr << "?" << e.what() << "\n";
            } catch (const mbasic::RuntimeError& e) {
                std::cerr << "?" << e.what() << "\n";
            }
        }
    }
}

int main(int argc, char* argv[]) {
    enum class Mode { TOKENIZE, PARSE, RUN };
    Mode mode = Mode::RUN;  // Default to run

    int file_arg = 1;

    // Parse flags
    while (file_arg < argc && argv[file_arg][0] == '-') {
        std::string flag = argv[file_arg];
        if (flag == "--parse") {
            mode = Mode::PARSE;
        } else if (flag == "--tokenize" || flag == "-t") {
            mode = Mode::TOKENIZE;
        } else if (flag == "--run" || flag == "-r") {
            mode = Mode::RUN;
        } else if (flag == "--help" || flag == "-h") {
            std::cout << "MBASIC 5.21 Interpreter (C++ Edition)\n\n";
            std::cout << "Usage: mbasicc [OPTIONS] [filename.bas]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --run, -r       Run the program (default)\n";
            std::cout << "  --parse         Parse and show AST structure\n";
            std::cout << "  --tokenize, -t  Tokenize and show tokens\n";
            std::cout << "  --help, -h      Show this help\n\n";
            std::cout << "If no file is specified, enters interactive REPL mode.\n";
            std::cout << "\nInteractive commands:\n";
            std::cout << "  NEW             Clear program\n";
            std::cout << "  RUN             Run program\n";
            std::cout << "  LIST [n[-m]]    List program lines\n";
            std::cout << "  LOAD \"file\"     Load program from file\n";
            std::cout << "  SAVE \"file\"     Save program to file\n";
            std::cout << "  SYSTEM          Exit interpreter\n";
            std::cout << "\nRepository: https://github.com/avwohl/mbasicc\n";
            std::cout << "Report bugs: https://github.com/avwohl/mbasicc/issues\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << flag << "\n";
            return 1;
        }
        file_arg++;
    }

    if (file_arg < argc) {
        // Load file
        std::string filename = argv[file_arg];
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error: Could not open file: " << filename << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        try {
            switch (mode) {
                case Mode::TOKENIZE: {
                    auto tokens = mbasic::tokenize(source);
                    print_tokens(tokens);
                    break;
                }
                case Mode::PARSE: {
                    auto program = mbasic::parse(source);
                    print_program(program);
                    break;
                }
                case Mode::RUN: {
                    run_program(source);
                    break;
                }
            }
        } catch (const mbasic::ParseError& e) {
            std::cerr << "?" << e.what() << "\n";
            return 1;
        } catch (const mbasic::LexerError& e) {
            std::cerr << "?" << e.what() << "\n";
            return 1;
        } catch (const mbasic::RuntimeError& e) {
            std::cerr << "?" << e.what() << "\n";
            return 1;
        }
    } else {
        // Interactive mode
        run_repl();
    }

    return 0;
}
