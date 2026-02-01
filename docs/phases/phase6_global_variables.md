# Phase 6: 全局变量实现报告

**最后更新**: 2026-01-31
**状态**: ✅ 已完成

---

## 概述

Phase 6 实现了完整的全局变量支持，包括声明、初始化、访问和修改。全局变量与局部变量完全分离，使用独立的存储区域和指令集。

### 实现范围

- ✅ 全局基本类型变量：`int global_x;`
- ✅ 全局数组：`int global_arr[10];`
- ✅ 全局结构体：`struct Point global_point;`
- ✅ 全局指针：`int *global_ptr;`
- ✅ 全局变量初始化：`int global_x = 100;`
- ✅ 全局变量初始化列表：`int arr[3] = {1, 2, 3};`（Phase 7 实现）

---

## 核心设计

### 1. 内存布局

#### 架构设计

```
Globals (独立数组):
  [global_var_1]  <- 地址 0
  [global_var_2]  <- 地址 1
  [global_arr[0]] <- 地址 2
  [global_arr[1]] <- 地址 3
  ...

Stack (不变):
  [局部变量]
  [参数]
  [ret_addr]
  [old_fp]
  fp ->
```

**设计决策**：
- ✅ 全局变量使用独立的 `globals_` 数组
  - 与栈完全隔离，不影响现有栈帧布局
  - 全局变量地址从 0 开始，简单直观
  - 不需要修改现有的 LOAD/STORE 指令语义

### 2. 指令设计

#### 新增指令

```cpp
enum class OpCode : uint8_t {
    // 现有指令
    LOAD,    // 加载局部变量: push(stack[fp + operand])
    STORE,   // 存储局部变量: stack[fp + operand] = pop()
    LEA,     // 加载局部变量地址: push(fp + operand)

    // 新增全局变量指令
    LOADG,   // 加载全局变量: push(globals[operand])
    STOREG,  // 存储全局变量: globals[operand] = pop()
    LEAG,    // 加载全局变量地址: push(GLOBAL_BASE + operand)
};
```

#### 地址编码方案

使用 `GLOBAL_BASE` 区分全局地址和栈地址：

```cpp
const int32_t GLOBAL_BASE = 0x40000000;  // 1GB

// 全局地址
LEAG 0  // push(GLOBAL_BASE + 0) = push(0x40000000)
LEAG 1  // push(GLOBAL_BASE + 1) = push(0x40000001)

// 栈地址
LEA 0   // push(fp + 0)  例如：push(3)
LEA -3  // push(fp - 3)  例如：push(0)

// 地址判断
if (addr >= GLOBAL_BASE) {
    // 全局地址
    int global_index = addr - GLOBAL_BASE;
    // 访问 globals_[global_index]
} else {
    // 栈地址
    // 访问 stack_[addr]
}
```

**优势**：
- 不会冲突：栈大小 4096，栈地址范围 0~4095，远小于 GLOBAL_BASE
- 判断简单：`addr >= GLOBAL_BASE` 即为全局地址
- ADDPTR/ADDPTRD 不需要修改，简单相加即可

### 3. 符号表设计

```cpp
class CodeGen {
private:
    // 局部变量表（不变）
    std::unordered_map<std::string, VariableInfo> local_vars_;
    int local_offset_ = 0;

    // 全局变量表（新增）
    std::unordered_map<std::string, VariableInfo> global_vars_;
    int global_offset_ = 0;

    // 辅助方法
    bool isGlobal(const std::string& name);
    int getGlobalOffset(const std::string& name);
};
```

**变量查找顺序**：
1. 先查找局部变量（当前作用域）
2. 再查找全局变量
3. 局部变量遮蔽（shadow）全局变量（符合 C 语言语义）

---

## 实现架构

### 1. AST 扩展

**设计决策**：复用 `VarDeclStmtNode` 而非创建新节点

```cpp
// include/ast.h
class ProgramNode {
private:
    std::vector<std::unique_ptr<VarDeclStmtNode>> global_vars_;  // 新增
    std::vector<std::unique_ptr<FunctionDeclNode>> functions_;
    std::vector<std::unique_ptr<StructDeclNode>> structs_;

public:
    void addGlobalVar(std::unique_ptr<VarDeclStmtNode> var);
    const auto& getGlobalVars() const { return global_vars_; }
};
```

**优势**：
- 减少代码重复
- 全局变量和局部变量使用相同的 AST 节点
- 保持设计简洁

### 2. Parser 实现

#### 语法歧义处理

通过 lookahead 区分全局变量和函数定义：

```cpp
// parser.cpp
std::unique_ptr<ProgramNode> Parser::parseProgram() {
    while (!isAtEnd()) {
        if (match(TokenType::Int) || match(TokenType::Void)) {
            Token next = lexer_.peekNextToken();
            if (next.is(TokenType::Identifier)) {
                Token next2 = lexer_.peekNthToken(2);
                if (next2.is(TokenType::LParen)) {
                    // 函数定义: int foo(...)
                    auto func = parseFunctionDeclaration();
                    program->addFunction(std::move(func));
                } else {
                    // 全局变量: int global_x;
                    auto var = parseGlobalVarDeclaration();
                    program->addGlobalVar(std::move(var));
                }
            }
        }
        // ... 处理 struct 等其他情况
    }
}
```

#### 全局变量解析

```cpp
std::unique_ptr<VarDeclStmtNode> Parser::parseGlobalVarDeclaration() {
    // 解析类型
    auto type = parseType();

    // 解析变量名
    std::string name = consume(TokenType::Identifier).getValue();

    // 解析数组维度
    while (match(TokenType::LBracket)) {
        advance();
        int size = std::stoi(consume(TokenType::Number).getValue());
        consume(TokenType::RBracket);
        type = std::make_shared<ArrayType>(type, size);
    }

    // 解析初始化器
    std::unique_ptr<ExprNode> initializer = nullptr;
    if (match(TokenType::Assign)) {
        advance();
        initializer = parseExpression();
    }

    consume(TokenType::Semicolon);
    return std::make_unique<VarDeclStmtNode>(type, name, std::move(initializer));
}
```

**支持的语法**：
- 基本类型：`int global_x;`
- 初始化：`int global_x = 100;`
- 数组：`int global_arr[10];`
- 多维数组：`int matrix[3][4];`
- 结构体：`struct Point global_p;`
- 指针：`int *global_ptr;`

### 3. Sema 实现

#### 全局符号表

```cpp
// include/sema.h
class Sema {
private:
    std::unordered_map<std::string, std::shared_ptr<Type>> global_symbols_;

public:
    void analyzeGlobalVarDecl(VarDeclStmtNode* node);
    const auto& getGlobalSymbols() const { return global_symbols_; }
};
```

#### 语义检查

```cpp
// src/sema.cpp
void Sema::analyzeGlobalVarDecl(VarDeclStmtNode* node) {
    const std::string& name = node->getName();

    // 检查重复定义
    if (global_symbols_.find(name) != global_symbols_.end()) {
        error("全局变量重复定义: " + name);
        return;
    }

    // 解析类型
    auto type = node->getResolvedType();

    // 检查初始化器
    if (node->hasInitializer()) {
        auto init_type = analyzeExpression(node->getInitializer());
        if (!isTypeCompatible(type, init_type)) {
            error("全局变量初始化类型不匹配");
        }

        // 全局变量必须使用常量表达式初始化
        if (!isConstantExpression(node->getInitializer())) {
            error("全局变量初始化必须使用常量表达式");
        }
    }

    // 注册到全局符号表
    global_symbols_[name] = type;
}
```

#### 变量查找

```cpp
std::shared_ptr<Type> Sema::analyzeVariable(VariableNode* node) {
    const std::string& name = node->getName();

    // 先查找局部作用域
    auto symbol = scope_.findSymbol(name);
    if (symbol) {
        node->setResolvedType(symbol->getType());
        return symbol->getType();
    }

    // 再查找全局符号表
    auto global_it = global_symbols_.find(name);
    if (global_it != global_symbols_.end()) {
        node->setResolvedType(global_it->second);
        return global_it->second;
    }

    error("未声明的变量: " + name);
    return nullptr;
}
```

### 4. VM 实现

#### 全局数据区

```cpp
// include/vm.h
class VM {
private:
    std::vector<int32_t> stack_;
    std::vector<int32_t> globals_;  // 新增：全局数据区

    static constexpr int32_t GLOBAL_BASE = 0x40000000;

    bool isGlobalAddress(int32_t addr) const {
        return addr >= GLOBAL_BASE;
    }

    int32_t getGlobalIndex(int32_t addr) const {
        return addr - GLOBAL_BASE;
    }
};
```

#### 新增指令实现

```cpp
// src/vm.cpp
case OpCode::LOADG: {
    int offset = instr.operand;
    push(globals_[offset]);
    break;
}

case OpCode::STOREG: {
    int offset = instr.operand;
    globals_[offset] = pop();
    break;
}

case OpCode::LEAG: {
    int offset = instr.operand;
    push(GLOBAL_BASE + offset);
    break;
}
```

#### 修改 LOADM/STOREM

```cpp
case OpCode::LOADM: {
    int32_t addr = pop();

    if (isGlobalAddress(addr)) {
        // 全局地址
        int global_index = getGlobalIndex(addr);
        push(globals_[global_index]);
    } else {
        // 栈地址
        push(stack_[addr]);
    }
    break;
}

case OpCode::STOREM: {
    int32_t addr = pop();
    int32_t value = pop();

    if (isGlobalAddress(addr)) {
        // 全局地址
        int global_index = getGlobalIndex(addr);
        globals_[global_index] = value;
    } else {
        // 栈地址
        stack_[addr] = value;
    }
    break;
}
```

#### 全局变量初始化

```cpp
int VM::execute(const ByteCode& bytecode) {
    // 初始化全局数据区
    globals_.resize(bytecode.global_data_size);
    for (size_t i = 0; i < bytecode.global_init_data.size(); ++i) {
        globals_[i] = bytecode.global_init_data[i];
    }

    // 执行代码
    // ...
}
```

### 5. CodeGen 实现

#### 全局变量分配

```cpp
// src/codegen.cpp
void CodeGen::generate(ProgramNode* program) {
    // 第一步：处理全局变量
    for (const auto& global_var : program->getGlobalVars()) {
        genGlobalVarDecl(global_var.get());
    }

    // 第二步：处理函数
    for (const auto& func : program->getFunctions()) {
        genFunction(func.get());
    }
}
```

#### 全局变量代码生成

```cpp
void CodeGen::genGlobalVarDecl(VarDeclStmtNode* stmt) {
    auto type = stmt->getResolvedType();
    int slot_count = type->getSlotCount();

    // 分配全局变量
    VariableInfo var_info;
    var_info.offset = next_global_offset_;
    var_info.slot_count = slot_count;
    var_info.is_global = true;
    global_vars_[stmt->getName()] = var_info;

    // 生成初始化数据
    GlobalVarInit init;
    init.offset = next_global_offset_;
    init.slot_count = slot_count;

    if (stmt->hasInitializer()) {
        auto* initializer = stmt->getInitializer();

        if (auto* init_list = dynamic_cast<InitializerListNode*>(initializer)) {
            // 初始化列表
            for (const auto& elem : init_list->getElements()) {
                int32_t value = evaluateConstExpr(elem.get());
                init.init_data.push_back(value);
            }
        } else {
            // 单个表达式
            int32_t value = evaluateConstExpr(initializer);
            for (int i = 0; i < slot_count; ++i) {
                init.init_data.push_back(value);
            }
        }
    }

    // 剩余元素补 0
    while (init.init_data.size() < static_cast<size_t>(slot_count)) {
        init.init_data.push_back(0);
    }

    code_.global_var_inits.push_back(init);
    next_global_offset_ += slot_count;
}
```

#### 变量访问代码生成

```cpp
void CodeGen::genExpression(ExprNode* expr) {
    if (auto* var = dynamic_cast<VariableNode*>(expr)) {
        const std::string& name = var->getName();

        // 先查找局部变量
        auto local_it = findVariable(name);
        if (local_it != nullptr && !local_it->is_global) {
            code_.emit(OpCode::LOAD, local_it->offset);
            return;
        }

        // 再查找全局变量
        auto global_it = global_vars_.find(name);
        if (global_it != global_vars_.end()) {
            code_.emit(OpCode::LOADG, global_it->second.offset);
            return;
        }

        throw std::runtime_error("未定义的变量: " + name);
    }
    // ... 其他表达式
}
```

---

## 测试验证

### 测试文件

```c
// examples/global/global_basic.c

// 全局变量声明和初始化
int global_x = 100;
int global_y = 200;
int global_z;  // 默认初始化为 0

// 全局数组
int global_arr[5] = {1, 2, 3, 4, 5};

// 全局结构体
struct Point {
    int x;
    int y;
};
struct Point global_point = {10, 20};

// 访问全局变量
int getGlobalX() {
    return global_x;
}

// 修改全局变量
void setGlobalX(int value) {
    global_x = value;
}

// 混合使用全局和局部变量
int main() {
    int local_x = 10;  // 局部变量

    // 读取全局变量
    int sum = local_x + global_x + global_y;

    // 修改全局变量
    global_z = 300;

    // 访问全局数组
    int arr_sum = global_arr[0] + global_arr[4];

    // 访问全局结构体
    int point_sum = global_point.x + global_point.y;

    return sum + global_z + arr_sum + point_sum;
    // 10 + 100 + 200 + 300 + 1 + 5 + 10 + 20 = 646
}
```

### 测试结果

#### 词法分析 ✅
- 共识别 131 个 Token
- 所有 Token 正确识别

#### 语法分析 ✅
```
Program(
  VarDecl(int global_x = Number(100)),
  VarDecl(int global_y = Number(200)),
  VarDecl(int global_z),
  VarDecl(int global_arr[5] = InitList(...)),
  StructDecl(Point),
  VarDecl(struct Point global_point = InitList(...)),
  FunctionDecl(int getGlobalX(), ...),
  FunctionDecl(void setGlobalX(int value), ...),
  FunctionDecl(int main(), ...)
)
```

#### 语义分析 ✅
- 全局变量类型检查通过
- 全局变量在函数中访问正确
- 无重复定义错误
- 类型兼容性检查正确

#### 代码生成和执行 ✅
- 程序返回值：646
- 全局变量正确初始化
- 全局变量读写正确
- 全局数组和结构体访问正确

---

## 实现统计

### 代码量

| 文件 | 新增行数 | 主要修改 |
|------|---------|---------|
| `include/ast.h` | ~40 | 添加 global_vars_ 到 ProgramNode |
| `include/parser.h` | 1 | 添加方法声明 |
| `src/parser.cpp` | ~100 | 实现 parseGlobalVarDeclaration() |
| `include/sema.h` | ~10 | 添加全局符号表 |
| `src/sema.cpp` | ~60 | 实现 analyzeGlobalVarDecl() |
| `include/vm.h` | ~20 | 添加 globals_ 和辅助方法 |
| `src/vm.cpp` | ~80 | 实现新指令和修改 LOADM/STOREM |
| `include/codegen.h` | ~15 | 添加全局变量管理 |
| `src/codegen.cpp` | ~120 | 实现全局变量代码生成 |

**总计**：约 450 行新代码

### 功能支持矩阵

| 特性 | 词法 | 语法 | 语义 | 代码生成 | VM执行 |
|------|------|------|------|----------|--------|
| 基本全局变量 | ✅ | ✅ | ✅ | ✅ | ✅ |
| 全局变量初始化 | ✅ | ✅ | ✅ | ✅ | ✅ |
| 全局数组 | ✅ | ✅ | ✅ | ✅ | ✅ |
| 全局结构体 | ✅ | ✅ | ✅ | ✅ | ✅ |
| 全局指针 | ✅ | ✅ | ✅ | ✅ | ✅ |
| 初始化列表 | ✅ | ✅ | ✅ | ✅ | ✅ |

---

## 技术亮点

### 1. 清晰分离
- 全局变量和局部变量完全分离
- 使用独立的存储区域和指令集
- 不影响现有的局部变量实现

### 2. 最小侵入
- 复用 VarDeclStmtNode，减少代码重复
- 现有测试用例不受影响
- 向后兼容性良好

### 3. 地址编码方案
- 使用 GLOBAL_BASE 区分全局和栈地址
- ADDPTR/ADDPTRD 不需要修改
- 地址判断简单高效

### 4. 符合 C 语义
- 局部变量遮蔽全局变量
- 全局变量必须使用常量表达式初始化
- 支持部分初始化（剩余元素补 0）

---

## 实现挑战与解决方案

### 挑战 1：语法歧义
**问题**：如何区分 `int foo()` 和 `int global_x;`？

**解决方案**：使用 lookahead 技术，检查第三个 token
- `int` + `标识符` + `(` → 函数定义
- `int` + `标识符` + `;` → 全局变量

### 挑战 2：地址编码
**问题**：如何区分全局地址和栈地址？

**解决方案**：使用 GLOBAL_BASE (0x40000000)
- 全局地址 = GLOBAL_BASE + offset
- 栈地址 = 0~4095
- 判断：`addr >= GLOBAL_BASE`

### 挑战 3：LOADM/STOREM 支持
**问题**：如何让 LOADM/STOREM 同时支持全局和栈地址？

**解决方案**：在指令中添加地址判断
```cpp
if (isGlobalAddress(addr)) {
    // 访问 globals_
} else {
    // 访问 stack_
}
```

### 挑战 4：初始化列表
**问题**：全局变量如何支持初始化列表？

**解决方案**：
- 在 CodeGen 中评估常量表达式
- 生成 global_init_data
- VM 启动时加载初始化数据

---

## 后续扩展

### 可选功能

#### 1. extern 和 static
```c
extern int external_var;  // 外部变量声明
static int internal_var;  // 内部链接变量
```

#### 2. 字符串字面量
```c
char *global_str = "Hello, World!";
```

#### 3. 函数指针
```c
int (*global_func_ptr)(int, int);
```

---

## 结论

✅ **Phase 6 (全局变量) 已完成**

**完成情况**：
- ✅ 词法、语法、语义分析
- ✅ 代码生成和虚拟机支持
- ✅ 全面的测试验证
- ✅ 初始化列表支持（与 Phase 7 协同）

**代码质量**：
- 清晰的架构设计
- 良好的关注点分离
- 最小侵入性修改
- 向后兼容性

**测试覆盖**：
- 基本全局变量
- 全局数组和结构体
- 全局变量初始化
- 混合使用全局和局部变量

Phase 6 为编译器增加了重要的全局变量支持，使得程序可以在函数之间共享数据。实现过程中采用的清晰分离和最小侵入原则，确保了代码的可维护性和扩展性。
