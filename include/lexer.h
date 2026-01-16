#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <string>
#include <fstream>

// lexer.h
// SimpleC编译器的词法分析器
// 第一阶段：将源代码转换为Token流

// 词法分析器类：将源代码字符串转换为Token序列
class Lexer {
public:
    // 构造函数 - 从字符串创建词法分析器
    explicit Lexer(const std::string& source);

    // 构造函数 - 从文件创建词法分析器
    explicit Lexer(const char* filename);

    // 析构函数
    ~Lexer();

    // 获取下一个Token
    Token getNextToken();

    // 预览下一个Token（不消耗）
    Token peekNextToken();

    // 预览第 n 个 Token（不消耗，n 从 1 开始，n=1 等同于 peekNextToken）
    Token peekNthToken(size_t n);

    // 检查是否到达输入结尾
    bool isAtEnd() const { return current_pos_ >= source_.length(); }

    // 获取当前位置的字符
    char getCurrentChar() const;

    // 前进到下一个字符
    void advance();

    // 跳过空白字符
    void skipWhitespace();

    // 读取数字Token
    Token readNumber();

    // 读取标识符Token
    Token readIdentifier();

    // 跳过单行注释
    void skipSingleLineComment();

    // 读取多字符运算符（统一处理 =, ==, !, !=, &&, || 等）
    Token readMultiCharOperator(TokenType type);

    // 检查字符串是否是关键字
    bool isKeyword(const std::string& str);

    // 获取关键字对应的Token类型
    TokenType getKeywordType(const std::string& str);

    // 重置词法分析器到开始位置
    void reset();

    // 获取当前行号
    int getCurrentLine() const { return line_; }

    // 获取当前列号
    int getCurrentColumn() const { return column_; }

private:
    std::string source_;          // 源代码
    size_t current_pos_;          // 当前位置
    int line_;                    // 当前行号
    int column_;                  // 当前列号
    Token current_token_;         // 当前缓存的Token
    bool has_cached_token_;       // 是否有缓存的Token

    // 初始化词法分析器状态
    void initialize();

    // 判断字符是否为数字
    bool isDigit(char c) const { return c >= '0' && c <= '9'; }

    // 判断字符是否为空白字符
    bool isWhitespace(char c) const { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

    // 从文件加载源代码
    std::string loadFromFile(const char* filename);
};

#endif // LEXER_H