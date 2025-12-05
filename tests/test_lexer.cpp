#include <iostream>
#include <cassert>
#include <string>
#include "mbasic/lexer.hpp"
#include "mbasic/error.hpp"

using namespace mbasic;

int tests_passed = 0;
int tests_failed = 0;

void test(const std::string& name, bool condition) {
    if (condition) {
        tests_passed++;
        std::cout << "  PASS: " << name << "\n";
    } else {
        tests_failed++;
        std::cout << "  FAIL: " << name << "\n";
    }
}

void test_basic_tokens() {
    std::cout << "\n=== Basic Token Tests ===\n";

    // Test numbers (use X=... to avoid line number parsing)
    auto tokens = tokenize("X=123");
    test("Integer number", tokens.size() == 4 && tokens[2].type == TokenType::NUMBER && tokens[2].value == "123");

    tokens = tokenize("X=3.14159");
    test("Floating point", tokens.size() == 4 && tokens[2].type == TokenType::NUMBER);

    tokens = tokenize("&HFF");
    test("Hex number", tokens.size() == 2 && tokens[0].type == TokenType::NUMBER && tokens[0].value == "255");

    tokens = tokenize("&O77");
    test("Octal number", tokens.size() == 2 && tokens[0].type == TokenType::NUMBER && tokens[0].value == "63");

    // Test strings
    tokens = tokenize("\"Hello World\"");
    test("String literal", tokens.size() == 2 && tokens[0].type == TokenType::STRING && tokens[0].value == "Hello World");

    // Test operators
    tokens = tokenize("+ - * / ^");
    test("Arithmetic operators", tokens.size() == 6 &&
         tokens[0].type == TokenType::PLUS &&
         tokens[1].type == TokenType::MINUS &&
         tokens[2].type == TokenType::MULTIPLY &&
         tokens[3].type == TokenType::DIVIDE &&
         tokens[4].type == TokenType::POWER);

    tokens = tokenize("<> <= >= < >");
    test("Relational operators", tokens.size() == 6 &&
         tokens[0].type == TokenType::NOT_EQUAL &&
         tokens[1].type == TokenType::LESS_EQUAL &&
         tokens[2].type == TokenType::GREATER_EQUAL &&
         tokens[3].type == TokenType::LESS_THAN &&
         tokens[4].type == TokenType::GREATER_THAN);
}

void test_keywords() {
    std::cout << "\n=== Keyword Tests ===\n";

    auto tokens = tokenize("PRINT");
    test("PRINT keyword", tokens.size() == 2 && tokens[0].type == TokenType::PRINT);

    tokens = tokenize("for next while wend");
    test("Control flow keywords", tokens.size() == 5 &&
         tokens[0].type == TokenType::FOR &&
         tokens[1].type == TokenType::NEXT &&
         tokens[2].type == TokenType::WHILE &&
         tokens[3].type == TokenType::WEND);

    tokens = tokenize("GOTO GOSUB RETURN");
    test("Branch keywords", tokens.size() == 4 &&
         tokens[0].type == TokenType::GOTO &&
         tokens[1].type == TokenType::GOSUB &&
         tokens[2].type == TokenType::RETURN);

    // Case insensitivity
    tokens = tokenize("Print PRINT print PrInT");
    test("Case insensitive keywords", tokens.size() == 5 &&
         tokens[0].type == TokenType::PRINT &&
         tokens[1].type == TokenType::PRINT &&
         tokens[2].type == TokenType::PRINT &&
         tokens[3].type == TokenType::PRINT);
}

void test_identifiers() {
    std::cout << "\n=== Identifier Tests ===\n";

    auto tokens = tokenize("X");
    test("Simple identifier", tokens.size() == 2 && tokens[0].type == TokenType::IDENTIFIER);

    tokens = tokenize("COUNT%");
    test("Integer variable", tokens.size() == 2 &&
         tokens[0].type == TokenType::IDENTIFIER &&
         tokens[0].value == "count%");

    tokens = tokenize("NAME$");
    test("String variable", tokens.size() == 2 &&
         tokens[0].type == TokenType::IDENTIFIER &&
         tokens[0].value == "name$");

    tokens = tokenize("RECORD.FIELD");
    test("Dotted identifier", tokens.size() == 2 &&
         tokens[0].type == TokenType::IDENTIFIER &&
         tokens[0].value == "record.field");
}

void test_line_numbers() {
    std::cout << "\n=== Line Number Tests ===\n";

    auto tokens = tokenize("10 PRINT \"HELLO\"");
    test("Line with number", tokens.size() == 4 &&
         tokens[0].type == TokenType::LINE_NUMBER &&
         tokens[0].value == "10" &&
         tokens[1].type == TokenType::PRINT);

    tokens = tokenize("100 REM This is a comment");
    test("REM comment", tokens.size() == 3 &&
         tokens[0].type == TokenType::LINE_NUMBER &&
         tokens[1].type == TokenType::REM);
}

void test_full_program() {
    std::cout << "\n=== Full Program Test ===\n";

    std::string program =
        "10 PRINT \"Hello World\"\n"
        "20 FOR I = 1 TO 10\n"
        "30 PRINT I\n"
        "40 NEXT I\n"
        "50 END\n";

    auto tokens = tokenize(program);
    test("Program tokenizes", tokens.size() > 20);

    // Count line numbers
    int line_count = 0;
    for (const auto& t : tokens) {
        if (t.type == TokenType::LINE_NUMBER) line_count++;
    }
    test("Five line numbers", line_count == 5);
}

void test_string_functions() {
    std::cout << "\n=== String Function Tests ===\n";

    auto tokens = tokenize("LEFT$(A$, 5)");
    test("LEFT$ function", tokens[0].type == TokenType::LEFT);

    tokens = tokenize("MID$(A$, 1, 3)");
    test("MID$ function", tokens[0].type == TokenType::MID);

    tokens = tokenize("CHR$(65)");
    test("CHR$ function", tokens[0].type == TokenType::CHR);
}

void test_comments() {
    std::cout << "\n=== Comment Tests ===\n";

    auto tokens = tokenize("10 REM This is a comment");
    test("REM comment", tokens[1].type == TokenType::REM &&
         tokens[1].value == "This is a comment");

    tokens = tokenize("10 X = 5 'inline comment");
    test("Apostrophe comment", tokens.back().type == TokenType::END_OF_FILE);
    // Find the apostrophe token
    bool found_apostrophe = false;
    for (const auto& t : tokens) {
        if (t.type == TokenType::APOSTROPHE) {
            found_apostrophe = true;
            break;
        }
    }
    test("Apostrophe token found", found_apostrophe);
}

int main() {
    std::cout << "MBASIC Lexer Tests\n";
    std::cout << "==================\n";

    test_basic_tokens();
    test_keywords();
    test_identifiers();
    test_line_numbers();
    test_full_program();
    test_string_functions();
    test_comments();

    std::cout << "\n==================\n";
    std::cout << "Tests passed: " << tests_passed << "\n";
    std::cout << "Tests failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
