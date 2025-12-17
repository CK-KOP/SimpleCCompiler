# SimpleC 编译器

一个从零开始实现的 C 语言编译器，用于学习编译原理。

## 快速开始

```bash
# 构建
make

# 语义分析（默认）
./build/simplec examples/demo.c

# 仅词法分析
./build/simplec examples/demo.c -l

# 仅语法分析
./build/simplec examples/demo.c -p

# 运行测试
make test
```

## 当前功能

### 词法分析 (Lexer)
- 数字字面量
- 标识符和关键字 (`int`, `void`, `return`, `if`, `else`, `while`, `for`, `do`, `break`, `continue`)
- 运算符 (`+`, `-`, `*`, `/`, `=`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`)
- 分隔符 (`(`, `)`, `{`, `}`, `;`, `,`)
- 单行注释 (`//`)

### 语法分析 (Parser)

**表达式**:
- 算术表达式: `a + b * c`
- 比较表达式: `x <= y`
- 逻辑表达式: `a && b || !c`
- 赋值表达式: `x = 10`
- 函数调用: `add(1, 2)`

**语句**:
- 变量声明: `int x = 10;`
- 返回语句: `return x + 1;`
- 条件语句: `if (x > 0) { ... } else { ... }`
- 循环语句: `while`, `for`, `do-while`
- 控制语句: `break`, `continue`
- 复合语句: `{ ... }`

**函数**:
- 函数定义: `int add(int a, int b) { return a + b; }`
- 函数调用: `result = add(x, y);`
- 支持 `int` 和 `void` 返回类型

### 示例

```c
int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int x = 10;
    int y = 20;
    int sum = add(x, y);
    int fact = factorial(5);

    if (sum > 25) {
        return 1;
    } else {
        return 0;
    }
}
```

运行结果:
```
识别到 3 个函数:
  - int add(int a, int b)
  - int factorial(int n)
  - int main()

进行语义检查...
✓ 语义检查通过
```

### 指针支持

**指针声明**:
```c
int x = 10;
int *p = &x;      // 一级指针
int **pp = &p;    // 二级指针
```

**指针操作**:
- 取地址: `&x` - 获取变量 x 的地址
- 解引用: `*p` - 读取指针 p 指向的值
- 解引用赋值: `*p = 20` - 通过指针修改值

**示例**:
```c
int main() {
    int x = 10;
    int *p = &x;
    *p = 20;        // x 变为 20
    int **pp = &p;
    **pp = 30;      // x 变为 30
    return x;       // 返回 30
}
```

### 语义分析 (Sema)

**类型系统**:
- 基本类型: `int`, `void`
- 指针类型: `int*`, `int**`, ...
- 函数类型: 返回类型 + 参数列表

**符号表**:
- 作用域管理（全局、函数、块级）
- 变量、函数、参数的符号注册

**语义检查**:
- 变量重复声明检测
- 未声明变量/函数检测
- 函数参数数量匹配
- 返回值类型检查（void 函数不应返回值，非 void 函数应返回值）

**错误检测示例**:
```c
int main() {
    int x = 10;
    int x = 20;         // 错误：变量重复声明
    y = 30;             // 错误：未声明的变量
    int z = foo(1, 2);  // 错误：未声明的函数
    return;             // 错误：非void函数应返回值
}
```

## 项目结构

```
SimpleCCompiler/
├── main.cpp            # 主程序入口
├── Makefile            # 构建脚本
├── include/            # 头文件
│   ├── token.h         # Token 定义
│   ├── lexer.h         # 词法分析器
│   ├── ast.h           # AST 节点定义
│   ├── parser.h        # 语法分析器
│   ├── type.h          # 类型系统
│   ├── scope.h         # 符号表和作用域
│   └── sema.h          # 语义分析器
├── src/                # 源文件
│   ├── token.cpp
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── type.cpp
│   └── sema.cpp
├── tests/              # 测试文件
│   ├── test_lexer.cpp
│   ├── test_parser.cpp
│   └── test_function.cpp
└── examples/           # 示例代码
    ├── demo.c          # 正确示例
    └── error_test.c    # 语义错误测试
```

## AST 节点类型

| 类型 | 说明 | 示例 |
|------|------|------|
| `NumberNode` | 数字字面量 | `123` |
| `VariableNode` | 变量引用 | `x` |
| `BinaryOpNode` | 二元运算 | `a + b` |
| `UnaryOpNode` | 一元运算 | `-x`, `!flag` |
| `FunctionCallNode` | 函数调用 | `add(1, 2)` |
| `VarDeclStmtNode` | 变量声明 | `int x = 10;` |
| `ReturnStmtNode` | 返回语句 | `return x;` |
| `IfStmtNode` | 条件语句 | `if (...) {...}` |
| `WhileStmtNode` | while 循环 | `while (...) {...}` |
| `ForStmtNode` | for 循环 | `for (...) {...}` |
| `CompoundStmtNode` | 复合语句 | `{ ... }` |
| `FunctionDeclNode` | 函数定义 | `int foo() {...}` |
| `ProgramNode` | 程序 | 多个函数 |

## 运算符优先级

从低到高:
1. `=` (赋值，右结合)
2. `||` (逻辑或)
3. `&&` (逻辑与)
4. `==`, `!=` (等值比较)
5. `<`, `<=`, `>`, `>=` (大小比较)
6. `+`, `-` (加减)
7. `*`, `/` (乘除)
8. `+x`, `-x`, `!x` (一元运算)
9. `()` (括号)

## 开发计划

- [x] 词法分析器
- [x] 表达式解析
- [x] 语句解析
- [x] 函数定义和调用
- [x] 类型系统
- [x] 符号表和作用域
- [x] 语义分析
- [x] 数组支持
- [x] 指针支持（基本指针、多级指针）
- [x] 代码生成
- [x] 栈式虚拟机
- [ ] 指针数组 `int *arr[10]`
- [ ] 数组指针 `int (*p)[10]`
- [ ] 结构体支持

## 构建要求

- C++17 兼容的编译器 (g++ 或 clang++)
- Make

## 许可证

MIT License
