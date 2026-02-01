# Phase 8: 访问者模式重构

**状态**：⏳ 计划中
**开始日期**：待定
**预计工作量**：4-5 天

---

## 🎯 目标

将 AST 遍历从基于 `dynamic_cast` 的方式重构为**访问者模式**（Visitor Pattern），以提升性能、代码质量和可维护性。

### 核心价值

1. **性能提升**：消除 `dynamic_cast` 的 RTTI 开销，预计提升 5-10 倍
2. **代码质量**：消除重复的 if-else 链，每个节点类型独立处理
3. **可维护性**：添加新操作只需新建 Visitor 子类，不修改节点类
4. **面试亮点**：展示设计模式应用能力和架构设计能力

---

## 📊 性能对比计划

### 基准测试设计

**测试用例**：使用 `examples/comprehensive/comprehensive.c`（最复杂的测试）

**测试指标**：
1. **编译时间**：从源码到字节码的总时间
2. **Sema 阶段时间**：语义分析的时间
3. **CodeGen 阶段时间**：代码生成的时间
4. **内存使用**：峰值内存占用

**测试方法**：
```cpp
// 在 main.cpp 中添加计时代码
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
// ... 执行编译
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "编译时间: " << duration.count() << " μs" << std::endl;
```

**预期结果**：

| 阶段 | 旧实现 (dynamic_cast) | 新实现 (Visitor) | 提升比例 |
|------|----------------------|------------------|----------|
| Lexer + Parser | 311 μs | 311 μs | 无变化 |
| Sema | 151 μs | 待测量 | 预计 5-10x |
| CodeGen | 100 μs | 待测量 | 预计 5-10x |
| VM | 18 μs | 18 μs | 无变化 |
| **总编译时间** | **562 μs** | **待测量** | **预计 3-5x** |

**测试文件**：`examples/struct/struct_comprehensive.c` (99 行，最复杂的测试)
**测试日期**：2026-02-01
**编译选项**：`-O0 -g` (Debug 模式，未优化)
**测试命令**：`./build/simplec examples/struct/struct_comprehensive.c --benchmark`

---

## 🏗️ 实现方案

### 阶段划分

```
阶段 1: 定义访问者接口 (1 天)
  ├─ 创建 ASTVisitor 基类
  ├─ 给所有 AST 节点添加 accept 方法
  ├─ 编译验证
  └─ 性能基准测试（旧实现）

阶段 2: 重构 Sema (1.5 天)
  ├─ Sema 继承 ASTVisitor
  ├─ 实现所有 visit 方法
  ├─ 新旧代码并存验证
  ├─ 删除旧代码
  └─ 运行所有测试

阶段 3: 重构 CodeGen (1.5 天)
  ├─ CodeGen 继承 ASTVisitor
  ├─ 实现所有 visit 方法
  ├─ 新旧代码并存验证
  ├─ 删除旧代码
  └─ 运行所有测试

阶段 4: 性能验证与文档 (0.5 天)
  ├─ 性能对比测试（新实现）
  ├─ 记录性能提升数据
  ├─ 更新文档
  └─ 代码审查
```

---

## 📚 访问者模式原理

### 什么是访问者模式？

访问者模式是一种**行为型设计模式**，它允许在不修改对象结构的前提下，定义作用于这些对象的新操作。

**核心思想**：把"数据结构"和"对数据的操作"分离。

### 经典类比

**传统方式**（当前代码）：
```
每个商店都要自己处理所有访客：
- 顾客来了 → 商店接待
- 清洁工来了 → 商店配合清洁
- 税务员来了 → 商店提供账目
```
每个商店（节点）都要实现所有操作，代码臃肿。

**访问者模式**：
```
商店只负责"接待访客"：
- 顾客来了 → 调用 visitor.visitAsCustomer(this)
- 清洁工来了 → 调用 visitor.visitAsCleaner(this)
- 税务员来了 → 调用 visitor.visitAsAuditor(this)
```
商店（节点）只需要一个 `accept(visitor)` 方法，具体操作由访问者实现。

### 核心组件

#### 1. Visitor 接口（访问者）

```cpp
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // 为每种节点类型定义一个 visit 方法
    virtual void visit(NumberNode* node) = 0;
    virtual void visit(VariableNode* node) = 0;
    virtual void visit(BinaryOpNode* node) = 0;
    // ... 其他节点类型
};
```

#### 2. Element 接口（被访问的元素）

```cpp
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor* visitor) = 0;  // 关键方法
};
```

#### 3. 具体 Element（具体节点）

```cpp
class NumberNode : public ExprNode {
public:
    void accept(ASTVisitor* visitor) override {
        visitor->visit(this);  // 双重分派！
    }
};
```

#### 4. 具体 Visitor（具体访问者）

```cpp
class Sema : public ASTVisitor {
public:
    void visit(NumberNode* node) override {
        // 语义分析：数字节点的类型是 int
        current_type_ = Type::getIntType();
    }

    void visit(BinaryOpNode* node) override {
        // 语义分析：检查二元运算的类型
        node->getLeft()->accept(this);
        auto left_type = current_type_;

        node->getRight()->accept(this);
        auto right_type = current_type_;

        // 类型检查...
    }
};
```

### 关键概念：双重分派（Double Dispatch）

这是访问者模式的核心技巧！

**单次分派**（普通虚函数）：
```cpp
node->someMethod();  // 根据 node 的实际类型调用对应方法
```

**双重分派**（访问者模式）：
```cpp
node->accept(visitor);  // 第一次分派：根据 node 类型
  └─> visitor->visit(this);  // 第二次分派：根据 visitor 类型
```

**为什么需要双重分派？**

因为 C++ 不支持**多态参数**（multimethods）。访问者模式通过两次虚函数调用实现了类似效果。

---

## 🔧 详细实现步骤

### 阶段 1: 定义访问者接口

#### Step 1.1: 性能基准测试（旧实现）

在开始重构前，先测量当前实现的性能：

```bash
# 编译当前版本
make clean && make

# 运行性能测试
./build/simplec examples/comprehensive/comprehensive.c --benchmark

# 记录结果到文档
```

#### Step 1.2: 创建 ASTVisitor 基类

在 `include/ast.h` 的开头添加：

```cpp
// 前向声明所有节点类型
class NumberNode;
class VariableNode;
class BinaryOpNode;
class UnaryOpNode;
class FunctionCallNode;
class ArrayAccessNode;
class MemberAccessNode;
class InitializerListNode;
class VarDeclStmtNode;
class ReturnStmtNode;
class IfStmtNode;
class WhileStmtNode;
class ForStmtNode;
class DoWhileStmtNode;
class BreakStmtNode;
class ContinueStmtNode;
class EmptyStmtNode;
class ExprStmtNode;
class CompoundStmtNode;
class FunctionDeclNode;
class StructDeclNode;
class ProgramNode;

// 访问者接口
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // 表达式节点 (8 个)
    virtual void visit(NumberNode* node) = 0;
    virtual void visit(VariableNode* node) = 0;
    virtual void visit(BinaryOpNode* node) = 0;
    virtual void visit(UnaryOpNode* node) = 0;
    virtual void visit(FunctionCallNode* node) = 0;
    virtual void visit(ArrayAccessNode* node) = 0;
    virtual void visit(MemberAccessNode* node) = 0;
    virtual void visit(InitializerListNode* node) = 0;

    // 语句节点 (11 个)
    virtual void visit(VarDeclStmtNode* node) = 0;
    virtual void visit(ReturnStmtNode* node) = 0;
    virtual void visit(IfStmtNode* node) = 0;
    virtual void visit(WhileStmtNode* node) = 0;
    virtual void visit(ForStmtNode* node) = 0;
    virtual void visit(DoWhileStmtNode* node) = 0;
    virtual void visit(BreakStmtNode* node) = 0;
    virtual void visit(ContinueStmtNode* node) = 0;
    virtual void visit(EmptyStmtNode* node) = 0;
    virtual void visit(ExprStmtNode* node) = 0;
    virtual void visit(CompoundStmtNode* node) = 0;

    // 顶层节点 (3 个)
    virtual void visit(FunctionDeclNode* node) = 0;
    virtual void visit(StructDeclNode* node) = 0;
    virtual void visit(ProgramNode* node) = 0;
};
```

**总计**：22 个 visit 方法

#### Step 1.3: 给 ASTNode 基类添加 accept 方法

```cpp
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
    virtual void accept(ASTVisitor* visitor) = 0;  // 新增
};
```

#### Step 1.4: 给每个具体节点实现 accept

**模板代码**（所有节点都一样）：

```cpp
void accept(ASTVisitor* visitor) override {
    visitor->visit(this);
}
```

**需要修改的节点**（22 个）：
- 表达式节点：NumberNode, VariableNode, BinaryOpNode, UnaryOpNode, FunctionCallNode, ArrayAccessNode, MemberAccessNode, InitializerListNode
- 语句节点：VarDeclStmtNode, ReturnStmtNode, IfStmtNode, WhileStmtNode, ForStmtNode, DoWhileStmtNode, BreakStmtNode, ContinueStmtNode, EmptyStmtNode, ExprStmtNode, CompoundStmtNode
- 顶层节点：FunctionDeclNode, StructDeclNode, ProgramNode

#### Step 1.5: 编译验证

```bash
make clean && make
# 确保编译通过，但此时还不使用访问者模式
```

---

### 阶段 2: 重构 Sema

#### Step 2.1: Sema 继承 ASTVisitor

```cpp
// include/sema.h
class Sema : public ASTVisitor {  // 继承 ASTVisitor
private:
    Scope scope_;
    std::shared_ptr<Type> current_function_return_type_;
    std::vector<std::string> errors_;

    // 新增：用于存储当前表达式的类型
    std::shared_ptr<Type> current_expr_type_;

    // 新增：用于存储全局符号
    std::unordered_map<std::string, std::shared_ptr<Symbol>> global_symbols_;

public:
    bool analyze(ProgramNode* program);

    // 实现 ASTVisitor 接口（22 个方法）
    void visit(NumberNode* node) override;
    void visit(VariableNode* node) override;
    void visit(BinaryOpNode* node) override;
    void visit(UnaryOpNode* node) override;
    void visit(FunctionCallNode* node) override;
    void visit(ArrayAccessNode* node) override;
    void visit(MemberAccessNode* node) override;
    void visit(InitializerListNode* node) override;

    void visit(VarDeclStmtNode* node) override;
    void visit(ReturnStmtNode* node) override;
    void visit(IfStmtNode* node) override;
    void visit(WhileStmtNode* node) override;
    void visit(ForStmtNode* node) override;
    void visit(DoWhileStmtNode* node) override;
    void visit(BreakStmtNode* node) override;
    void visit(ContinueStmtNode* node) override;
    void visit(EmptyStmtNode* node) override;
    void visit(ExprStmtNode* node) override;
    void visit(CompoundStmtNode* node) override;

    void visit(FunctionDeclNode* node) override;
    void visit(StructDeclNode* node) override;
    void visit(ProgramNode* node) override;

private:
    void error(const std::string& msg);
    bool isTypeCompatible(std::shared_ptr<Type> t1, std::shared_ptr<Type> t2);
    // ... 其他辅助方法
};
```

#### Step 2.2: 实现所有 visit 方法

**关键实现示例**：

```cpp
// src/sema.cpp

void Sema::visit(NumberNode* node) {
    current_expr_type_ = Type::getIntType();
    node->setResolvedType(current_expr_type_);
}

void Sema::visit(VariableNode* node) {
    auto symbol = scope_.findSymbol(node->getName());
    if (!symbol) {
        error("未定义的变量: " + node->getName());
        current_expr_type_ = nullptr;
        return;
    }
    current_expr_type_ = symbol->getType();
    node->setResolvedType(current_expr_type_);
}

void Sema::visit(BinaryOpNode* node) {
    // 递归访问左右子节点
    node->getLeft()->accept(this);
    auto left_type = current_expr_type_;

    node->getRight()->accept(this);
    auto right_type = current_expr_type_;

    // 类型检查逻辑...

    node->setResolvedType(current_expr_type_);
}

void Sema::visit(CompoundStmtNode* node) {
    scope_.enterScope();
    for (const auto& stmt : node->getStatements()) {
        stmt->accept(this);  // 使用访问者模式递归
    }
    scope_.exitScope();
}
```

#### Step 2.3: 新旧代码并存验证

保留旧的 `analyzeExpression` 等方法，添加注释：

```cpp
// TODO: 删除 - 已被访问者模式替代
std::shared_ptr<Type> Sema::analyzeExpression(ExprNode* expr) {
    // 旧实现...
}
```

同时添加新的入口：

```cpp
std::shared_ptr<Type> Sema::analyzeExpressionNew(ExprNode* expr) {
    expr->accept(this);
    return current_expr_type_;
}
```

运行测试，对比结果。

#### Step 2.4: 删除旧代码

确认新实现正确后，删除所有旧方法：
- `analyzeExpression(ExprNode*)`
- `analyzeBinaryOp(BinaryOpNode*)`
- `analyzeUnaryOp(UnaryOpNode*)`
- `analyzeFunctionCall(FunctionCallNode*)`
- `analyzeArrayAccess(ArrayAccessNode*)`
- `analyzeMemberAccess(MemberAccessNode*)`

#### Step 2.5: 运行所有测试

```bash
# 运行所有测试用例
for test in examples/*/*.c; do
    echo "Testing $test..."
    ./build/simplec "$test" || exit 1
done

echo "✓ 所有测试通过"
```

---

### 阶段 3: 重构 CodeGen

#### Step 3.1: CodeGen 继承 ASTVisitor

```cpp
// include/codegen.h
class CodeGen : public ASTVisitor {
private:
    ByteCode code_;
    std::unordered_map<std::string, VariableInfo> variables_;
    int next_local_offset_ = 0;
    // ... 其他成员

public:
    ByteCode generate(ProgramNode* program);

    // 实现 ASTVisitor 接口（22 个方法）
    void visit(NumberNode* node) override;
    void visit(VariableNode* node) override;
    void visit(BinaryOpNode* node) override;
    // ... 其他 visit 方法

private:
    void emit(OpCode op, int32_t operand = 0);
    VariableInfo* findVariable(const std::string& name);
    // ... 其他辅助方法
};
```

#### Step 3.2: 实现所有 visit 方法

**关键实现示例**：

```cpp
// src/codegen.cpp

void CodeGen::visit(NumberNode* node) {
    emit(OpCode::PUSH, node->getValue());
}

void CodeGen::visit(VariableNode* node) {
    auto* info = findVariable(node->getName());
    if (info->is_global) {
        emit(OpCode::LOADG, info->offset);
    } else {
        emit(OpCode::LOAD, info->offset);
    }
}

void CodeGen::visit(BinaryOpNode* node) {
    // 赋值运算符特殊处理
    if (node->getOperator() == TokenType::Assign) {
        // ... 赋值逻辑
        return;
    }

    // 其他二元运算符
    node->getLeft()->accept(this);
    node->getRight()->accept(this);

    switch (node->getOperator()) {
        case TokenType::Plus:    emit(OpCode::ADD); break;
        case TokenType::Minus:   emit(OpCode::SUB); break;
        case TokenType::Multiply: emit(OpCode::MUL); break;
        case TokenType::Divide:  emit(OpCode::DIV); break;
        case TokenType::Modulo:  emit(OpCode::MOD); break;
        // ... 其他运算符
    }
}

void CodeGen::visit(CompoundStmtNode* node) {
    for (const auto& stmt : node->getStatements()) {
        stmt->accept(this);
    }
}
```

#### Step 3.3: 新旧代码并存验证

类似 Sema，保留旧代码并添加新实现，对比验证。

#### Step 3.4: 删除旧代码

删除所有旧的生成方法：
- `genExpression(ExprNode*)`
- `genBinaryOp(BinaryOpNode*)`
- `genUnaryOp(UnaryOpNode*)`
- 等等...

#### Step 3.5: 运行所有测试

再次运行所有测试，确保功能正确。

---

### 阶段 4: 性能验证与文档

#### Step 4.1: 性能对比测试

```bash
# 测试新实现的性能
./build/simplec examples/comprehensive/comprehensive.c --benchmark

# 对比旧实现的基准数据
```

#### Step 4.2: 记录性能数据

在本文档中更新性能对比表格。

#### Step 4.3: 更新相关文档

- 更新 `development-plan.md`：标记 Phase 8 完成
- 更新 `dev-notes.md`：记录重构过程中的问题和解决方案
- 更新 `.Claude.md`：更新项目状态

#### Step 4.4: 代码审查

检查代码质量：
- 是否所有 visit 方法都实现了？
- 是否删除了所有旧代码？
- 是否有遗漏的 dynamic_cast？
- 代码风格是否一致？

---

## ✅ 验收标准

### 功能验收

- [ ] 所有现有测试用例通过
- [ ] 编译器行为与重构前完全一致
- [ ] 没有引入新的 bug

### 性能验收

- [ ] Sema 阶段性能提升 > 3x
- [ ] CodeGen 阶段性能提升 > 3x
- [ ] 总编译时间提升 > 2x

### 代码质量验收

- [ ] 没有 dynamic_cast（除了必要的地方）
- [ ] 所有 AST 节点都实现了 accept
- [ ] Sema 和 CodeGen 都继承 ASTVisitor
- [ ] 代码结构清晰，易于理解

### 文档验收

- [ ] 性能对比数据完整
- [ ] 实现过程记录详细
- [ ] 遇到的问题和解决方案已记录

---

## 🎯 访问者模式的优势

### 1. 性能提升

**理论分析**：

```
dynamic_cast 方式：
- 每次调用需要 RTTI 查询
- 时间复杂度：O(depth of inheritance tree)
- 估计：每次 50-100 CPU 周期

访问者模式：
- 虚函数调用（vtable 查找）
- 时间复杂度：O(1)
- 估计：每次 5-10 CPU 周期

理论提升：5-10倍
```

**实际测试**：待测量

### 2. 代码清晰度

**对比**：

```cpp
// 旧代码：巨大的 if-else 链（约 200 行）
std::shared_ptr<Type> Sema::analyzeExpression(ExprNode* expr) {
    if (auto* num = dynamic_cast<NumberNode*>(expr)) { ... }
    if (auto* var = dynamic_cast<VariableNode*>(expr)) { ... }
    if (auto* bin = dynamic_cast<BinaryOpNode*>(expr)) { ... }
    // ... 20+ 个 if
}

// 新代码：每个节点独立处理（每个方法 10-30 行）
void Sema::visit(NumberNode* node) { ... }
void Sema::visit(VariableNode* node) { ... }
void Sema::visit(BinaryOpNode* node) { ... }
```

### 3. 易于扩展

**添加新操作**：只需新建一个 Visitor 子类

```cpp
// 添加优化器
class Optimizer : public ASTVisitor {
    void visit(BinaryOpNode* node) override {
        // 常量折叠：2 + 3 -> 5
    }
};

// 添加 AST 打印器
class ASTPrinter : public ASTVisitor {
    void visit(BinaryOpNode* node) override {
        std::cout << "BinaryOp(" << ... << ")";
    }
};
```

### 4. 类型安全

编译器会检查是否实现了所有 visit 方法，不会漏掉节点类型。

---

## 🚨 风险与应对

### 风险 1: 破坏现有功能

**应对**：
- 新旧代码并存一段时间
- 每个阶段都运行完整测试
- 使用 git 分支，随时可以回退

### 风险 2: 性能提升不明显

**应对**：
- 先做基准测试，确认瓶颈
- 如果提升不明显，分析原因
- 即使性能提升小，代码质量提升也是值得的

### 风险 3: 工作量超出预期

**应对**：
- 分阶段实施，每个阶段独立
- 可以先完成 Sema，CodeGen 后续再做
- 保持灵活，根据实际情况调整

---

## 📝 开发笔记

### 遇到的问题

（待填写）

### 解决方案

（待填写）

### 经验教训

（待填写）

---

## 📚 参考资料

### 设计模式

- 《设计模式：可复用面向对象软件的基础》- Visitor Pattern
- 《Effective C++》- Item 31: Make functions virtual with respect to more than one object

### 编译器实现

- LLVM 源码：大量使用访问者模式遍历 IR
- GCC 源码：tree-walker 机制

### 性能分析

- `dynamic_cast` 的实现原理和性能开销
- 虚函数调用的性能特性

---

**最后更新**：2026-02-01
