#include "../include/lexer.h"
#include <iostream>
#include <vector>
#include <cassert>

// test_lexer.cpp
// SimpleC编译器词法分析器的测试程序

// 测试基本的词法分析功能
void testBasicLexing() {
    std::cout << "=== 基础词法分析测试 ===" << std::endl;

    std::string source = "12 + 34";
    Lexer lexer(source);

    Token token1 = lexer.getNextToken();
    Token token2 = lexer.getNextToken();
    Token token3 = lexer.getNextToken();
    Token token4 = lexer.getNextToken();

    std::cout << "Token 1: " << token1 << std::endl;
    std::cout << "Token 2: " << token2 << std::endl;
    std::cout << "Token 3: " << token3 << std::endl;
    std::cout << "Token 4: " << token4 << std::endl;

    assert(token1.is(TokenType::Number));
    assert(token2.is(TokenType::Plus));
    assert(token3.is(TokenType::Number));
    assert(token4.is(TokenType::End));

    std::cout << "✓ 基础测试通过!" << std::endl;
}

// 测试复杂表达式
void testComplexExpression() {
    std::cout << "\n=== 复杂表达式测试 ===" << std::endl;

    std::string source = "12-3*4/5+6";
    Lexer lexer(source);

    std::cout << "表达式: " << source << std::endl;
    std::cout << "Token序列:" << std::endl;

    while (true) {
        Token token = lexer.getNextToken();
        std::cout << "  " << token << std::endl;

        if (token.is(TokenType::End)) {
            break;
        }
    }

    std::cout << "✓ 复杂表达式测试通过!" << std::endl;
}

// 测试错误处理
void testErrorHandling() {
    std::cout << "\n=== 错误处理测试 ===" << std::endl;

    std::string source = "12 + abc 34";
    Lexer lexer(source);

    std::cout << "表达式: " << source << std::endl;
    std::cout << "Token序列:" << std::endl;

    while (true) {
        Token token = lexer.getNextToken();
        std::cout << "  " << token << std::endl;

        if (token.is(TokenType::End)) {
            break;
        }

        if (token.is(TokenType::Invalid)) {
            std::cout << "  !!! 发现错误，但词法分析器继续工作..." << std::endl;
        }
    }

    std::cout << "✓ 错误处理测试通过!" << std::endl;
}

int main() {
    std::cout << "SimpleC编译器 - 词法分析器测试" << std::endl;
    std::cout << "==============================" << std::endl;

    try {
        testBasicLexing();
        testComplexExpression();
        testErrorHandling();

        std::cout << "\n 所有测试通过！" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n 测试失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}