#include "../include/lexer.h"
#include <iostream>
#include <cassert>

// test_simple.cpp
// SimpleC编译器1.2阶段简单测试

// 测试关键字和标识符
void testKeywords() {
    std::cout << "=== 测试关键字和标识符 ===" << std::endl;

    std::string source = "int return x y123 _var";
    Lexer lexer(source);

    std::cout << "源代码: " << source << std::endl;
    std::cout << "Token序列:" << std::endl;

    Token token1 = lexer.getNextToken();
    Token token2 = lexer.getNextToken();
    Token token3 = lexer.getNextToken();
    Token token4 = lexer.getNextToken();
    Token token5 = lexer.getNextToken();

    std::cout << "  " << token1 << std::endl;
    std::cout << "  " << token2 << std::endl;
    std::cout << "  " << token3 << std::endl;
    std::cout << "  " << token4 << std::endl;
    std::cout << "  " << token5 << std::endl;

    assert(token1.is(TokenType::Int));
    assert(token2.is(TokenType::Return));
    assert(token3.is(TokenType::Identifier));
    assert(token4.is(TokenType::Identifier));
    assert(token5.is(TokenType::Identifier));

    std::cout << "✓ 关键字和标识符测试通过!" << std::endl;
}

// 测试基本符号
void testSymbols() {
    std::cout << "\n=== 测试基本符号 ===" << std::endl;

    std::string source = "( ) ; , = ==";
    Lexer lexer(source);

    std::cout << "源代码: " << source << std::endl;
    std::cout << "Token序列:" << std::endl;

    Token token1 = lexer.getNextToken();
    Token token2 = lexer.getNextToken();
    Token token3 = lexer.getNextToken();
    Token token4 = lexer.getNextToken();
    Token token5 = lexer.getNextToken();
    Token token6 = lexer.getNextToken();

    std::cout << "  " << token1 << std::endl;
    std::cout << "  " << token2 << std::endl;
    std::cout << "  " << token3 << std::endl;
    std::cout << "  " << token4 << std::endl;
    std::cout << "  " << token5 << std::endl;
    std::cout << "  " << token6 << std::endl;

    assert(token1.is(TokenType::LParen));
    assert(token2.is(TokenType::RParen));
    assert(token3.is(TokenType::Semicolon));
    assert(token4.is(TokenType::Comma));
    assert(token5.is(TokenType::Assign));
    assert(token6.is(TokenType::Equal));

    std::cout << "✓ 基本符号测试通过!" << std::endl;
}

// 测试注释
void testComments() {
    std::cout << "\n=== 测试注释 ===" << std::endl;

    std::string source = "123 // 注释\n456";
    Lexer lexer(source);

    std::cout << "源代码: " << source << std::endl;
    std::cout << "Token序列:" << std::endl;

    Token token1 = lexer.getNextToken();
    Token token2 = lexer.getNextToken();
    Token token3 = lexer.getNextToken();

    std::cout << "  " << token1 << std::endl;
    std::cout << "  " << token2 << std::endl;
    std::cout << "  " << token3 << std::endl;

    assert(token1.is(TokenType::Number));
    assert(token1.getValue() == "123");
    assert(token2.is(TokenType::Number));
    assert(token2.getValue() == "456");
    assert(token3.is(TokenType::End));

    std::cout << "✓ 注释测试通过!" << std::endl;
}

int main() {
    std::cout << "SimpleC编译器 - 1.2阶段简单测试" << std::endl;
    std::cout << "===============================" << std::endl;

    try {
        testKeywords();
        testSymbols();
        testComments();

        std::cout << "\n🎉 1.2阶段测试全部通过！" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}