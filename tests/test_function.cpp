#include "../include/lexer.h"
#include "../include/parser.h"
#include <iostream>

void testFunctionParsing() {
    std::cout << "SimpleC编译器 - 函数解析测试\n";
    std::cout << "============================\n\n";

    // 测试1: 简单函数
    {
        std::cout << "=== 测试1: 简单函数 ===\n";
        std::string code = R"(
int main() {
    return 0;
}
)";
        std::cout << "源代码:\n" << code << "\n";
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parseProgram();
        std::cout << "AST: " << program->toString() << "\n";
        std::cout << "✓ 解析成功！\n\n";
    }

    // 测试2: 带参数的函数
    {
        std::cout << "=== 测试2: 带参数的函数 ===\n";
        std::string code = R"(
int add(int a, int b) {
    return a + b;
}
)";
        std::cout << "源代码:\n" << code << "\n";
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parseProgram();
        std::cout << "AST: " << program->toString() << "\n";
        std::cout << "✓ 解析成功！\n\n";
    }

    // 测试3: void函数
    {
        std::cout << "=== 测试3: void函数 ===\n";
        std::string code = R"(
void doNothing() {
    return;
}
)";
        std::cout << "源代码:\n" << code << "\n";
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parseProgram();
        std::cout << "AST: " << program->toString() << "\n";
        std::cout << "✓ 解析成功！\n\n";
    }

    // 测试4: 函数调用
    {
        std::cout << "=== 测试4: 函数调用 ===\n";
        std::string code = R"(
int main() {
    int x = add(1, 2);
    return x;
}
)";
        std::cout << "源代码:\n" << code << "\n";
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parseProgram();
        std::cout << "AST: " << program->toString() << "\n";
        std::cout << "✓ 解析成功！\n\n";
    }

    // 测试5: 多个函数
    {
        std::cout << "=== 测试5: 多个函数 ===\n";
        std::string code = R"(
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(3, 4);
    return result;
}
)";
        std::cout << "源代码:\n" << code << "\n";
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parseProgram();
        std::cout << "AST: " << program->toString() << "\n";
        std::cout << "✓ 解析成功！\n\n";
    }

    // 测试6: 复杂函数体
    {
        std::cout << "=== 测试6: 复杂函数体 ===\n";
        std::string code = R"(
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
)";
        std::cout << "源代码:\n" << code << "\n";
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parseProgram();
        std::cout << "AST: " << program->toString() << "\n";
        std::cout << "✓ 解析成功！\n\n";
    }

    std::cout << "🎉 函数解析测试全部通过！\n";
}

int main() {
    try {
        testFunctionParsing();
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
