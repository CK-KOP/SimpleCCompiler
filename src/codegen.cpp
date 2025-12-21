#include "../include/codegen.h"
#include <stdexcept>

ByteCode CodeGen::generate(ProgramNode* program) {
    // 生成所有函数的代码
    for (const auto& func : program->getFunctions()) {
        genFunction(func.get());
    }

    // 设置入口点
    auto it = code_.functions.find("main");
    if (it != code_.functions.end()) {
        code_.entry_point = it->second;
    }

    return code_;
}

void CodeGen::genFunction(FunctionDeclNode* func) {
    // 记录函数地址
    code_.functions[func->getName()] = code_.currentAddress();

    // 重置局部变量表
    locals_.clear();
    array_sizes_.clear();
    local_offset_ = 0;

    // 记录参数 slot 数 (用于计算 ret_slot_offset)
    // TODO: 支持 struct 参数时，需计算总 slot 数
    const auto& params = func->getParams();
    current_param_slots_ = params.size();

    // 为参数分配空间（参数在调用前已压栈，位于负偏移）
    // 栈帧布局:
    //   [ret_slot]   fp - 3 - param_slots (由 caller 预留)
    //   [param_n]    fp - 3 - (n-1)
    //   ...
    //   [param_1]    fp - 3
    //   [ret_addr]   fp - 2
    //   [old_fp]     fp - 1
    //   fp ->
    for (size_t i = 0; i < params.size(); ++i) {
        locals_[params[i].name] = -(int)i - 3;
    }

    // 生成函数体
    genCompoundStmt(func->getBody());

    // 如果函数没有显式 return，添加默认返回
    if (code_.code.empty() || code_.code.back().op != OpCode::RET) {
        code_.emit(OpCode::PUSH, 0);  // 默认返回值 0
        // ret_slot_offset = -3 - param_slots
        // TODO: 支持 struct 返回值时，需调整偏移计算
        code_.emit(OpCode::RET, -3 - current_param_slots_);
    }
}

void CodeGen::genStatement(StmtNode* stmt) {
    if (auto* compound = dynamic_cast<CompoundStmtNode*>(stmt)) {
        genCompoundStmt(compound);
    } else if (auto* var_decl = dynamic_cast<VarDeclStmtNode*>(stmt)) {
        genVarDecl(var_decl);
    } else if (auto* if_stmt = dynamic_cast<IfStmtNode*>(stmt)) {
        genIfStmt(if_stmt);
    } else if (auto* while_stmt = dynamic_cast<WhileStmtNode*>(stmt)) {
        genWhileStmt(while_stmt);
    } else if (auto* for_stmt = dynamic_cast<ForStmtNode*>(stmt)) {
        genForStmt(for_stmt);
    } else if (auto* do_while = dynamic_cast<DoWhileStmtNode*>(stmt)) {
        genDoWhileStmt(do_while);
    } else if (auto* ret_stmt = dynamic_cast<ReturnStmtNode*>(stmt)) {
        genReturnStmt(ret_stmt);
    } else if (auto* expr_stmt = dynamic_cast<ExprStmtNode*>(stmt)) {
        genExprStmt(expr_stmt);
    } else if (dynamic_cast<BreakStmtNode*>(stmt)) {
        int jmp_addr = code_.currentAddress();
        code_.emit(OpCode::JMP, 0);
        break_targets_.push_back(jmp_addr);
    } else if (dynamic_cast<ContinueStmtNode*>(stmt)) {
        int jmp_addr = code_.currentAddress();
        code_.emit(OpCode::JMP, 0);
        continue_targets_.push_back(jmp_addr);
    }
    // EmptyStmtNode 不生成代码
}

void CodeGen::genCompoundStmt(CompoundStmtNode* stmt) {
    // 保存当前作用域状态
    int saved_offset = local_offset_;
    auto saved_locals = locals_;
    auto saved_arrays = array_sizes_;

    for (const auto& s : stmt->getStatements()) {
        genStatement(s.get());
    }

    // 恢复作用域状态（回收局部变量空间）
    int vars_to_pop = local_offset_ - saved_offset;
    if (vars_to_pop > 0) {
        code_.emit(OpCode::ADJSP, vars_to_pop);
    }
    local_offset_ = saved_offset;
    locals_ = saved_locals;
    array_sizes_ = saved_arrays;
}

void CodeGen::genVarDecl(VarDeclStmtNode* stmt) {
    auto type = stmt->getResolvedType();
    if (!type) {
        throw std::runtime_error("Variable type not resolved: " + stmt->getName());
    }

    // 使用 getSlotCount() 获取总slot数（支持多维数组和指针数组）
    int slot_count = type->getSlotCount();

    if (type->isArray()) {
        int offset = allocArray(stmt->getName(), slot_count);
        for (int i = 0; i < slot_count; i++) {
            code_.emit(OpCode::PUSH, 0);
        }
        (void)offset;
    } else {
        int offset = allocLocal(stmt->getName());
        if (stmt->hasInitializer()) {
            genExpression(stmt->getInitializer());
        } else {
            code_.emit(OpCode::PUSH, 0);
        }
        (void)offset;
    }
}

void CodeGen::genIfStmt(IfStmtNode* stmt) {
    genExpression(stmt->getCondition());

    int jz_addr = code_.currentAddress();
    code_.emit(OpCode::JZ, 0);  // 条件为假跳转

    genStatement(stmt->getThenStmt());

    if (stmt->hasElseStmt() || !stmt->getElseIfs().empty()) {
        // 收集所有需要跳转到末尾的地址
        std::vector<int> jmp_ends;

        jmp_ends.push_back(code_.currentAddress());
        code_.emit(OpCode::JMP, 0);  // 跳过 else

        code_.patch(jz_addr, code_.currentAddress());

        // 处理 else if
        for (const auto& else_if : stmt->getElseIfs()) {
            genExpression(else_if->condition.get());
            int else_if_jz = code_.currentAddress();
            code_.emit(OpCode::JZ, 0);

            genStatement(else_if->statement.get());

            jmp_ends.push_back(code_.currentAddress());
            code_.emit(OpCode::JMP, 0);
            code_.patch(else_if_jz, code_.currentAddress());
        }

        if (stmt->hasElseStmt()) {
            genStatement(stmt->getElseStmt());
        }

        // 统一回填所有跳转到末尾
        for (int addr : jmp_ends) {
            code_.patch(addr, code_.currentAddress());
        }
    } else {
        code_.patch(jz_addr, code_.currentAddress());
    }
}

void CodeGen::genWhileStmt(WhileStmtNode* stmt) {
    int loop_start = code_.currentAddress();

    genExpression(stmt->getCondition());
    int jz_addr = code_.currentAddress();
    code_.emit(OpCode::JZ, 0);

    size_t break_start = break_targets_.size();
    size_t continue_start = continue_targets_.size();
    genStatement(stmt->getBody());

    // 回填 continue 到 loop_start（条件检查）
    for (size_t i = continue_start; i < continue_targets_.size(); ++i) {
        code_.patch(continue_targets_[i], loop_start);
    }
    continue_targets_.resize(continue_start);

    code_.emit(OpCode::JMP, loop_start);
    code_.patch(jz_addr, code_.currentAddress());

    // 回填 break
    for (size_t i = break_start; i < break_targets_.size(); ++i) {
        code_.patch(break_targets_[i], code_.currentAddress());
    }
    break_targets_.resize(break_start);
}

void CodeGen::genForStmt(ForStmtNode* stmt) {
    if (stmt->hasInit()) {
        genStatement(stmt->getInit());
    }

    int loop_start = code_.currentAddress();
    int jz_addr = -1;

    if (stmt->hasCondition()) {
        genExpression(stmt->getCondition());
        jz_addr = code_.currentAddress();
        code_.emit(OpCode::JZ, 0);
    }

    size_t break_start = break_targets_.size();
    size_t continue_start = continue_targets_.size();
    genStatement(stmt->getBody());

    // 回填 continue 到 increment
    int increment_addr = code_.currentAddress();
    for (size_t i = continue_start; i < continue_targets_.size(); ++i) {
        code_.patch(continue_targets_[i], increment_addr);
    }
    continue_targets_.resize(continue_start);

    if (stmt->hasIncrement()) {
        genExpression(stmt->getIncrement());
        code_.emit(OpCode::POP);  // 丢弃增量表达式的值
    }

    code_.emit(OpCode::JMP, loop_start);

    if (jz_addr >= 0) {
        code_.patch(jz_addr, code_.currentAddress());
    }

    // 回填 break
    for (size_t i = break_start; i < break_targets_.size(); ++i) {
        code_.patch(break_targets_[i], code_.currentAddress());
    }
    break_targets_.resize(break_start);
}

void CodeGen::genDoWhileStmt(DoWhileStmtNode* stmt) {
    int loop_start = code_.currentAddress();

    size_t break_start = break_targets_.size();
    size_t continue_start = continue_targets_.size();
    genStatement(stmt->getBody());

    // 回填 continue 到条件检查
    int cond_addr = code_.currentAddress();
    for (size_t i = continue_start; i < continue_targets_.size(); ++i) {
        code_.patch(continue_targets_[i], cond_addr);
    }
    continue_targets_.resize(continue_start);

    genExpression(stmt->getCondition());
    code_.emit(OpCode::JNZ, loop_start);

    // 回填 break
    for (size_t i = break_start; i < break_targets_.size(); ++i) {
        code_.patch(break_targets_[i], code_.currentAddress());
    }
    break_targets_.resize(break_start);
}

void CodeGen::genReturnStmt(ReturnStmtNode* stmt) {
    if (stmt->hasExpression()) {
        genExpression(stmt->getExpression());
    } else {
        code_.emit(OpCode::PUSH, 0);  // void 函数默认返回 0
    }
    // ret_slot_offset = -3 - param_slots
    // TODO: 支持 struct 返回值时，需调整偏移计算
    code_.emit(OpCode::RET, -3 - current_param_slots_);
}

void CodeGen::genExprStmt(ExprStmtNode* stmt) {
    genExpression(stmt->getExpression());
    code_.emit(OpCode::POP);  // 丢弃表达式结果
}

void CodeGen::genExpression(ExprNode* expr) {
    if (auto* num = dynamic_cast<NumberNode*>(expr)) {
        code_.emit(OpCode::PUSH, num->getValue());
    } else if (auto* var = dynamic_cast<VariableNode*>(expr)) {
        int offset = getLocal(var->getName());
        code_.emit(OpCode::LOAD, offset);
    } else if (auto* binary = dynamic_cast<BinaryOpNode*>(expr)) {
        genBinaryOp(binary);
    } else if (auto* unary = dynamic_cast<UnaryOpNode*>(expr)) {
        genUnaryOp(unary);
    } else if (auto* call = dynamic_cast<FunctionCallNode*>(expr)) {
        genFunctionCall(call);
    } else if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr)) {
        genArrayAccess(arr);
    }
}

void CodeGen::genBinaryOp(BinaryOpNode* expr) {
    // 赋值运算符特殊处理
    if (expr->getOperator() == TokenType::Assign) {
        // 检查是否是数组赋值（支持多维）
        if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr->getLeft())) {
            // 获取元素大小
            auto array_type = arr->getArray()->getResolvedType();
            int elem_size = 1;
            if (array_type && array_type->isArray()) {
                auto* arr_t = static_cast<ArrayType*>(array_type.get());
                elem_size = arr_t->getElementType()->getSlotCount();
            }

            // arr[index] = value
            genExpression(expr->getRight());  // 计算值
            genExpression(arr->getIndex());   // 计算下标

            // 生成数组基地址
            if (auto* var = dynamic_cast<VariableNode*>(arr->getArray())) {
                code_.emit(OpCode::LEA, getLocal(var->getName()));
            } else if (auto* inner = dynamic_cast<ArrayAccessNode*>(arr->getArray())) {
                genArrayAccessAddr(inner);
            }

            code_.emit(OpCode::ADDPTRD, elem_size);
            code_.emit(OpCode::STOREM);

            // 赋值表达式应该返回值，重新加载
            genExpression(arr->getIndex());
            if (auto* var = dynamic_cast<VariableNode*>(arr->getArray())) {
                code_.emit(OpCode::LEA, getLocal(var->getName()));
            } else if (auto* inner = dynamic_cast<ArrayAccessNode*>(arr->getArray())) {
                genArrayAccessAddr(inner);
            }
            code_.emit(OpCode::ADDPTRD, elem_size);
            code_.emit(OpCode::LOADM);
            return;
        }

        // 检查是否是解引用赋值 *p = value
        if (auto* unary = dynamic_cast<UnaryOpNode*>(expr->getLeft())) {
            if (unary->getOperator() == TokenType::Multiply) {
                genExpression(expr->getRight());     // 计算要存储的值
                genExpression(unary->getOperand());  // 计算指针值（地址）
                code_.emit(OpCode::STOREM);          // 存储到地址
                // 赋值表达式返回值，重新加载
                genExpression(unary->getOperand());
                code_.emit(OpCode::LOADM);
                return;
            }
        }

        // 普通变量赋值
        auto* var = dynamic_cast<VariableNode*>(expr->getLeft());
        if (!var) {
            throw std::runtime_error("Invalid assignment target");
        }
        genExpression(expr->getRight());
        int offset = getLocal(var->getName());
        code_.emit(OpCode::STORE, offset);
        code_.emit(OpCode::LOAD, offset);  // 赋值表达式返回值
        return;
    }

    genExpression(expr->getLeft());
    genExpression(expr->getRight());

    switch (expr->getOperator()) {
        case TokenType::Plus:         code_.emit(OpCode::ADD); break;
        case TokenType::Minus:        code_.emit(OpCode::SUB); break;
        case TokenType::Multiply:     code_.emit(OpCode::MUL); break;
        case TokenType::Divide:       code_.emit(OpCode::DIV); break;
        case TokenType::Equal:        code_.emit(OpCode::EQ);  break;
        case TokenType::NotEqual:     code_.emit(OpCode::NE);  break;
        case TokenType::Less:         code_.emit(OpCode::LT);  break;
        case TokenType::LessEqual:    code_.emit(OpCode::LE);  break;
        case TokenType::Greater:      code_.emit(OpCode::GT);  break;
        case TokenType::GreaterEqual: code_.emit(OpCode::GE);  break;
        case TokenType::LogicalAnd:   code_.emit(OpCode::AND); break;
        case TokenType::LogicalOr:    code_.emit(OpCode::OR);  break;
        default:
            throw std::runtime_error("Unknown binary operator");
    }
}

void CodeGen::genUnaryOp(UnaryOpNode* expr) {
    // 取地址运算符 &
    if (expr->getOperator() == TokenType::Ampersand) {
        if (auto* var = dynamic_cast<VariableNode*>(expr->getOperand())) {
            int offset = getLocal(var->getName());
            code_.emit(OpCode::LEA, offset);  // 将变量地址压栈
            return;
        }
        throw std::runtime_error("Cannot take address of non-variable");
    }

    // 解引用运算符 * (读取)
    if (expr->getOperator() == TokenType::Multiply) {
        genExpression(expr->getOperand());  // 计算指针值（地址）
        code_.emit(OpCode::LOADM);          // 从该地址加载值
        return;
    }

    genExpression(expr->getOperand());

    switch (expr->getOperator()) {
        case TokenType::Plus:       break;  // 一元+不需要操作
        case TokenType::Minus:      code_.emit(OpCode::NEG); break;
        case TokenType::LogicalNot: code_.emit(OpCode::NOT); break;
        default:
            throw std::runtime_error("Unknown unary operator");
    }
}

void CodeGen::genFunctionCall(FunctionCallNode* expr) {
    // 新 ABI: caller 预留 return slot，调用后清理参数
    // 栈布局 (调用前):
    //   [ret_slot]   <- caller 预留，用于接收返回值
    //   [param_n]
    //   ...
    //   [param_1]
    // TODO: 支持 struct 返回值时，ret_slot 可能占多个 slot

    // 1. 预留 return slot (当前固定 1 slot)
    code_.emit(OpCode::PUSH, 0);

    // 2. 压入参数（从右到左）
    // TODO: 支持 struct 参数时，每个参数可能占多个 slot
    for (int i = expr->getArgs().size() - 1; i >= 0; --i) {
        genExpression(expr->getArgs()[i].get());
    }

    // 3. 查找函数地址并调用
    auto it = code_.functions.find(expr->getName());
    if (it == code_.functions.end()) {
        throw std::runtime_error("Unknown function: " + expr->getName());
    }
    code_.emit(OpCode::CALL, it->second);

    // 4. caller 清理参数，return slot 留在栈顶
    // TODO: 支持 struct 参数时，需计算参数总 slot 数
    if (!expr->getArgs().empty()) {
        code_.emit(OpCode::ADJSP, expr->getArgs().size());
    }
}

int CodeGen::allocLocal(const std::string& name) {
    int offset = local_offset_++;
    locals_[name] = offset;
    return offset;
}

int CodeGen::getLocal(const std::string& name) {
    auto it = locals_.find(name);
    if (it == locals_.end()) {
        throw std::runtime_error("Unknown variable: " + name);
    }
    return it->second;
}

void CodeGen::genArrayAccess(ArrayAccessNode* expr) {
    // 获取数组表达式的类型来确定元素大小
    auto array_type = expr->getArray()->getResolvedType();
    int elem_size = 1;
    if (array_type && array_type->isArray()) {
        auto* arr = static_cast<ArrayType*>(array_type.get());
        elem_size = arr->getElementType()->getSlotCount();
    }

    genExpression(expr->getIndex());  // 计算下标

    // 生成数组基地址
    if (auto* var = dynamic_cast<VariableNode*>(expr->getArray())) {
        code_.emit(OpCode::LEA, getLocal(var->getName()));
    } else if (auto* inner = dynamic_cast<ArrayAccessNode*>(expr->getArray())) {
        // 嵌套数组访问，递归生成地址（不加载值）
        genArrayAccessAddr(inner);
    }

    code_.emit(OpCode::ADDPTRD, elem_size);  // base + index * elem_size
    code_.emit(OpCode::LOADM);               // 加载值
}

// 生成数组访问的地址（不加载值），用于多维数组
void CodeGen::genArrayAccessAddr(ArrayAccessNode* expr) {
    auto array_type = expr->getArray()->getResolvedType();
    int elem_size = 1;
    if (array_type && array_type->isArray()) {
        auto* arr = static_cast<ArrayType*>(array_type.get());
        elem_size = arr->getElementType()->getSlotCount();
    }

    genExpression(expr->getIndex());

    if (auto* var = dynamic_cast<VariableNode*>(expr->getArray())) {
        code_.emit(OpCode::LEA, getLocal(var->getName()));
    } else if (auto* inner = dynamic_cast<ArrayAccessNode*>(expr->getArray())) {
        genArrayAccessAddr(inner);
    }

    code_.emit(OpCode::ADDPTRD, elem_size);
}

int CodeGen::allocArray(const std::string& name, int size) {
    int offset = local_offset_;
    locals_[name] = offset;
    array_sizes_[name] = size;
    local_offset_ += size;  // 预留 size 个位置
    return offset;
}

bool CodeGen::isArray(const std::string& name) {
    return array_sizes_.find(name) != array_sizes_.end();
}
