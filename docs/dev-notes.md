# SimpleC 编译器开发笔记

记录开发过程中遇到的问题和解决方案，供学习参考。

---

# 第一部分：词法分析 & 语法分析

## 1. 空语句的表示问题

### 问题描述
最初实现空语句 `;` 时，用 `NumberNode(0)` 来表示，这在语义上是错误的。

### 原始代码
```cpp
// 错误的实现
if (match(TokenType::Semicolon)) {
    return std::make_unique<NumberNode>(0);  // 空语句返回数字 0？
}
```

### 问题
- 空语句不应该有值
- 混淆了表达式和语句的概念
- 后续语义分析会出问题

### 解决方案
创建专门的 `EmptyStmtNode` 类：

```cpp
// ast.h
class EmptyStmtNode : public StmtNode {
public:
    std::string toString(int indent = 0) const override {
        return std::string(indent, ' ') + "EmptyStmt";
    }
};

// parser.cpp
if (match(TokenType::Semicolon)) {
    return std::make_unique<EmptyStmtNode>();
}
```

### 教训
- 区分表达式和语句：表达式有值，语句没有
- 为每种语法结构创建对应的 AST 节点类型

---

## 2. 表达式语句的必要性

### 问题描述
最初不理解为什么需要"表达式语句"这个概念，认为表达式和语句是完全分开的。

### 疑问
```c
x = 10;      // 这是表达式还是语句？
foo();       // 函数调用是表达式还是语句？
```

### 解答
在 C 语言中，任何表达式后面加分号就变成语句：

```
表达式语句 = 表达式 + ";"
```

这就是为什么需要 `ExprStmtNode`：

```cpp
class ExprStmtNode : public StmtNode {
    std::unique_ptr<ExprNode> expr_;  // 包装一个表达式
};
```

### 语法分析流程
```
parseStatement()
  → 尝试匹配 if/while/for/return 等关键字
  → 都不匹配？尝试解析表达式语句
  → parseExpressionStatement()
      → parseExpression()
      → expect(Semicolon)
      → return ExprStmtNode(expr)
```

### 教训
- 表达式语句是连接表达式和语句的桥梁
- 理解语法规则的层次结构很重要

---

## 3. const 方法调用非 const 方法

### 问题描述
`isTypeKeyword()` 被声明为 const 方法，但内部调用了非 const 的 `match()` 方法。

### 错误信息
```
error: passing 'const Parser' as 'this' argument discards qualifiers
```

### 原始代码
```cpp
bool isTypeKeyword() const {
    return match(TokenType::Int) || match(TokenType::Void);
    // match() 不是 const 方法！
}
```

### 解决方案
改用 const 方法 `currentToken_.is()`：

```cpp
bool isTypeKeyword() const {
    return currentToken_.is(TokenType::Int) ||
           currentToken_.is(TokenType::Void);
}
```

### 教训
- const 方法只能调用其他 const 方法
- 区分"查看"操作（const）和"修改"操作（非 const）

---

## 4. 缺少 void 关键字支持

### 问题描述
实现函数定义时，发现词法分析器不支持 `void` 关键字。

### 错误现象
```c
void test() { }  // 解析失败：void 被识别为标识符
```

### 解决方案
需要在三个地方添加 void 支持：

```cpp
// token.h - 添加枚举值
enum class TokenType {
    // ...
    Void,  // void 关键字
};

// lexer.cpp - 添加关键字映射
keywords_["void"] = TokenType::Void;

// token.cpp - 添加字符串转换
case TokenType::Void: return "Void";
```

### 教训
- 添加新关键字需要修改多个文件
- 保持词法分析器和语法分析器的同步

---

## 5. Makefile 通配符规则问题

### 问题描述
使用通配符规则编译 .cpp 文件时出现循环依赖。

### 原始 Makefile
```makefile
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@
```

### 问题
某些情况下 make 无法正确解析依赖关系。

### 解决方案
显式列出每个目标文件的编译规则：

```makefile
$(BUILDDIR)/lexer.o: $(SRCDIR)/lexer.cpp | $(BUILDDIR)
    $(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/parser.o: $(SRCDIR)/parser.cpp | $(BUILDDIR)
    $(CXX) $(CXXFLAGS) -c $< -o $@

# ... 其他文件
```

### 教训
- 显式规则比隐式规则更可靠
- 添加新源文件时记得更新 Makefile

---

# 第二部分：语义分析

## 6. 符号表作用域管理

### 问题描述
需要正确处理嵌套作用域，比如函数内的块级作用域。

### 示例
```c
int main() {
    int x = 1;
    {
        int x = 2;  // 这是新变量，不是重复声明
        // 这里 x = 2
    }
    // 这里 x = 1
}
```

### 解决方案
使用作用域栈：

```cpp
class Scope {
    std::vector<std::shared_ptr<Env>> envs_;  // 作用域栈

    void enterScope() {
        envs_.push_back(std::make_shared<Env>());
    }

    void exitScope() {
        envs_.pop_back();
    }

    // 查找符号：从内向外搜索
    Symbol* findSymbol(const std::string& name) {
        for (auto it = envs_.rbegin(); it != envs_.rend(); ++it) {
            if (auto sym = (*it)->find(name)) {
                return sym;
            }
        }
        return nullptr;
    }

    // 只在当前作用域查找（用于检测重复声明）
    Symbol* findSymbolInCurrentScope(const std::string& name) {
        return envs_.back()->find(name);
    }
};
```

### 教训
- 区分"查找符号"和"检测重复声明"
- 作用域栈是实现嵌套作用域的标准方法

---

## 7. 函数返回值类型检查

### 问题描述
需要检查 return 语句是否与函数返回类型匹配。

### 错误示例
```c
void test() {
    return 42;  // 错误：void 函数不应返回值
}

int main() {
    return;     // 错误：非 void 函数应返回值
}
```

### 解决方案
在分析函数时记录当前函数的返回类型：

```cpp
class Sema {
    std::shared_ptr<Type> current_function_return_type_;

    void analyzeFunction(FunctionDeclNode* func) {
        current_function_return_type_ = getReturnType(func);
        // 分析函数体...
        current_function_return_type_ = nullptr;
    }

    void analyzeReturnStatement(ReturnStmtNode* stmt) {
        if (stmt->hasExpression()) {
            if (current_function_return_type_->isVoid()) {
                error("void 函数不应返回值");
            }
        } else {
            if (!current_function_return_type_->isVoid()) {
                error("非 void 函数应返回值");
            }
        }
    }
};
```

### 教训
- 语义分析需要维护上下文信息（如当前函数）
- 类型检查是语义分析的核心任务

---

# 第三部分：代码生成 & 虚拟机

## 8. TokenType 名称不匹配（代码生成）

### 问题描述
在实现代码生成器时，假设了错误的 TokenType 枚举名称：

```cpp
// 错误的假设
case TokenType::Star:     // 实际不存在
case TokenType::Slash:    // 实际不存在
case TokenType::LessEq:   // 实际不存在
case TokenType::And:      // 实际不存在
```

### 错误信息
```
error: 'Star' is not a member of 'TokenType'
error: 'Slash' is not a member of 'TokenType'
```

### 解决方案
查看 `token.h` 中的实际定义，使用正确的名称：

```cpp
// 正确的名称
case TokenType::Multiply:     // *
case TokenType::Divide:       // /
case TokenType::LessEqual:    // <=
case TokenType::LogicalAnd:   // &&
case TokenType::LogicalOr:    // ||
case TokenType::LogicalNot:   // !
```

### 教训
- 在使用其他模块的类型时，先查看其定义
- 不要假设命名约定，不同项目可能有不同风格

---

## 9. 虚拟机入口帧缺失

### 问题描述
直接从 main 函数开始执行，没有设置调用帧。当 main 执行 `RETV` 时，尝试从空栈中 pop 返回地址和帧指针，导致崩溃。

### 错误信息
```
错误: Stack underflow
```

### 原始代码
```cpp
int VM::execute(const ByteCode& bytecode) {
    pc_ = bytecode.entry_point;
    sp_ = 0;
    fp_ = 0;  // 问题：没有调用帧
    running_ = true;
    // ...
}
```

### 解决方案
在执行前设置一个虚拟调用帧，返回地址设为 -1 表示程序结束：

```cpp
int VM::execute(const ByteCode& bytecode) {
    // 设置虚拟调用帧
    sp_ = 0;
    push(-1);   // 返回地址（-1 表示结束）
    push(0);    // 旧的帧指针
    fp_ = sp_;
    pc_ = bytecode.entry_point;
    running_ = true;
    // ...
}

// 同时修改循环条件，检测 pc_ == -1 时退出
while (running_ && pc_ >= 0 && pc_ < (int)code.size()) {
```

### 教训
- 函数调用约定需要一致：调用者和被调用者对栈布局的假设必须匹配
- 入口点也需要遵循调用约定，即使是 main 函数

---

## 10. 局部变量空间被覆盖

### 问题描述
变量声明时，先 PUSH 初始值，然后 STORE 到变量位置。但 STORE 会 pop 值，导致栈指针回退。下一个 PUSH 会覆盖之前的变量。

### 示例
```c
int a = 10;  // PUSH 10, STORE 0 → 值存到 stack[fp+0]，但 sp 回退
int b = 20;  // PUSH 20 → 覆盖了 stack[fp+0]！
```

### 调试输出
```
[0] PUSH 10  (sp=2, fp=2)
[1] STORE 0  (sp=3, fp=2)  // sp 变成 2
[2] PUSH 20  (sp=2, fp=2)  // 覆盖了 a 的位置！
```

### 解决方案
变量声明时只 PUSH 值，不 STORE。变量的值直接保留在栈上：

```cpp
void CodeGen::genVarDecl(VarDeclStmtNode* stmt) {
    int offset = allocLocal(stmt->getName());

    if (stmt->hasInitializer()) {
        genExpression(stmt->getInitializer());
        // 值已在栈顶，就是变量的存储位置
    } else {
        code_.emit(OpCode::PUSH, 0);  // 默认值
    }
    // 变量值保留在栈上，offset 用于后续 LOAD/STORE
}
```

### 栈布局
```
[fp+0] = 10  (变量 a)
[fp+1] = 20  (变量 b)
[fp+2] = 30  (变量 c = a + b)
```

### 教训
- 栈式虚拟机中，局部变量可以直接使用栈空间，不需要额外的 STORE
- 理解每条指令对栈指针的影响非常重要

---

## 11. 函数调用后参数未清理

### 问题描述
函数调用后，返回值在栈顶，但参数还在下面。这导致后续变量的偏移计算错误。

### 示例
```c
int result = add(10, 20);
```

### 调用前后的栈布局
```
调用前：
[fp+0] = 10  (参数 a)
[fp+1] = 20  (参数 b)
sp = fp+2

调用后（未清理）：
[fp+0] = 10  (参数 a，应该清理)
[fp+1] = 20  (参数 b，应该清理)
[fp+2] = 30  (返回值)
sp = fp+3

期望的结果：
[fp+0] = 30  (result)
sp = fp+1
```

### 解决方案
添加 POPN 指令，在函数调用后清理参数但保留返回值：

```cpp
// vm.h
enum class OpCode : uint8_t {
    // ...
    POPN        // 弹出 N 个值（保留栈顶）
};

// vm.cpp
case OpCode::POPN: {
    int n = instr.operand;
    if (n > 0) {
        int32_t top = stack_[sp_ - 1];  // 保存返回值
        sp_ -= n;                        // 清理参数
        stack_[sp_ - 1] = top;           // 恢复返回值
    }
    break;
}

// codegen.cpp
void CodeGen::genFunctionCall(FunctionCallNode* expr) {
    // 压入参数
    for (const auto& arg : expr->getArgs()) {
        genExpression(arg.get());
    }

    code_.emit(OpCode::CALL, funcAddr);

    // 清理参数，保留返回值
    if (!expr->getArgs().empty()) {
        code_.emit(OpCode::POPN, expr->getArgs().size());
    }
}
```

### 教训
- 函数调用约定必须明确：谁负责清理参数（调用者 vs 被调用者）
- 本项目采用"调用者清理"约定，更灵活但需要额外指令

---

## 12. 参数访问偏移计算

### 问题描述
函数参数在调用前压栈，位于帧指针之前。需要正确计算负偏移。

### 栈布局（调用 `add(10, 20)` 时）
```
调用者栈帧：
[0] = 10  (参数 a)
[1] = 20  (参数 b)
[2] = 返回地址
[3] = 旧 fp
fp = 4    (被调用者帧指针)
```

### 参数偏移计算
```cpp
// 参数位于 fp 之前
// 对于 add(int a, int b)：
//   a 在 fp-4 (stack[0])
//   b 在 fp-3 (stack[1])

const auto& params = func->getParams();
for (int i = params.size() - 1; i >= 0; --i) {
    locals_[params[i].name] = -(int)(params.size() - i) - 2;
}
// 结果：a -> -4, b -> -3
```

### 教训
- 画出栈布局图有助于理解偏移计算
- 参数和局部变量使用不同的偏移方向（负 vs 正）

---

# 第四部分：调试与设计

## 13. 调试技巧

### 查看生成的字节码
```bash
./build/simplec examples/test.c -c
```

### 调试模式运行
```bash
./build/simplec examples/test.c -d
```

输出每条指令执行时的状态：
```
[0] PUSH 10  (sp=2, fp=2)
[1] PUSH 20  (sp=3, fp=2)
[2] ADD      (sp=4, fp=2)
```

### 常见调试方法
1. 先用简单程序测试（只有 return 常量）
2. 逐步增加复杂度（变量、运算、函数调用）
3. 对比预期栈布局和实际栈布局
4. 在 VM 中添加临时打印语句

---

## 14. 设计决策

### 为什么选择栈式虚拟机？
- 实现简单，不需要寄存器分配
- 代码生成直观，表达式求值自然映射到栈操作
- 适合学习编译原理

### 调用约定选择
- 参数从左到右压栈
- 调用者负责清理参数
- 返回值通过栈顶传递

### 变量存储
- 局部变量直接使用栈空间
- 通过帧指针 + 偏移访问
- 参数使用负偏移，局部变量使用正偏移

---

## 总结

编译器开发中最常见的问题：
1. **接口不匹配** - 仔细检查类型定义和函数签名
2. **栈布局错误** - 画图理解每条指令对栈的影响
3. **调用约定不一致** - 明确定义并严格遵守
4. **偏移计算错误** - 区分参数和局部变量的偏移方向

调试方法：
1. 从最简单的程序开始测试
2. 使用调试模式查看执行过程
3. 对比预期和实际的栈状态
4. 逐步增加程序复杂度

---

# 第五部分：结构体实现

## 15. 结构体指针类型解析顺序问题

### 问题描述
在实现 Phase 2 时，`stringToType()` 函数无法正确解析 `struct Point*` 类型。

### 错误现象
```c
struct Point *ptr = &p;  // 解析失败
```

### 原始代码
```cpp
std::shared_ptr<Type> Sema::stringToType(const std::string& type_name) {
    if (type_name == "int") return Type::getIntType();
    if (type_name == "void") return Type::getVoidType();

    // 先检查结构体类型
    if (type_name.size() > 7 && type_name.substr(0, 7) == "struct ") {
        std::string struct_name = type_name.substr(7);
        // 问题：struct_name 可能是 "Point*"，无法找到
        auto it = struct_types_.find(struct_name);
        if (it != struct_types_.end()) {
            return it->second;
        }
    }

    // 再检查指针类型
    if (type_name.back() == '*') {
        auto base_type = stringToType(type_name.substr(0, type_name.size() - 1));
        return std::make_shared<PointerType>(base_type);
    }

    return nullptr;
}
```

### 问题分析
对于 `struct Point*`：
1. 先匹配 `struct ` 前缀 ✓
2. 提取结构体名 `Point*` ✗（包含了 `*`）
3. 在 `struct_types_` 中查找 `Point*` 失败 ✗

### 解决方案
**必须先处理指针，再处理结构体**：

```cpp
std::shared_ptr<Type> Sema::stringToType(const std::string& type_name) {
    if (type_name == "int") return Type::getIntType();
    if (type_name == "void") return Type::getVoidType();

    // 1. 先处理指针类型（递归剥离 *）
    if (type_name.size() > 1 && type_name.back() == '*') {
        auto base_type = stringToType(type_name.substr(0, type_name.size() - 1));
        if (base_type) {
            return std::make_shared<PointerType>(base_type);
        }
    }

    // 2. 再处理结构体类型
    if (type_name.size() > 7 && type_name.substr(0, 7) == "struct ") {
        std::string struct_name = type_name.substr(7);
        // 现在 struct_name 是纯净的 "Point"
        auto it = struct_types_.find(struct_name);
        if (it != struct_types_.end()) {
            return it->second;
        }
    }

    return nullptr;
}
```

### 解析流程
```
"struct Point*"
  → 检测到 '*'，递归调用 stringToType("struct Point")
    → 匹配 "struct " 前缀
    → 提取 "Point"
    → 查找成功，返回 StructType(Point)
  → 包装为 PointerType(StructType(Point))
```

### 教训
- 类型解析的顺序很重要
- 递归处理可以简化复杂类型的解析
- 先处理外层修饰符（指针），再处理基础类型

---

## 16. 结构体成员指针类型解析失败

### 问题描述
在结构体定义中使用指针类型成员时，解析器无法正确处理 `*`。

### 错误现象
```c
struct Complex {
    struct Point *ptr;  // 解析失败：期望成员名，但得到 *
};
```

### 原始代码
```cpp
std::unique_ptr<StructDeclNode> Parser::parseStructDeclaration() {
    // ...
    while (!match(TokenType::RightBrace)) {
        std::string member_type;
        if (match(TokenType::Struct)) {
            advance();
            member_type = "struct " + currentToken_.getValue();
            advance();
            // 问题：没有处理后面的 *
        } else {
            member_type = currentToken_.getValue();
            advance();
            // 只有基本类型才处理 *
            while (match(TokenType::Multiply)) {
                member_type += "*";
                advance();
            }
        }
        // 期望成员名，但遇到了 *
    }
}
```

### 解决方案
在解析结构体类型后，也要处理指针：

```cpp
if (match(TokenType::Struct)) {
    advance();
    if (!match(TokenType::Identifier)) {
        throw std::runtime_error("期望结构体类型名");
    }
    member_type = "struct " + currentToken_.getValue();
    advance();

    // 添加：处理结构体指针类型成员
    while (match(TokenType::Multiply)) {
        member_type += "*";
        advance();
    }
}
```

### 教训
- 代码重复是潜在bug的信号
- 两个分支处理相似逻辑时，要确保一致性
- 测试边界情况（结构体指针成员）

---

## 17. 箭头操作符的AST表示

### 问题描述
如何在AST中表示箭头操作符 `ptr->member`？

### 设计选择

**方案1：创建新的 ArrowAccessNode**
```cpp
class ArrowAccessNode : public ExprNode {
    std::unique_ptr<ExprNode> pointer_;
    std::string member_;
};
```
- 优点：语义清晰
- 缺点：需要在多处添加处理逻辑

**方案2：在解析时转换为 `(*ptr).member`**
```cpp
// parser.cpp
if (match(TokenType::Arrow)) {
    advance();
    std::string member = currentToken_.getValue();
    advance();

    // 转换：ptr->member => (*ptr).member
    auto deref = std::make_unique<UnaryOpNode>(TokenType::Multiply, std::move(expr));
    expr = std::make_unique<MemberAccessNode>(std::move(deref), member);
}
```
- 优点：复用现有的解引用和成员访问逻辑
- 缺点：AST不直接反映源代码

### 选择
采用**方案2**，因为：
1. `ptr->member` 在语义上就等价于 `(*ptr).member`
2. 减少代码重复
3. 语义分析和代码生成无需特殊处理

### 教训
- 语法糖可以在解析阶段脱糖（desugaring）
- 选择合适的AST表示可以简化后续阶段

---

## 18. 成员访问地址计算的递归处理

### 问题描述
如何生成 `ptr->member` 的代码？最初的实现只支持简单变量的成员访问。

### 错误现象
```c
c.ptr->x = 10;  // 错误：Unsupported member access pattern
```

### 原始代码
```cpp
void CodeGen::genMemberAccessAddr(MemberAccessNode* expr) {
    auto struct_type = std::dynamic_pointer_cast<StructType>(object_type);
    int member_offset = struct_type->getMemberOffset(expr->getMember());

    if (auto* var = dynamic_cast<VariableNode*>(expr->getObject())) {
        // 简单情况：obj.member
        int base_offset = getLocal(var->getName());
        code_.emit(OpCode::LEA, base_offset + member_offset);
    } else if (auto* inner_member = dynamic_cast<MemberAccessNode*>(expr->getObject())) {
        // 链式成员访问：obj.inner.member
        genMemberAccessAddr(inner_member);
        code_.emit(OpCode::ADDPTR, member_offset);
    } else {
        throw std::runtime_error("Unsupported member access pattern");
    }
}
```

### 问题分析
`ptr->member` 被转换为 `(*ptr).member`，对象是 `UnaryOpNode`（解引用），不在支持的模式中。

### 解决方案
添加对解引用表达式的支持：

```cpp
void CodeGen::genMemberAccessAddr(MemberAccessNode* expr) {
    // ... 前面的代码 ...

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
```

### 代码生成流程
```
c.ptr->x
  → (*c.ptr).x
  → genMemberAccessAddr(MemberAccessNode)
    → object = UnaryOpNode(*, MemberAccessNode(c, ptr))
    → genExpression(MemberAccessNode(c, ptr))
      → 计算 c.ptr 的地址
      → LOADM（加载指针值）
    → ADDPTR member_offset_of_x
```

### 教训
- 递归处理可以支持任意复杂的表达式组合
- 每种表达式类型都要考虑作为子表达式的情况
- 使用 `dynamic_cast` 识别不同的表达式模式

---

## 19. 类型兼容性检查的实现

### 问题描述
用户建议添加类型检查，防止类型不匹配的赋值。

### 需求
```c
struct Point *ptr = &rectangle;  // 应该报错
int num = &p;                    // 应该报错
int **ptr2 = ptr1;               // 应该报错
```

### 实现方案
添加递归的类型兼容性检查函数：

```cpp
bool Sema::isTypeCompatible(const std::shared_ptr<Type>& left,
                            const std::shared_ptr<Type>& right) {
    // 1. 完全相同
    if (left == right) return true;

    // 2. int 类型兼容
    if (left->isInt() && right->isInt()) return true;

    // 3. 指针类型：递归检查基类型
    if (left->isPointer() && right->isPointer()) {
        auto left_ptr = std::dynamic_pointer_cast<PointerType>(left);
        auto right_ptr = std::dynamic_pointer_cast<PointerType>(right);
        auto left_base = left_ptr->getBaseType();
        auto right_base = right_ptr->getBaseType();

        // 结构体指针：必须指向相同的结构体
        if (left_base->isStruct() && right_base->isStruct()) {
            auto left_struct = std::dynamic_pointer_cast<StructType>(left_base);
            auto right_struct = std::dynamic_pointer_cast<StructType>(right_base);
            return left_struct->getName() == right_struct->getName();
        }

        // 其他指针类型：递归检查
        return isTypeCompatible(left_base, right_base);
    }

    // 4. 结构体类型：必须是相同名称的结构体
    if (left->isStruct() && right->isStruct()) {
        auto left_struct = std::dynamic_pointer_cast<StructType>(left);
        auto right_struct = std::dynamic_pointer_cast<StructType>(right);
        return left_struct->getName() == right_struct->getName();
    }

    // 5. 其他情况不兼容
    return false;
}
```

### 应用位置
1. **赋值表达式**：
```cpp
if (expr->getOperator() == TokenType::Assign) {
    // ... 左值检查 ...
    if (!isTypeCompatible(left_type, right_type)) {
        error("赋值类型不兼容：不能将 " + right_type->toString() +
              " 类型赋值给 " + left_type->toString() + " 类型");
    }
}
```

2. **变量初始化**：
```cpp
if (stmt->hasInitializer()) {
    auto init_type = analyzeExpression(stmt->getInitializer());
    if (!isTypeCompatible(var_type, init_type)) {
        error("初始化类型不兼容：不能将 " + init_type->toString() +
              " 类型赋值给 " + var_type->toString() + " 类型");
    }
}
```

### 测试结果
```c
// ✓ 正确检测
struct Point *ptr = &rectangle;  // 错误：不能将 Rectangle* 赋给 Point*
int num = &p;                    // 错误：不能将 Point* 赋给 int
int **ptr2 = ptr1;               // 错误：不能将 int* 赋给 int**
p = r;                           // 错误：不能将 Rectangle 赋给 Point
```

### 教训
- 类型检查是编译器的重要功能
- 递归检查可以处理复杂的嵌套类型
- 用户反馈可以帮助发现遗漏的功能

---

## 20. 结构体整体赋值（已实现）✅

### 问题描述
最初测试时发现 `p2 = p1` 只复制了第一个成员。

### 原因分析
```cpp
// 原始的赋值实现
if (auto* var = dynamic_cast<VariableNode*>(expr->getLeft())) {
    genExpression(expr->getRight());
    int offset = getLocal(var->getName());
    code_.emit(OpCode::STORE, offset);  // 只存储一个 slot
    code_.emit(OpCode::LOAD, offset);
    return;
}
```

`STORE` 指令只处理单个slot，但结构体占用多个slot。

### 解决方案（已实现）
实现了多slot复制，使用 MEMCPY 指令：

```cpp
if (left_type->isStruct()) {
    int slot_count = left_type->getSlotCount();
    // 计算源地址和目标地址
    genExpressionAddr(expr->getRight());  // 源地址
    genExpressionAddr(expr->getLeft());   // 目标地址
    // 执行内存复制
    code_.emit(OpCode::MEMCPY, slot_count);
}
```

### 当前状态
- ✅ 支持结构体整体赋值
- ✅ 支持嵌套结构体赋值
- ✅ 支持包含数组的结构体赋值
- ✅ 支持函数返回结构体的赋值

### 实现细节
**codegen.cpp:498-544** 完整实现了结构体整体赋值：
1. 检测左边类型是否为结构体
2. 计算源地址和目标地址
3. 使用 MEMCPY 指令复制多个 slot
4. 特殊处理函数返回值的情况
5. 支持连续赋值（p3 = p2 = p1）

### 测试验证
测试文件 `examples/struct/struct_assign.c` 包含：
- 测试1：基本结构体赋值
- 测试2：嵌套结构体赋值
- 测试3：包含数组的结构体赋值
- 测试4：连续赋值

### 教训
- MEMCPY 指令是实现多 slot 复制的有效方案
- 需要特殊处理函数返回值的情况
- 类型系统提供的 getSlotCount() 方法非常关键

---

## 21. 测试文件组织

### 问题
主目录下有大量测试文件，不便管理。

### 解决方案
创建专门的测试目录结构：

```
tests/
  struct/
    test_struct_basic.c           # 基础功能测试
    test_struct_comprehensive.c   # 综合功能测试
    test_struct_advanced.c        # 高级功能测试
    test_type_errors.c            # 错误检测测试
```

### 测试文件设计原则
1. **覆盖度优先**：每个测试覆盖多个相关功能
2. **预期值明确**：注释中标注预期返回值
3. **错误测试集中**：将所有错误用例放在一个文件中
4. **避免不支持的功能**：不测试结构体整体赋值、自引用等

### 测试结果验证
```bash
# 基础测试
./build/simplec tests/struct/test_struct_basic.c
# 预期返回值: 350 ✓

# 综合测试
./build/simplec tests/struct/test_struct_comprehensive.c
# 预期返回值: 610 ✓

# 高级测试
./build/simplec tests/struct/test_struct_advanced.c
# 预期返回值: 995 ✓

# 错误检测
./build/simplec tests/struct/test_type_errors.c -s
# 预期: 发现 10 个语义错误 ✓
```

### 教训
- 测试文件应该有清晰的组织结构
- 每个测试应该有明确的目的和预期结果
- 定期清理临时测试文件

---

## 总结：结构体实现的关键点

### 成功经验
1. **分阶段实现**：Phase 1 基础功能 → Phase 2 指针和数组 → Phase 3 函数参数 → Phase 4 整体赋值
2. **类型解析顺序**：先处理指针，再处理结构体
3. **AST脱糖**：箭头操作符转换为解引用+成员访问
4. **递归处理**：支持任意复杂的嵌套表达式
5. **类型检查**：添加完整的类型兼容性验证
6. **MEMCPY指令**：高效实现多slot复制

### 遇到的挑战
1. ✅ 类型字符串解析的顺序问题
2. ✅ 结构体成员指针类型的解析
3. ✅ 成员访问的多种模式支持
4. ✅ 结构体整体赋值（使用MEMCPY解决）

### 改进方向
1. ✅ 实现结构体整体赋值（已完成）
2. 支持自引用结构体（链表等）
3. ✅ 支持结构体作为函数参数和返回值（已完成）
4. 添加结构体初始化列表（阶段五）

---
