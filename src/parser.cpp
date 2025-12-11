#include "parser.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(Lexer& lexer) : lexer_(lexer) {
    // 获取第一个Token
    currentToken_ = lexer_.getNextToken();
}

Token Parser::getCurrentToken() const {
    return currentToken_;
}

bool Parser::is(TokenType type) const {
    return currentToken_.is(type);
}

bool Parser::isAtEnd() const {
    return currentToken_.is(TokenType::End);
}

std::unique_ptr<ExprNode> Parser::parseExpression() {
    // 递归下降解析器入口点
    // 表达式语法优先级链（从低到高）:
    // assignment -> logical_or -> logical_and -> equality -> comparison -> term -> factor -> unary -> primary
    return parseAssignment();
}

// 解析完整表达式并检查语法错误
std::unique_ptr<ExprNode> Parser::parseCompleteExpression() {
    auto expr = parseExpression();

    // 检查是否还有未消费的Token
    if (!isAtEnd()) {
        throw std::runtime_error("语法错误：表达式后有多余的Token: " + currentToken_.toString());
    }

    return expr;
}

// 解析赋值表达式（最低优先级，右结合）
std::unique_ptr<ExprNode> Parser::parseAssignment() {
    // 先解析右边的高优先级表达式
    std::unique_ptr<ExprNode> expr = parseLogicalOr();

    if (match(TokenType::Assign)) {
        advance(); // 消费=
        auto right = parseAssignment(); // 右结合：递归调用自身，a = b = c 解析为 a = (b = c)

        // 安全检查：赋值运算符左边必须是变量
        if (dynamic_cast<VariableNode*>(expr.get())) {
            return std::make_unique<BinaryOpNode>(std::move(expr), TokenType::Assign, std::move(right));
        } else {
            throw std::runtime_error("赋值运算符左边必须是变量");
        }
    }

    return expr;
}

// 解析逻辑或表达式（优先级低于逻辑与，左结合）
std::unique_ptr<ExprNode> Parser::parseLogicalOr() {
    std::unique_ptr<ExprNode> expr = parseLogicalAnd();

    // 检查 || 运算符（短路逻辑，左结合）
    while (match(TokenType::LogicalOr)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费||
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

// 解析逻辑与表达式（优先级低于比较运算符，左结合）
std::unique_ptr<ExprNode> Parser::parseLogicalAnd() {
    std::unique_ptr<ExprNode> expr = parseEquality();

    // 检查 && 运算符（短路逻辑，左结合）
    while (match(TokenType::LogicalAnd)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费&&
        auto right = parseEquality();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseEquality() {
    // 解析等值比较表达式: == 和 !=
    std::unique_ptr<ExprNode> expr = parseComparison();

    // 检查是否有 == 或 != 运算符
    while (match(TokenType::Equal) || match(TokenType::NotEqual)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseComparison();
        // 构建二元运算节点，左结合
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseComparison() {
    // 解析大小比较表达式: <, <=, >, >=
    std::unique_ptr<ExprNode> expr = parseTerm();

    // 检查所有大小比较运算符
    while (match(TokenType::Less) || match(TokenType::LessEqual) ||
           match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseTerm();
        // 构建二元运算节点，左结合
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseTerm() {
    // 解析加减法表达式: + 和 -
    std::unique_ptr<ExprNode> expr = parseFactor();

    // 检查加减法运算符（左结合）
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseFactor();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseFactor() {
    // 解析乘除法表达式: * 和 /
    std::unique_ptr<ExprNode> expr = parseUnary();

    // 检查乘除法运算符（左结合，比加减法优先级高）
    while (match(TokenType::Multiply) || match(TokenType::Divide)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseUnary();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
    // 解析一元运算符: +x, -x, !x（优先级很高，仅次于括号）
    if (match(TokenType::Plus)) {
        advance(); // 消费+
        return parseUnary(); // 递归处理+号，正号通常不改变值
    }

    if (match(TokenType::Minus)) {
        advance(); // 消费-
        auto operand = parseUnary(); // 递归解析操作数，支持--x这样的表达式
        return std::make_unique<UnaryOpNode>(TokenType::Minus, std::move(operand));
    }

    if (match(TokenType::LogicalNot)) {
        advance(); // 消费!
        auto operand = parseUnary(); // 递归解析操作数
        return std::make_unique<UnaryOpNode>(TokenType::LogicalNot, std::move(operand));
    }

    // 没有一元运算符，直接解析基础表达式
    return parsePrimary();
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    // 解析基础表达式：数字、变量、括号表达式（最高优先级）
    if (match(TokenType::Number)) {
        int value = std::stoi(currentToken_.getValue());
        advance();
        return std::make_unique<NumberNode>(value);
    }

    if (match(TokenType::Identifier)) {
        std::string name = currentToken_.getValue();
        advance();
        return std::make_unique<VariableNode>(name);
    }

    if (match(TokenType::LParen)) {
        advance(); // 消费(
        auto expr = parseExpression(); // 递归解析括号内的完整表达式
        consume(TokenType::RParen, "期望 ')'");
        return expr; // 返回括号内的表达式，括号在AST中消失
    }

    throw std::runtime_error("意外的Token: " + currentToken_.toString());
}

// 前进到下一个Token（更新currentToken_）
void Parser::advance() {
    if (!isAtEnd()) {
        currentToken_ = lexer_.getNextToken();
    }
}

// 消费指定类型的Token，如果不匹配则抛出错误
Token Parser::consume(TokenType type, const std::string& message) {
    if (currentToken_.is(type)) {
        Token token = currentToken_;
        advance();
        return token;
    }
    throw std::runtime_error(message + ", 但得到: " + currentToken_.toString());
}

// 检查当前Token是否是指定类型（不消费Token）
bool Parser::match(TokenType type) {
    if (currentToken_.is(type)) {
        return true;
    }
    return false;
}

// 获取运算符的优先级（数值越大优先级越高）
Parser::Precedence Parser::getOperatorPrecedence(TokenType op) {
    switch (op) {
        case TokenType::Assign:
            return PREC_ASSIGN;           // = (最低优先级，右结合)
        case TokenType::LogicalOr:
            return PREC_LOGICAL_OR;       // ||
        case TokenType::LogicalAnd:
            return PREC_LOGICAL_AND;      // &&
        case TokenType::Equal:
        case TokenType::NotEqual:
            return PREC_EQUALITY;         // ==, !=
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
            return PREC_COMPARISON;       // <, <=, >, >=
        case TokenType::Plus:
        case TokenType::Minus:
            return PREC_TERM;             // +, -
        case TokenType::Multiply:
        case TokenType::Divide:
            return PREC_FACTOR;           // *, /
        default:
            return PREC_LOWEST;           // 未知运算符最低优先级
    }
}