# Phase 6: 全局变量实现进度记录

**更新日期**: 2026-01-25
**状态**: 第一阶段完成（词法/语法/语义分析）

---

## ✅ 已完成的工作

### 1. AST 扩展（2026-01-25）

**设计决策**：复用 `VarDeclStmtNode` 而非创建新的节点类

**实现内容**：
- 在 `ProgramNode` 中添加 `global_vars_` 列表
  ```cpp
  std::vector<std::unique_ptr<VarDeclStmtNode>> global_vars_;
  ```
- 添加 `addGlobalVar()` 方法
- 修改 `toString()` 方法以显示全局变量

**优势**：
- 减少代码重复
- 保持设计简洁
- 全局变量和局部变量使用相同的AST节点

**修改文件**：
- `include/ast.h`

---

### 2. Parser 扩展（2026-01-25）

**实现内容**：
- 在 `parser.h` 中添加 `parseGlobalVarDeclaration()` 方法声明
- 在 `parser.cpp` 中实现完整的解析逻辑：
  - 支持基本类型：`int global_x;`
  - 支持初始化：`int global_x = 100;`
  - 支持数组：`int global_arr[10];`（包括多维数组）
  - 支持结构体：`struct Point global_p;`
  - 支持指针：`int *global_ptr;` 和 `struct Point *global_ptr;`

**关键实现**：
- 修改 `parseProgram()` 以区分全局变量和函数定义
  - `int foo(...)` → 函数定义
  - `int global_x;` → 全局变量声明
- 复用 `parseVariableDeclaration()` 的逻辑模式
- 使用 lookahead 技术判断声明类型

**修改文件**：
- `include/parser.h`
- `src/parser.cpp`

---

### 3. Sema 扩展（2026-01-25）

**实现内容**：
- 添加全局符号表 `global_symbols_`
  ```cpp
  std::unordered_map<std::string, std::shared_ptr<Type>> global_symbols_;
  ```
- 实现 `analyzeGlobalVarDecl()` 方法：
  - 检查全局变量重复定义
  - 解析变量类型（包括数组、结构体、指针）
  - 支持初始化表达式的类型检查
- 修改 `analyzeVariable()` 方法：
  - 先查找局部作用域
  - 再查找全局符号表
  - 支持全局变量在函数中的访问
- 添加 `getGlobalSymbols()` 方法供 CodeGen 使用

**实现细节**：
- 全局变量在函数定义之前分析
- 全局变量不支持前向声明
- 局部变量优先于全局变量（符合 C 语言语义）

**修改文件**：
- `include/sema.h`
- `src/sema.cpp`

---

## ✅ 测试验证

### 测试文件：`examples/global/global_basic.c`

**测试内容**：
- 基本全局变量声明
- 全局变量初始化
- 全局变量在函数中的读取
- 全局变量的修改
- 未初始化的全局变量（应为 0）
- 全局变量作为函数参数

**测试结果**：

#### 词法分析 ✅
```
共识别 131 个Token
```
所有Token正确识别，包括：
- `int`, `global_x`, `=`, `100`, `;`
- 函数名、参数、各种运算符

#### 语法分析 ✅
```
Program(
  VarDecl(int global_x = Number(100)),
  VarDecl(int global_y = Number(200)),
  VarDecl(int global_z),
  FunctionDecl(int getGlobalX(), ...),
  ...
)
```
全局变量被正确识别为 `VarDecl` 节点

#### 语义分析 ✅
```
✓ 语义检查通过
```
- 全局变量类型检查通过
- 全局变量在函数中访问正确
- 无重复定义错误
- 类型兼容性检查正确

---

## 📋 待实现的工作

### 第二阶段：VM 扩展

**目标**：添加全局变量运行时支持

**需要添加**：
1. `globals_` 数组存储全局变量值
2. `GLOBAL_BASE` 常量（建议：0x40000000）
3. 新指令：
   - `LOADG offset` - 加载全局变量到栈顶
   - `STOREG offset` - 从栈顶存储到全局变量
   - `LEAG offset` - 加载全局变量地址到栈顶
4. 修改 `LOADM/STOREM` 以支持全局地址
5. 全局变量初始化逻辑

**修改文件**：
- `include/vm.h`
- `src/vm.cpp`

---

### 第三阶段：CodeGen 扩展

**目标**：生成全局变量访问的字节码

**需要添加**：
1. 全局变量符号表 `globals_`
2. 全局偏移计数器 `global_offset_`
3. `genGlobalVarDecl()` 方法
4. 修改 `genExpression()` 以区分全局和局部变量
5. 辅助方法：`isGlobal()`, `getGlobal()`

**修改文件**：
- `include/codegen.h`
- `src/codegen.cpp`

---

## 📊 实现统计

### 代码量统计

| 文件 | 新增行数 | 主要修改 |
|------|---------|---------|
| `include/ast.h` | ~40 | 添加 `global_vars_` 到 ProgramNode |
| `include/parser.h` | 1 | 添加方法声明 |
| `src/parser.cpp` | ~100 | 实现 `parseGlobalVarDeclaration()` 和 `parseProgram()` 修改 |
| `include/sema.h` | ~10 | 添加全局符号表和方法 |
| `src/sema.cpp` | ~60 | 实现 `analyzeGlobalVarDecl()` 和 `analyzeVariable()` 修改 |

**总计**：约 210 行新代码

### 功能支持矩阵

| 特性 | 词法 | 语法 | 语义 | 代码生成 | VM执行 |
|------|------|------|------|----------|--------|
| 基本全局变量 | ✅ | ✅ | ✅ | ⏳ | ⏳ |
| 全局变量初始化 | ✅ | ✅ | ✅ | ⏳ | ⏳ |
| 全局数组 | ✅ | ✅ | ✅ | ⏳ | ⏳ |
| 全局结构体 | ✅ | ✅ | ✅ | ⏳ | ⏳ |
| 全局指针 | ✅ | ✅ | ✅ | ⏳ | ⏳ |

---

## 🔑 关键设计决策

### 决策1：复用 VarDeclStmtNode

**方案对比**：
- ❌ 创建新的 `GlobalVarDeclNode` 类
- ✅ 复用现有的 `VarDeclStmtNode` 类

**选择原因**：
- 全局变量和局部变量的声明语法完全相同
- 减少代码重复
- 简化实现，减少维护成本
- 类型系统已经完全支持

### 决策2：变量查找顺序

**实现**：先局部后全局
```cpp
auto symbol = scope_.findSymbol(name);
if (symbol) return symbol->getType();

auto global_it = global_symbols_.find(name);
if (global_it != global_symbols_.end()) return global_it->second;

error("未声明的变量");
```

**符合C语言语义**：局部变量遮蔽（shadow）全局变量

---

## 🐛 已知问题

### 1. AST 显示的小问题

**现象**：AST输出中全局变量和函数之间有多余的逗号
```
VarDecl(int global_z), , FunctionDecl(...)
```

**原因**：`ProgramNode::toString()` 中处理空 `global_vars_` 的逻辑

**影响**：仅影响显示，不影响功能

**修复**：待优化（优先级低）

---

## 📝 开发笔记

### 重要经验

1. **复用优于创新**：
   - 复用 `VarDeclStmtNode` 避免了重复代码
   - 利用现有的类型系统和解析逻辑

2. **分阶段实现的优势**：
   - 先完成词法/语法/语义分析，可以提前发现问题
   - 为第二阶段（VM和CodeGen）打下坚实基础

3. **测试驱动**：
   - 使用实际测试文件（`global_basic.c`）验证功能
   - 词法、语法、语义三个层次都通过测试

### 后续计划

1. 实现 VM 扩展（LOADG/STOREG/LEAG 指令）
2. 实现 CodeGen 扩展（全局变量代码生成）
3. 完整测试所有功能
4. 性能优化和错误处理

---

**文档结束**
