#include "../include/lexer.h"
#include <iostream>
#include <string>

// lexer_demo.cpp
// SimpleC编译器词法分析器的演示程序

// 简单的词法分析器演示
void simpleDemo() {
    std::cout << "=== 简单词法分析演示 ===" << std::endl;
    std::cout << "输入: \"12 + 34 - 5 * 6 / 7\"" << std::endl;
    std::cout << "输出:" << std::endl;

    std::string source = "12 + 34 - 5 * 6 / 7";
    Lexer lexer(source);

    while (true) {
        Token token = lexer.getNextToken();
        std::cout << "  " << token.toString() << std::endl;

        if (token.is(TokenType::End)) {
            break;
        }

        if (token.is(TokenType::Invalid)) {
            std::cout << "  !!!  错误: " << token.getValue() << std::endl;
            break;
        }
    }
}

// 交互式演示
void interactiveDemo() {
    std::cout << "\n=== 交互式演示 ===" << std::endl;
    std::cout << "输入简单的数学表达式（只支持数字和+-*/运算符）" << std::endl;
    std::cout << "输入 'quit' 退出" << std::endl;

    while (true) {
        std::cout << "\n> ";
        std::string line;
        std::getline(std::cin, line);

        // 检查输入流是否结束（EOF）
        if (std::cin.eof()) {
            std::cout << "\n输入结束，退出演示程序。" << std::endl;
            break;
        }

        if (line == "quit" || line == "exit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        Lexer lexer(line);
        std::cout << "Token序列:" << std::endl;

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
        }

        if (!hasError) {
            std::cout << "词法分析成功！" << std::endl;
        } else {
            std::cout << "发现错误，请检查输入！" << std::endl;
        }
    }
}

int main() {
    std::cout << "SimpleC编译器 - 词法分析器演示 (第一阶段)" << std::endl;
    std::cout << "=========================================" << std::endl;

    simpleDemo();
    interactiveDemo();

    std::cout << "\n感谢使用SimpleC词法分析器！" << std::endl;

    return 0;
}