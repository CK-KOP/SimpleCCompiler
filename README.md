# SimpleC编译器项目

一个从零开始的C语言编译器项目，旨在通过循序渐进的方式实现一个完整的C编译器，帮助理解编译器的工作原理。

## 项目概述

SimpleC是一个教育性质的C编译器项目，采用增量开发的方式，从最基础的词法分析开始，逐步构建成支持完整C语言特性的编译器。项目强调可理解性和可学习性，每个阶段都有明确的目标和测试用例。

## 设计理念

- **循序渐进**: 从最简单的功能开始，逐步增加复杂性
- **测试驱动**: 每个模块都有对应的测试用例
- **可理解性**: 代码结构清晰，注释详细
- **模块化**: 各个组件职责明确，便于独立开发和测试

## 整体架构

编译器分为以下主要阶段：

```
源代码 → 词法分析器 → 语法分析器 → 语义分析器 → 代码生成器 → 目标代码
```

### 核心组件

1. **词法分析器 (Lexer)**: 将源代码转换为Token流
2. **语法分析器 (Parser)**: 将Token流转换为抽象语法树(AST)
3. **语义分析器 (Semantic Analyzer)**: 类型检查和语义验证
4. **代码生成器 (Code Generator)**: 生成目标代码(字节码或LLVM IR)

## 开发路线图

### 第一阶段：基础词法分析 (Lexer)

#### 1.1 超简单词法分析器 [当前阶段]
**目标**: 识别数字和基本运算符

**功能要求**:
- Token类型：Number, Plus, Minus, Multiply, Divide, End
- 跳过空白字符
- 识别多位数字
- 识别四则运算符 +, -, *, /
- 基础错误处理

**测试用例**:
```
"12 + 34" → [Number(12), Plus(+), Number(34), End]
"12-3*4" → [Number(12), Minus(-), Number(3), Multiply(*), Number(4), End]
```

**实现要点**:
- 使用状态机或正则表达式识别Token
- 维护源代码位置信息(行号、列号)
- 简单的错误恢复机制

#### 1.2 扩展词法分析器
**目标**: 支持更多Token类型

**新增Token**:
- 括号：LParen, RParen
- 比较运算符：Equal, NotEqual, Less, Greater
- 赋值运算符：Assign
- 关键字：Int, Return
- 标识符：Identifier
- 分隔符：Semicolon, Comma

**新功能**:
- 识别标识符(字母开头的字母数字串)
- 关键字识别
- 支持单行注释(//)

#### 1.3 完整词法分析器
**目标**: 支持完整的C语言Token

**所有Token类型**:
- 字面量：Number, String, Char
- 标识符和关键字
- 所有运算符(包括位运算、逻辑运算)
- 所有分隔符
- 预处理指令

**新功能**:
- 支持多行注释(/* */)
- 支持字符串字面量
- 支持字符字面量
- 支持预处理器指令(#include, #define等)

### 第二阶段：语法分析 (Parser) - 表达式

#### 2.1 简单表达式解析
**目标**: 解析数学表达式

**支持语法**:
```
Expr → Term ((+|-) Term)*
Term → Factor ((*|/) Factor)*
Factor → Number | '(' Expr ')'
```

**AST节点类型**:
- BinaryExpr: 二元运算表达式
- NumberExpr: 数字字面量

#### 2.2 扩展表达式解析
**目标**: 支持更多表达式类型

**新增语法**:
- 一元运算符：+expr, -expr
- 比较运算符：expr == expr, expr != expr等
- 逻辑运算符：expr && expr, expr || expr, !expr

**运算符优先级**:
1. 括号：()
2. 一元运算符：+ - ! ~ ++ --
3. 乘除法：* / %
4. 加减法：+ -
5. 比较运算符：< <= > >=
6. 等值运算符：== !=
7. 位运算符：& ^ |
8. 逻辑运算符：&& ||

#### 2.3 变量和赋值表达式
**目标**: 支持变量和赋值

**新增语法**:
- 变量引用：identifier
- 赋值表达式：identifier = expr
- 复合赋值：identifier += expr等

#### 2.4 函数调用和数组
**目标**: 支持函数调用和数组访问

**新增语法**:
- 函数调用：identifier(args...)
- 数组访问：identifier[index]
- 成员访问：identifier.field, identifier->field

### 第三阶段：语句解析 (Statements)

#### 3.1 基本语句
**目标**: 解析基本语句

**支持语句**:
- 表达式语句：expr;
- 空语句：;
- 复合语句：{ stmt... }

#### 3.2 控制流语句
**目标**: 支持条件语句

**支持语句**:
- if语句：if (expr) stmt [else stmt]

#### 3.3 循环语句
**目标**: 支持循环

**支持语句**:
- while循环：while (expr) stmt
- for循环：for (init; cond; incr) stmt

#### 3.4 跳转语句
**目标**: 支持程序控制跳转

**支持语句**:
- return语句：return [expr];
- break语句：break;
- continue语句：continue;

### 第四阶段：声明和作用域

#### 4.1 变量声明
**语法**:
```
Decl → Type Name [= Init]
Type → int | char | float | double
Init → Expr
```

#### 4.2 函数声明和定义
**语法**:
```
FuncDecl → ReturnType Name '(' ParamList ')' [Body]
ParamList → Param (',' Param)* | ε
Param → Type Name
Body → CompoundStmt
```

#### 4.3 结构体和类型定义
**语法**:
```
StructDecl → struct Name '{' FieldList '}'
FieldList → FieldDecl (';' FieldDecl)*
FieldDecl → Type Name
```

#### 4.4 作用域和符号表
**功能**:
- 全局作用域、函数作用域、块作用域
- 变量查找和作用域嵌套

### 第五阶段：语义分析 (Semantic Analysis)

#### 5.1 类型检查
- 变量声明检查
- 表达式类型推导
- 类型兼容性检查
- 类型转换

#### 5.2 函数调用检查
- 函数声明检查
- 参数类型匹配
- 参数数量检查
- 返回类型检查

#### 5.3 控制流检查
- break/continue位置检查
- return语句检查
- 未使用变量警告
- 死代码检测

### 第六阶段：代码生成 (Code Generation)

#### 6.1 栈式虚拟机设计
**指令集**:
```
PUSH const     // 推入常数
POP            // 弹出
LOAD addr      // 加载变量
STORE addr     // 存储变量
ADD, SUB, MUL, DIV // 四则运算
JMP label      // 跳转
JZ label       // 条件跳转
CALL func      // 函数调用
RET            // 函数返回
```

#### 6.2-6.4 表达式、语句、函数的代码生成
- 后序遍历AST生成栈式指令
- 控制流指令生成
- 函数调用栈管理

### 第七阶段：LLVM后端集成

#### 7.1-7.4 LLVM IR生成
- LLVM基础集成
- 表达式到LLVM IR
- 控制流到LLVM IR
- 函数到LLVM IR

### 第八阶段：优化和错误处理

#### 8.1 代码优化
- 常量折叠
- 死代码消除
- 公共子表达式消除
- 寄存器分配

#### 8.2 错误处理和恢复
- 语法错误恢复
- 错误位置精确定位
- 友好的错误信息
- 警告系统

### 第九阶段：扩展特性

#### 9.1 预处理器
- #include处理
- #define宏展开
- 条件编译

#### 9.2 更多数据类型
- 浮点数支持
- 枚举类型
- 联合体
- 指针类型
- 数组类型

#### 9.3 标准库支持
- 基础IO函数
- 字符串操作
- 数学函数

## 当前开发状态

### 版本 0.2.0 - 扩展词法分析器 (1.2阶段完成)
**已完成**:
- ✅ 1.1阶段：基础词法分析器（数字和基本运算符）
- ✅ 1.2阶段：扩展词法分析器
  - 标识符和关键字识别（int, return）
  - 括号和分隔符（(), ;, ,）
  - 赋值运算符（=）
  - 完整比较运算符（==, !=, <, <=, >, >=）
  - 单行注释支持（//）
  - 完善的错误处理和位置跟踪
- ✅ 通用Makefile系统（自动发现文件）
- ✅ 完整的测试覆盖和演示程序

**超出计划的扩展**：
- ✅ 完整的比较运算符支持（包含<=, >=）
- ✅ 交互式演示程序
- ✅ 构建系统自动发现新文件

**当前Token支持**：
```
字面量：     Number (123)
关键字：     Int (int), Return (return)
标识符：     Identifier (变量名, _var123)
运算符：     +, -, *, /, =, ==, !=, <, <=, >, >=
分隔符：     (, ), ;, ,
注释：       // 单行注释
```

**下一阶段计划 (1.3)**：
- 大括号支持（{ }）
- 字符串和字符字面量
- 更多关键字（if, else, while, for等）
- 多行注释（/* */）
- 逻辑运算符（&&, ||, !）

## 项目结构

```
SimpleCCompiler/
├── README.md           # 项目说明文档
├── src/                # 源代码目录
│   ├── token.cpp       # Token实现
│   └── lexer.cpp       # 词法分析器实现
├── include/            # 头文件目录
│   ├── token.h         # Token类型定义
│   └── lexer.h         # 词法分析器接口
├── tests/              # 测试代码
│   ├── test_lexer.cpp  # 基础词法分析器测试(1.1)
│   ├── test_simple.cpp # 1.2阶段基础测试
│   └── test_all_operators.cpp # 完整运算符测试
├── examples/           # 示例代码
│   └── lexer_demo.cpp # 交互式词法分析器演示
├── docs/               # 设计文档
├── build/              # 构建输出目录
├── .gitignore          # Git忽略文件
└── Makefile            # 通用构建系统
```

## 开发指南

## 构建和运行

### 使用Makefile
```bash
# 查看所有可用命令
make help

# 编译并运行测试
make test

# 编译并运行演示程序（交互式）
make run-demo

# 查看项目信息
make info

# 清理构建文件
make clean
```

### 交互式测试
```bash
# 测试基本表达式
echo "12 + 34 * 5" | make run-demo

# 测试C语言语句
echo "int x = 10; return x + 5;" | make run-demo

# 测试比较运算符
echo "x <= y && x != z" | make run-demo
```

### 编码规范
- 使用C++17标准
- 使用简洁的注释风格
- 遵循模块化设计原则

### 测试策略
- 单元测试：`make test`
- 交互式测试：`make run-demo`
- 自动发现新测试文件
- 完整的错误处理测试

### 调试技巧
- 添加详细的调试输出
- 可视化Token流和AST结构
- 使用断点调试关键算法
- 记录设计决策和实现细节

## 学习资源

### 推荐书籍
- 《编译原理》(龙书) - 经典教材
- 《Crafting Interpreters》- 实践导向
- 《现代编译原理》- 现代方法

### 在线资源
- [LLVM官方教程](https://llvm.org/docs/tutorial/)
- [编译器设计课程笔记](https://www.cs.cornell.edu/courses/cs4120/2023sp/)
- [开源编译器源码](https://github.com/tinycc/tinycc)

### 相关工具
- LLVM/Clang
- Flex/Bison
- Graphviz(用于AST可视化)
- Valgrind(内存检查)

## 贡献指南

欢迎贡献代码和建议！请遵循以下步骤：

1. Fork项目仓库
2. 创建特性分支
3. 编写代码和测试
4. 提交Pull Request
5. 参与代码评审

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 联系方式

如有问题或建议，请提交Issue或联系维护者。

---

**注意**: 这是一个教育性质的项目，目标是学习编译器的实现原理，而不是替代现有的编译器。