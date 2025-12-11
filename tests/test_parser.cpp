#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include <iostream>
#include <cassert>

// test_parser.cpp
// SimpleC编译器Parser测试
// 第二阶段：表达式语法分析

// 打印AST树的辅助函数
void printAST(const ExprNode* node, int indent = 0) {
    if (!node) return;

    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }
    std::cout << node->toString() << std::endl;

    // 如果是二元运算符，递归打印子节点
    if (const auto* binaryOp = dynamic_cast<const BinaryOpNode*>(node)) {
        printAST(binaryOp->getLeft(), indent + 1);
        printAST(binaryOp->getRight(), indent + 1);
    }

    // 如果是一元运算符，递归打印操作数
    if (const auto* unaryOp = dynamic_cast<const UnaryOpNode*>(node)) {
        printAST(unaryOp->getOperand(), indent + 1);
    }
}

// 测试基本表达式解析
void testBasicExpressions() {
    std::cout << "=== 测试基本表达式解析 ===" << std::endl;

    struct TestCase {
        std::string input;
        std::string description;
    };

    std::vector<TestCase> tests = {
        {"123", "单个数字"},
        {"x", "单个变量"},
        {"1 + 2", "加法"},
        {"10 - 5", "减法"},
        {"3 * 4", "乘法"},
        {"8 / 2", "除法"}
    };

    for (const auto& test : tests) {
        std::cout << "\n输入: " << test.input << " (" << test.description << ")" << std::endl;
        std::cout << "AST:" << std::endl;

        try {
            Lexer lexer(test.input);
            Parser parser(lexer);
            auto ast = parser.parseExpression();
            printAST(ast.get());
            std::cout << "✓ 解析成功！" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ 解析失败: " << e.what() << std::endl;
        }
    }
}

// 测试带括号的表达式
void testParentheses() {
    std::cout << "\n=== 测试带括号的表达式 ===" << std::endl;

    std::vector<std::string> tests = {
        "(1 + 2) * 3",
        "1 + (2 * 3)",
        "((1 + 2) * 3) - 4",
        "x + (y - z) * w"
    };

    for (const auto& test : tests) {
        std::cout << "\n输入: " << test << std::endl;
        std::cout << "AST:" << std::endl;

        try {
            Lexer lexer(test);
            Parser parser(lexer);
            auto ast = parser.parseExpression();
            printAST(ast.get());
            std::cout << "✓ 解析成功！" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ 解析失败: " << e.what() << std::endl;
        }
    }
}

// 测试比较运算符
void testComparisonOperators() {
    std::cout << "\n=== 测试比较运算符 ===" << std::endl;

    std::vector<std::string> tests = {
        "1 == 2",
        "x != y",
        "a < b",
        "x <= y",
        "z > w",
        "m >= n"
    };

    for (const auto& test : tests) {
        std::cout << "\n输入: " << test << std::endl;
        std::cout << "AST:" << std::endl;

        try {
            Lexer lexer(test);
            Parser parser(lexer);
            auto ast = parser.parseExpression();
            printAST(ast.get());
            std::cout << "✓ 解析成功！" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ 解析失败: " << e.what() << std::endl;
        }
    }
}

// 测试复杂表达式
void testComplexExpressions() {
    std::cout << "\n=== 测试复杂表达式 ===" << std::endl;

    std::vector<std::string> tests = {
        "a + b * c - d / e",
        "1 + 2 * 3 - 4 / 5 + 6",
        "x == y && a != b",
        "(a + b) * (c - d) / e",
        "result <= max && result >= min"
    };

    for (const auto& test : tests) {
        std::cout << "\n输入: " << test << std::endl;
        std::cout << "AST:" << std::endl;

        try {
            Lexer lexer(test);
            Parser parser(lexer);
            auto ast = parser.parseExpression();
            printAST(ast.get());
            std::cout << "✓ 解析成功！" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ 解析失败: " << e.what() << std::endl;
        }
    }
}

// 测试错误情况
void testErrors() {
    std::cout << "\n=== 测试错误情况 ===" << std::endl;

    std::vector<std::string> tests = {
        "+",           // 只有运算符
        "1 + * 2",    // 连续运算符
        "1 + )",       // 括号不匹配
        "( 1 + 2",     // 括号不匹配
        "abc 123",     // 标识符后接数字
    };

    for (const auto& test : tests) {
        std::cout << "\n输入: " << test << std::endl;
        std::cout << "结果:" << std::endl;

        try {
            Lexer lexer(test);
            Parser parser(lexer);
            auto ast = parser.parseCompleteExpression();
            printAST(ast.get());
            std::cout << "意外成功" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "预期错误: " << e.what() << std::endl;
        }
    }
}

int main() {
    std::cout << "SimpleC编译器 - Parser测试" << std::endl;
    std::cout << "=========================" << std::endl;

    try {
        testBasicExpressions();
        testParentheses();
        testComparisonOperators();
        testComplexExpressions();
        testErrors();

        std::cout << "\n🎉 Parser测试完成！" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}