#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>

// parser.h
// SimpleC编译器的语法分析器
// 第二阶段：表达式语法分析

class Parser {
public:
    explicit Parser(Lexer& lexer);

    // 获取当前Token
    Token getCurrentToken() const;

    // 检查当前Token类型
    bool is(TokenType type) const;

    // 是否到达结尾
    bool isAtEnd() const;

    // 解析表达式 - 公共接口
    std::unique_ptr<ExprNode> parseExpression();

    // 解析完整表达式并检查语法错误
    std::unique_ptr<ExprNode> parseCompleteExpression();

    // 语句解析功能
    std::unique_ptr<StmtNode> parseStatement();
    std::unique_ptr<CompoundStmtNode> parseCompoundStatement();

    // 函数和程序解析
    std::unique_ptr<FunctionDeclNode> parseFunctionDeclaration();
    std::unique_ptr<ProgramNode> parseProgram();

    // 声明和特定语句解析功能
    std::unique_ptr<VarDeclStmtNode> parseVariableDeclaration();
    std::unique_ptr<ReturnStmtNode> parseReturnStatement();

    // 控制流语句解析功能
    std::unique_ptr<IfStmtNode> parseIfStatement();
    std::unique_ptr<WhileStmtNode> parseWhileStatement();
    std::unique_ptr<ForStmtNode> parseForStatement();
    std::unique_ptr<DoWhileStmtNode> parseDoWhileStatement();
    std::unique_ptr<BreakStmtNode> parseBreakStatement();
    std::unique_ptr<ContinueStmtNode> parseContinueStatement();

private:
    Lexer& lexer_;            // 词法分析器引用
    Token currentToken_;       // 当前Token

    // 运算符优先级定义
    enum Precedence {
        PREC_LOWEST = 1,
        PREC_ASSIGN = 2,       // =
        PREC_LOGICAL_OR = 3,   // ||
        PREC_LOGICAL_AND = 4,  // &&
        PREC_EQUALITY = 5,     // ==, !=
        PREC_COMPARISON = 6,   // <, <=, >, >=
        PREC_TERM = 7,         // +, -
        PREC_FACTOR = 8,       // *, /
        PREC_UNARY = 9,        // +x, -x, !x
        PREC_PRIMARY = 10,     // (), 数字, 变量
        PREC_HIGHEST = 11
    };

  
    // 表达式基础解析方法
    std::unique_ptr<ExprNode> parsePrimary();
    std::unique_ptr<ExprNode> parseUnary();
    std::unique_ptr<ExprNode> parseFactor();
    std::unique_ptr<ExprNode> parseTerm();
    std::unique_ptr<ExprNode> parseComparison();
    std::unique_ptr<ExprNode> parseEquality();
    std::unique_ptr<ExprNode> parseLogicalAnd();
    std::unique_ptr<ExprNode> parseLogicalOr();
    std::unique_ptr<ExprNode> parseAssignment();

    // 辅助方法
    void advance();                          // 前进到下一个Token
    Token consume(TokenType type, const std::string& message); // 消费指定类型的Token
    bool match(TokenType type);             // 检查并消费Token
    bool isTypeKeyword() const;             // 检查是否是类型关键字
    Precedence getOperatorPrecedence(TokenType op); // 获取运算符优先级

    // 函数调用解析
    std::unique_ptr<FunctionCallNode> parseFunctionCall(const std::string& name);
};

#endif // PARSER_H