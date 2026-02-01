#include "../include/sema.h"

std::shared_ptr<Type> Sema::stringToType(const std::string& type_name) {
    if (type_name == "int") return Type::getIntType();
    if (type_name == "void") return Type::getVoidType();

    // 处理指针类型: int*, int**, struct Point*, ...
    if (type_name.size() > 1 && type_name.back() == '*') {
        // 递归获取基类型
        auto base_type = stringToType(type_name.substr(0, type_name.size() - 1));
        if (base_type) {
            return std::make_shared<PointerType>(base_type);
        }
    }

    // 处理结构体类型: struct Point
    if (type_name.size() > 7 && type_name.substr(0, 7) == "struct ") {
        std::string struct_name = type_name.substr(7);
        auto it = struct_types_.find(struct_name);
        if (it != struct_types_.end()) {
            return it->second;
        }
        return nullptr;
    }

    return nullptr;
}

bool Sema::analyze(ProgramNode* program) {
    // 先分析所有结构体定义（结构体前向声明需要）
    for (const auto& struct_decl : program->getStructs()) {
        analyzeStructDecl(struct_decl.get());
    }

    // 按照源文件声明顺序分析全局变量和函数
    // 这样可以检测出"使用未声明的全局变量"的错误
    const auto& decl_order = program->getDeclarationOrder();
    size_t struct_idx = 0;
    size_t global_idx = 0;
    size_t func_idx = 0;

    for (int decl_type : decl_order) {
        if (decl_type == 1) {  // global_var
            if (global_idx < program->getGlobalVars().size()) {
                analyzeGlobalVarDecl(program->getGlobalVars()[global_idx].get());
                global_idx++;
            }
        } else if (decl_type == 2) {  // function
            if (func_idx < program->getFunctions().size()) {
                analyzeFunction(program->getFunctions()[func_idx].get());
                func_idx++;
            }
        }
        // struct (0) 已经在上面分析过了
    }

    return !hasErrors();
}

void Sema::analyzeFunction(FunctionDeclNode* func) {
    // 获取返回类型
    auto return_type = stringToType(func->getReturnType());
    if (!return_type) {
        error("未知的返回类型: " + func->getReturnType());
        return;
    }

    // 设置函数返回类型到AST
    func->setResolvedReturnType(return_type);

    // 构建函数类型
    std::vector<FunctionType::Param> params;
    for (auto& param : func->getParams()) {
        auto param_type = stringToType(param.type);
        if (!param_type) {
            error("未知的参数类型: " + param.type);
            return;
        }
        // 设置参数类型到AST
        param.setResolvedType(param_type);
        params.emplace_back(param_type, param.name);
    }
    auto func_type = std::make_shared<FunctionType>(return_type, params);

    // 检查函数是否重复定义
    if (scope_.findSymbolInCurrentScope(func->getName())) {
        error("函数重复定义: " + func->getName());
        return;
    }

    // 添加函数到符号表
    scope_.addSymbol(func->getName(), func_type);

    // 进入函数作用域
    scope_.enterScope();
    current_function_return_type_ = return_type;

    // 添加参数到作用域
    for (const auto& param : func->getParams()) {
        auto param_type = stringToType(param.type);
        if (!scope_.addSymbol(param.name, param_type)) {
            error("参数名重复: " + param.name);
        }
    }

    // 分析函数体
    analyzeCompoundStatement(func->getBody());

    // 退出函数作用域
    current_function_return_type_ = nullptr;
    scope_.exitScope();
}

void Sema::analyzeStatement(StmtNode* stmt) {
    if (auto* compound = dynamic_cast<CompoundStmtNode*>(stmt)) {
        scope_.enterScope();
        analyzeCompoundStatement(compound);
        scope_.exitScope();
    } else if (auto* var_decl = dynamic_cast<VarDeclStmtNode*>(stmt)) {
        analyzeVarDecl(var_decl);
    } else if (auto* ret_stmt = dynamic_cast<ReturnStmtNode*>(stmt)) {
        analyzeReturnStatement(ret_stmt);
    } else if (auto* if_stmt = dynamic_cast<IfStmtNode*>(stmt)) {
        analyzeIfStatement(if_stmt);
    } else if (auto* while_stmt = dynamic_cast<WhileStmtNode*>(stmt)) {
        analyzeWhileStatement(while_stmt);
    } else if (auto* for_stmt = dynamic_cast<ForStmtNode*>(stmt)) {
        analyzeForStatement(for_stmt);
    } else if (auto* do_while = dynamic_cast<DoWhileStmtNode*>(stmt)) {
        analyzeDoWhileStatement(do_while);
    } else if (auto* expr_stmt = dynamic_cast<ExprStmtNode*>(stmt)) {
        analyzeExprStatement(expr_stmt);
    }
    // EmptyStmtNode, BreakStmtNode, ContinueStmtNode 不需要语义检查
}

void Sema::analyzeCompoundStatement(CompoundStmtNode* stmt) {
    for (const auto& s : stmt->getStatements()) {
        analyzeStatement(s.get());
    }
}

void Sema::analyzeVarDecl(VarDeclStmtNode* stmt) {
    auto base_type = stringToType(stmt->getType());
    if (!base_type) {
        error("未知的变量类型: " + stmt->getType());
        return;
    }

    if (base_type->isVoid()) {
        error("变量不能声明为 void 类型: " + stmt->getName());
        return;
    }

    // 检查是否重复声明
    if (scope_.findSymbolInCurrentScope(stmt->getName())) {
        error("变量重复声明: " + stmt->getName());
        return;
    }

    // 确定实际类型（普通变量或多维数组）
    std::shared_ptr<Type> var_type = base_type;
    const auto& dims = stmt->getArrayDims();
    if (!dims.empty()) {
        // 从最内层开始构建数组类型：int arr[3][4] -> ArrayType(ArrayType(int, 4), 3)
        for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
            if (*it <= 0) {
                error("数组大小必须为正整数: " + stmt->getName());
                return;
            }
            var_type = std::make_shared<ArrayType>(var_type, *it);
        }
    }

    // 填充 AST 节点的类型信息
    stmt->setResolvedType(var_type);

    // 添加到符号表
    scope_.addSymbol(stmt->getName(), var_type);

    // 分析初始化表达式
    if (stmt->hasInitializer()) {
        auto* initializer = stmt->getInitializer();

        // 检查是否是初始化列表
        if (auto* init_list = dynamic_cast<InitializerListNode*>(initializer)) {
            // 初始化列表：可以用于数组、结构体或标量类型
            if (auto* array_type = dynamic_cast<ArrayType*>(var_type.get())) {
                checkArrayInitializer(init_list, array_type, false);  // false = 局部变量
            } else if (auto* struct_type = dynamic_cast<StructType*>(var_type.get())) {
                checkStructInitializer(init_list, struct_type, false);
            } else {
                // 标量类型：只能有一个元素
                if (init_list->getElementCount() != 1) {
                    error("标量类型的初始化列表只能包含一个元素");
                    return;
                }
                // 检查元素类型
                auto elem_type = analyzeExpression(init_list->getElements()[0].get());
                if (elem_type->isVoid()) {
                    error("void 类型的值不能用于初始化变量");
                    return;
                }
                if (!isTypeCompatible(var_type, elem_type)) {
                    error("初始化类型不兼容：不能将 " + elem_type->toString() + " 类型赋值给 " + var_type->toString() + " 类型");
                    return;
                }
            }
        } else {
            // 单个表达式初始化
            auto init_type = analyzeExpression(initializer);
            if (init_type->isVoid()) {
                error("void 类型的值不能用于初始化变量");
            }
            // 检查初始化器类型是否与变量类型兼容
            if (!isTypeCompatible(var_type, init_type)) {
                error("初始化类型不兼容：不能将 " + init_type->toString() + " 类型赋值给 " + var_type->toString() + " 类型");
            }
        }
    }
}

void Sema::analyzeReturnStatement(ReturnStmtNode* stmt) {
    if (!current_function_return_type_) {
        error("return 语句不在函数内");
        return;
    }

    if (stmt->hasExpression()) {
        auto expr_type = analyzeExpression(stmt->getExpression());
        if (current_function_return_type_->isVoid()) {
            error("void 函数不应返回值");
        } else {
            // 检查返回值类型是否与函数返回类型兼容
            if (!isTypeCompatible(current_function_return_type_, expr_type)) {
                error("返回值类型不匹配：期望 " + current_function_return_type_->toString() +
                      "，实际 " + expr_type->toString());
            }
        }
    } else {
        if (!current_function_return_type_->isVoid()) {
            error("非 void 函数应返回值");
        }
    }
}

void Sema::analyzeIfStatement(IfStmtNode* stmt) {
    analyzeExpression(stmt->getCondition());
    analyzeStatement(stmt->getThenStmt());

    for (const auto& else_if : stmt->getElseIfs()) {
        analyzeExpression(else_if->condition.get());
        analyzeStatement(else_if->statement.get());
    }

    if (stmt->hasElseStmt()) {
        analyzeStatement(stmt->getElseStmt());
    }
}

void Sema::analyzeWhileStatement(WhileStmtNode* stmt) {
    analyzeExpression(stmt->getCondition());
    analyzeStatement(stmt->getBody());
}

void Sema::analyzeForStatement(ForStmtNode* stmt) {
    scope_.enterScope();

    if (stmt->hasInit()) {
        analyzeStatement(stmt->getInit());
    }
    if (stmt->hasCondition()) {
        analyzeExpression(stmt->getCondition());
    }
    if (stmt->hasIncrement()) {
        analyzeExpression(stmt->getIncrement());
    }
    analyzeStatement(stmt->getBody());

    scope_.exitScope();
}

void Sema::analyzeDoWhileStatement(DoWhileStmtNode* stmt) {
    analyzeStatement(stmt->getBody());
    analyzeExpression(stmt->getCondition());
}

void Sema::analyzeExprStatement(ExprStmtNode* stmt) {
    analyzeExpression(stmt->getExpression());
}

std::shared_ptr<Type> Sema::analyzeExpression(ExprNode* expr) {
    std::shared_ptr<Type> type;

    if (auto* num = dynamic_cast<NumberNode*>(expr)) {
        type = Type::getIntType();
    } else if (auto* var = dynamic_cast<VariableNode*>(expr)) {
        type = analyzeVariable(var);
    } else if (auto* binary = dynamic_cast<BinaryOpNode*>(expr)) {
        type = analyzeBinaryOp(binary);
    } else if (auto* unary = dynamic_cast<UnaryOpNode*>(expr)) {
        type = analyzeUnaryOp(unary);
    } else if (auto* call = dynamic_cast<FunctionCallNode*>(expr)) {
        type = analyzeFunctionCall(call);
    } else if (auto* arr = dynamic_cast<ArrayAccessNode*>(expr)) {
        type = analyzeArrayAccess(arr);
    } else if (auto* member = dynamic_cast<MemberAccessNode*>(expr)) {
        type = analyzeMemberAccess(member);
    } else {
        type = Type::getIntType();
    }

    expr->setResolvedType(type);
    return type;
}

std::shared_ptr<Type> Sema::analyzeVariable(VariableNode* expr) {
    // 先在局部作用域中查找
    auto symbol = scope_.findSymbol(expr->getName());
    if (symbol) {
        if (symbol->getType()->isFunction()) {
            // 函数名作为表达式使用（函数指针，暂不支持）
            return symbol->getType();
        }
        return symbol->getType();
    }

    // 如果局部作用域找不到，查找全局符号表
    auto global_it = global_symbols_.find(expr->getName());
    if (global_it != global_symbols_.end()) {
        return global_it->second;
    }

    error("未声明的变量: " + expr->getName());
    return Type::getIntType();
}

std::shared_ptr<Type> Sema::analyzeBinaryOp(BinaryOpNode* expr) {
    auto left_type = analyzeExpression(expr->getLeft());
    auto right_type = analyzeExpression(expr->getRight());

    // 赋值运算符：左边必须是变量、数组元素、成员访问或解引用表达式
    if (expr->getOperator() == TokenType::Assign) {
        bool is_lvalue = dynamic_cast<VariableNode*>(expr->getLeft()) ||
                         dynamic_cast<ArrayAccessNode*>(expr->getLeft()) ||
                         dynamic_cast<MemberAccessNode*>(expr->getLeft());
        // 检查是否是解引用表达式 *p
        if (auto* unary = dynamic_cast<UnaryOpNode*>(expr->getLeft())) {
            if (unary->getOperator() == TokenType::Multiply) {
                is_lvalue = true;
            }
        }
        if (!is_lvalue) {
            error("赋值运算符左边必须是变量、数组元素、成员访问或解引用表达式");
        }
        // 检查右边不能是 void
        if (right_type->isVoid()) {
            error("void 类型的值不能用于赋值");
        }
        // 检查类型兼容性
        if (!isTypeCompatible(left_type, right_type)) {
            error("赋值类型不兼容：不能将 " + right_type->toString() + " 类型赋值给 " + left_type->toString() + " 类型");
        }
    } else {
        // 其他二元运算：操作数不能是 void
        if (left_type->isVoid() || right_type->isVoid()) {
            error("void 类型的值不能用于表达式");
        }

        // 取模运算符要求操作数必须是整数类型
        if (expr->getOperator() == TokenType::Modulo) {
            if (!left_type->isInt() || !right_type->isInt()) {
                error("取模运算符 % 的操作数必须是整数类型");
            }
        }
    }

    // 简化：所有二元运算返回 int
    return Type::getIntType();
}

std::shared_ptr<Type> Sema::analyzeUnaryOp(UnaryOpNode* expr) {
    auto operand_type = analyzeExpression(expr->getOperand());

    // void 类型不能用于任何一元运算
    if (operand_type->isVoid()) {
        error("void 类型的值不能用于表达式");
    }

    // 取地址运算符 &
    if (expr->getOperator() == TokenType::Ampersand) {
        // 操作数必须是左值（变量或数组元素）
        if (!dynamic_cast<VariableNode*>(expr->getOperand()) &&
            !dynamic_cast<ArrayAccessNode*>(expr->getOperand())) {
            error("取地址运算符 & 的操作数必须是左值");
        }
        // 返回指针类型
        return std::make_shared<PointerType>(operand_type);
    }

    // 解引用运算符 *
    if (expr->getOperator() == TokenType::Multiply) {
        // 操作数必须是指针类型
        if (!operand_type->isPointer()) {
            error("解引用运算符 * 的操作数必须是指针类型");
            return Type::getIntType();
        }
        // 返回指针指向的类型
        auto ptr_type = std::dynamic_pointer_cast<PointerType>(operand_type);
        return ptr_type->getBaseType();
    }

    // 一元 + 和 - 运算符：操作数必须是整数类型
    if (expr->getOperator() == TokenType::Plus || expr->getOperator() == TokenType::Minus) {
        if (!operand_type->isInt()) {
            error("一元 +/- 运算符的操作数必须是整数类型");
        }
        return Type::getIntType();
    }

    return Type::getIntType();
}

std::shared_ptr<Type> Sema::analyzeFunctionCall(FunctionCallNode* expr) {
    auto symbol = scope_.findSymbol(expr->getName());
    if (!symbol) {
        error("未声明的函数: " + expr->getName());
        return Type::getIntType();
    }

    if (!symbol->getType()->isFunction()) {
        error("'" + expr->getName() + "' 不是函数");
        return Type::getIntType();
    }

    auto func_type = std::dynamic_pointer_cast<FunctionType>(symbol->getType());
    if (!func_type) {
        return Type::getIntType();
    }

    // 检查参数数量
    if (expr->getArgs().size() != func_type->getParams().size()) {
        error("函数 '" + expr->getName() + "' 参数数量不匹配: 期望 " +
              std::to_string(func_type->getParams().size()) + " 个，实际 " +
              std::to_string(expr->getArgs().size()) + " 个");
    }

    // 分析每个参数并检查类型兼容性
    for (size_t i = 0; i < expr->getArgs().size() && i < func_type->getParams().size(); ++i) {
        auto arg_type = analyzeExpression(expr->getArgs()[i].get());
        auto param_type = func_type->getParams()[i].type;

        // 检查参数类型是否兼容
        if (!isTypeCompatible(param_type, arg_type)) {
            error("函数 '" + expr->getName() + "' 第 " + std::to_string(i + 1) +
                  " 个参数类型不匹配：期望 " + param_type->toString() +
                  "，实际 " + arg_type->toString());
        }
    }

    return func_type->getReturnType();
}

std::shared_ptr<Type> Sema::analyzeArrayAccess(ArrayAccessNode* expr) {
    // 分析数组表达式（可能是变量或另一个数组访问）
    auto array_type = analyzeExpression(expr->getArray());

    // 分析下标表达式
    analyzeExpression(expr->getIndex());

    // 检查是否可以进行下标访问
    if (array_type->isArray()) {
        auto arr_type = std::dynamic_pointer_cast<ArrayType>(array_type);
        return arr_type->getElementType();
    } else if (array_type->isPointer()) {
        auto ptr_type = std::dynamic_pointer_cast<PointerType>(array_type);
        return ptr_type->getBaseType();
    } else {
        error("下标运算符只能用于数组或指针类型");
        return Type::getIntType();
    }
}

// 分析结构体定义
void Sema::analyzeStructDecl(StructDeclNode* struct_decl) {
    // 检查结构体是否重复定义
    if (struct_types_.find(struct_decl->getName()) != struct_types_.end()) {
        error("结构体重复定义: " + struct_decl->getName());
        return;
    }

    // 创建结构体类型
    auto struct_type = std::make_shared<StructType>(struct_decl->getName());

    // 分析每个成员
    for (const auto& member : struct_decl->getMembers()) {
        // 解析成员类型
        std::shared_ptr<Type> member_type;

        if (member.isArray()) {
            // 数组成员：从最内层开始构建类型
            member_type = stringToType(member.type);
            if (!member_type) {
                error("未知的成员类型: " + member.type);
                continue;
            }

            // 从右到左构建多维数组类型
            for (auto it = member.array_dims.rbegin(); it != member.array_dims.rend(); ++it) {
                member_type = std::make_shared<ArrayType>(member_type, *it);
            }
        } else {
            // 普通成员
            member_type = stringToType(member.type);
            if (!member_type) {
                error("未知的成员类型: " + member.type);
                continue;
            }
        }

        // 检查成员类型是否为void
        if (member_type->isVoid()) {
            error("结构体成员不能是void类型: " + member.name);
            continue;
        }

        // 添加成员到结构体类型
        struct_type->addMember(member.name, member_type);
    }

    // 注册结构体类型
    struct_types_[struct_decl->getName()] = struct_type;
}

// 分析成员访问表达式
std::shared_ptr<Type> Sema::analyzeMemberAccess(MemberAccessNode* expr) {
    // 分析对象表达式
    auto object_type = analyzeExpression(expr->getObject());

    // 检查对象是否是结构体类型
    if (!object_type->isStruct()) {
        error("成员访问运算符只能用于结构体类型");
        return Type::getIntType();
    }

    // 获取结构体类型
    auto struct_type = std::dynamic_pointer_cast<StructType>(object_type);

    // 查找成员类型
    auto member_type = struct_type->getMemberType(expr->getMember());
    if (!member_type) {
        error("结构体 " + struct_type->getName() + " 没有成员: " + expr->getMember());
        return Type::getIntType();
    }

    return member_type;
}

// 检查两个类型是否兼容（用于赋值）
bool Sema::isTypeCompatible(const std::shared_ptr<Type>& left, const std::shared_ptr<Type>& right) {
    // 如果类型完全相同
    if (left == right) {
        return true;
    }

    // int 类型兼容
    if (left->isInt() && right->isInt()) {
        return true;
    }

    // 指针类型检查
    if (left->isPointer() && right->isPointer()) {
        auto left_ptr = std::dynamic_pointer_cast<PointerType>(left);
        auto right_ptr = std::dynamic_pointer_cast<PointerType>(right);

        auto left_base = left_ptr->getBaseType();
        auto right_base = right_ptr->getBaseType();

        // 递归检查基类型
        if (left_base->isStruct() && right_base->isStruct()) {
            auto left_struct = std::dynamic_pointer_cast<StructType>(left_base);
            auto right_struct = std::dynamic_pointer_cast<StructType>(right_base);
            // 结构体指针必须指向相同的结构体类型
            return left_struct->getName() == right_struct->getName();
        }

        // 其他指针类型递归检查
        return isTypeCompatible(left_base, right_base);
    }

    // 数组类型检查
    if (left->isArray() && right->isArray()) {
        auto left_arr = std::dynamic_pointer_cast<ArrayType>(left);
        auto right_arr = std::dynamic_pointer_cast<ArrayType>(right);
        // 数组元素类型必须兼容
        return isTypeCompatible(left_arr->getElementType(), right_arr->getElementType());
    }

    // 结构体类型检查
    if (left->isStruct() && right->isStruct()) {
        auto left_struct = std::dynamic_pointer_cast<StructType>(left);
        auto right_struct = std::dynamic_pointer_cast<StructType>(right);
        // 结构体必须是相同的类型
        return left_struct->getName() == right_struct->getName();
    }

    // 其他情况不兼容
    return false;
}

// 分析全局变量声明
void Sema::analyzeGlobalVarDecl(VarDeclStmtNode* global_var) {
    // 检查是否重复定义
    if (global_symbols_.find(global_var->getName()) != global_symbols_.end()) {
        error("全局变量重复定义: " + global_var->getName());
        return;
    }

    // 解析变量类型
    std::shared_ptr<Type> var_type;
    if (global_var->isArray()) {
        // 数组类型
        auto element_type = stringToType(global_var->getType());
        if (!element_type) {
            error("未知的数组元素类型: " + global_var->getType());
            return;
        }

        // 计算数组总大小
        int total_size = 1;
        for (int dim : global_var->getArrayDims()) {
            total_size *= dim;
        }

        var_type = std::make_shared<ArrayType>(element_type, total_size);
    } else {
        // 普通类型
        var_type = stringToType(global_var->getType());
        if (!var_type) {
            error("未知的变量类型: " + global_var->getType());
            return;
        }
    }

    // 设置类型到AST
    global_var->setResolvedType(var_type);

    // 添加到全局符号表
    global_symbols_[global_var->getName()] = var_type;

    // 如果有初始化器，分析初始化表达式
    if (global_var->hasInitializer()) {
        auto* initializer = global_var->getInitializer();

        // 检查是否是初始化列表
        if (auto* init_list = dynamic_cast<InitializerListNode*>(initializer)) {
            // 初始化列表：可以用于数组、结构体或标量类型
            if (auto* array_type = dynamic_cast<ArrayType*>(var_type.get())) {
                checkArrayInitializer(init_list, array_type, true);
            } else if (auto* struct_type = dynamic_cast<StructType*>(var_type.get())) {
                checkStructInitializer(init_list, struct_type, true);
            } else {
                // 标量类型：只能有一个元素
                if (init_list->getElementCount() != 1) {
                    error("标量类型的初始化列表只能包含一个元素");
                    return;
                }
                // 检查元素类型
                auto elem_type = analyzeExpression(init_list->getElements()[0].get());
                if (!isTypeCompatible(var_type, elem_type)) {
                    error("初始化类型不匹配: 不能将 " + elem_type->toString() +
                          " 类型赋值给 " + var_type->toString() + " 类型");
                    return;
                }
                // 检查是否是常量表达式
                if (!isConstantExpression(init_list->getElements()[0].get())) {
                    error("全局变量 '" + global_var->getName() + "' 的初始化器必须是编译时常量表达式");
                    return;
                }
            }
        } else {
            // 单个表达式初始化
            auto init_type = analyzeExpression(initializer);
            if (!init_type) {
                error("无法分析初始化表达式的类型");
                return;
            }

            // 检查类型兼容性
            if (!isTypeCompatible(var_type, init_type)) {
                error("全局变量初始化类型不匹配: 不能将 " + init_type->toString() +
                      " 类型赋值给 " + var_type->toString() + " 类型");
                return;
            }

            // ========== Phase 6: 检查是否是常量表达式 ==========
            if (!isConstantExpression(initializer)) {
                error("全局变量 '" + global_var->getName() + "' 的初始化器必须是编译时常量表达式");
                return;
            }
        }
    }
}

// ========== 常量表达式检查 (Phase 6) ==========

bool Sema::isConstantExpression(ExprNode* expr) {
    if (!expr) return false;

    // 1. 数字字面量是常量
    if (dynamic_cast<NumberNode*>(expr)) {
        return true;
    }

    // 2. 二元运算：两个操作数都是常量
    if (auto* binop = dynamic_cast<BinaryOpNode*>(expr)) {
        return isConstantExpression(binop->getLeft()) &&
               isConstantExpression(binop->getRight());
    }

    // 3. 一元运算
    if (auto* unary = dynamic_cast<UnaryOpNode*>(expr)) {
        switch (unary->getOperator()) {
            case TokenType::Minus:      // 负数: -expr
            case TokenType::LogicalNot: // 逻辑非: !expr
                return isConstantExpression(unary->getOperand());

            case TokenType::Ampersand: {
                // 取地址: &global_var
                // 只允许取全局变量的地址
                auto* var = dynamic_cast<VariableNode*>(unary->getOperand());
                if (!var) {
                    return false;  // 只能取变量的地址
                }
                // 检查是否是全局变量
                return global_symbols_.find(var->getName()) != global_symbols_.end();
            }

            default:
                return false;
        }
    }

    // 4. 变量引用：不是常量（即使是全局变量）
    if (dynamic_cast<VariableNode*>(expr)) {
        return false;
    }

    // 5. 其他表达式类型（函数调用、数组访问等）都不是常量
    return false;
}

// ========== 初始化列表检查 (Phase 7) ==========

void Sema::checkArrayInitializer(InitializerListNode* init_list, ArrayType* array_type, bool is_global) {
    const auto& elements = init_list->getElements();

    // 1. 检查数量：初始化列表元素不能超过数组大小
    if (elements.size() > static_cast<size_t>(array_type->getSize())) {
        error("数组初始化列表元素过多：数组大小为 " + std::to_string(array_type->getSize()) +
              "，但提供了 " + std::to_string(elements.size()) + " 个元素");
        return;
    }

    // 2. 检查每个元素
    for (size_t i = 0; i < elements.size(); i++) {
        auto& elem = elements[i];

        // Phase 1: 不支持嵌套初始化列表
        if (dynamic_cast<InitializerListNode*>(elem.get())) {
            error("暂不支持嵌套初始化列表");
            return;
        }

        // 全局变量：必须是常量表达式
        if (is_global && !isConstantExpression(elem.get())) {
            error("全局数组初始化列表的第 " + std::to_string(i + 1) + " 个元素必须是编译时常量表达式");
            return;
        }

        // 类型检查：元素类型必须与数组元素类型兼容
        auto elem_type = analyzeExpression(elem.get());
        if (!isTypeCompatible(array_type->getElementType(), elem_type)) {
            error("数组初始化列表的第 " + std::to_string(i + 1) + " 个元素类型不匹配：期望 " +
                  array_type->getElementType()->toString() + "，实际 " + elem_type->toString());
            return;
        }
    }
}

void Sema::checkStructInitializer(InitializerListNode* init_list, StructType* struct_type, bool is_global) {
    const auto& elements = init_list->getElements();
    const auto& members = struct_type->getMembers();

    // 1. 检查数量：初始化列表元素不能超过结构体成员数
    if (elements.size() > members.size()) {
        error("结构体初始化列表元素过多：结构体有 " + std::to_string(members.size()) +
              " 个成员，但提供了 " + std::to_string(elements.size()) + " 个元素");
        return;
    }

    // 2. 检查每个元素
    for (size_t i = 0; i < elements.size(); i++) {
        auto& elem = elements[i];

        // Phase 1: 不支持嵌套初始化列表
        if (dynamic_cast<InitializerListNode*>(elem.get())) {
            error("暂不支持嵌套初始化列表");
            return;
        }

        // 全局变量：必须是常量表达式
        if (is_global && !isConstantExpression(elem.get())) {
            error("全局结构体初始化列表的第 " + std::to_string(i + 1) + " 个元素必须是编译时常量表达式");
            return;
        }

        // 类型检查：元素类型必须与对应成员类型兼容
        auto elem_type = analyzeExpression(elem.get());
        const auto& [member_name, member_type] = members[i];
        if (!isTypeCompatible(member_type, elem_type)) {
            error("结构体初始化列表的第 " + std::to_string(i + 1) + " 个元素类型不匹配：期望 " +
                  member_type->toString() + "（成员 '" + member_name + "'），实际 " + elem_type->toString());
            return;
        }
    }
}
