#include "include/lexer.h"
#include "include/parser.h"
#include "include/sema.h"
#include "include/codegen.h"
#include "include/vm.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

void printUsage(const char* program) {
    std::cout << "SimpleC 编译器\n";
    std::cout << "用法: " << program << " <源文件> [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  -l, --lexer      仅进行词法分析\n";
    std::cout << "  -p, --parser     仅进行语法分析\n";
    std::cout << "  -s, --sema       进行语义分析\n";
    std::cout << "  -r, --run        编译并运行（默认）\n";
    std::cout << "  -c, --code       显示生成的字节码\n";
    std::cout << "  -d, --debug      调试模式运行\n";
    std::cout << "  -b, --benchmark  性能测试模式\n";
    std::cout << "  -h, --help       显示帮助信息\n";
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

enum class Mode { Lexer, Parser, Sema, Run, Code, Benchmark };

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
        } else if (arg == "-b" || arg == "--benchmark") {
            mode = Mode::Benchmark;
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
            case Mode::Benchmark: {
                std::cout << "=== 性能测试模式 ===\n\n";

                // 测试 Lexer + Parser
                auto start_parse = std::chrono::high_resolution_clock::now();
                Lexer lexer1(source);
                Parser parser1(lexer1);
                auto program = parser1.parseProgram();
                auto end_parse = std::chrono::high_resolution_clock::now();
                auto parse_time = std::chrono::duration_cast<std::chrono::microseconds>(end_parse - start_parse);

                // 测试 Sema
                auto start_sema = std::chrono::high_resolution_clock::now();
                Sema sema;
                bool sema_success = sema.analyze(program.get());
                auto end_sema = std::chrono::high_resolution_clock::now();
                auto sema_time = std::chrono::duration_cast<std::chrono::microseconds>(end_sema - start_sema);

                if (!sema_success) {
                    std::cout << "✗ 语义分析失败\n";
                    return 1;
                }

                // 测试 CodeGen
                auto start_codegen = std::chrono::high_resolution_clock::now();
                CodeGen codegen;
                ByteCode bytecode = codegen.generate(program.get());
                auto end_codegen = std::chrono::high_resolution_clock::now();
                auto codegen_time = std::chrono::duration_cast<std::chrono::microseconds>(end_codegen - start_codegen);

                // 测试 VM
                auto start_vm = std::chrono::high_resolution_clock::now();
                VM vm;
                int result = vm.execute(bytecode);
                auto end_vm = std::chrono::high_resolution_clock::now();
                auto vm_time = std::chrono::duration_cast<std::chrono::microseconds>(end_vm - start_vm);

                // 总时间
                auto total_time = parse_time + sema_time + codegen_time + vm_time;

                // 输出结果
                std::cout << "性能测试结果:\n";
                std::cout << "----------------------------------------\n";
                std::cout << "Lexer + Parser: " << parse_time.count() << " μs\n";
                std::cout << "Sema:           " << sema_time.count() << " μs\n";
                std::cout << "CodeGen:        " << codegen_time.count() << " μs\n";
                std::cout << "VM:             " << vm_time.count() << " μs\n";
                std::cout << "----------------------------------------\n";
                std::cout << "总编译时间:     " << (parse_time + sema_time + codegen_time).count() << " μs\n";
                std::cout << "总执行时间:     " << total_time.count() << " μs\n";
                std::cout << "程序返回值:     " << result << "\n";
                break;
            }
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
                ByteCode bytecode = codegen.generate(program.get());

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
