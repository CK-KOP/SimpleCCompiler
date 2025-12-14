# SimpleC 编译器完整流程演示

本文档以一个完整的示例，展示从源代码到执行结果的全过程。

## 示例代码

```c
int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int main() {
    int x = 10;
    int y = 25;
    return max(x, y);
}
```

---

## 阶段 1：词法分析 (Lexer)

词法分析器将源代码字符流转换为 Token 流。

### 输入
```
int max(int a, int b) { if (a > b) { return a; } return b; } ...
```

### 输出
```
Token 流：

INT  IDENT("max")  LPAREN  INT  IDENT("a")  COMMA  INT  IDENT("b")  RPAREN  LBRACE
    IF  LPAREN  IDENT("a")  GREATER  IDENT("b")  RPAREN  LBRACE
        RETURN  IDENT("a")  SEMICOLON
    RBRACE
    RETURN  IDENT("b")  SEMICOLON
RBRACE

INT  IDENT("main")  LPAREN  RPAREN  LBRACE
    INT  IDENT("x")  ASSIGN  NUMBER(10)  SEMICOLON
    INT  IDENT("y")  ASSIGN  NUMBER(25)  SEMICOLON
    RETURN  IDENT("max")  LPAREN  IDENT("x")  COMMA  IDENT("y")  RPAREN  SEMICOLON
RBRACE

END
```

### 关键实现
- 跳过空白字符和注释
- 识别关键字（int, if, return 等）
- 识别标识符和数字
- 识别运算符和分隔符

---

## 阶段 2：语法分析 (Parser)

语法分析器将 Token 流转换为抽象语法树 (AST)。

### 输出：AST 结构

```
ProgramNode
│
├── FunctionDecl: int max(int a, int b)
│   │
│   └── CompoundStmt (函数体)
│       │
│       ├── IfStmt
│       │   │
│       │   ├── condition: BinaryOp(GREATER)
│       │   │               ├── left:  Variable("a")
│       │   │               └── right: Variable("b")
│       │   │
│       │   └── thenStmt: CompoundStmt
│       │                 └── ReturnStmt
│       │                     └── expr: Variable("a")
│       │
│       └── ReturnStmt
│           └── expr: Variable("b")
│
└── FunctionDecl: int main()
    │
    └── CompoundStmt (函数体)
        │
        ├── VarDeclStmt: int x
        │   └── initializer: Number(10)
        │
        ├── VarDeclStmt: int y
        │   └── initializer: Number(25)
        │
        └── ReturnStmt
            └── expr: FunctionCall("max")
                      ├── arg[0]: Variable("x")
                      └── arg[1]: Variable("y")
```

### 关键实现
- 递归下降解析
- 运算符优先级处理
- 构建各种 AST 节点

---

## 阶段 3：语义分析 (Sema)

语义分析器检查程序的语义正确性，构建符号表。

### 符号表构建过程

```
═══════════════════════════════════════════════════════════
全局作用域 (Global Scope)
═══════════════════════════════════════════════════════════
  max  → Function, 返回类型: int, 参数: [int a, int b]
  main → Function, 返回类型: int, 参数: []

═══════════════════════════════════════════════════════════
分析 max 函数
═══════════════════════════════════════════════════════════
  进入函数作用域
  │
  ├── a → Parameter, 类型: int
  ├── b → Parameter, 类型: int
  │
  ├── 检查表达式 a > b
  │   ├── 查找 a → 找到，类型 int ✓
  │   ├── 查找 b → 找到，类型 int ✓
  │   └── int > int → 合法 ✓
  │
  ├── 检查 return a
  │   ├── 表达式类型: int
  │   ├── 函数返回类型: int
  │   └── 匹配 ✓
  │
  └── 检查 return b
      └── 同上 ✓

  退出函数作用域

═══════════════════════════════════════════════════════════
分析 main 函数
═══════════════════════════════════════════════════════════
  进入函数作用域
  │
  ├── x → Variable, 类型: int, 偏移: 0
  ├── y → Variable, 类型: int, 偏移: 1
  │
  ├── 检查函数调用 max(x, y)
  │   ├── 查找 max → 找到，是函数 ✓
  │   ├── 期望参数: 2 个
  │   ├── 实际参数: 2 个 ✓
  │   ├── 参数类型匹配 ✓
  │   └── 返回类型: int
  │
  └── 检查 return max(x, y)
      └── int 匹配 int ✓

  退出函数作用域

═══════════════════════════════════════════════════════════
✓ 语义检查通过，无错误
═══════════════════════════════════════════════════════════
```

### 关键检查项
- 变量/函数是否声明
- 变量是否重复声明
- 函数参数数量和类型
- 返回值类型匹配

---

## 阶段 4：代码生成 (CodeGen)

代码生成器遍历 AST，生成栈式虚拟机字节码。

### 生成过程详解

#### 生成 max 函数

```
函数 max 的参数布局（相对于帧指针 fp）：
  a → offset -4
  b → offset -3

生成 if (a > b) { return a; } return b;

步骤 1: 生成条件表达式 a > b
  地址 0: LOAD -4     ; 加载 a
  地址 1: LOAD -3     ; 加载 b
  地址 2: GT          ; 比较，结果 0 或 1

步骤 2: 生成条件跳转（使用回填技术）
  地址 3: JZ ???      ; 目标地址未知，先占位
  记住地址 3，稍后回填

步骤 3: 生成 then 分支
  地址 4: LOAD -4     ; 加载 a
  地址 5: RETV        ; 返回 a

步骤 4: 回填跳转地址
  当前地址是 6
  patch(3, 6)         ; 将地址 3 的指令改为 JZ 6

步骤 5: 生成 else 部分（这里是函数剩余代码）
  地址 6: LOAD -3     ; 加载 b
  地址 7: RETV        ; 返回 b

步骤 6: 添加默认返回（安全措施）
  地址 8: RET
```

#### 生成 main 函数

```
main 函数的局部变量布局（相对于帧指针 fp）：
  x → offset 0
  y → offset 1

步骤 1: 生成变量声明 int x = 10
  地址 9: PUSH 10     ; 值留在栈上，就是 x 的存储位置

步骤 2: 生成变量声明 int y = 25
  地址 10: PUSH 25    ; 值留在栈上，就是 y 的存储位置

步骤 3: 生成函数调用 max(x, y)
  地址 11: LOAD 0     ; 加载 x 作为第一个参数
  地址 12: LOAD 1     ; 加载 y 作为第二个参数
  地址 13: CALL 0     ; 调用 max（地址 0）
  地址 14: POPN 2     ; 清理 2 个参数，保留返回值

步骤 4: 生成 return
  地址 15: RETV       ; 返回 max 的结果

步骤 5: 添加默认返回
  地址 16: RET
```

### 最终字节码

```
; ========== max 函数 (地址 0) ==========
0:  LOAD -4      ; 加载参数 a
1:  LOAD -3      ; 加载参数 b
2:  GT           ; a > b ?
3:  JZ 6         ; 如果假，跳到地址 6
4:  LOAD -4      ; 加载 a
5:  RETV         ; 返回 a
6:  LOAD -3      ; 加载 b
7:  RETV         ; 返回 b
8:  RET          ; 默认返回

; ========== main 函数 (地址 9，入口点) ==========
9:  PUSH 10      ; int x = 10
10: PUSH 25      ; int y = 25
11: LOAD 0       ; 加载 x
12: LOAD 1       ; 加载 y
13: CALL 0       ; 调用 max
14: POPN 2       ; 清理参数
15: RETV         ; 返回结果
16: RET          ; 默认返回

入口点: 9
```

---

## 阶段 5：VM 执行

虚拟机解释执行字节码。

### 执行过程详解

```
初始状态：
  pc = 9 (入口点)
  sp = 2, fp = 2
  栈: [0]=-1, [1]=0, [2]=?, [3]=?, ...
             ↑fp,sp

  注：[0]=-1 是虚拟返回地址，[1]=0 是虚拟旧帧指针

═══════════════════════════════════════════════════════════
执行 main 函数
═══════════════════════════════════════════════════════════

[9] PUSH 10
    栈: [-1, 0, 10, ?, ?, ?, ?, ?]
              ↑fp     ↑sp
    x = 10 存储在 stack[2]

[10] PUSH 25
    栈: [-1, 0, 10, 25, ?, ?, ?, ?]
              ↑fp         ↑sp
    y = 25 存储在 stack[3]

[11] LOAD 0
    读取 stack[fp+0] = stack[2] = 10
    栈: [-1, 0, 10, 25, 10, ?, ?, ?]
              ↑fp             ↑sp
    准备第一个参数

[12] LOAD 1
    读取 stack[fp+1] = stack[3] = 25
    栈: [-1, 0, 10, 25, 10, 25, ?, ?]
              ↑fp                 ↑sp
    准备第二个参数

[13] CALL 0
    保存返回信息：
    - push(pc) = push(14)  ; 返回地址
    - push(fp) = push(2)   ; 旧帧指针
    - fp = sp = 8
    - pc = 0

    栈: [-1, 0, 10, 25, 10, 25, 14, 2, ?]
              ↑旧fp                   ↑新fp,sp

    参数位置：
    - a = stack[fp-4] = stack[4] = 10
    - b = stack[fp-3] = stack[5] = 25

═══════════════════════════════════════════════════════════
执行 max 函数
═══════════════════════════════════════════════════════════

[0] LOAD -4
    读取 stack[fp-4] = stack[4] = 10
    栈: [..., 14, 2, 10]
                  ↑fp    ↑sp

[1] LOAD -3
    读取 stack[fp-3] = stack[5] = 25
    栈: [..., 14, 2, 10, 25]
                  ↑fp        ↑sp

[2] GT
    弹出 25 和 10，比较 10 > 25 → 0 (假)
    栈: [..., 14, 2, 0]
                  ↑fp   ↑sp

[3] JZ 6
    弹出 0，因为是 0，跳转到地址 6
    栈: [..., 14, 2]
                  ↑fp,sp
    pc = 6

[6] LOAD -3
    读取 stack[fp-3] = stack[5] = 25
    栈: [..., 14, 2, 25]
                  ↑fp    ↑sp

[7] RETV
    返回值 = pop() = 25
    sp = fp = 8
    fp = pop() = 2
    pc = pop() = 14
    push(25)

    栈: [-1, 0, 10, 25, 10, 25, 25]
              ↑fp                 ↑sp

═══════════════════════════════════════════════════════════
返回 main 函数继续执行
═══════════════════════════════════════════════════════════

[14] POPN 2
    保留栈顶 25，移除 2 个参数
    栈: [-1, 0, 10, 25, 25]
              ↑fp          ↑sp

    返回值 25 现在在栈顶

[15] RETV
    返回值 = pop() = 25
    sp = fp = 2
    fp = pop() = 0
    pc = pop() = -1  ← 程序结束标志
    push(25)

    栈: [25]
         ↑sp

═══════════════════════════════════════════════════════════
程序结束
═══════════════════════════════════════════════════════════

pc = -1，退出执行循环
返回值 = stack[sp-1] = 25

✓ 程序返回值: 25
```

---

## 实际运行验证

```bash
$ ./build/simplec examples/max_demo.c -c
=== 生成的字节码 ===

0:  LOAD -4
1:  LOAD -3
2:  GT
3:  JZ 6
4:  LOAD -4
5:  RETV
6:  LOAD -3
7:  RETV
8:  RET
9:  PUSH 10
10: PUSH 25
11: LOAD 0
12: LOAD 1
13: CALL 0
14: POPN 2
15: RETV
16: RET

入口点: 9

$ ./build/simplec examples/max_demo.c
=== 运行程序 ===

程序返回值: 25
```

---

## 总结

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   源代码     │ →  │   Token流   │ →  │    AST      │
│  (字符串)    │    │   (词法)    │    │   (语法)    │
└─────────────┘    └─────────────┘    └─────────────┘
                                            │
                                            ↓
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   执行结果   │ ←  │   字节码    │ ←  │  检查后AST  │
│    (25)     │    │  (代码生成)  │    │   (语义)    │
└─────────────┘    └─────────────┘    └─────────────┘
```

每个阶段的职责：
1. **词法分析** - 识别单词（Token）
2. **语法分析** - 理解结构（AST）
3. **语义分析** - 检查含义（类型、作用域）
4. **代码生成** - 翻译成指令（字节码）
5. **虚拟机** - 执行指令（得到结果）
