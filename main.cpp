#include "include/lexer.h"
#include "include/parser.h"
#include "include/sema.h"
#include "include/codegen.h"
#include "include/vm.h"
#include <iostream>
#include <fstream>
#include <sstream>

void printUsage(const char* program) {
    std::cout << "SimpleC 编译器\n";
    std::cout << "用法: " << program << " <源文件> [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  -l, --lexer   仅进行词法分析\n";
    std::cout << "  -p, --parser  仅进行语法分析\n";
    std::cout << "  -s, --sema    进行语义分析\n";
    std::cout << "  -r, --run     编译并运行（默认）\n";
    std::cout << "  -c, --code    显示生成的字节码\n";
    std::cout << "  -d, --debug   调试模式运行\n";
    std::cout << "  -h, --help    显示帮助信息\n";
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void runLexer(const std::string& source) {
    std::cout << "=== 词法分析结果 ===\n\n";
    Lexer lexer(source);

    int count = 0;
    while (true) {
        Token token = lexer.getNextToken();
        std::cout << "  " << token.toString() << "\n";
        count++;
        if (token.is(TokenType::End)) break;
    }
    std::cout << "\n共识别 " << count << " 个Token\n";
}

void runParser(const std::string& source) {
    std::cout << "=== 语法分析结果 ===\n\n";
    Lexer lexer(source);
    Parser parser(lexer);

    auto program = parser.parseProgram();

    std::cout << "AST:\n" << program->toString() << "\n\n";

    const auto& functions = program->getFunctions();
    std::cout << "识别到 " << functions.size() << " 个函数:\n";
    for (const auto& func : functions) {
        std::cout << "  - " << func->getReturnType() << " " << func->getName() << "(";
        const auto& params = func->getParams();
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << params[i].type << " " << params[i].name;
        }
        std::cout << ")\n";
    }
}

void runSema(const std::string& source) {
    std::cout << "=== 语义分析结果 ===\n\n";
    Lexer lexer(source);
    Parser parser(lexer);

    auto program = parser.parseProgram();

    // 打印函数列表
    const auto& functions = program->getFunctions();
    std::cout << "识别到 " << functions.size() << " 个函数:\n";
    for (const auto& func : functions) {
        std::cout << "  - " << func->getReturnType() << " " << func->getName() << "(";
        const auto& params = func->getParams();
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << params[i].type << " " << params[i].name;
        }
        std::cout << ")\n";
    }

    // 语义分析
    std::cout << "\n进行语义检查...\n";
    Sema sema;
    bool success = sema.analyze(program.get());

    if (success) {
        std::cout << "✓ 语义检查通过\n";
    } else {
        std::cout << "✗ 发现 " << sema.getErrors().size() << " 个语义错误:\n";
        for (const auto& err : sema.getErrors()) {
            std::cout << "  错误: " << err.message << "\n";
        }
    }
}

enum class Mode { Lexer, Parser, Sema, Run, Code };

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string filename;
    Mode mode = Mode::Run;  // 默认编译运行
    bool debug = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-l" || arg == "--lexer") {
            mode = Mode::Lexer;
        } else if (arg == "-p" || arg == "--parser") {
            mode = Mode::Parser;
        } else if (arg == "-s" || arg == "--sema") {
            mode = Mode::Sema;
        } else if (arg == "-r" || arg == "--run") {
            mode = Mode::Run;
        } else if (arg == "-c" || arg == "--code") {
            mode = Mode::Code;
        } else if (arg == "-d" || arg == "--debug") {
            debug = true;
        } else if (arg[0] != '-') {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "错误: 未指定源文件\n";
        printUsage(argv[0]);
        return 1;
    }

    try {
        std::string source = readFile(filename);

        std::cout << "源文件: " << filename << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << source;
        std::cout << "----------------------------------------\n\n";

        switch (mode) {
            case Mode::Lexer:
                runLexer(source);
                break;
            case Mode::Parser:
                runParser(source);
                break;
            case Mode::Sema:
                runSema(source);
                break;
            case Mode::Run:
            case Mode::Code: {
                Lexer lexer(source);
                Parser parser(lexer);
                auto program = parser.parseProgram();

                Sema sema;
                if (!sema.analyze(program.get())) {
                    std::cout << "✗ 发现 " << sema.getErrors().size() << " 个语义错误:\n";
                    for (const auto& err : sema.getErrors()) {
                        std::cout << "  错误: " << err.message << "\n";
                    }
                    return 1;
                }

                CodeGen codegen;
                ByteCode bytecode = codegen.generate(program.get(), &sema);

                if (mode == Mode::Code) {
                    std::cout << "=== 生成的字节码 ===\n\n";
                    std::cout << bytecode.toString();
                    std::cout << "\n入口点: " << bytecode.entry_point << "\n";
                } else {
                    std::cout << "=== 运行程序 ===\n\n";
                    VM vm;
                    vm.setDebug(debug);
                    int result = vm.execute(bytecode);
                    std::cout << "\n程序返回值: " << result << "\n";
                }
                break;
            }
        }

        std::cout << "\n✓ 完成\n";
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
