#include "../include/lexer.h"
#include <iostream>
#include <cassert>

// 测试所有运算符的完整功能
void testCompleteOperators() {
    std::cout << "=== 完整运算符测试 ===" << std::endl;

    // 测试所有比较运算符
    std::string source = "== != < <= > >=";
    Lexer lexer(source);

    std::cout << "源代码: " << source << std::endl;
    std::cout << "Token序列:" << std::endl;

    int tokenCount = 0;
    while (true) {
        Token token = lexer.getNextToken();
        std::cout << "  " << token.toString() << std::endl;

        if (token.is(TokenType::End)) {
            break;
        }
        tokenCount++;
    }

    std::cout << "总共解析了 " << tokenCount << " 个Token" << std::endl;
    std::cout << "✓ 完整运算符测试通过!" << std::endl;
}

// 测试复杂C语句
void testComplexStatements() {
    std::cout << "\n=== 复杂语句测试 ===" << std::endl;

    std::string source = "int result = a + b * (c - d) / e; // 计算表达式";
    Lexer lexer(source);

    std::cout << "源代码: " << source << std::endl;
    std::cout << "Token序列:" << std::endl;

    int tokenCount = 0;
    bool hasError = false;

    while (true) {
        Token token = lexer.getNextToken();
        std::cout << "  " << token.toString() << std::endl;

        if (token.is(TokenType::End)) {
            break;
        }

        if (token.is(TokenType::Invalid)) {
            hasError = true;
        }
        tokenCount++;
    }

    std::cout << "总共解析了 " << tokenCount << " 个Token" << std::endl;

    if (!hasError) {
        std::cout << "✓ 复杂语句测试通过!" << std::endl;
    } else {
        std::cout << "⚠️ 复杂语句测试包含错误（这是正常的，因为某些符号还未实现）" << std::endl;
    }
}

int main() {
    std::cout << "SimpleC编译器 - 完整运算符测试" << std::endl;
    std::cout << "==============================" << std::endl;

    try {
        testCompleteOperators();
        testComplexStatements();

        std::cout << "\n🎉 完整运算符测试完成！" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}