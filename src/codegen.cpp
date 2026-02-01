#include "../include/codegen.h"
#include <stdexcept>

// ========== 类型判断辅助函数实现 ==========

bool CodeGen::isStructType(ExprNode* node) const {
    if (!node) return false;
    auto type = node->getResolvedType();
    return type && type->isStruct();
}

bool CodeGen::isArrayType(ExprNode* node) const {
    if (!node) return false;
    auto type = node->getResolvedType();
    return type && type->isArray();
}

bool CodeGen::isPointerType(ExprNode* node) const {
    if (!node) return false;
    auto type = node->getResolvedType();
    return type && type->isPointer();
}

bool CodeGen::isIntType(ExprNode* node) const {
    if (!node) return false;
    auto type = node->getResolvedType();
    return type && type->isInt();
}

int CodeGen::getSlotCount(ExprNode* node) const {
    if (!node) return 1;
    auto type = node->getResolvedType();
    return type ? type->getSlotCount() : 1;
}

int CodeGen::getSlotCount(std::shared_ptr<Type> type) const {
    return type ? type->getSlotCount() : 1;
}

bool CodeGen::hasValidType(ExprNode* node) const {
    return node && node->getResolvedType() != nullptr;
}

std::shared_ptr<Type> CodeGen::getType(ExprNode* node) const {
    return node ? node->getResolvedType() : nullptr;
}

// ==========================================

ByteCode CodeGen::generate(ProgramNode* program) {
    // ========== Phase 6: 处理全局变量 ==========
    // 1. 为所有全局变量分配空间
    for (const auto& global_var : program->getGlobalVars()) {
        auto type = global_var->getResolvedType();
        if (!type) {
            throw std::runtime_error("Global variable type not resolved: " + global_var->getName());
        }
        allocateGlobalVariable(global_var->getName(), type);
    }

    // 2. 收集全局变量初始化信息（包括未初始化的）
    for (const auto& global_var : program->getGlobalVars()) {
        auto type = global_var->getResolvedType();
        auto* info = findVariable(global_var->getName());
        if (!info || !info->is_global) {
            throw std::runtime_error("Global variable not allocated: " + global_var->getName());
        }

        GlobalVarInit init;
        init.offset = info->offset;
        init.slot_count = info->slot_count;

        if (global_var->hasInitializer()) {
            auto* initializer = global_var->getInitializer();

            // 检查是否是初始化列表
            if (auto* init_list = dynamic_cast<InitializerListNode*>(initializer)) {
                // 初始化列表：逐个求值元素
                try {
                    for (const auto& elem : init_list->getElements()) {
                        int32_t value = evaluateConstExpr(elem.get());
                        init.init_data.push_back(value);
                    }
                } catch (const std::runtime_error& e) {
                    throw std::runtime_error("全局变量 '" + global_var->getName() + "' 初始化失败: " + e.what());
                }
            } else {
                // 单个表达式初始化
                try {
                    int32_t value = evaluateConstExpr(initializer);
                    init.init_data.push_back(value);
                } catch (const std::runtime_error& e) {
                    throw std::runtime_error("全局变量 '" + global_var->getName() + "' 初始化失败: " + e.what());
                }
            }
        }
        // 如果没有初始化器，init_data 为空，VM 会自动初始化为 0

        code_.global_inits.push_back(init);
    }

    // 3. 生成函数代码（正常流程）
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

    // ========== 重置新的变量管理系统 ==========
    variables_.clear();
    next_local_offset_ = 0;
    next_param_offset_ = -3;

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

    // ========== 使用新的变量管理系统记录参数 ==========
    int param_offset = -3;  // 第一个参数从 fp-3 开始
    for (size_t i = 0; i < params.size(); ++i) {
        auto param_type = params[i].getResolvedType();
        int slot_count = param_type ? param_type->getSlotCount() : 1;

        // 对于多slot的参数（如结构体），应该指向最低地址（最后一个slot）
        int offset = param_offset - slot_count + 1;

        // 记录到新系统
        variables_[params[i].name] = VariableInfo(offset, slot_count, false, true);

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
    // ========== 使用新的变量管理系统管理作用域 ==========
    // 保存当前作用域状态
    int saved_offset = next_local_offset_;
    auto saved_variables = variables_;

    for (const auto& s : stmt->getStatements()) {
        genStatement(s.get());
    }

    // 恢复作用域状态（回收局部变量空间）
    int vars_to_pop = next_local_offset_ - saved_offset;
    if (vars_to_pop > 0) {
        code_.emit(OpCode::ADJSP, vars_to_pop);
    }
    next_local_offset_ = saved_offset;
    variables_ = saved_variables;
}

void CodeGen::genVarDecl(VarDeclStmtNode* stmt) {
    auto type = stmt->getResolvedType();
    if (!type) {
        throw std::runtime_error("Variable type not resolved: " + stmt->getName());
    }

    // ========== 使用统一的变量分配接口 ==========
    // 自动计算 slot 数并分配空间
    int offset = allocateVariable(stmt->getName(), type);
    int slot_count = type->getSlotCount();

    // 初始化变量
    if (stmt->hasInitializer()) {
        auto* initializer = stmt->getInitializer();

        // 检查是否是初始化列表
        if (auto* init_list = dynamic_cast<InitializerListNode*>(initializer)) {
            // 初始化列表：逐个求值并压栈
            const auto& elements = init_list->getElements();

            // 1. 先求值所有元素并压栈
            for (const auto& elem : elements) {
                genExpression(elem.get());
            }

            // 2. 如果元素数量少于 slot_count，补0
            for (size_t i = elements.size(); i < static_cast<size_t>(slot_count); i++) {
                code_.emit(OpCode::PUSH, 0);
            }
        } else {
            // 单个表达式初始化
            genExpression(initializer);
            // 初始化器会将所有 slot 压栈，这些 slot 就是变量的存储空间
        }
    } else {
        // 无初始化器：初始化为 0
        for (int i = 0; i < slot_count; i++) {
            code_.emit(OpCode::PUSH, 0);
        }
    }

    (void)offset;  // 暂时未使用，后续会用到
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

        // ========== 使用类型判断辅助函数 ==========
        if (isStructType(expr)) {
            // 结构体返回值：需要将结构体复制到 ret_slot
            int slot_count = getSlotCount(expr);

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
        // ========== Phase 6: 支持全局变量访问 ==========
        auto* info = findVariable(var->getName());
        if (!info) {
            throw std::runtime_error("Unknown variable: " + var->getName());
        }

        if (info->is_global) {
            // 全局变量：使用 LOADG
            if (isStructType(var)) {
                // 全局结构体：需要加载所有 slot
                int slot_count = getSlotCount(var);
                for (int i = 0; i < slot_count; ++i) {
                    code_.emit(OpCode::LOADG, info->offset + i);
                }
            } else {
                // 普通全局变量：加载 1 个 slot
                code_.emit(OpCode::LOADG, info->offset);
            }
        } else {
            // 局部变量：使用 LOAD
            if (isStructType(var)) {
                int slot_count = getSlotCount(var);
                for (int i = 0; i < slot_count; ++i) {
                    code_.emit(OpCode::LOAD, info->offset + i);
                }
            } else {
                code_.emit(OpCode::LOAD, info->offset);
            }
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
            // ========== 使用类型判断辅助函数 ==========
            if (isStructType(member)) {
                // 结构体成员赋值：需要复制多个 slot
                int slot_count = getSlotCount(member);

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

        // ========== 使用类型判断辅助函数 ==========
        // 检查是否是结构体赋值
        if (isStructType(expr->getLeft())) {
            // 结构体整体赋值：使用 MEMCPY
            int slot_count = getSlotCount(expr->getLeft());

            // 计算源地址（右边）
            // 右边可以是变量或函数调用
            if (auto* right_var = dynamic_cast<VariableNode*>(expr->getRight())) {
                // 右边是变量：直接获取地址
                auto* src_info = findVariable(right_var->getName());
                if (!src_info) {
                    throw std::runtime_error("Unknown variable: " + right_var->getName());
                }
                if (src_info->is_global) {
                    code_.emit(OpCode::LEAG, src_info->offset);  // 源地址（全局）
                } else {
                    code_.emit(OpCode::LEA, src_info->offset);  // 源地址（局部）
                }
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
            auto* dst_info = findVariable(var->getName());
            if (!dst_info) {
                throw std::runtime_error("Unknown variable: " + var->getName());
            }

            // 先把目标地址压栈（用于 MEMCPY）
            if (dst_info->is_global) {
                code_.emit(OpCode::LEAG, dst_info->offset);  // 目标地址（全局）
            } else {
                code_.emit(OpCode::LEA, dst_info->offset);  // 目标地址（局部）
            }

            // 执行内存复制
            code_.emit(OpCode::MEMCPY, slot_count);

            // 赋值表达式返回值：加载第一个 slot（简化处理）
            if (dst_info->is_global) {
                code_.emit(OpCode::LOADG, dst_info->offset);
            } else {
                code_.emit(OpCode::LOAD, dst_info->offset);
            }
            return;
        }

        // 普通变量赋值（int、指针等）
        genExpression(expr->getRight());

        // ========== Phase 6: 支持全局变量赋值 ==========
        auto* info = findVariable(var->getName());
        if (!info) {
            throw std::runtime_error("Unknown variable: " + var->getName());
        }

        if (info->is_global) {
            // 全局变量：使用 STOREG
            code_.emit(OpCode::STOREG, info->offset);
            code_.emit(OpCode::LOADG, info->offset);  // 赋值表达式返回值
        } else {
            // 局部变量：使用 STORE
            code_.emit(OpCode::STORE, info->offset);
            code_.emit(OpCode::LOAD, info->offset);  // 赋值表达式返回值
        }
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
            // ========== Phase 6: 支持全局变量取地址 ==========
            auto* info = findVariable(var->getName());
            if (!info) {
                throw std::runtime_error("Unknown variable: " + var->getName());
            }

            if (info->is_global) {
                // 全局变量：使用 LEAG
                code_.emit(OpCode::LEAG, info->offset);
            } else {
                // 局部变量：使用 LEA
                code_.emit(OpCode::LEA, info->offset);
            }
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

        // ========== 使用类型判断辅助函数 ==========
        if (isStructType(arg)) {
            // 结构体参数：需要压入多个 slot
            int slot_count = getSlotCount(arg);
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

// ========== 新的统一变量管理系统实现 ==========

int CodeGen::allocateVariable(const std::string& name, std::shared_ptr<Type> type) {
    if (!type) {
        throw std::runtime_error("Cannot allocate variable without type: " + name);
    }

    // 自动计算 slot 数
    int slot_count = type->getSlotCount();
    int offset = next_local_offset_;
    next_local_offset_ += slot_count;

    // 记录到新系统
    variables_[name] = VariableInfo(offset, slot_count, false, false);

    return offset;
}

int CodeGen::allocateGlobalVariable(const std::string& name, std::shared_ptr<Type> type) {
    if (!type) {
        throw std::runtime_error("Cannot allocate global variable without type: " + name);
    }

    int slot_count = type->getSlotCount();
    int offset = next_global_offset_;
    next_global_offset_ += slot_count;

    // 记录到全局变量表（标记为全局变量）
    global_variables_[name] = VariableInfo(offset, slot_count, true, false);

    return offset;
}

const VariableInfo* CodeGen::findVariable(const std::string& name) const {
    // 先查局部变量（包括参数）
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return &it->second;
    }

    // 再查全局变量
    auto git = global_variables_.find(name);
    if (git != global_variables_.end()) {
        return &git->second;
    }

    return nullptr;
}

int CodeGen::getVariableOffset(const std::string& name) const {
    auto* info = findVariable(name);
    if (!info) {
        throw std::runtime_error("Unknown variable: " + name);
    }
    return info->offset;
}

int CodeGen::getVariableSlotCount(const std::string& name) const {
    auto* info = findVariable(name);
    if (!info) {
        throw std::runtime_error("Unknown variable: " + name);
    }
    return info->slot_count;
}

bool CodeGen::isGlobalVariable(const std::string& name) const {
    auto* info = findVariable(name);
    return info && info->is_global;
}

bool CodeGen::isParameter(const std::string& name) const {
    auto* info = findVariable(name);
    return info && info->is_parameter;
}

int CodeGen::getLocal(const std::string& name) {
    // ========== 只使用新的变量管理系统 ==========
    auto* info = findVariable(name);
    if (info) {
        return info->offset;
    }

    throw std::runtime_error("Unknown variable: " + name);
}

void CodeGen::genArrayAccess(ArrayAccessNode* expr) {
    genArrayAccessAddr(expr);
    code_.emit(OpCode::LOADM);
}

// 生成数组访问的地址（不加载值），用于多维数组
void CodeGen::genArrayAccessAddr(ArrayAccessNode* expr) {
    // ========== 使用类型判断辅助函数 ==========
    int elem_size = 1;
    if (isArrayType(expr->getArray())) {
        auto array_type = expr->getArray()->getResolvedType();
        auto* arr = static_cast<ArrayType*>(array_type.get());
        elem_size = arr->getElementType()->getSlotCount();
    }

    genExpression(expr->getIndex());

    if (auto* var = dynamic_cast<VariableNode*>(expr->getArray())) {
        // ========== Phase 6: 支持全局数组 ==========
        auto* info = findVariable(var->getName());
        if (!info) {
            throw std::runtime_error("Unknown variable: " + var->getName());
        }

        if (info->is_global) {
            // 全局数组：使用 LEAG
            code_.emit(OpCode::LEAG, info->offset);
        } else {
            // 局部数组：使用 LEA
            code_.emit(OpCode::LEA, info->offset);
        }
    } else if (auto* inner = dynamic_cast<ArrayAccessNode*>(expr->getArray())) {
        genArrayAccessAddr(inner);
    } else if (auto* member = dynamic_cast<MemberAccessNode*>(expr->getArray())) {
        // 成员访问返回的数组：c.arr[0]
        genMemberAccessAddr(member);
    }

    code_.emit(OpCode::ADDPTRD, elem_size);
}

// 生成成员访问表达式（加载值）
void CodeGen::genMemberAccess(MemberAccessNode* expr) {
    genMemberAccessAddr(expr);
    code_.emit(OpCode::LOADM);
}

// 生成成员访问地址
void CodeGen::genMemberAccessAddr(MemberAccessNode* expr) {
    // ========== 使用类型判断辅助函数 ==========
    if (!isStructType(expr->getObject())) {
        throw std::runtime_error("Member access on non-struct type");
    }

    auto object_type = expr->getObject()->getResolvedType();
    auto struct_type = std::dynamic_pointer_cast<StructType>(object_type);
    int member_offset = struct_type->getMemberOffset(expr->getMember());

    // 计算对象基地址 + 成员偏移
    if (auto* var = dynamic_cast<VariableNode*>(expr->getObject())) {
        // ========== Phase 6: 支持全局结构体成员 ==========
        auto* info = findVariable(var->getName());
        if (!info) {
            throw std::runtime_error("Unknown variable: " + var->getName());
        }

        if (info->is_global) {
            // 全局结构体：使用 LEAG
            code_.emit(OpCode::LEAG, info->offset + member_offset);
        } else {
            // 局部结构体：使用 LEA
            code_.emit(OpCode::LEA, info->offset + member_offset);
        }
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

// ========== 常量表达式求值 (Phase 6) ==========

int32_t CodeGen::evaluateConstExpr(ExprNode* expr) {
    // 1. 数字字面量
    if (auto* num = dynamic_cast<NumberNode*>(expr)) {
        return num->getValue();
    }

    // 2. 二元运算表达式
    if (auto* binop = dynamic_cast<BinaryOpNode*>(expr)) {
        int32_t left = evaluateConstExpr(binop->getLeft());
        int32_t right = evaluateConstExpr(binop->getRight());

        switch (binop->getOperator()) {
            case TokenType::Plus:     return left + right;
            case TokenType::Minus:    return left - right;
            case TokenType::Multiply: return left * right;
            case TokenType::Divide:
                if (right == 0) {
                    throw std::runtime_error("常量表达式中除以零");
                }
                return left / right;
            case TokenType::Equal:        return left == right ? 1 : 0;
            case TokenType::NotEqual:     return left != right ? 1 : 0;
            case TokenType::Less:         return left < right ? 1 : 0;
            case TokenType::LessEqual:    return left <= right ? 1 : 0;
            case TokenType::Greater:      return left > right ? 1 : 0;
            case TokenType::GreaterEqual: return left >= right ? 1 : 0;
            case TokenType::LogicalAnd:   return (left && right) ? 1 : 0;
            case TokenType::LogicalOr:    return (left || right) ? 1 : 0;
            default:
                throw std::runtime_error("常量表达式中不支持的二元运算符");
        }
    }

    // 3. 一元运算表达式
    if (auto* unary = dynamic_cast<UnaryOpNode*>(expr)) {
        switch (unary->getOperator()) {
            case TokenType::Minus: {
                // 负数: -expr
                int32_t value = evaluateConstExpr(unary->getOperand());
                return -value;
            }
            case TokenType::LogicalNot: {
                // 逻辑非: !expr
                int32_t value = evaluateConstExpr(unary->getOperand());
                return value == 0 ? 1 : 0;
            }
            case TokenType::Ampersand: {
                // 取地址: &global_var
                auto* var = dynamic_cast<VariableNode*>(unary->getOperand());
                if (!var) {
                    throw std::runtime_error("取地址运算符只能用于变量");
                }

                // 检查是否是全局变量
                if (!isGlobalVariable(var->getName())) {
                    throw std::runtime_error("全局变量初始化中只能取全局变量的地址: " + var->getName());
                }

                // 返回全局地址: GLOBAL_BASE + offset
                const VariableInfo* info = findVariable(var->getName());
                if (!info) {
                    throw std::runtime_error("未找到全局变量: " + var->getName());
                }
                return VM::GLOBAL_BASE + info->offset;
            }
            default:
                throw std::runtime_error("常量表达式中不支持的一元运算符");
        }
    }

    // 4. 变量引用（只允许全局变量，但这通常不是常量）
    if (auto* var = dynamic_cast<VariableNode*>(expr)) {
        throw std::runtime_error("全局变量初始化不能使用其他变量的值: " + var->getName());
    }

    // 5. 其他表达式类型不支持
    throw std::runtime_error("全局变量初始化必须是编译时常量表达式");
}
