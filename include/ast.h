#ifndef AST_H
#define AST_H

#include "../include/token.h"
#include <string>
#include <memory>
#include <vector>

// ast.h
// SimpleC编译器的抽象语法树(AST)节点定义
// 第二阶段：表达式语法分析

// AST节点基类
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

// 表达式节点基类
class ExprNode : public ASTNode {
public:
    ~ExprNode() override = default;
    virtual std::string toString() const override = 0;
};

// 数字字面量节点
class NumberNode : public ExprNode {
private:
    int value_;

public:
    explicit NumberNode(int value) : value_(value) {}

    int getValue() const { return value_; }

    std::string toString() const override {
        return "Number(" + std::to_string(value_) + ")";
    }
};

// 变量引用节点
class VariableNode : public ExprNode {
private:
    std::string name_;

public:
    explicit VariableNode(const std::string& name) : name_(name) {}

    const std::string& getName() const { return name_; }

    std::string toString() const override {
        return "Variable(" + name_ + ")";
    }
};

// 二元运算符节点
class BinaryOpNode : public ExprNode {
private:
    std::unique_ptr<ExprNode> left_;
    std::unique_ptr<ExprNode> right_;
    TokenType op_;

public:
    BinaryOpNode(std::unique_ptr<ExprNode> left, TokenType op, std::unique_ptr<ExprNode> right)
        : left_(std::move(left)), right_(std::move(right)), op_(op) {}

    ExprNode* getLeft() const { return left_.get(); }
    ExprNode* getRight() const { return right_.get(); }
    TokenType getOperator() const { return op_; }

    std::string toString() const override {
        return "BinaryOp(" + Token::typeToString(op_) + ", " +
               left_->toString() + ", " + right_->toString() + ")";
    }
};

// 一元运算符节点
class UnaryOpNode : public ExprNode {
private:
    std::unique_ptr<ExprNode> operand_;
    TokenType op_;

public:
    UnaryOpNode(TokenType op, std::unique_ptr<ExprNode> operand)
        : operand_(std::move(operand)), op_(op) {}

    ExprNode* getOperand() const { return operand_.get(); }
    TokenType getOperator() const { return op_; }

    std::string toString() const override {
        return "UnaryOp(" + Token::typeToString(op_) + ", " + operand_->toString() + ")";
    }
};

#endif // AST_H