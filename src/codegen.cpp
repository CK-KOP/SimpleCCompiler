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
    const auto& params = func->getParams();
    current_param_slots_ = 0;
    for (const auto& param : params) {
        auto param_type = param.getResolvedType();
        if (!param_type) {
            throw std::runtime_error("Parameter type not resolved: " + param.name);
        }
        current_param_slots_ += param_type->getSlotCount();
    }

    // 为参数分配空间（参数在调用前已压栈，位于负偏移）
    // 栈帧布局:
    //   [ret_slot]   fp - 3 - param_slots (由 caller 预留)
    //   [param_n]    fp - 3 - (n-1)
    //   ...
    //   [param_1]    fp - 3
    //   [ret_addr]   fp - 2
    //   [old_fp]     fp - 1
    //   fp ->
    int param_offset = -3;  // 第一个参数从 fp-3 开始
    for (size_t i = 0; i < params.size(); ++i) {
        auto param_type = params[i].getResolvedType();
        int slot_count = param_type ? param_type->getSlotCount() : 1;

        // 对于多slot的参数（如结构体），locals应该指向最低地址（最后一个slot）
        locals_[params[i].name] = param_offset - slot_count + 1;
        param_offset -= slot_count;
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

    // 使用 getSlotCount() 获取总slot数（支持多维数组、指针数组和结构体）
    int slot_count = type->getSlotCount();

    if (type->isArray()) {
        int offset = allocArray(stmt->getName(), slot_count);
        for (int i = 0; i < slot_count; i++) {
            code_.emit(OpCode::PUSH, 0);
        }
        (void)offset;
    } else if (type->isStruct()) {
        // 结构体变量：分配多个slot
        if (stmt->hasInitializer()) {
            // 有初始化器（通常是函数调用返回结构体）
            // 先执行初始化器，将结构体压栈
            genExpression(stmt->getInitializer());
            // 初始化器会将结构体的所有 slot 压栈（ret_slot）
            // 然后分配变量，这些 slot 就是变量的存储空间
            int offset = allocStruct(stmt->getName(), slot_count);
            (void)offset;
        } else {
            // 无初始化器，先分配空间，再初始化为0
            int offset = allocStruct(stmt->getName(), slot_count);
            for (int i = 0; i < slot_count; i++) {
                code_.emit(OpCode::PUSH, 0);
            }
            (void)offset;
        }
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
        auto expr = stmt->getExpression();
        auto expr_type = expr->getResolvedType();

        if (expr_type && expr_type->isStruct()) {
            // 结构体返回值：需要将结构体复制到 ret_slot
            int slot_count = expr_type->getSlotCount();

            // 计算 ret_slot 的位置：fp - 3 - param_slots - (slot_count - 1)
            // ret_slot 占据多个 slot，从 fp - 3 - param_slots - (slot_count - 1) 开始
            int ret_slot_base = -3 - current_param_slots_ - (slot_count - 1);

            // 执行表达式，将结构体的所有 slot 压栈
            genExpression(expr);

            // 现在栈顶有 slot_count 个值，需要将它们存储到 ret_slot
            // 从栈顶弹出并存储到 ret_slot（从高地址到低地址）
            for (int i = slot_count - 1; i >= 0; --i) {
                code_.emit(OpCode::STORE, ret_slot_base + i);
            }
        } else {
            // 普通返回值（int、指针等）
            genExpression(expr);
        }
    } else {
        code_.emit(OpCode::PUSH, 0);  // void 函数默认返回 0
    }

    // ret_slot_offset = -3 - param_slots
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
        auto var_type = var->getResolvedType();
        if (var_type && var_type->isStruct()) {
            // 结构体变量：需要加载所有 slot
            int slot_count = var_type->getSlotCount();
            for (int i = 0; i < slot_count; ++i) {
                code_.emit(OpCode::LOAD, offset + i);
            }
        } else {
            // 普通变量：加载 1 个 slot
            code_.emit(OpCode::LOAD, offset);
        }
    } else if (auto* binary = dynamic_cast<BinaryOpNode*>(expr)) {
        genBinaryOp(binary);
    } else if (auto* unary = dynamic_cast<UnaryOpNode*>(expr)) {
        genUnaryOp(unary);
    } else if (auto* call = dynamic_cast<FunctionCallNode*>(expr)) {
        genFunctionCall(call);
    } else if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr)) {
        genArrayAccess(arr);
    } else if (auto* member = dynamic_cast<MemberAccessNode*>(expr)) {
        genMemberAccess(member);
    }
}

void CodeGen::genBinaryOp(BinaryOpNode* expr) {
    // 赋值运算符特殊处理
    if (expr->getOperator() == TokenType::Assign) {
        // 检查是否是数组赋值（支持多维）
        if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr->getLeft())) {
            // arr[index] = value
            genExpression(expr->getRight());  // 计算值
            genArrayAccessAddr(arr);          // 计算地址
            code_.emit(OpCode::STOREM);       // 存储

            // 赋值表达式应该返回值，重新加载
            genArrayAccessAddr(arr);          // 重新计算地址
            code_.emit(OpCode::LOADM);        // 加载返回值
            return;
        }

        // 检查是否是成员访问赋值 obj.member = value
        if (auto* member = dynamic_cast<MemberAccessNode*>(expr->getLeft())) {
            auto member_type = member->getResolvedType();

            if (member_type && member_type->isStruct()) {
                // 结构体成员赋值：需要复制多个 slot
                int slot_count = member_type->getSlotCount();

                // 计算右值（可能是函数调用返回结构体）
                genExpression(expr->getRight());
                // 现在栈顶有 slot_count 个值：[val_0, val_1, ..., val_n-1]

                // 计算目标成员的地址
                genMemberAccessAddr(member);
                // 栈顶现在是：[val_0, val_1, ..., val_n-1, addr]

                // 我们需要将 addr 移到最底下，然后逐个 STOREM
                // 但是这很复杂，让我们用另一种方法：
                // 从栈顶弹出 addr，然后逐个弹出值并存储

                // 方案：使用 MEMCPY 的逆向操作
                // 栈布局：[val_0, val_1, ..., val_n-1, addr]
                // 我们需要：逐个存储 val_i 到 addr+i

                // 先将所有值和地址重新排列
                // 实际上，我们可以直接从高地址到低地址存储

                // 弹出地址，保存到栈底
                // 然后从栈顶弹出值，通过地址存储

                // 简化方案：直接计算每个成员的地址并存储
                for (int i = slot_count - 1; i >= 0; --i) {
                    // 栈顶是 addr（如果 i == slot_count-1）或者是 val_i+1
                    if (i == slot_count - 1) {
                        // 第一次：栈顶是 addr
                        code_.emit(OpCode::ADDPTR, i);  // addr + i
                        code_.emit(OpCode::STOREM);     // 存储 val_i
                    } else {
                        // 后续：需要重新计算地址
                        genMemberAccessAddr(member);
                        code_.emit(OpCode::ADDPTR, i);
                        code_.emit(OpCode::STOREM);
                    }
                }

                // 赋值表达式返回值：加载第一个 slot
                genMemberAccessAddr(member);
                code_.emit(OpCode::LOADM);
                return;
            } else {
                // 普通成员赋值（int、指针等）
                genExpression(expr->getRight());  // 计算值
                genMemberAccessAddr(member);      // 计算成员地址
                code_.emit(OpCode::STOREM);       // 存储

                // 赋值表达式返回值，重新加载
                genMemberAccessAddr(member);
                code_.emit(OpCode::LOADM);
                return;
            }
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

        // 检查是否是结构体赋值
        auto left_type = expr->getLeft()->getResolvedType();
        if (left_type->isStruct()) {
            // 结构体整体赋值：使用 MEMCPY
            int slot_count = left_type->getSlotCount();

            // 计算源地址（右边）
            // 右边可以是变量或函数调用
            if (auto* right_var = dynamic_cast<VariableNode*>(expr->getRight())) {
                // 右边是变量：直接获取地址
                int src_offset = getLocal(right_var->getName());
                code_.emit(OpCode::LEA, src_offset);  // 源地址
            } else if (auto* right_call = dynamic_cast<FunctionCallNode*>(expr->getRight())) {
                // 右边是函数调用：函数返回值会在栈上（ret_slot位置）
                // 先调用函数，ret_slot（多个slot）会被压栈
                genFunctionCall(right_call);
                // 现在栈顶有 slot_count 个值（ret_slot）
                // 我们需要将它们复制到目标变量
                // 计算目标地址
                int dst_offset = getLocal(var->getName());
                code_.emit(OpCode::LEA, dst_offset);  // 目标地址压栈

                // 现在栈布局：[ret_slot_0, ret_slot_1, ..., dst_addr]
                // 我们需要：[dst_addr, src_addr]，然后 MEMCPY
                // 但是 ret_slot 已经在栈上了，我们需要获取它的地址

                // 方案：逐个 STORE 到目标位置
                // 先弹出目标地址
                code_.emit(OpCode::POP);  // 弹出 dst_addr（暂时不用）

                // 从栈顶弹出 slot_count 个值，存储到目标变量
                for (int i = slot_count - 1; i >= 0; --i) {
                    code_.emit(OpCode::STORE, dst_offset + i);
                }

                // 赋值表达式返回值：加载第一个 slot
                code_.emit(OpCode::LOAD, dst_offset);
                return;
            } else {
                throw std::runtime_error("结构体赋值的右边必须是变量或函数调用");
            }

            // 计算目标地址（左边）
            int dst_offset = getLocal(var->getName());
            code_.emit(OpCode::LEA, dst_offset);  // 目标地址

            // 执行内存复制
            code_.emit(OpCode::MEMCPY, slot_count);

            // 赋值表达式返回值：加载第一个 slot（简化处理）
            code_.emit(OpCode::LOAD, dst_offset);
            return;
        }

        // 普通变量赋值（int、指针等）
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
        } else if (auto* member = dynamic_cast<MemberAccessNode*>(expr->getOperand())) {
            // 取结构体成员的地址：&obj.member
            genMemberAccessAddr(member);
            return;
        } else if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr->getOperand())) {
            // 取数组元素的地址：&arr[i]
            genArrayAccessAddr(arr);
            return;
        }
        throw std::runtime_error("Cannot take address of non-lvalue");
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
    //   [ret_slot]   <- caller 预留，用于接收返回值（可能多个slot）
    //   [param_n]
    //   ...
    //   [param_1]

    // 1. 预留 return slot（根据返回类型的 slot 数）
    auto return_type = expr->getResolvedType();
    int ret_slot_count = return_type ? return_type->getSlotCount() : 1;
    for (int i = 0; i < ret_slot_count; ++i) {
        code_.emit(OpCode::PUSH, 0);
    }

    // 2. 压入参数（从右到左）
    // 对于结构体参数，需要压入多个 slot
    int total_param_slots = 0;
    for (int i = expr->getArgs().size() - 1; i >= 0; --i) {
        auto arg = expr->getArgs()[i].get();
        auto arg_type = arg->getResolvedType();

        if (arg_type && arg_type->isStruct()) {
            // 结构体参数：需要压入多个 slot
            int slot_count = arg_type->getSlotCount();
            total_param_slots += slot_count;

            // 获取结构体变量的地址，然后逐个 slot 压栈
            if (auto* var = dynamic_cast<VariableNode*>(arg)) {
                int offset = getLocal(var->getName());
                // 从低地址到高地址压入（保持内存布局）
                for (int j = 0; j < slot_count; ++j) {
                    code_.emit(OpCode::LOAD, offset + j);
                }
            } else if (auto* member = dynamic_cast<MemberAccessNode*>(arg)) {
                // 结构体成员访问：先获取地址，然后逐个加载
                genMemberAccessAddr(member);
                // 地址在栈顶，需要逐个加载 slot
                for (int j = 0; j < slot_count; ++j) {
                    if (j > 0) {
                        // 重新计算地址（因为 LOADM 会消耗栈顶）
                        genMemberAccessAddr(member);
                        code_.emit(OpCode::ADDPTR, j);
                    }
                    code_.emit(OpCode::LOADM);
                }
            } else {
                throw std::runtime_error("Unsupported struct argument type");
            }
        } else {
            // 普通参数（int、指针等）：压入 1 个 slot
            genExpression(arg);
            total_param_slots += 1;
        }
    }

    // 3. 查找函数地址并调用
    auto it = code_.functions.find(expr->getName());
    if (it == code_.functions.end()) {
        throw std::runtime_error("Unknown function: " + expr->getName());
    }
    code_.emit(OpCode::CALL, it->second);

    // 4. caller 清理参数，return slot 留在栈顶
    if (total_param_slots > 0) {
        code_.emit(OpCode::ADJSP, total_param_slots);
    }

    // 5. 函数调用结束后，ret_slot（可能多个slot）留在栈顶
    // 对于 int 返回值：栈顶是 1 个 slot
    // 对于 struct 返回值：栈顶是多个 slot，就像一个"临时结构体变量"
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
    genArrayAccessAddr(expr);
    code_.emit(OpCode::LOADM);
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
    } else if (auto* member = dynamic_cast<MemberAccessNode*>(expr->getArray())) {
        // 成员访问返回的数组：c.arr[0]
        genMemberAccessAddr(member);
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

int CodeGen::allocStruct(const std::string& name, int slot_count) {
    int offset = local_offset_;
    locals_[name] = offset;
    local_offset_ += slot_count;  // 预留 slot_count 个位置
    return offset;
}

bool CodeGen::isArray(const std::string& name) {
    return array_sizes_.find(name) != array_sizes_.end();
}

// 生成成员访问表达式（加载值）
void CodeGen::genMemberAccess(MemberAccessNode* expr) {
    genMemberAccessAddr(expr);
    code_.emit(OpCode::LOADM);
}

// 生成成员访问地址
void CodeGen::genMemberAccessAddr(MemberAccessNode* expr) {
    // 获取对象类型
    auto object_type = expr->getObject()->getResolvedType();
    if (!object_type || !object_type->isStruct()) {
        throw std::runtime_error("Member access on non-struct type");
    }

    auto struct_type = std::dynamic_pointer_cast<StructType>(object_type);
    int member_offset = struct_type->getMemberOffset(expr->getMember());

    // 计算对象基地址 + 成员偏移
    if (auto* var = dynamic_cast<VariableNode*>(expr->getObject())) {
        // 简单情况：obj.member
        int base_offset = getLocal(var->getName());
        code_.emit(OpCode::LEA, base_offset + member_offset);
    } else if (auto* inner_member = dynamic_cast<MemberAccessNode*>(expr->getObject())) {
        // 链式成员访问：obj.inner.member
        genMemberAccessAddr(inner_member);
        code_.emit(OpCode::ADDPTR, member_offset);
    } else if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr->getObject())) {
        // 数组元素的成员访问：arr[i].member
        genArrayAccessAddr(arr);
        code_.emit(OpCode::ADDPTR, member_offset);
    } else if (auto* deref = dynamic_cast<UnaryOpNode*>(expr->getObject())) {
        // 解引用的成员访问：(*ptr).member 或 ptr->member
        if (deref->getOperator() == TokenType::Multiply) {
            genExpression(deref->getOperand());  // 计算指针值（地址）
            code_.emit(OpCode::ADDPTR, member_offset);
        } else {
            throw std::runtime_error("Unsupported unary operator in member access");
        }
    } else {
        throw std::runtime_error("Unsupported member access pattern");
    }
}
