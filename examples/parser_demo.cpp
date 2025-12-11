#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include <iostream>
#include <string>

// parser_demo.cpp
// SimpleC编译器Parser演示程序
// 第二阶段：表达式语法分析

// 打印AST树的辅助函数
void printAST(const ExprNode* node, int indent = 0) {
    if (!node) return;

    // 缩进
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

int main() {
    std::cout << "SimpleC编译器 - Parser演示程序" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << "输入表达式，将生成并显示AST结构" << std::endl;
    std::cout << "支持: 数字, 变量, +, -, *, /, (, ), ==, !=, <, <=, >, >=" << std::endl;
    std::cout << "输入 'quit' 退出" << std::endl;
    std::cout << std::endl;

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty()) {
            continue;
        }

        if (line == "quit" || line == "exit") {
            break;
        }

        try {
            // 词法分析
            Lexer lexer(line);
            Parser parser(lexer);

            // 语法分析
            auto ast = parser.parseExpression();

            // 显示AST
            std::cout << "AST结构:" << std::endl;
            printAST(ast.get());
            std::cout << std::endl;

        } catch (const std::exception& e) {
            std::cout << "错误: " << e.what() << std::endl;
            std::cout << std::endl;
        }
    }

    std::cout << "感谢使用SimpleC Parser！" << std::endl;
    return 0;
}