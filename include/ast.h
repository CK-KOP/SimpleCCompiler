#ifndef AST_H
#define AST_H

#include "../include/token.h"
#include "../include/type.h"
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
private:
    std::shared_ptr<Type> resolved_type_;

public:
    ~ExprNode() override = default;
    virtual std::string toString() const override = 0;

    void setResolvedType(std::shared_ptr<Type> type) { resolved_type_ = type; }
    std::shared_ptr<Type> getResolvedType() const { return resolved_type_; }
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

// 函数调用节点：foo(arg1, arg2, ...)
class FunctionCallNode : public ExprNode {
private:
    std::string name_;
    std::vector<std::unique_ptr<ExprNode>> args_;

public:
    FunctionCallNode(const std::string& name, std::vector<std::unique_ptr<ExprNode>> args)
        : name_(name), args_(std::move(args)) {}

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<ExprNode>>& getArgs() const { return args_; }

    std::string toString() const override {
        std::string result = "FunctionCall(" + name_;
        for (const auto& arg : args_) {
            result += ", " + arg->toString();
        }
        result += ")";
        return result;
    }
};

// 数组访问节点：arr[index] 或 arr[i][j]
class ArrayAccessNode : public ExprNode {
private:
    std::unique_ptr<ExprNode> array_;         // 数组表达式（可以是变量或另一个数组访问）
    std::unique_ptr<ExprNode> index_;         // 下标表达式

public:
    // 新构造函数：支持链式数组访问
    ArrayAccessNode(std::unique_ptr<ExprNode> array, std::unique_ptr<ExprNode> index)
        : array_(std::move(array)), index_(std::move(index)) {}

    // 兼容旧构造函数
    ArrayAccessNode(const std::string& name, std::unique_ptr<ExprNode> index)
        : array_(std::make_unique<VariableNode>(name)), index_(std::move(index)) {}

    ExprNode* getArray() const { return array_.get(); }
    ExprNode* getIndex() const { return index_.get(); }

    // 兼容旧接口：获取最底层的数组名
    const std::string& getArrayName() const {
        if (auto* var = dynamic_cast<VariableNode*>(array_.get())) {
            return var->getName();
        }
        // 递归获取
        if (auto* arr = dynamic_cast<ArrayAccessNode*>(array_.get())) {
            return arr->getArrayName();
        }
        static std::string empty;
        return empty;
    }

    std::string toString() const override {
        return "ArrayAccess(" + array_->toString() + ", " + index_->toString() + ")";
    }
};

// 语句节点基类
class StmtNode : public ASTNode {
public:
    virtual ~StmtNode() = default;
    virtual std::string toString() const override = 0;
};

// 变量声明语句节点（支持普通变量、指针和多维数组）
class VarDeclStmtNode : public StmtNode {
private:
    std::string type_;                        // 基础类型（含指针，如 "int*"）
    std::string name_;
    std::unique_ptr<ExprNode> initializer_;
    std::vector<int> array_dims_;             // 数组各维度大小，空表示非数组
    std::shared_ptr<Type> resolved_type_;

public:
    // 普通变量声明
    VarDeclStmtNode(const std::string& type, const std::string& name, std::unique_ptr<ExprNode> initializer = nullptr)
        : type_(type), name_(name), initializer_(std::move(initializer)) {}

    // 数组声明（单维，兼容旧接口）
    VarDeclStmtNode(const std::string& type, const std::string& name, int array_size)
        : type_(type), name_(name), initializer_(nullptr), array_dims_({array_size}) {}

    // 多维数组声明
    VarDeclStmtNode(const std::string& type, const std::string& name, std::vector<int> dims)
        : type_(type), name_(name), initializer_(nullptr), array_dims_(std::move(dims)) {}

    const std::string& getType() const { return type_; }
    const std::string& getName() const { return name_; }
    ExprNode* getInitializer() const { return initializer_.get(); }
    bool hasInitializer() const { return initializer_ != nullptr; }
    bool isArray() const { return !array_dims_.empty(); }
    int getArraySize() const { return array_dims_.empty() ? -1 : array_dims_[0]; }
    const std::vector<int>& getArrayDims() const { return array_dims_; }

    void setResolvedType(std::shared_ptr<Type> type) { resolved_type_ = type; }
    std::shared_ptr<Type> getResolvedType() const { return resolved_type_; }

    std::string toString() const override {
        std::string result = "VarDecl(" + type_ + " " + name_;
        for (int dim : array_dims_) {
            result += "[" + std::to_string(dim) + "]";
        }
        if (initializer_) {
            result += " = " + initializer_->toString();
        }
        result += ")";
        return result;
    }
};

// 返回语句节点
class ReturnStmtNode : public StmtNode {
private:
    std::unique_ptr<ExprNode> expr_;                     // 返回表达式（可为空）

public:
    explicit ReturnStmtNode(std::unique_ptr<ExprNode> expr = nullptr)
        : expr_(std::move(expr)) {}

    ExprNode* getExpression() const { return expr_.get(); }
    bool hasExpression() const { return expr_ != nullptr; }

    std::string toString() const override {
        if (expr_) {
            return "Return(" + expr_->toString() + ")";
        } else {
            return "Return()";
        }
    }
};

// ElseIf分支结构
struct ElseIfBranch {
    std::unique_ptr<ExprNode> condition;               // 条件表达式
    std::unique_ptr<StmtNode> statement;               // 分支语句

    ElseIfBranch(std::unique_ptr<ExprNode> cond, std::unique_ptr<StmtNode> stmt)
        : condition(std::move(cond)), statement(std::move(stmt)) {}
};

// If语句节点：if (condition) then_stmt [else if (condition) stmt ...] [else else_stmt]
class IfStmtNode : public StmtNode {
private:
    std::unique_ptr<ExprNode> condition_;               // 主条件表达式
    std::unique_ptr<StmtNode> then_stmt_;               // then分支语句
    std::vector<std::unique_ptr<ElseIfBranch>> else_ifs_; // else if分支列表
    std::unique_ptr<StmtNode> else_stmt_;               // else分支语句（可为空）

public:
    IfStmtNode(std::unique_ptr<ExprNode> condition,
               std::unique_ptr<StmtNode> then_stmt)
        : condition_(std::move(condition)), then_stmt_(std::move(then_stmt)) {}

    // 添加else if分支
    void addElseIf(std::unique_ptr<ExprNode> condition, std::unique_ptr<StmtNode> statement) {
        else_ifs_.push_back(std::make_unique<ElseIfBranch>(std::move(condition), std::move(statement)));
    }

    // 设置else分支
    void setElseStmt(std::unique_ptr<StmtNode> else_stmt) {
        else_stmt_ = std::move(else_stmt);
    }

    // 访问器方法
    ExprNode* getCondition() const { return condition_.get(); }
    StmtNode* getThenStmt() const { return then_stmt_.get(); }
    StmtNode* getElseStmt() const { return else_stmt_.get(); }
    const std::vector<std::unique_ptr<ElseIfBranch>>& getElseIfs() const { return else_ifs_; }
    bool hasElseIfs() const { return !else_ifs_.empty(); }
    bool hasElseStmt() const { return else_stmt_ != nullptr; }

    std::string toString() const override {
        std::string result = "If(" + condition_->toString() + ", " + then_stmt_->toString();

        // 添加else if分支
        for (const auto& else_if : else_ifs_) {
            result += ", ElseIf(" + else_if->condition->toString() + ", " + else_if->statement->toString() + ")";
        }

        // 添加else分支
        if (else_stmt_) {
            result += ", Else(" + else_stmt_->toString() + ")";
        }

        result += ")";
        return result;
    }
};

// While语句节点：while (condition) stmt
class WhileStmtNode : public StmtNode {
private:
    std::unique_ptr<ExprNode> condition_;               // 循环条件
    std::unique_ptr<StmtNode> body_;                    // 循环体

public:
    WhileStmtNode(std::unique_ptr<ExprNode> condition, std::unique_ptr<StmtNode> body)
        : condition_(std::move(condition)), body_(std::move(body)) {}

    ExprNode* getCondition() const { return condition_.get(); }
    StmtNode* getBody() const { return body_.get(); }

    std::string toString() const override {
        return "While(" + condition_->toString() + ", " + body_->toString() + ")";
    }
};

// For语句节点：for (init; condition; increment) stmt
class ForStmtNode : public StmtNode {
private:
    std::unique_ptr<StmtNode> init_;                    // 初始化语句（可为空）
    std::unique_ptr<ExprNode> condition_;               // 循环条件（可为空）
    std::unique_ptr<ExprNode> increment_;               // 增量表达式（可为空）
    std::unique_ptr<StmtNode> body_;                    // 循环体

public:
    ForStmtNode(std::unique_ptr<StmtNode> init,
                std::unique_ptr<ExprNode> condition,
                std::unique_ptr<ExprNode> increment,
                std::unique_ptr<StmtNode> body)
        : init_(std::move(init)), condition_(std::move(condition)),
          increment_(std::move(increment)), body_(std::move(body)) {}

    StmtNode* getInit() const { return init_.get(); }
    ExprNode* getCondition() const { return condition_.get(); }
    ExprNode* getIncrement() const { return increment_.get(); }
    StmtNode* getBody() const { return body_.get(); }
    bool hasInit() const { return init_ != nullptr; }
    bool hasCondition() const { return condition_ != nullptr; }
    bool hasIncrement() const { return increment_ != nullptr; }

    std::string toString() const override {
        std::string result = "For(";
        if (init_) result += init_->toString(); else result += "null";
        result += ", ";
        if (condition_) result += condition_->toString(); else result += "null";
        result += ", ";
        if (increment_) result += increment_->toString(); else result += "null";
        result += ", " + body_->toString() + ")";
        return result;
    }
};

// Do-While语句节点：do stmt while (condition)
class DoWhileStmtNode : public StmtNode {
private:
    std::unique_ptr<StmtNode> body_;                    // 循环体
    std::unique_ptr<ExprNode> condition_;               // 循环条件

public:
    DoWhileStmtNode(std::unique_ptr<StmtNode> body, std::unique_ptr<ExprNode> condition)
        : body_(std::move(body)), condition_(std::move(condition)) {}

    StmtNode* getBody() const { return body_.get(); }
    ExprNode* getCondition() const { return condition_.get(); }

    std::string toString() const override {
        return "DoWhile(" + body_->toString() + ", " + condition_->toString() + ")";
    }
};

// Break语句节点：break;
class BreakStmtNode : public StmtNode {
public:
    BreakStmtNode() = default;

    std::string toString() const override {
        return "Break()";
    }
};

// Continue语句节点：continue;
class ContinueStmtNode : public StmtNode {
public:
    ContinueStmtNode() = default;

    std::string toString() const override {
        return "Continue()";
    }
};

// 空语句节点：;
class EmptyStmtNode : public StmtNode {
public:
    EmptyStmtNode() = default;

    std::string toString() const override {
        return "EmptyStmt()";
    }
};

// 表达式语句节点：expr;
class ExprStmtNode : public StmtNode {
private:
    std::unique_ptr<ExprNode> expr_;

public:
    explicit ExprStmtNode(std::unique_ptr<ExprNode> expr)
        : expr_(std::move(expr)) {}

    ExprNode* getExpression() const { return expr_.get(); }

    std::string toString() const override {
        return "ExprStmt(" + expr_->toString() + ")";
    }
};

// 复合语句节点：{ stmt1; stmt2; ... }
class CompoundStmtNode : public StmtNode {
private:
    std::vector<std::unique_ptr<StmtNode>> statements_;

public:
    void addStatement(std::unique_ptr<StmtNode> stmt) {
        statements_.push_back(std::move(stmt));
    }

    const std::vector<std::unique_ptr<StmtNode>>& getStatements() const {
        return statements_;
    }

    std::string toString() const override {
        std::string result = "CompoundStmt(";
        for (size_t i = 0; i < statements_.size(); ++i) {
            if (i > 0) result += ", ";
            result += statements_[i]->toString();
        }
        result += ")";
        return result;
    }
};

// 函数参数
struct FunctionParam {
    std::string type;
    std::string name;

    FunctionParam(const std::string& t, const std::string& n) : type(t), name(n) {}
};

// 函数定义节点：int foo(int a, int b) { ... }
class FunctionDeclNode : public ASTNode {
private:
    std::string return_type_;
    std::string name_;
    std::vector<FunctionParam> params_;
    std::unique_ptr<CompoundStmtNode> body_;

public:
    FunctionDeclNode(const std::string& return_type, const std::string& name,
                     std::vector<FunctionParam> params, std::unique_ptr<CompoundStmtNode> body)
        : return_type_(return_type), name_(name), params_(std::move(params)), body_(std::move(body)) {}

    const std::string& getReturnType() const { return return_type_; }
    const std::string& getName() const { return name_; }
    const std::vector<FunctionParam>& getParams() const { return params_; }
    CompoundStmtNode* getBody() const { return body_.get(); }

    std::string toString() const override {
        std::string result = "FunctionDecl(" + return_type_ + " " + name_ + "(";
        for (size_t i = 0; i < params_.size(); ++i) {
            if (i > 0) result += ", ";
            result += params_[i].type + " " + params_[i].name;
        }
        result += "), " + body_->toString() + ")";
        return result;
    }
};

// 程序节点：顶层，包含多个函数定义
class ProgramNode : public ASTNode {
private:
    std::vector<std::unique_ptr<FunctionDeclNode>> functions_;

public:
    void addFunction(std::unique_ptr<FunctionDeclNode> func) {
        functions_.push_back(std::move(func));
    }

    const std::vector<std::unique_ptr<FunctionDeclNode>>& getFunctions() const {
        return functions_;
    }

    std::string toString() const override {
        std::string result = "Program(";
        for (size_t i = 0; i < functions_.size(); ++i) {
            if (i > 0) result += ", ";
            result += functions_[i]->toString();
        }
        result += ")";
        return result;
    }
};

#endif // AST_H