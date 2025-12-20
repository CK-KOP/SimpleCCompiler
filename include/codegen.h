#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "vm.h"
#include <string>
#include <unordered_map>

// 前向声明
class Sema;

// 代码生成器：将 AST 转换为字节码
class CodeGen {
private:
    ByteCode code_;
    const Sema* sema_;  // Sema 引用（用于查询类型信息）

    // 当前函数的局部变量表: 变量名 -> 栈偏移
    std::unordered_map<std::string, int> locals_;
    // 数组大小表: 数组名 -> 大小 (非数组变量不在此表中)
    std::unordered_map<std::string, int> array_sizes_;
    int local_offset_ = 0;  // 下一个局部变量的偏移

    // 用于 break/continue
    std::vector<int> break_targets_;
    std::vector<int> continue_targets_;

public:
    ByteCode generate(ProgramNode* program, const Sema* sema);

private:
    void genFunction(FunctionDeclNode* func);
    void genStatement(StmtNode* stmt);
    void genCompoundStmt(CompoundStmtNode* stmt);
    void genVarDecl(VarDeclStmtNode* stmt);
    void genIfStmt(IfStmtNode* stmt);
    void genWhileStmt(WhileStmtNode* stmt);
    void genForStmt(ForStmtNode* stmt);
    void genDoWhileStmt(DoWhileStmtNode* stmt);
    void genReturnStmt(ReturnStmtNode* stmt);
    void genExprStmt(ExprStmtNode* stmt);

    void genExpression(ExprNode* expr);
    void genBinaryOp(BinaryOpNode* expr);
    void genUnaryOp(UnaryOpNode* expr);
    void genFunctionCall(FunctionCallNode* expr);
    void genArrayAccess(ArrayAccessNode* expr);

    int allocLocal(const std::string& name);
    int allocArray(const std::string& name, int size);
    int getLocal(const std::string& name);
    bool isArray(const std::string& name);
};

#endif // CODEGEN_H
