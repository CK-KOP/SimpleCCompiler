#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "scope.h"
#include "type.h"
#include <string>
#include <vector>

// 语义错误
struct SemanticError {
    std::string message;
    int line;

    SemanticError(const std::string& msg, int ln = 0)
        : message(msg), line(ln) {}
};

// 语义分析器
class Sema {
private:
    Scope scope_;
    std::vector<SemanticError> errors_;
    std::unordered_map<std::string, std::shared_ptr<StructType>> struct_types_;

    // 当前正在分析的函数的返回类型
    std::shared_ptr<Type> current_function_return_type_;

    void error(const std::string& msg, int line = 0) {
        errors_.emplace_back(msg, line);
    }

public:
    Sema() = default;

    // 分析整个程序
    bool analyze(ProgramNode* program);

    // 获取错误列表
    const std::vector<SemanticError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    // 分析结构体定义
    void analyzeStructDecl(StructDeclNode* struct_decl);

    // 分析函数定义
    void analyzeFunction(FunctionDeclNode* func);

    // 分析语句
    void analyzeStatement(StmtNode* stmt);
    void analyzeCompoundStatement(CompoundStmtNode* stmt);
    void analyzeVarDecl(VarDeclStmtNode* stmt);
    void analyzeReturnStatement(ReturnStmtNode* stmt);
    void analyzeIfStatement(IfStmtNode* stmt);
    void analyzeWhileStatement(WhileStmtNode* stmt);
    void analyzeForStatement(ForStmtNode* stmt);
    void analyzeDoWhileStatement(DoWhileStmtNode* stmt);
    void analyzeExprStatement(ExprStmtNode* stmt);

    // 分析表达式，返回表达式的类型
    std::shared_ptr<Type> analyzeExpression(ExprNode* expr);
    std::shared_ptr<Type> analyzeVariable(VariableNode* expr);
    std::shared_ptr<Type> analyzeBinaryOp(BinaryOpNode* expr);
    std::shared_ptr<Type> analyzeUnaryOp(UnaryOpNode* expr);
    std::shared_ptr<Type> analyzeFunctionCall(FunctionCallNode* expr);
    std::shared_ptr<Type> analyzeArrayAccess(ArrayAccessNode* expr);
    std::shared_ptr<Type> analyzeMemberAccess(MemberAccessNode* expr);

    // 辅助方法
    std::shared_ptr<Type> stringToType(const std::string& type_name);
};

#endif // SEMA_H
