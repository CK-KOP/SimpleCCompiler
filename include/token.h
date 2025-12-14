#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <iostream>

// token.h
// SimpleC编译器的Token类型定义
// 第一阶段：支持数字和基本运算符

// Token类型枚举
enum class TokenType {
    // 字面量
    Number,         // 数字字面量

    // 运算符
    Plus,           // +
    Minus,          // -
    Multiply,       // *
    Divide,         // /

    // 括号
    LParen,         // (
    RParen,         // )
    LBrace,         // {
    RBrace,         // }

    // 分隔符
    Semicolon,      // ;
    Comma,          // ,

    // 赋值和比较运算符
    Assign,         // =
    Equal,          // ==
    NotEqual,       // !=
    Less,           // <
    LessEqual,      // <=
    Greater,        // >
    GreaterEqual,   // >=

    // 逻辑运算符
    LogicalAnd,     // &&
    LogicalOr,      // ||
    LogicalNot,     // !

    // 关键字
    Int,            // int
    Void,           // void
    Return,         // return
    If,             // if
    Else,           // else
    While,          // while
    For,            // for
    Do,             // do
    Break,          // break
    Continue,       // continue

    // 标识符
    Identifier,     // 变量名、函数名等

    // 特殊Token
    End,            // 输入结束
    Invalid         // 无效字符
};

// Token类：表示词法分析器识别出的一个Token
class Token {
public:
    // 默认构造函数
    Token() : type_(TokenType::Invalid), value_(""), line_(1), column_(1) {}

    // 构造函数
    Token(TokenType type, const std::string& value = "", int line = 1, int column = 1);

    // 获取Token类型
    TokenType getType() const { return type_; }

    // 获取Token的字面值
    const std::string& getValue() const { return value_; }

    // 获取行号
    int getLine() const { return line_; }

    // 获取列号
    int getColumn() const { return column_; }

    // 将Token转换为字符串表示
    std::string toString() const;

    // 判断Token是否为指定类型
    bool is(TokenType type) const { return type_ == type; }

    // 获取Token类型的字符串表示
    static std::string typeToString(TokenType type);

private:
    TokenType type_;      // Token类型
    std::string value_;   // Token的字面值
    int line_;           // 行号
    int column_;         // 列号
};

// Token流输出运算符
std::ostream& operator<<(std::ostream& os, const Token& token);

// TokenType输出运算符
std::ostream& operator<<(std::ostream& os, const TokenType& type);

#endif // TOKEN_H