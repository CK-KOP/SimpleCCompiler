#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "vm.h"
#include <string>
#include <unordered_map>

// 代码生成器：将 AST 转换为字节码
class CodeGen {
private:
    ByteCode code_;

    // 当前函数的局部变量表: 变量名 -> 栈偏移
    std::unordered_map<std::string, int> locals_;
    // 数组大小表: 数组名 -> 大小 (非数组变量不在此表中)
    std::unordered_map<std::string, int> array_sizes_;
    int local_offset_ = 0;  // 下一个局部变量的偏移

    // 用于 break/continue
    std::vector<int> break_targets_;
    std::vector<int> continue_targets_;

    // 当前函数的参数 slot 数 (用于计算 ret_slot_offset)
    // TODO: 支持 struct 参数时，需改为计算总 slot 数而非参数个数
    int current_param_slots_ = 0;

public:
    ByteCode generate(ProgramNode* program);

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
    void genArrayAccessAddr(ArrayAccessNode* expr);
    void genMemberAccess(MemberAccessNode* expr);
    void genMemberAccessAddr(MemberAccessNode* expr);

    int allocLocal(const std::string& name);
    int allocArray(const std::string& name, int size);
    int allocStruct(const std::string& name, int slot_count);
    int getLocal(const std::string& name);
    bool isArray(const std::string& name);

    // ========== 类型判断辅助函数 ==========
    bool isStructType(ExprNode* node) const;
    bool isArrayType(ExprNode* node) const;
    bool isPointerType(ExprNode* node) const;
    bool isIntType(ExprNode* node) const;
    int getSlotCount(ExprNode* node) const;
    int getSlotCount(std::shared_ptr<Type> type) const;
    bool hasValidType(ExprNode* node) const;
    std::shared_ptr<Type> getType(ExprNode* node) const;
};

#endif // CODEGEN_H
