#include "../include/sema.h"

std::shared_ptr<Type> Sema::stringToType(const std::string& type_name) {
    if (type_name == "int") return Type::getIntType();
    if (type_name == "void") return Type::getVoidType();

    // 处理结构体类型: struct Point
    if (type_name.substr(0, 7) == "struct ") {
        std::string struct_name = type_name.substr(7);
        auto it = struct_types_.find(struct_name);
        if (it != struct_types_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 处理指针类型: int*, int**, ...
    if (type_name.size() > 1 && type_name.back() == '*') {
        // 递归获取基类型
        auto base_type = stringToType(type_name.substr(0, type_name.size() - 1));
        if (base_type) {
            return std::make_shared<PointerType>(base_type);
        }
    }
    return nullptr;
}

bool Sema::analyze(ProgramNode* program) {
    // 先分析所有结构体定义
    for (const auto& struct_decl : program->getStructs()) {
        analyzeStructDecl(struct_decl.get());
    }

    // 再分析所有函数定义
    for (const auto& func : program->getFunctions()) {
        analyzeFunction(func.get());
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

    // 构建函数类型
    std::vector<FunctionType::Param> params;
    for (const auto& param : func->getParams()) {
        auto param_type = stringToType(param.type);
        if (!param_type) {
            error("未知的参数类型: " + param.type);
            return;
        }
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

    // 分析初始化表达式（数组暂不支持初始化）
    if (stmt->hasInitializer()) {
        auto init_type = analyzeExpression(stmt->getInitializer());
        if (init_type->isVoid()) {
            error("void 类型的值不能用于初始化变量");
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
    auto symbol = scope_.findSymbol(expr->getName());
    if (!symbol) {
        error("未声明的变量: " + expr->getName());
        return Type::getIntType();
    }

    if (symbol->getType()->isFunction()) {
        // 函数名作为表达式使用（函数指针，暂不支持）
        return symbol->getType();
    }

    return symbol->getType();
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
    } else {
        // 其他二元运算：操作数不能是 void
        if (left_type->isVoid() || right_type->isVoid()) {
            error("void 类型的值不能用于表达式");
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

    // 分析每个参数
    for (const auto& arg : expr->getArgs()) {
        analyzeExpression(arg.get());
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
