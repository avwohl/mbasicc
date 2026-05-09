// readline.cpp - Wrapper for editline/readline library
//
// This file isolates all editline/readline dependencies.
//
// Build options:
//   - Default: uses editline/readline (link with -ledit)
//   - Define MBASIC_NO_EDITLINE: uses std::getline fallback
//     (no history, no line editing). Used by FreeDOS/DJGPP, WebAssembly,
//     and any platform without editline available.

#include "mbasic/readline.hpp"
#include <cstdlib>

#ifndef MBASIC_NO_EDITLINE

// ============================================================================
// EDITLINE IMPLEMENTATION
// ============================================================================

#include <editline/readline.h>

namespace mbasic {

// Global for pre-input hook
static std::string g_prefill_text;

// Pre-input hook for readline - inserts text before user input
// macOS editline uses (const char*, int) signature, GNU readline uses (void)
#ifdef __APPLE__
static int prefill_hook(const char*, int) {
#else
static int prefill_hook() {
#endif
    if (!g_prefill_text.empty()) {
        rl_insert_text(g_prefill_text.c_str());
        g_prefill_text.clear();
    }
    return 0;
}

void readline_init() {
    // Nothing special needed for editline
}

void readline_shutdown() {
    // Nothing special needed for editline
}

std::string readline_getline(const char* prompt) {
    char* line = readline(prompt);
    if (line == nullptr) {
        return std::string("\x04");  // EOF marker
    }
    std::string result(line);
    free(line);
    return result;
}

std::string readline_getline_prefilled(const char* prompt, const std::string& prefill) {
    g_prefill_text = prefill;
    rl_startup_hook = prefill_hook;
    char* line = readline(prompt);
    rl_startup_hook = nullptr;
    if (line == nullptr) {
        return std::string("\x04");  // EOF marker
    }
    std::string result(line);
    free(line);
    return result;
}

void readline_add_history(const std::string& line) {
    if (!line.empty()) {
        add_history(line.c_str());
    }
}

} // namespace mbasic

#else // MBASIC_NO_EDITLINE

// ============================================================================
// FALLBACK IMPLEMENTATION (no editline)
// ============================================================================

#include <iostream>

namespace mbasic {

void readline_init() {
    // Nothing to do
}

void readline_shutdown() {
    // Nothing to do
}

std::string readline_getline(const char* prompt) {
    if (prompt && *prompt) {
        std::cout << prompt << std::flush;
    }
    std::string line;
    if (!std::getline(std::cin, line)) {
        return std::string("\x04");  // EOF marker
    }
    return line;
}

std::string readline_getline_prefilled(const char* prompt, const std::string& prefill) {
    // Without editline, just show the prefill and let user type new content
    if (prompt && *prompt) {
        std::cout << prompt;
    }
    std::cout << prefill << "\n";
    std::cout << "Enter new line (or empty to keep): " << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return std::string("\x04");  // EOF marker
    }
    return line.empty() ? prefill : line;
}

void readline_add_history(const std::string& line) {
    // No history support in fallback mode
    (void)line;
}

} // namespace mbasic

#endif // MBASIC_NO_EDITLINE
