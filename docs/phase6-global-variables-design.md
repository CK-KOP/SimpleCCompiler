# Phase 6: 全局变量实现设计方案

**文档版本**: v1.0
**创建日期**: 2026-01-20
**状态**: 设计阶段

---

## 1. 概述

### 1.1 目标

实现全局变量支持，包括：
- 全局基本类型变量 (`int global_x;`)
- 全局数组 (`int global_arr[10];`)
- 全局结构体 (`struct Point global_point;`)
- 全局变量初始化 (`int global_x = 100;`)

### 1.2 设计原则

1. **最小侵入性**：尽量不破坏现有的局部变量实现
2. **清晰分离**：全局变量和局部变量使用不同的存储区域和指令
3. **向后兼容**：现有测试用例不受影响
4. **渐进实现**：先实现基本功能，再扩展初始化

---

## 2. 核心设计决策

### 2.1 内存布局设计

#### 当前架构（仅局部变量）
```
Stack:
  [局部变量]
  [参数]
  [ret_addr]
  [old_fp]
  fp ->
```

#### 新架构（全局变量 + 局部变量）
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

**关键决策**：
- ✅ **全局变量使用独立的 `globals_` 数组**
  - 优点：与栈完全隔离，不影响现有栈帧布局
  - 优点：全局变量地址从 0 开始，简单直观
  - 优点：不需要修改现有的 LOAD/STORE 指令语义
- ❌ 不使用栈底存储全局变量
  - 缺点：会影响栈帧布局，需要修改所有偏移计算
  - 缺点：全局变量和局部变量混在一起，难以区分

### 2.2 指令设计

#### 新增指令

```cpp
enum class OpCode : uint8_t {
    // 现有指令...
    LOAD,    // 加载局部变量: push(stack[fp + operand])
    STORE,   // 存储局部变量: stack[fp + operand] = pop()
    LEA,     // 加载局部变量地址: push(fp + operand)

    // 新增全局变量指令
    LOADG,   // 加载全局变量: push(globals[operand])
    STOREG,  // 存储全局变量: globals[operand] = pop()
    LEAG,    // 加载全局变量地址: push(GLOBAL_BASE + operand)
             // GLOBAL_BASE 是一个特殊标记，用于区分全局地址和栈地址
};
```

**关键决策**：
- ✅ **使用独立的 LOADG/STOREG/LEAG 指令**
  - 优点：语义清晰，全局变量和局部变量完全分离
  - 优点：不需要修改现有指令的实现
  - 优点：便于调试和理解
- ❌ 不复用 LOAD/STORE 指令
  - 缺点：需要在运行时判断地址是全局还是局部
  - 缺点：增加指令执行的复杂度

#### LEAG 指令的地址表示

**问题**：全局变量地址和栈地址如何区分？

**方案1：使用负数表示全局地址**（❌ 不可行）
```cpp
// 全局地址：负数，绝对值是 globals 数组的索引
// 例如：-1 表示 globals[0], -2 表示 globals[1]
LEAG 0  // push(-1)  表示 globals[0] 的地址
LEAG 1  // push(-2)  表示 globals[1] 的地址

// 栈地址：正数，表示相对 fp 的偏移
LEA 0   // push(fp + 0)
LEA 1   // push(fp + 1)
```

**为什么不可行**：
- ❌ **与参数地址冲突**：参数使用负偏移（fp - 3, fp - 4, ...）
- ❌ 当 fp = 3 时，参数地址 = fp - 3 = 0，可能是负数或小正数
- ❌ 无法区分 "全局地址 -1" 和 "栈地址 fp - 4 = -1"

**方案2：使用 GLOBAL_BASE 表示全局地址**（✅ 推荐）
```cpp
const int32_t GLOBAL_BASE = 0x40000000;  // 1GB = 1,073,741,824
LEAG 0  // push(GLOBAL_BASE + 0) = push(0x40000000)
LEAG 1  // push(GLOBAL_BASE + 1) = push(0x40000001)

// 栈地址：小的正数或负数
LEA 0   // push(fp + 0)  例如：push(3)
LEA -3  // push(fp - 3)  例如：push(0)
```

**为什么可行**：
- ✅ **不会冲突**：栈大小 4096，栈地址范围 0~4095，远小于 GLOBAL_BASE
- ✅ **判断简单**：`addr >= GLOBAL_BASE` 即为全局地址
- ✅ **符合真实架构**：类似真实 CPU 的段基址或绝对地址

### 2.3 符号表设计

#### 当前架构
```cpp
class CodeGen {
private:
    std::unordered_map<std::string, int> locals_;  // 局部变量表
    int local_offset_ = 0;
};
```

#### 新架构
```cpp
class CodeGen {
private:
    // 局部变量表（不变）
    std::unordered_map<std::string, int> locals_;
    int local_offset_ = 0;

    // 全局变量表（新增）
    std::unordered_map<std::string, int> globals_;
    int global_offset_ = 0;

    // 辅助方法
    bool isGlobal(const std::string& name);
    int getGlobal(const std::string& name);
};
```

**关键决策**：
- ✅ **全局变量表和局部变量表完全分离**
  - 优点：清晰的作用域区分
  - 优点：不会相互干扰
- ✅ **全局变量在 `generate()` 开始时分配，局部变量在 `genFunction()` 中分配**
  - 优点：全局变量在所有函数之前初始化
  - 优点：符合 C 语言的语义

---

## 3. 实现难点与解决方案

### 3.1 难点1：LOADM/STOREM 指令需要支持全局地址

**问题描述**：
```c
int global_arr[10];
int main() {
    global_arr[0] = 42;  // 需要通过地址访问全局数组
}
```

生成的代码：
```
PUSH 42           // 值
LEAG 0            // 全局数组基地址 -> push(-1)
PUSH 0            // 索引
ADDPTRD 1         // 计算地址 -> pop(0), pop(-1), push(-1 + 0*1) = push(-1)
STOREM            // 存储：addr = pop(-1), value = pop(42), stack[addr] = value
```

**问题**：STOREM 指令当前假设地址是栈地址，直接使用 `stack_[addr]`。但全局地址是负数，需要特殊处理。

**解决方案**：
```cpp
case OpCode::STOREM: {
    int32_t addr = pop();
    int32_t value = pop();

    if (addr < 0) {
        // 全局地址：负数，绝对值是 globals 数组的索引
        int global_index = -(addr + 1);  // -1 -> 0, -2 -> 1
        globals_[global_index] = value;
    } else {
        // 栈地址：正数
        stack_[addr] = value;
    }
    break;
}

case OpCode::LOADM: {
    int32_t addr = pop();

    if (addr < 0) {
        // 全局地址
        int global_index = -(addr + 1);
        push(globals_[global_index]);
    } else {
        // 栈地址
        push(stack_[addr]);
    }
    break;
}
```

**影响范围**：
- `vm.cpp`: 修改 LOADM/STOREM 指令的实现
- **不影响现有代码**：局部变量的地址始终是正数，行为不变

### 3.2 难点2：ADDPTR/ADDPTRD 指令需要保持地址的全局/局部属性

**问题描述**：
```c
int global_arr[10];
int main() {
    global_arr[5] = 42;  // 地址计算：base(-1) + offset(5) = ?
}
```

**当前实现**：
```cpp
case OpCode::ADDPTR: {
    int32_t addr = pop();
    push(addr + instr.operand);  // 简单相加
    break;
}
```

**问题**：
- 全局地址 -1 + 偏移 5 = 4（正数），被误认为是栈地址！

**解决方案**：
```cpp
case OpCode::ADDPTR: {
    int32_t addr = pop();
    int32_t offset = instr.operand;

    if (addr < 0) {
        // 全局地址：保持负数形式
        // -1 + offset -> -(1 - offset) = -(1 + offset) + offset
        // 实际上：globals[0 + offset]
        int global_index = -(addr + 1);
        push(-(global_index + offset + 1));
    } else {
        // 栈地址：正常相加
        push(addr + offset);
    }
    break;
}
```

**更简单的方案**：
使用特殊的地址编码，避免负数运算的复杂性。

**重新设计地址编码**（推荐）：
```cpp
// 地址编码：
// - 栈地址：直接使用栈索引（0, 1, 2, ...）
// - 全局地址：使用 GLOBAL_BASE + 索引
const int GLOBAL_BASE = 0x40000000;  // 1GB，足够大

LEAG 0  // push(GLOBAL_BASE + 0)
LEAG 1  // push(GLOBAL_BASE + 1)

// LOADM/STOREM 判断：
if (addr >= GLOBAL_BASE) {
    int global_index = addr - GLOBAL_BASE;
    // 访问 globals_[global_index]
} else {
    // 访问 stack_[addr]
}

// ADDPTR 不需要特殊处理：
push(addr + offset);  // 无论全局还是局部，都是简单相加
```

**最终决策**：使用 GLOBAL_BASE 方案
- 优点：ADDPTR/ADDPTRD 不需要修改
- 优点：地址计算简单直观
- 缺点：需要定义 GLOBAL_BASE 常量

### 3.3 难点3：全局变量初始化

**问题描述**：
```c
int global_x = 100;
int global_arr[3] = {1, 2, 3};  // 暂不支持初始化列表
```

**分阶段实现**：

**阶段1：仅支持声明，不支持初始化**
```c
int global_x;  // 初始化为 0
int global_arr[10];  // 所有元素初始化为 0
```

**阶段2：支持常量初始化**
```c
int global_x = 100;  // 编译时常量
```

**实现方案**：
```cpp
// ByteCode 添加全局初始化数据
class ByteCode {
public:
    std::vector<int32_t> global_init_data;  // 全局变量初始值
    int global_data_size = 0;
};

// CodeGen 生成全局初始化数据
void CodeGen::genGlobalVarDecl(GlobalVarDeclNode* node) {
    int slot_count = node->getType()->getSlotCount();

    if (node->hasInitializer()) {
        // 计算初始值（必须是编译时常量）
        int init_value = evaluateConstExpr(node->getInitializer());
        for (int i = 0; i < slot_count; ++i) {
            code_.global_init_data.push_back(init_value);
        }
    } else {
        // 默认初始化为 0
        for (int i = 0; i < slot_count; ++i) {
            code_.global_init_data.push_back(0);
        }
    }

    globals_[node->getName()] = global_offset_;
    global_offset_ += slot_count;
}

// VM 初始化全局数据
int VM::execute(const ByteCode& bytecode) {
    // 初始化全局数据区
    globals_.resize(bytecode.global_data_size);
    for (size_t i = 0; i < bytecode.global_init_data.size(); ++i) {
        globals_[i] = bytecode.global_init_data[i];
    }

    // ... 执行代码
}
```

**阶段3：支持初始化列表**（Phase 5 完成后）
```c
int global_arr[3] = {1, 2, 3};
```

### 3.4 难点4：语法歧义已解决，但需要扩展

**当前状态**：
```cpp
// parser.cpp:395-403
else if (next1.is(TokenType::Identifier) && next2.is(TokenType::Identifier)) {
    Token next3 = lexer_.peekNthToken(3);
    if (next3.is(TokenType::LParen)) {
        // 函数定义
        auto func = parseFunctionDeclaration();
        program->addFunction(std::move(func));
    } else {
        // 全局变量声明（暂不支持）
        throw std::runtime_error("暂不支持全局变量声明");
    }
}
```

**需要扩展的情况**：
```c
struct Point global_point;        // next3 = Semicolon
struct Point global_arr[10];      // next3 = LBracket
struct Point *global_ptr;         // next2 = Multiply
```

**解决方案**：
```cpp
else if (next1.is(TokenType::Identifier) && next2.is(TokenType::Identifier)) {
    Token next3 = lexer_.peekNthToken(3);
    if (next3.is(TokenType::LParen)) {
        // 函数定义
        auto func = parseFunctionDeclaration();
        program->addFunction(std::move(func));
    } else if (next3.is(TokenType::Semicolon) || next3.is(TokenType::LBracket) || next3.is(TokenType::Assign)) {
        // 全局变量声明
        auto global_var = parseGlobalVarDeclaration();
        program->addGlobalVar(std::move(global_var));
    } else {
        throw std::runtime_error("未知的全局声明");
    }
} else if (next1.is(TokenType::Identifier) && next2.is(TokenType::Multiply)) {
    // struct Point *global_ptr;
    auto global_var = parseGlobalVarDeclaration();
    program->addGlobalVar(std::move(global_var));
}
```

---

## 4. 实现计划

### 4.1 实现步骤

#### 步骤1：AST 扩展
- [ ] 添加 `GlobalVarDeclNode` 类
- [ ] 修改 `ProgramNode`，添加 `global_vars_` 和 `addGlobalVar()` 方法

#### 步骤2：Parser 扩展
- [ ] 修改 `parseProgram()`，处理全局变量声明
- [ ] 添加 `parseGlobalVarDeclaration()` 方法
- [ ] 处理各种全局变量语法：基本类型、数组、结构体、指针

#### 步骤3：Sema 扩展
- [ ] 添加全局符号表 `global_vars_`
- [ ] 添加 `analyzeGlobalVarDecl()` 方法
- [ ] 修改 `analyzeVariable()`，支持查找全局变量
- [ ] 检查全局变量重复定义

#### 步骤4：VM 扩展
- [ ] 添加 `globals_` 数组
- [ ] 定义 `GLOBAL_BASE` 常量
- [ ] 添加 LOADG/STOREG/LEAG 指令
- [ ] 修改 LOADM/STOREM 指令，支持全局地址
- [ ] 添加全局数据初始化逻辑

#### 步骤5：CodeGen 扩展
- [ ] 添加 `globals_` 符号表和 `global_offset_`
- [ ] 添加 `genGlobalVarDecl()` 方法
- [ ] 修改 `generate()`，先处理全局变量
- [ ] 修改 `genExpression()`，区分全局和局部变量
- [ ] 修改 `genBinaryOp()`，支持全局变量赋值
- [ ] 添加 `isGlobal()` 和 `getGlobal()` 辅助方法

#### 步骤6：测试
- [ ] 编写基本全局变量测试
- [ ] 编写全局数组测试
- [ ] 编写全局结构体测试
- [ ] 编写全局变量和局部变量混合测试
- [ ] 验证现有测试用例不受影响

### 4.2 实现顺序建议

**第一阶段：基础设施**
1. VM 扩展（LOADG/STOREG/LEAG 指令）
2. AST 扩展（GlobalVarDeclNode）
3. ByteCode 扩展（global_data_size）

**第二阶段：基本功能**
4. Parser 扩展（parseGlobalVarDeclaration）
5. Sema 扩展（全局符号表）
6. CodeGen 扩展（genGlobalVarDecl）

**第三阶段：测试验证**
7. 编写测试用例
8. 调试和修复问题

---

## 5. 潜在风险与缓解措施

### 5.1 风险1：破坏现有局部变量功能

**风险描述**：修改 LOADM/STOREM 可能影响现有代码

**缓解措施**：
- 使用 GLOBAL_BASE 方案，确保局部地址（< GLOBAL_BASE）行为不变
- 在修改前运行所有现有测试用例
- 在修改后再次运行所有测试用例

### 5.2 风险2：地址计算错误

**风险描述**：全局数组的地址计算可能出错

**缓解措施**：
- 编写详细的测试用例，覆盖各种地址计算场景
- 使用 debug 模式打印地址计算过程
- 参考现有的局部数组实现

### 5.3 风险3：符号表冲突

**风险描述**：全局变量和局部变量同名

**缓解措施**：
- 在 Sema 中检查全局变量和局部变量的作用域
- 局部变量优先于全局变量（符合 C 语言语义）
- 在 CodeGen 中先查找局部变量，再查找全局变量

### 5.4 风险4：初始化顺序问题

**风险描述**：全局变量初始化顺序不确定

**缓解措施**：
- 第一阶段不支持初始化，所有全局变量初始化为 0
- 第二阶段仅支持常量初始化，避免依赖关系
- 明确文档说明不支持全局变量之间的依赖初始化

---

## 6. 向后兼容性

### 6.1 现有代码不受影响

- 所有现有测试用例不使用全局变量，因此不受影响
- 局部变量的实现完全不变
- 新增的指令和数据结构不影响现有功能

### 6.2 新旧代码可以共存

```c
// 全局变量
int global_x = 100;

// 函数使用局部变量和全局变量
int main() {
    int local_x = 10;  // 局部变量
    return local_x + global_x;  // 混合使用
}
```

---

## 7. 未来扩展

### 7.1 Phase 5 完成后：支持全局变量初始化列表

```c
int global_arr[5] = {1, 2, 3, 4, 5};
struct Point global_point = {10, 20};
```

### 7.2 可选扩展：支持 extern 和 static

```c
extern int external_var;  // 外部变量声明
static int internal_var;  // 内部链接变量
```

---

## 8. 关键代码示例

### 8.1 VM 地址判断

```cpp
const int GLOBAL_BASE = 0x40000000;

bool isGlobalAddress(int32_t addr) {
    return addr >= GLOBAL_BASE;
}

int32_t getGlobalIndex(int32_t addr) {
    return addr - GLOBAL_BASE;
}

int32_t makeGlobalAddress(int32_t index) {
    return GLOBAL_BASE + index;
}
```

### 8.2 CodeGen 变量查找

```cpp
void CodeGen::genExpression(ExprNode* expr) {
    if (auto* var = dynamic_cast<VariableNode*>(expr)) {
        // 先查找局部变量
        if (locals_.find(var->getName()) != locals_.end()) {
            int offset = getLocal(var->getName());
            code_.emit(OpCode::LOAD, offset);
        }
        // 再查找全局变量
        else if (globals_.find(var->getName()) != globals_.end()) {
            int offset = getGlobal(var->getName());
            code_.emit(OpCode::LOADG, offset);
        }
        else {
            throw std::runtime_error("未定义的变量: " + var->getName());
        }
    }
}
```

---

## 9. 总结

### 9.1 设计亮点

1. **清晰分离**：全局变量和局部变量完全分离，不相互干扰
2. **最小侵入**：现有代码几乎不需要修改
3. **易于扩展**：为未来的初始化列表和 extern/static 留下空间
4. **符合语义**：符合 C 语言的全局变量语义

### 9.2 实现复杂度

- **AST 扩展**：简单（1 个新节点）
- **Parser 扩展**：中等（需要处理多种语法）
- **Sema 扩展**：简单（添加全局符号表）
- **VM 扩展**：中等（3 个新指令 + 地址判断）
- **CodeGen 扩展**：中等（全局变量生成 + 变量查找）

**总体评估**：中等复杂度，预计需要 200-300 行新代码。

### 9.3 建议

1. **先实现基本功能**：不支持初始化，所有全局变量初始化为 0
2. **充分测试**：每个步骤都编写测试用例
3. **渐进式开发**：按照实现步骤逐步完成，每步都验证
4. **保持简单**：避免过度设计，先实现核心功能

---

## 10. 待讨论问题

### 10.1 地址编码方案

**问题**：使用负数还是 GLOBAL_BASE？

**建议**：使用 GLOBAL_BASE（0x40000000）
- 理由：ADDPTR/ADDPTRD 不需要修改
- 理由：地址计算更简单

### 10.2 初始化支持

**问题**：第一阶段是否支持常量初始化？

**建议**：第一阶段不支持，所有全局变量初始化为 0
- 理由：简化实现，降低风险
- 理由：可以在第二阶段添加

### 10.3 全局变量作用域

**问题**：全局变量和局部变量同名时如何处理？

**建议**：局部变量优先（符合 C 语言语义）
- 理由：符合程序员的直觉
- 理由：实现简单（先查局部，再查全局）

---

**文档结束**
