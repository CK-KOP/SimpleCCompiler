#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "vm.h"
#include "type.h"
#include <string>
#include <unordered_map>

// 变量信息结构
struct VariableInfo {
    int offset;       // 栈偏移（局部变量）或全局偏移（全局变量）
    int slot_count;   // 占多少个 slot
    bool is_global;   // 是否是全局变量
    bool is_parameter; // 是否是参数

    VariableInfo() : offset(0), slot_count(0), is_global(false), is_parameter(false) {}

    VariableInfo(int off, int slots, bool global = false, bool param = false)
        : offset(off), slot_count(slots), is_global(global), is_parameter(param) {}
};

// 代码生成器：将 AST 转换为字节码
class CodeGen {
private:
    ByteCode code_;

    // ========== 新的统一变量管理系统 ==========
    // 统一的变量表：管理局部变量、全局变量、参数
    std::unordered_map<std::string, VariableInfo> variables_;

    int next_local_offset_ = 0;   // 下一个局部变量偏移
    int next_global_offset_ = 0;  // 下一个全局变量偏移（Phase 6）
    int next_param_offset_ = -3;  // 下一个参数偏移（从 fp-3 开始）

    // ========== 旧的变量管理系统（兼容，逐步迁移） ==========
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

    // ========== 统一变量分配接口 ==========
    int allocateVariable(const std::string& name, std::shared_ptr<Type> type);
    int allocateGlobalVariable(const std::string& name, std::shared_ptr<Type> type);  // Phase 6

    // 变量查询
    const VariableInfo* findVariable(const std::string& name) const;
    int getVariableOffset(const std::string& name) const;
    int getVariableSlotCount(const std::string& name) const;
    bool isGlobalVariable(const std::string& name) const;
    bool isParameter(const std::string& name) const;

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
