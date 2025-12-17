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

    // 为参数分配空间（参数在调用前已压栈，位于负偏移）
    const auto& params = func->getParams();
    for (int i = params.size() - 1; i >= 0; --i) {
        // 参数位于 fp 之前：fp-2 是返回地址，fp-3 是第一个参数...
        locals_[params[i].name] = -(int)(params.size() - i) - 2;
    }

    // 生成函数体
    genCompoundStmt(func->getBody());

    // 如果函数没有显式 return，添加默认返回
    if (code_.code.empty() || code_.code.back().op != OpCode::RET) {
        code_.emit(OpCode::RET);
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
    if (stmt->isArray()) {
        // 数组声明：分配连续空间并初始化为0
        int size = stmt->getArraySize();
        int offset = allocArray(stmt->getName(), size);
        for (int i = 0; i < size; i++) {
            code_.emit(OpCode::PUSH, 0);
        }
        (void)offset;
    } else {
        // 普通变量声明
        int offset = allocLocal(stmt->getName());

        if (stmt->hasInitializer()) {
            genExpression(stmt->getInitializer());
            // 值已在栈顶，就是变量的存储位置
        } else {
            // 默认初始化为 0
            code_.emit(OpCode::PUSH, 0);
        }
        // 变量值保留在栈上，offset 指向它
        (void)offset;  // offset 用于后续 LOAD/STORE
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
        code_.emit(OpCode::RETV);
    } else {
        code_.emit(OpCode::RET);
    }
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
        // 检查是否是数组赋值
        if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr->getLeft())) {
            // arr[index] = value
            int base = getLocal(arr->getArrayName());
            genExpression(arr->getIndex());   // 计算下标，压入栈
            genExpression(expr->getRight());  // 计算值，压入栈
            code_.emit(OpCode::STOREI, base); // 存储：弹出value和index
            // 赋值表达式应该返回值，重新加载
            genExpression(arr->getIndex());
            code_.emit(OpCode::LOADI, base);
            return;
        }

        // 检查是否是解引用赋值 *p = value
        if (auto* unary = dynamic_cast<UnaryOpNode*>(expr->getLeft())) {
            if (unary->getOperator() == TokenType::Multiply) {
                genExpression(unary->getOperand());  // 计算指针值（地址）
                genExpression(expr->getRight());     // 计算要存储的值
                code_.emit(OpCode::STOREP);          // 存储到地址
                // 赋值表达式返回值，重新加载
                genExpression(unary->getOperand());
                code_.emit(OpCode::LOADP);
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
        code_.emit(OpCode::LOADP);          // 从该地址加载值
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
    // 压入参数（从左到右）
    for (const auto& arg : expr->getArgs()) {
        genExpression(arg.get());
    }

    // 查找函数地址
    auto it = code_.functions.find(expr->getName());
    if (it == code_.functions.end()) {
        throw std::runtime_error("Unknown function: " + expr->getName());
    }

    code_.emit(OpCode::CALL, it->second);

    // 调用后清理参数，保留返回值
    if (!expr->getArgs().empty()) {
        code_.emit(OpCode::POPN, expr->getArgs().size());
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
    int base = getLocal(expr->getArrayName());
    genExpression(expr->getIndex());  // 计算下标，压入栈
    code_.emit(OpCode::LOADI, base);  // 间接加载
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
