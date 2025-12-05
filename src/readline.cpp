// readline.cpp - Wrapper for editline/readline library
//
// This file isolates all editline/readline dependencies.
// To port to a system without editline:
//   1. Replace the implementations below with simple std::getline() calls
//   2. Remove -ledit from LDFLAGS in Makefile
//   3. Or implement using a different line-editing library

#include "mbasic/readline.hpp"
#include <cstdlib>

// ============================================================================
// EDITLINE IMPLEMENTATION
// Comment out this section and uncomment FALLBACK below if editline unavailable
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

// ============================================================================
// FALLBACK IMPLEMENTATION (no editline)
// Uncomment this section and comment out EDITLINE above if needed
// ============================================================================

/*
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
*/
