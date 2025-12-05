#pragma once

#include <string>

namespace mbasic {

// Initialize the readline subsystem (call once at startup)
void readline_init();

// Shutdown the readline subsystem (call once at exit)
void readline_shutdown();

// Read a line with optional prompt
// Returns the line read, or "\x04" on EOF (Ctrl+D)
std::string readline_getline(const char* prompt = "");

// Read a line with pre-filled text for editing
// Returns the line read, or "\x04" on EOF (Ctrl+D)
std::string readline_getline_prefilled(const char* prompt, const std::string& prefill);

// Add a line to the history
void readline_add_history(const std::string& line);

} // namespace mbasic
