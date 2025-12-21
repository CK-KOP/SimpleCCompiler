#include "parser.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(Lexer& lexer) : lexer_(lexer) {
    // 获取第一个Token
    currentToken_ = lexer_.getNextToken();
}

Token Parser::getCurrentToken() const {
    return currentToken_;
}

bool Parser::is(TokenType type) const {
    return currentToken_.is(type);
}

bool Parser::isAtEnd() const {
    return currentToken_.is(TokenType::End);
}

std::unique_ptr<ExprNode> Parser::parseExpression() {
    // 递归下降解析器入口点
    // 表达式语法优先级链（从低到高）:
    // assignment -> logical_or -> logical_and -> equality -> comparison -> term -> factor -> unary -> primary
    return parseAssignment();
}

// 解析完整表达式并检查语法错误
std::unique_ptr<ExprNode> Parser::parseCompleteExpression() {
    auto expr = parseExpression();

    // 检查是否还有未消费的Token
    if (!isAtEnd()) {
        throw std::runtime_error("语法错误：表达式后有多余的Token: " + currentToken_.toString());
    }

    return expr;
}

// 解析赋值表达式（最低优先级，右结合）
std::unique_ptr<ExprNode> Parser::parseAssignment() {
    // 先解析右边的高优先级表达式
    std::unique_ptr<ExprNode> expr = parseLogicalOr();

    if (match(TokenType::Assign)) {
        advance(); // 消费=
        auto right = parseAssignment(); // 右结合：递归调用自身，a = b = c 解析为 a = (b = c)

        // 安全检查：赋值运算符左边必须是变量、数组访问或解引用表达式
        if (dynamic_cast<VariableNode*>(expr.get()) || dynamic_cast<ArrayAccessNode*>(expr.get())) {
            return std::make_unique<BinaryOpNode>(std::move(expr), TokenType::Assign, std::move(right));
        }
        // 检查是否是解引用表达式 *p = value
        if (auto* unary = dynamic_cast<UnaryOpNode*>(expr.get())) {
            if (unary->getOperator() == TokenType::Multiply) {
                return std::make_unique<BinaryOpNode>(std::move(expr), TokenType::Assign, std::move(right));
            }
        }
        throw std::runtime_error("赋值运算符左边必须是变量、数组元素或解引用表达式");
    }

    return expr;
}

// 解析逻辑或表达式（优先级低于逻辑与，左结合）
std::unique_ptr<ExprNode> Parser::parseLogicalOr() {
    std::unique_ptr<ExprNode> expr = parseLogicalAnd();

    // 检查 || 运算符（短路逻辑，左结合）
    while (match(TokenType::LogicalOr)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费||
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

// 解析逻辑与表达式（优先级低于比较运算符，左结合）
std::unique_ptr<ExprNode> Parser::parseLogicalAnd() {
    std::unique_ptr<ExprNode> expr = parseEquality();

    // 检查 && 运算符（短路逻辑，左结合）
    while (match(TokenType::LogicalAnd)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费&&
        auto right = parseEquality();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseEquality() {
    // 解析等值比较表达式: == 和 !=
    std::unique_ptr<ExprNode> expr = parseComparison();

    // 检查是否有 == 或 != 运算符
    while (match(TokenType::Equal) || match(TokenType::NotEqual)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseComparison();
        // 构建二元运算节点，左结合
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseComparison() {
    // 解析大小比较表达式: <, <=, >, >=
    std::unique_ptr<ExprNode> expr = parseTerm();

    // 检查所有大小比较运算符
    while (match(TokenType::Less) || match(TokenType::LessEqual) ||
           match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseTerm();
        // 构建二元运算节点，左结合
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseTerm() {
    // 解析加减法表达式: + 和 -
    std::unique_ptr<ExprNode> expr = parseFactor();

    // 检查加减法运算符（左结合）
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseFactor();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseFactor() {
    // 解析乘除法表达式: * 和 /
    std::unique_ptr<ExprNode> expr = parseUnary();

    // 检查乘除法运算符（左结合，比加减法优先级高）
    while (match(TokenType::Multiply) || match(TokenType::Divide)) {
        TokenType op = currentToken_.getType();
        advance(); // 消费运算符
        std::unique_ptr<ExprNode> right = parseUnary();
        expr = std::make_unique<BinaryOpNode>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
    // 解析一元运算符: +x, -x, !x, &x, *p（优先级很高，仅次于括号）
    if (match(TokenType::Plus)) {
        advance(); // 消费+
        auto operand = parseUnary(); // 递归解析操作数
        return std::make_unique<UnaryOpNode>(TokenType::Plus, std::move(operand));
    }

    if (match(TokenType::Minus)) {
        advance(); // 消费-
        auto operand = parseUnary(); // 递归解析操作数，支持--x这样的表达式
        return std::make_unique<UnaryOpNode>(TokenType::Minus, std::move(operand));
    }

    if (match(TokenType::LogicalNot)) {
        advance(); // 消费!
        auto operand = parseUnary(); // 递归解析操作数
        return std::make_unique<UnaryOpNode>(TokenType::LogicalNot, std::move(operand));
    }

    if (match(TokenType::Ampersand)) {
        advance(); // 消费&
        auto operand = parseUnary(); // 递归解析操作数
        return std::make_unique<UnaryOpNode>(TokenType::Ampersand, std::move(operand));
    }

    if (match(TokenType::Multiply)) {
        advance(); // 消费*
        auto operand = parseUnary(); // 递归解析操作数，支持**p这样的表达式
        return std::make_unique<UnaryOpNode>(TokenType::Multiply, std::move(operand));
    }

    // 没有一元运算符，直接解析基础表达式
    return parsePrimary();
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    // 解析基础表达式：数字、变量、函数调用、数组访问、括号表达式
    if (match(TokenType::Number)) {
        int value = std::stoi(currentToken_.getValue());
        advance();
        return std::make_unique<NumberNode>(value);
    }

    if (match(TokenType::Identifier)) {
        std::string name = currentToken_.getValue();
        advance();
        // 检查是否是函数调用
        if (match(TokenType::LParen)) {
            return parseFunctionCall(name);
        }
        // 检查是否是数组访问（支持多维）
        std::unique_ptr<ExprNode> expr = std::make_unique<VariableNode>(name);
        while (match(TokenType::LBracket)) {
            advance(); // 消费[
            auto index = parseExpression();
            consume(TokenType::RBracket, "期望 ']' 在数组下标后");
            expr = std::make_unique<ArrayAccessNode>(std::move(expr), std::move(index));
        }
        return expr;
    }

    if (match(TokenType::LParen)) {
        advance(); // 消费(
        auto expr = parseExpression(); // 递归解析括号内的完整表达式
        consume(TokenType::RParen, "期望 ')'");
        return expr; // 返回括号内的表达式，括号在AST中消失
    }

    throw std::runtime_error("意外的Token: " + currentToken_.toString());
}

// 解析函数调用：foo(arg1, arg2, ...)
std::unique_ptr<FunctionCallNode> Parser::parseFunctionCall(const std::string& name) {
    advance(); // 消费(
    std::vector<std::unique_ptr<ExprNode>> args;

    // 解析参数列表
    if (!match(TokenType::RParen)) {
        args.push_back(parseExpression());
        while (match(TokenType::Comma)) {
            advance(); // 消费,
            args.push_back(parseExpression());
        }
    }

    consume(TokenType::RParen, "期望 ')' 在函数调用后");
    return std::make_unique<FunctionCallNode>(name, std::move(args));
}

// 前进到下一个Token（更新currentToken_）
void Parser::advance() {
    if (!isAtEnd()) {
        currentToken_ = lexer_.getNextToken();
    }
}

// 消费指定类型的Token，如果不匹配则抛出错误
Token Parser::consume(TokenType type, const std::string& message) {
    if (currentToken_.is(type)) {
        Token token = currentToken_;
        advance();
        return token;
    }
    throw std::runtime_error(message + ", 但得到: " + currentToken_.toString());
}

// 检查当前Token是否是指定类型（不消费Token）
bool Parser::match(TokenType type) {
    if (currentToken_.is(type)) {
        return true;
    }
    return false;
}

// 解析语句
std::unique_ptr<StmtNode> Parser::parseStatement() {
    if (match(TokenType::LBrace)) {
        // 复合语句：{ ... }
        advance(); // 消费{
        return parseCompoundStatement();
    }

    if (match(TokenType::Int)) {
        // 变量声明语句：int x; 或 int y = 5;
        return parseVariableDeclaration();
    }

    if (match(TokenType::Return)) {
        // 返回语句：return; 或 return x;
        return parseReturnStatement();
    }

    if (match(TokenType::If)) {
        // If语句：if (condition) stmt [else stmt]
        return parseIfStatement();
    }

    if (match(TokenType::While)) {
        // While语句：while (condition) stmt
        return parseWhileStatement();
    }

    if (match(TokenType::For)) {
        // For语句：for (init; condition; increment) stmt
        return parseForStatement();
    }

    if (match(TokenType::Do)) {
        // Do-While语句：do stmt while (condition);
        return parseDoWhileStatement();
    }

    if (match(TokenType::Break)) {
        // Break语句：break;
        return parseBreakStatement();
    }

    if (match(TokenType::Continue)) {
        // Continue语句：continue;
        return parseContinueStatement();
    }

    if (match(TokenType::Semicolon)) {
        // 空语句：;
        advance(); // 消费分号
        return std::make_unique<EmptyStmtNode>();
    }

    // 表达式语句：表达式;
    auto expr = parseExpression();
    consume(TokenType::Semicolon, "期望分号");
    return std::make_unique<ExprStmtNode>(std::move(expr));
}

// 检查是否是类型关键字
bool Parser::isTypeKeyword() const {
    return currentToken_.is(TokenType::Int) || currentToken_.is(TokenType::Void);
}

// 解析程序（函数定义的序列）
std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>();

    while (!isAtEnd()) {
        try {
            auto func = parseFunctionDeclaration();
            program->addFunction(std::move(func));
        } catch (const std::exception& e) {
            throw std::runtime_error("在第" + std::to_string(currentToken_.getLine()) +
                                    "行: " + std::string(e.what()));
        }
    }

    return program;
}

// 解析函数定义：int foo(int a, int b) { ... }
std::unique_ptr<FunctionDeclNode> Parser::parseFunctionDeclaration() {
    // 解析返回类型
    std::string return_type;
    if (match(TokenType::Int)) {
        return_type = "int";
        advance();
    } else if (match(TokenType::Void)) {
        return_type = "void";
        advance();
    } else {
        throw std::runtime_error("期望函数返回类型，但得到: " + currentToken_.toString());
    }

    // 解析函数名
    if (!match(TokenType::Identifier)) {
        throw std::runtime_error("期望函数名，但得到: " + currentToken_.toString());
    }
    std::string func_name = currentToken_.getValue();
    advance();

    // 解析参数列表
    consume(TokenType::LParen, "期望 '(' 在函数名后");
    std::vector<FunctionParam> params;

    if (!match(TokenType::RParen)) {
        // 解析第一个参数
        if (!isTypeKeyword()) {
            throw std::runtime_error("期望参数类型");
        }
        std::string param_type = currentToken_.getValue();
        advance();
        // 处理指针类型参数
        while (match(TokenType::Multiply)) {
            param_type += "*";
            advance();
        }

        if (!match(TokenType::Identifier)) {
            throw std::runtime_error("期望参数名");
        }
        std::string param_name = currentToken_.getValue();
        advance();
        params.emplace_back(param_type, param_name);

        // 解析剩余参数
        while (match(TokenType::Comma)) {
            advance(); // 消费,

            if (!isTypeKeyword()) {
                throw std::runtime_error("期望参数类型");
            }
            param_type = currentToken_.getValue();
            advance();
            // 处理指针类型参数
            while (match(TokenType::Multiply)) {
                param_type += "*";
                advance();
            }

            if (!match(TokenType::Identifier)) {
                throw std::runtime_error("期望参数名");
            }
            param_name = currentToken_.getValue();
            advance();
            params.emplace_back(param_type, param_name);
        }
    }

    consume(TokenType::RParen, "期望 ')' 在参数列表后");

    // 解析函数体
    consume(TokenType::LBrace, "期望 '{' 在函数体开始");
    auto body = parseCompoundStatement();

    return std::make_unique<FunctionDeclNode>(return_type, func_name, std::move(params), std::move(body));
}

// 解析复合语句：{ stmt1; stmt2; ... }
std::unique_ptr<CompoundStmtNode> Parser::parseCompoundStatement() {
    auto compound = std::make_unique<CompoundStmtNode>();

    while (!isAtEnd() && !match(TokenType::RBrace)) {
        try {
            auto stmt = parseStatement();
            compound->addStatement(std::move(stmt));
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("在代码块中: ") + e.what());
        }
    }

    consume(TokenType::RBrace, "期望 '}'");
    return compound;
}

// 获取运算符的优先级（数值越大优先级越高）
Parser::Precedence Parser::getOperatorPrecedence(TokenType op) {
    switch (op) {
        case TokenType::Assign:
            return PREC_ASSIGN;           // = (最低优先级，右结合)
        case TokenType::LogicalOr:
            return PREC_LOGICAL_OR;       // ||
        case TokenType::LogicalAnd:
            return PREC_LOGICAL_AND;      // &&
        case TokenType::Equal:
        case TokenType::NotEqual:
            return PREC_EQUALITY;         // ==, !=
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
            return PREC_COMPARISON;       // <, <=, >, >=
        case TokenType::Plus:
        case TokenType::Minus:
            return PREC_TERM;             // +, -
        case TokenType::Multiply:
        case TokenType::Divide:
            return PREC_FACTOR;           // *, /
        default:
            return PREC_LOWEST;           // 未知运算符最低优先级
    }
}

// 解析变量声明语句：int x; 或 int y = 5; 或 int arr[10]; 或 int *p; 或 int arr[3][4]; 或 int *arr[10];
std::unique_ptr<VarDeclStmtNode> Parser::parseVariableDeclaration() {
    advance(); // 消费int

    // 构建类型字符串，处理指针类型 int*, int**, ...
    std::string varType = "int";
    while (match(TokenType::Multiply)) {
        varType += "*";
        advance(); // 消费*
    }

    // 获取变量名
    if (!match(TokenType::Identifier)) {
        throw std::runtime_error("期望变量名，但得到: " + currentToken_.toString());
    }

    std::string varName = currentToken_.getValue();
    advance(); // 消费变量名

    // 检查是否是数组声明（支持多维）
    std::vector<int> dims;
    while (match(TokenType::LBracket)) {
        advance(); // 消费[
        if (!match(TokenType::Number)) {
            throw std::runtime_error("期望数组大小，但得到: " + currentToken_.toString());
        }
        dims.push_back(std::stoi(currentToken_.getValue()));
        advance(); // 消费数字
        consume(TokenType::RBracket, "期望 ']' 在数组大小后");
    }

    if (!dims.empty()) {
        consume(TokenType::Semicolon, "期望分号");
        return std::make_unique<VarDeclStmtNode>(varType, varName, std::move(dims));
    }

    std::unique_ptr<ExprNode> initializer = nullptr;

    // 检查是否有初始化值
    if (match(TokenType::Assign)) {
        advance(); // 消费=
        initializer = parseExpression(); // 解析初始化表达式
    }

    consume(TokenType::Semicolon, "期望分号");

    return std::make_unique<VarDeclStmtNode>(varType, varName, std::move(initializer));
}

// 解析返回语句：return; 或 return x;
std::unique_ptr<ReturnStmtNode> Parser::parseReturnStatement() {
    advance(); // 消费return

    std::unique_ptr<ExprNode> expr = nullptr;

    // 检查是否有返回值
    if (!match(TokenType::Semicolon)) {
        expr = parseExpression(); // 解析返回表达式
    }

    consume(TokenType::Semicolon, "期望分号");

    return std::make_unique<ReturnStmtNode>(std::move(expr));
}

// 解析If语句：if (condition) stmt [else if (condition) stmt ...] [else stmt]
std::unique_ptr<IfStmtNode> Parser::parseIfStatement() {
    advance(); // 消费if

    consume(TokenType::LParen, "期望 '(' 在if条件后");

    // 解析条件表达式
    auto condition = parseExpression();

    consume(TokenType::RParen, "期望 ')' 在if条件后");

    // 解析then分支
    auto then_stmt = parseStatement();

    // 创建If节点
    auto if_node = std::make_unique<IfStmtNode>(std::move(condition), std::move(then_stmt));

    // 解析可选的else if和else分支
    while (match(TokenType::Else)) {
        advance(); // 消费else

        if (match(TokenType::If)) {
            // else if 分支
            advance(); // 消费if

            consume(TokenType::LParen, "期望 '(' 在else if条件后");

            auto else_if_condition = parseExpression();

            consume(TokenType::RParen, "期望 ')' 在else if条件后");

            auto else_if_stmt = parseStatement();

            // 添加else if分支
            if_node->addElseIf(std::move(else_if_condition), std::move(else_if_stmt));
        } else {
            // else 分支
            auto else_stmt = parseStatement();
            if_node->setElseStmt(std::move(else_stmt));
            break; // else后面不能再有else if
        }
    }

    return if_node;
}

// 解析While语句：while (condition) stmt
std::unique_ptr<WhileStmtNode> Parser::parseWhileStatement() {
    advance(); // 消费while

    consume(TokenType::LParen, "期望 '(' 在while条件后");

    // 解析条件表达式
    auto condition = parseExpression();

    consume(TokenType::RParen, "期望 ')' 在while条件后");

    // 解析循环体
    auto body = parseStatement();

    return std::make_unique<WhileStmtNode>(std::move(condition), std::move(body));
}

// 解析For语句：for (init; condition; increment) stmt
std::unique_ptr<ForStmtNode> Parser::parseForStatement() {
    advance(); // 消费for

    consume(TokenType::LParen, "期望 '(' 在for后");

    // 解析初始化部分（可为空）
    std::unique_ptr<StmtNode> init = nullptr;
    if (!match(TokenType::Semicolon)) {
        // 可能是变量声明或表达式
        if (match(TokenType::Int)) {
            init = parseVariableDeclaration();
        } else {
            auto init_expr = parseExpression();
            consume(TokenType::Semicolon, "期望分号在for初始化后");
            init = std::make_unique<ExprStmtNode>(std::move(init_expr));
        }
    } else {
        advance(); // 消费空初始化的分号
    }

    // 解析条件部分（可为空）
    std::unique_ptr<ExprNode> condition = nullptr;
    if (!match(TokenType::Semicolon)) {
        condition = parseExpression();
        consume(TokenType::Semicolon, "期望分号在for条件后");
    } else {
        advance(); // 消费空条件的分号
    }

    // 解析增量部分（可为空）
    std::unique_ptr<ExprNode> increment = nullptr;
    if (!match(TokenType::RParen)) {
        increment = parseExpression();
        consume(TokenType::RParen, "期望 ')' 在for增量后");
    } else {
        advance(); // 消费右括号
    }

    // 解析循环体
    auto body = parseStatement();

    return std::make_unique<ForStmtNode>(std::move(init), std::move(condition), std::move(increment), std::move(body));
}

// 解析Do-While语句：do stmt while (condition);
std::unique_ptr<DoWhileStmtNode> Parser::parseDoWhileStatement() {
    advance(); // 消费do

    // 解析循环体
    auto body = parseStatement();

    consume(TokenType::While, "期望while关键字");
    consume(TokenType::LParen, "期望 '(' 在while条件后");

    // 解析条件表达式
    auto condition = parseExpression();

    consume(TokenType::RParen, "期望 ')' 在while条件后");
    consume(TokenType::Semicolon, "期望分号在do-while语句后");

    return std::make_unique<DoWhileStmtNode>(std::move(body), std::move(condition));
}

// 解析Break语句：break;
std::unique_ptr<BreakStmtNode> Parser::parseBreakStatement() {
    advance(); // 消费break

    consume(TokenType::Semicolon, "期望分号在break语句后");

    return std::make_unique<BreakStmtNode>();
}

// 解析Continue语句：continue;
std::unique_ptr<ContinueStmtNode> Parser::parseContinueStatement() {
    advance(); // 消费continue

    consume(TokenType::Semicolon, "期望分号在continue语句后");

    return std::make_unique<ContinueStmtNode>();
}