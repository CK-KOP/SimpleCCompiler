# 开发计划

## 实现分阶段

### 阶段一：基础结构体（当前）

**目标**：实现最基本的结构体定义、声明和成员访问。

#### 支持的功能

```c
// 1. 全局结构体定义
struct Point {
    int x;
    int y;
};

struct Data {
    int value;
    int *ptr;      // 支持指针成员
    int arr[5];    // 支持数组成员
};

// 2. 结构体变量声明和使用
int main() {
    struct Point p;
    struct Data d;

    // 成员赋值
    p.x = 10;
    p.y = 20;
    d.value = 100;

    // 成员读取
    int sum = p.x + p.y;

    // 指针成员
    int val = 42;
    d.ptr = &val;
    int result = *d.ptr;

    // 数组成员
    d.arr[0] = 1;
    d.arr[1] = 2;

    return sum;
}
```

#### 功能清单

- ✅ 全局结构体定义
- ✅ 结构体变量声明
- ✅ 成员访问运算符 `.`
- ✅ 成员类型支持：
  - `int` 基本类型
  - `int*` 指针类型
  - `int[N]` 数组类型
- ✅ 成员赋值和读取

#### 不支持的功能

- ❌ 结构体初始化 `struct Point p = {10, 20};`
- ❌ 结构体整体赋值 `p2 = p1;`（需要实现多slot复制）
- ❌ 自引用结构体 `struct Node { struct Node *next; };`
- ❌ 结构体作为函数参数/返回值

---

### 阶段二：结构体指针、嵌套、数组（已完成）

**目标**：扩展结构体的组合能力。

**状态**：✅ 已完成并通过测试

#### 新增功能

```c
struct Point {
    int x;
    int y;
};

struct Line {
    struct Point start;  // 嵌套结构体
    struct Point end;
};

int main() {
    // 1. 结构体指针
    struct Point p;
    struct Point *ptr = &p;
    ptr->x = 10;         // 指针成员访问
    ptr->y = 20;

    // 2. 嵌套结构体
    struct Line line;
    line.start.x = 0;    // 链式成员访问
    line.start.y = 0;
    line.end.x = 10;
    line.end.y = 10;

    // 3. 结构体数组
    struct Point points[3];
    points[0].x = 1;
    points[1].x = 2;
    points[2].x = 3;

    return ptr->x + line.end.x + points[0].x;
}
```

#### 功能清单

- ✅ 结构体指针类型 `struct Point *`
- ✅ `->` 运算符（指针成员访问）
- ✅ 嵌套结构体（单向）
- ✅ 链式成员访问 `obj.member.submember`
- ✅ 结构体数组 `struct Point arr[10]`
- ✅ 数组元素成员访问 `arr[i].x`
- ✅ 结构体指针成员 `struct Point *ptr;`（成员）

#### 实现要点

**1. 结构体数组的内存布局**
```c
struct Point { int x; int y; };  // 每个 Point 占 2 slots
struct Point arr[3];              // 总共占 6 slots
// arr[0].x -> offset 0, arr[0].y -> offset 1
// arr[1].x -> offset 2, arr[1].y -> offset 3
// arr[2].x -> offset 4, arr[2].y -> offset 5
```

**2. 数组元素成员访问的地址计算**
```c
points[i].x
// 地址 = base + i * sizeof(Point) + offset_of(x)
```

**3. 链式成员访问的偏移累加**
```c
line.end.y
// 地址 = base + offset_of(end) + offset_of(y)
```

**4. `->` 运算符实现**
```c
ptr->x  // 等价于 (*ptr).x
// 1. 解引用指针得到结构体地址
// 2. 加上成员偏移
// 3. 加载值
```

#### 约束

- ⚠️ 不支持前向声明和互相引用
  ```c
  // ❌ 不支持互相引用
  struct A { struct B *b; };
  struct B { struct A *a; };

  // ❌ 不支持自引用（链表等数据结构）
  struct Node { struct Node *next; };
  ```
- ⚠️ 结构体成员必须是已定义的类型
  ```c
  // ❌ 不支持
  struct Container {
      struct Point p;  // Point 未定义
  };
  struct Point { int x; int y; };
  ```
- ⚠️ 不支持结构体整体赋值
  ```c
  struct Point p1, p2;
  p1.x = 10;
  p1.y = 20;
  p2 = p1;  // ❌ 不支持，需要逐成员赋值
  ```

#### 类型检查增强

在Phase 2实现过程中，添加了完整的类型兼容性检查系统：

- ✅ 赋值类型检查：检测类型不匹配的赋值
- ✅ 指针类型检查：递归检查指针基类型
- ✅ 结构体类型检查：确保结构体名称匹配
- ✅ 指针层级检查：防止 `int*` 赋值给 `int**`

**示例错误检测**：
```c
struct Point *ptr = &rectangle;  // ✗ 错误：类型不匹配
int *ptr = &struct_var;          // ✗ 错误：不能将结构体指针赋给int指针
int **ptr2 = ptr1;               // ✗ 错误：指针层级不匹配
```

---

### 阶段三：函数参数和返回值（计划中）

**目标**：支持结构体在函数间传递。

**状态**：⏳ 待实现

#### 新增功能

```c
struct Point {
    int x;
    int y;
};

// 结构体作为返回值
struct Point createPoint(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

// 结构体作为参数（值传递）
int getX(struct Point p) {
    return p.x;
}

// 结构体指针作为参数（引用传递）
void setX(struct Point *p, int x) {
    p->x = x;
}

int main() {
    struct Point p = createPoint(10, 20);
    int x = getX(p);
    setX(&p, 30);
    return p.x;
}
```

#### 功能清单

- ⏳ 结构体作为函数参数（值传递）
- ⏳ 结构体作为返回值
- ✅ 结构体指针作为参数（引用传递）- 已支持

#### 实现挑战

**1. 调用约定修改**
- 当前调用约定：参数从左到右压栈，每个参数占 1 slot
- 需要支持：结构体参数占多个 slot

**2. 参数传递**
```c
// 当前：int add(int a, int b)
// 栈布局：[a][b][ret_addr][old_fp]

// 需要：int getSum(struct Point p)  // Point 占 2 slots
// 栈布局：[p.x][p.y][ret_addr][old_fp]
```

**3. 返回值处理**
- 当前：返回值通过栈顶传递（1 slot）
- 需要：多 slot 返回值
  - 方案1：在栈上连续压入多个 slot
  - 方案2：调用者预留空间，被调用者写入

**4. 涉及的修改点**
- `Parser`: 解析结构体类型的参数和返回值
- `Sema`: 类型检查，计算参数和返回值的 slot 数
- `CodeGen`:
  - 函数调用时压入多 slot 参数
  - 函数返回时处理多 slot 返回值
  - 修改 `ADJSP` 指令的参数清理逻辑
- `VM`:
  - 修改 `RET` 指令支持多 slot 返回值
  - 可能需要新增 `RETV` 指令（返回结构体）

#### 详细实现方案

见下文 **Phase 3 实现指南**

---

### 阶段四：结构体整体赋值（计划中）

**目标**：支持结构体变量之间的整体赋值。

**状态**：⏳ 待实现

#### 新增功能

```c
struct Point {
    int x;
    int y;
};

int main() {
    struct Point p1;
    p1.x = 10;
    p1.y = 20;

    struct Point p2;
    p2 = p1;  // 整体赋值，复制所有成员

    return p2.x + p2.y;  // 返回 30
}
```

#### 功能清单

- ⏳ 结构体整体赋值 `p2 = p1`
- ⏳ 嵌套结构体的整体赋值
- ⏳ 包含数组成员的结构体赋值

#### 实现挑战

**1. 多 slot 复制**
- 当前 `STORE` 指令只处理单个 slot
- 需要循环复制多个 slot

**2. 实现方案**

**方案1：添加 MEMCPY 指令**
```cpp
// vm.h
enum class OpCode : uint8_t {
    // ...
    MEMCPY,  // 内存复制: size = operand; dst = pop(); src = pop();
             // memcpy(stack[dst], stack[src], size)
};

// codegen.cpp
void CodeGen::genBinaryOp(BinaryOpNode* expr) {
    if (expr->getOperator() == TokenType::Assign) {
        auto left_type = expr->getLeft()->getResolvedType();
        if (left_type->isStruct()) {
            int slot_count = left_type->getSlotCount();

            // 计算源地址和目标地址
            genExpressionAddr(expr->getRight());  // 源地址
            genExpressionAddr(expr->getLeft());   // 目标地址

            // 执行内存复制
            code_.emit(OpCode::MEMCPY, slot_count);
            return;
        }
    }
    // ... 其他情况
}
```

**方案2：展开为循环复制**
```cpp
// 生成类似以下的代码：
// for (int i = 0; i < slot_count; i++) {
//     dst[i] = src[i];
// }

void CodeGen::genStructAssign(ExprNode* left, ExprNode* right, int slot_count) {
    for (int i = 0; i < slot_count; i++) {
        // 加载源地址 + i
        genExpressionAddr(right);
        code_.emit(OpCode::ADDPTR, i);
        code_.emit(OpCode::LOADM);

        // 存储到目标地址 + i
        genExpressionAddr(left);
        code_.emit(OpCode::ADDPTR, i);
        code_.emit(OpCode::STOREM);
    }
}
```

**推荐方案**：方案1（MEMCPY 指令）
- 优点：代码简洁，执行效率高
- 缺点：需要添加新指令

#### 涉及的修改点

- `VM`: 添加 `MEMCPY` 指令
- `CodeGen`: 在赋值表达式中检测结构体类型，生成 MEMCPY
- `Sema`: 确保类型兼容性检查已实现（Phase 2 已完成）

#### 详细实现方案

见下文 **Phase 4 实现指南**

---

### 阶段五：初始化列表（计划中）

**目标**：支持数组和结构体的初始化列表语法（扁平化模式）。

**状态**：⏳ 待实现

#### 新增功能

```c
struct Point {
    int x;
    int y;
};

struct Line {
    struct Point start;
    struct Point end;
};

int main() {
    // 1. 数组初始化（优先实现）
    int arr[5] = {1, 2, 3, 4, 5};
    int partial[5] = {1, 2};  // 剩余元素初始化为 0: {1, 2, 0, 0, 0}

    // 2. 多维数组初始化（扁平化）
    int matrix[2][3] = {1, 2, 3, 4, 5, 6};
    // 等价于: matrix[0][0]=1, matrix[0][1]=2, matrix[0][2]=3,
    //         matrix[1][0]=4, matrix[1][1]=5, matrix[1][2]=6

    // 3. 结构体初始化（扁平化）
    struct Point p = {10, 20};
    struct Line line = {0, 0, 10, 10};  // start.x, start.y, end.x, end.y

    // 4. 包含数组成员的结构体（扁平化）
    struct Data {
        int value;
        int arr[3];
    };
    struct Data d = {100, 1, 2, 3};  // value=100, arr[0]=1, arr[1]=2, arr[2]=3

    // 5. 部分初始化（剩余成员置 0）
    struct Point p2 = {5};  // p2.x = 5, p2.y = 0
    int arr2[10] = {1, 2, 3};  // 剩余 7 个元素为 0

    return arr[0] + p.x + line.end.x + d.arr[0];
}
```

#### 功能清单

- ⏳ 数组初始化列表 `int arr[5] = {1, 2, 3, 4, 5}`
- ⏳ 多维数组初始化（扁平化）`int m[2][3] = {1, 2, 3, 4, 5, 6}`
- ⏳ 结构体初始化列表（扁平化）`struct Point p = {10, 20}`
- ⏳ 部分初始化（剩余元素/成员置 0）
- ⏳ 包含数组成员的结构体初始化

#### 设计决策：为什么选择扁平化模式

**扁平化模式**：按内存布局顺序填充，不需要嵌套大括号
```c
struct Line line = {1, 2, 3, 4};  // ✅ 支持
int matrix[2][3] = {1, 2, 3, 4, 5, 6};  // ✅ 支持
```

**嵌套模式**：需要显式的嵌套大括号
```c
struct Line line = {{1, 2}, {3, 4}};  // ❌ 暂不支持
int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};  // ❌ 暂不支持
```

**选择扁平化的理由**：

1. **实现简单**：
   - 只需按顺序填充 slot，无需递归解析
   - 不需要维护嵌套层级的状态栈
   - 代码量约 100-150 行

2. **符合内存布局**：
   - 直接反映数据在内存中的实际排列
   - 有助于理解编译器的内存管理

3. **教学价值**：
   - 清晰展示"初始化列表 = 按顺序填充内存"的本质
   - 避免复杂的嵌套解析逻辑

4. **实用性**：
   - 覆盖 90% 的实际使用场景
   - 对于简单的结构体和数组完全够用

**未来扩展**：
- 如果需要，可以在后续版本中添加嵌套大括号支持
- 扁平化模式作为基础，扩展嵌套模式只需增加递归逻辑

#### 实现挑战

**1. 语法解析**
- 解析 `{expr1, expr2, ...}` 语法
- 区分初始化列表和复合语句（都用大括号）
- 处理逗号分隔的表达式列表

**2. 类型检查**
- 验证初始化列表的元素数量不超过类型的 slot 数量
- 检查每个表达式的类型与对应位置的类型兼容
- 处理部分初始化（元素数量少于 slot 数量）

**3. 代码生成**
- 按顺序为每个 slot 生成初始化代码
- 为未指定的 slot 生成 0 初始化代码
- 计算正确的内存偏移量

**4. 数组特殊处理**
- 多维数组的 slot 计算：`arr[2][3]` 占 6 个 slot
- 数组元素的偏移计算：`arr[i][j]` 的偏移 = `i * 3 + j`

#### 实现方案

**阶段 5.1：数组初始化（优先实现）**

数组初始化比结构体简单，建议先实现：

```c
// 一维数组
int arr[5] = {1, 2, 3, 4, 5};

// 多维数组（扁平化）
int matrix[2][3] = {1, 2, 3, 4, 5, 6};

// 部分初始化
int arr2[10] = {1, 2, 3};  // 剩余 7 个元素为 0
```

**实现要点**：

1. **AST 节点**：
```cpp
// ast.h
class InitListNode : public ExprNode {
private:
    std::vector<std::unique_ptr<ExprNode>> elements_;

public:
    void addElement(std::unique_ptr<ExprNode> element) {
        elements_.push_back(std::move(element));
    }

    size_t size() const { return elements_.size(); }
    ExprNode* getElement(size_t i) const { return elements_[i].get(); }

    std::string toString() const override {
        std::string result = "{";
        for (size_t i = 0; i < elements_.size(); i++) {
            if (i > 0) result += ", ";
            result += elements_[i]->toString();
        }
        result += "}";
        return result;
    }
};

// 修改 VarDeclStmtNode，添加初始化列表支持
class VarDeclStmtNode : public StmtNode {
private:
    std::string name_;
    std::shared_ptr<Type> type_;
    std::unique_ptr<ExprNode> init_expr_;  // 可能是 InitListNode

public:
    bool hasInitializer() const { return init_expr_ != nullptr; }
    ExprNode* getInitializer() const { return init_expr_.get(); }
    bool isInitList() const {
        return dynamic_cast<InitListNode*>(init_expr_.get()) != nullptr;
    }
};
```

2. **Parser 修改**：
```cpp
// parser.cpp
std::unique_ptr<ExprNode> Parser::parsePrimaryExpression() {
    // 在 primary expression 中添加初始化列表的解析
    if (current().type == TokenType::LBRACE) {
        return parseInitList();
    }
    // ... 其他情况
}

std::unique_ptr<InitListNode> Parser::parseInitList() {
    expect(TokenType::LBRACE);
    auto init_list = std::make_unique<InitListNode>();

    // 空初始化列表: {}
    if (current().type == TokenType::RBRACE) {
        advance();
        return init_list;
    }

    // 解析表达式列表
    while (true) {
        auto expr = parseAssignmentExpression();
        init_list->addElement(std::move(expr));

        if (current().type == TokenType::COMMA) {
            advance();
            // 允许尾随逗号: {1, 2, 3,}
            if (current().type == TokenType::RBRACE) {
                break;
            }
        } else {
            break;
        }
    }

    expect(TokenType::RBRACE);
    return init_list;
}

std::unique_ptr<VarDeclStmtNode> Parser::parseVarDecl() {
    // ... 解析类型和变量名 ...

    std::unique_ptr<ExprNode> init_expr = nullptr;
    if (current().type == TokenType::ASSIGN) {
        advance();

        // 检查是否是初始化列表
        if (current().type == TokenType::LBRACE) {
            init_expr = parseInitList();
        } else {
            init_expr = parseExpression();
        }
    }

    expect(TokenType::SEMICOLON);
    return std::make_unique<VarDeclStmtNode>(type, name, std::move(init_expr));
}
```

3. **Sema 类型检查**：
```cpp
// sema.cpp
void Sema::analyzeVarDecl(VarDeclStmtNode* stmt) {
    auto var_type = stmt->getResolvedType();

    if (stmt->hasInitializer() && stmt->isInitList()) {
        auto init_list = static_cast<InitListNode*>(stmt->getInitializer());
        checkInitList(init_list, var_type);
    }
}

void Sema::checkInitList(InitListNode* init_list, std::shared_ptr<Type> expected_type) {
    int expected_slots = expected_type->getSlotCount();
    int provided_count = init_list->size();

    // 检查元素数量
    if (provided_count > expected_slots) {
        error("初始化列表元素过多：期望最多 " + std::to_string(expected_slots) +
              " 个，实际提供 " + std::to_string(provided_count) + " 个");
        return;
    }

    // 检查每个元素的类型（简化版：假设都是 int）
    for (size_t i = 0; i < init_list->size(); i++) {
        auto element = init_list->getElement(i);
        analyzeExpression(element);

        auto element_type = element->getResolvedType();
        if (!element_type->isInt()) {
            error("初始化列表第 " + std::to_string(i) + " 个元素类型错误");
        }
    }
}
```

4. **CodeGen 代码生成**：
```cpp
// codegen.cpp
void CodeGen::genVarDecl(VarDeclStmtNode* stmt) {
    auto type = stmt->getResolvedType();
    int slot_count = type->getSlotCount();
    int base_offset = allocLocal(stmt->getName(), slot_count);

    if (stmt->hasInitializer() && stmt->isInitList()) {
        // 初始化列表
        auto init_list = static_cast<InitListNode*>(stmt->getInitializer());
        genInitList(base_offset, type, init_list);
    } else if (stmt->hasInitializer()) {
        // 单个表达式初始化
        genExpression(stmt->getInitializer());
        code_.emit(OpCode::LEA, base_offset);
        code_.emit(OpCode::STOREM);
    } else {
        // 无初始化，全部置 0
        for (int i = 0; i < slot_count; i++) {
            code_.emit(OpCode::PUSH, 0);
            code_.emit(OpCode::LEA, base_offset + i);
            code_.emit(OpCode::STOREM);
        }
    }
}

void CodeGen::genInitList(int base_offset, std::shared_ptr<Type> type, InitListNode* init_list) {
    int slot_count = type->getSlotCount();

    // 按顺序填充提供的元素
    for (size_t i = 0; i < init_list->size(); i++) {
        auto element = init_list->getElement(i);

        // 生成元素表达式的值
        genExpression(element);

        // 存储到对应的 slot
        code_.emit(OpCode::LEA, base_offset + i);
        code_.emit(OpCode::STOREM);
    }

    // 剩余 slot 初始化为 0
    for (size_t i = init_list->size(); i < slot_count; i++) {
        code_.emit(OpCode::PUSH, 0);
        code_.emit(OpCode::LEA, base_offset + i);
        code_.emit(OpCode::STOREM);
    }
}
```

**阶段 5.2：结构体初始化**

数组初始化完成后，结构体初始化使用相同的逻辑：

```c
struct Point { int x; int y; };
struct Point p = {10, 20};  // 扁平化，按顺序填充
```

**实现要点**：
- 复用数组初始化的代码
- 结构体的 `getSlotCount()` 已经实现
- 按成员顺序填充即可

**阶段 5.3：混合场景**

```c
struct Data {
    int value;
    int arr[3];
};
struct Data d = {100, 1, 2, 3};  // value=100, arr={1,2,3}
```

**实现要点**：
- 扁平化模式下，数组成员和普通成员没有区别
- 按内存布局顺序填充：slot[0]=100, slot[1]=1, slot[2]=2, slot[3]=3

#### 涉及的修改点

1. **AST** (`include/ast.h`):
   - 添加 `InitListNode` 类
   - 修改 `VarDeclStmtNode`，添加初始化列表支持

2. **Parser** (`src/parser.cpp`):
   - 添加 `parseInitList()` 方法
   - 修改 `parseVarDecl()`，支持初始化列表语法

3. **Sema** (`src/sema.cpp`):
   - 添加 `checkInitList()` 方法
   - 验证元素数量和类型

4. **CodeGen** (`src/codegen.cpp`):
   - 添加 `genInitList()` 方法
   - 按顺序生成初始化代码

5. **无需修改**：
   - Lexer（已有 `{` 和 `}` token）
   - Type 系统（已有 `getSlotCount()` 方法）
   - VM（使用现有指令）

#### 测试用例

```c
// test_init_list_array.c
int main() {
    // 测试1：完整初始化
    int arr1[5] = {1, 2, 3, 4, 5};

    // 测试2：部分初始化
    int arr2[5] = {10, 20};  // {10, 20, 0, 0, 0}

    // 测试3：多维数组
    int matrix[2][3] = {1, 2, 3, 4, 5, 6};

    // 测试4：空初始化
    int arr3[3] = {};  // {0, 0, 0}

    return arr1[0] + arr2[0] + matrix[1][2] + arr3[0];  // 1 + 10 + 6 + 0 = 17
}

// test_init_list_struct.c
struct Point { int x; int y; };
struct Line { struct Point start; struct Point end; };

int main() {
    // 测试1：基本结构体
    struct Point p1 = {10, 20};

    // 测试2：嵌套结构体（扁平化）
    struct Line line = {0, 0, 10, 10};

    // 测试3：部分初始化
    struct Point p2 = {5};  // {5, 0}

    return p1.x + p1.y + line.end.x + p2.x;  // 10 + 20 + 10 + 5 = 45
}

// test_init_list_mixed.c
struct Data {
    int value;
    int arr[3];
};

int main() {
    struct Data d = {100, 1, 2, 3};
    return d.value + d.arr[0] + d.arr[1] + d.arr[2];  // 100 + 1 + 2 + 3 = 106
}
```

#### 详细实现指南

见下文 **Phase 5 实现指南**

---

### 阶段六：全局变量（计划中）

**目标**：支持全局变量的定义和使用。

**状态**：⏳ 待实现

#### 新增功能

```c
// 全局变量
int global_x = 100;
int global_arr[5];
struct Point global_point;

int getGlobalX() {
    return global_x;
}

int main() {
    global_x = 200;
    global_arr[0] = 10;
    global_point.x = 5;
    global_point.y = 10;

    return getGlobalX() + global_arr[0] + global_point.x;
}
```

#### 功能清单

- ⏳ 全局基本类型变量
- ⏳ 全局数组
- ⏳ 全局结构体
- ⏳ 全局变量初始化

#### 实现挑战

**1. 内存布局修改**
- 当前：所有变量都是局部变量，存储在栈上
- 需要：全局变量存储在独立的全局数据区

**2. VM 架构调整**
```cpp
class VM {
private:
    std::vector<int32_t> stack_;      // 栈（局部变量）
    std::vector<int32_t> globals_;    // 全局数据区（新增）
    // ...
};
```

**3. 新增指令**
```cpp
enum class OpCode : uint8_t {
    // ...
    LOADG,   // 加载全局变量: push(globals[operand])
    STOREG,  // 存储全局变量: globals[operand] = pop()
    LEAG,    // 加载全局变量地址: push(GLOBAL_BASE + operand)
};
```

**4. 符号表修改**
- 区分全局符号和局部符号
- 全局符号使用绝对地址，局部符号使用相对 fp 的偏移

#### 详细实现方案

见下文 **Phase 6 实现指南**

---

### 阶段七：自引用结构体（可选）

**目标**：支持结构体自引用，实现链表等数据结构。

**状态**：⏳ 待实现（优先级最低）

#### 新增功能

```c
// 链表节点
struct Node {
    int data;
    struct Node *next;  // 自引用
};

int main() {
    struct Node n1, n2, n3;

    n1.data = 1;
    n1.next = &n2;

    n2.data = 2;
    n2.next = &n3;

    n3.data = 3;
    n3.next = 0;  // NULL

    // 遍历链表
    struct Node *p = &n1;
    int sum = 0;
    while (p != 0) {
        sum = sum + p->data;
        p = p->next;
    }

    return sum;  // 返回 6
}
```

#### 功能清单

- ⏳ 自引用结构体定义
- ⏳ 指针成员指向同类型结构体

#### 实现挑战

**1. 类型解析顺序问题**
- 解析 `struct Node *next` 时，`Node` 类型尚未完全定义
- 需要支持"不完整类型"（incomplete type）

**2. 简化实现方案（无需前向声明）**

**核心思路**：延迟类型解析

```cpp
// sema.cpp
void Sema::analyzeStructDecl(StructDeclNode* node) {
    auto struct_type = std::make_shared<StructType>(node->getName());

    // 先注册结构体名称（创建不完整类型）
    struct_types_[node->getName()] = struct_type;

    // 再解析成员类型
    for (const auto& [type_str, member_name] : node->getMembers()) {
        auto member_type = stringToType(type_str);

        // 如果是指向自身的指针，此时可以找到类型
        // 例如：type_str = "struct Node*"
        //   → stringToType 会查找 struct_types_["Node"]
        //   → 找到刚注册的 struct_type（虽然成员还未完全填充）
        //   → 创建 PointerType(struct_type)

        struct_type->addMember(member_name, member_type);
    }
}
```

**关键点**：
- 只允许指针形式的自引用 `struct Node *next` ✅
- 不允许值形式的自引用 `struct Node next` ❌（会导致无限递归）
- 指针类型不需要知道结构体的完整布局，只需要知道名称

**3. 类型检查**
```cpp
// 在 stringToType 中处理自引用
std::shared_ptr<Type> Sema::stringToType(const std::string& type_name) {
    // 先处理指针
    if (type_name.back() == '*') {
        auto base_type = stringToType(type_name.substr(0, type_name.size() - 1));
        if (base_type) {
            return std::make_shared<PointerType>(base_type);
        }
    }

    // 再处理结构体
    if (type_name.substr(0, 7) == "struct ") {
        std::string struct_name = type_name.substr(7);
        auto it = struct_types_.find(struct_name);
        if (it != struct_types_.end()) {
            // 即使结构体尚未完全定义，也可以返回
            return it->second;
        }
    }

    return nullptr;
}
```

**4. 值形式自引用的检测**
```cpp
void Sema::analyzeStructDecl(StructDeclNode* node) {
    // ... 前面的代码 ...

    for (const auto& [type_str, member_name] : node->getMembers()) {
        auto member_type = stringToType(type_str);

        // 检测值形式的自引用
        if (member_type->isStruct()) {
            auto member_struct = std::dynamic_pointer_cast<StructType>(member_type);
            if (member_struct->getName() == node->getName()) {
                error("结构体不能包含自身类型的成员（只能包含指向自身的指针）");
                continue;
            }
        }

        struct_type->addMember(member_name, member_type);
    }
}
```

#### 涉及的修改点

- `Sema`:
  - 修改 `analyzeStructDecl`，先注册结构体名称
  - 添加值形式自引用的检测
- 无需修改 Parser、CodeGen、VM（指针操作已支持）

#### 详细实现方案

见下文 **Phase 7 实现指南**

---

## 技术实现细节

### 1. Lexer 修改

添加 `struct` 关键字：

```cpp
// token.h
enum class TokenType {
    // ...
    STRUCT,      // struct
    DOT,         // .
    ARROW,       // -> (阶段二)
    // ...
};
```

### 2. Type 系统

```cpp
// type.h
class StructType : public Type {
private:
    std::string name_;
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> members_;
    mutable int cached_slot_count_ = -1;

public:
    StructType(const std::string& name) : Type(TypeKind::Struct), name_(name) {}

    void addMember(const std::string& name, std::shared_ptr<Type> type) {
        members_.push_back({name, type});
        cached_slot_count_ = -1;  // 失效缓存
    }

    int getSlotCount() const override {
        if (cached_slot_count_ >= 0) return cached_slot_count_;
        int total = 0;
        for (const auto& [name, type] : members_) {
            total += type->getSlotCount();
        }
        cached_slot_count_ = total;
        return total;
    }

    int getMemberOffset(const std::string& member) const {
        int offset = 0;
        for (const auto& [name, type] : members_) {
            if (name == member) return offset;
            offset += type->getSlotCount();
        }
        throw std::runtime_error("Unknown member: " + member);
    }

    std::shared_ptr<Type> getMemberType(const std::string& member) const {
        for (const auto& [name, type] : members_) {
            if (name == member) return type;
        }
        return nullptr;
    }

    const std::string& getName() const { return name_; }
    bool isStruct() const override { return true; }
};
```

### 3. AST 节点

```cpp
// ast.h

// 结构体定义节点
class StructDeclNode : public ASTNode {
private:
    std::string name_;
    std::vector<std::pair<std::string, std::string>> members_;  // (type, name)

public:
    StructDeclNode(const std::string& name) : name_(name) {}

    void addMember(const std::string& type, const std::string& name) {
        members_.push_back({type, name});
    }

    const std::string& getName() const { return name_; }
    const auto& getMembers() const { return members_; }

    std::string toString() const override {
        std::string result = "StructDecl(" + name_ + ") {\n";
        for (const auto& [type, name] : members_) {
            result += "  " + type + " " + name + ";\n";
        }
        result += "}";
        return result;
    }
};

// 成员访问节点
class MemberAccessNode : public ExprNode {
private:
    std::unique_ptr<ExprNode> object_;
    std::string member_;

public:
    MemberAccessNode(std::unique_ptr<ExprNode> object, const std::string& member)
        : object_(std::move(object)), member_(member) {}

    ExprNode* getObject() const { return object_.get(); }
    const std::string& getMember() const { return member_; }

    std::string toString() const override {
        return object_->toString() + "." + member_;
    }
};
```

### 4. Parser 修改

```cpp
// parser.h
class Parser {
    // ...
    std::unique_ptr<StructDeclNode> parseStructDecl();
    std::unique_ptr<ExprNode> parseMemberAccess(std::unique_ptr<ExprNode> object);
};

// parser.cpp
std::unique_ptr<StructDeclNode> Parser::parseStructDecl() {
    expect(TokenType::STRUCT);
    std::string name = expect(TokenType::IDENT).value;
    expect(TokenType::LBRACE);

    auto structDecl = std::make_unique<StructDeclNode>(name);

    while (current().type != TokenType::RBRACE) {
        std::string type = expect(TokenType::IDENT).value;
        std::string memberName = expect(TokenType::IDENT).value;

        // 处理数组成员 int arr[10];
        if (current().type == TokenType::LBRACKET) {
            // ...
        }

        structDecl->addMember(type, memberName);
        expect(TokenType::SEMICOLON);
    }

    expect(TokenType::RBRACE);
    expect(TokenType::SEMICOLON);
    return structDecl;
}

std::unique_ptr<ExprNode> Parser::parseMemberAccess(std::unique_ptr<ExprNode> object) {
    expect(TokenType::DOT);
    std::string member = expect(TokenType::IDENT).value;
    return std::make_unique<MemberAccessNode>(std::move(object), member);
}
```

### 5. Sema 修改

```cpp
// sema.h
class Sema {
    std::unordered_map<std::string, std::shared_ptr<StructType>> struct_types_;

    void analyzeStructDecl(StructDeclNode* node);
    std::shared_ptr<Type> analyzeMemberAccess(MemberAccessNode* node);
};

// sema.cpp
void Sema::analyzeStructDecl(StructDeclNode* node) {
    auto structType = std::make_shared<StructType>(node->getName());

    for (const auto& [type, name] : node->getMembers()) {
        auto memberType = resolveType(type);
        structType->addMember(name, memberType);
    }

    struct_types_[node->getName()] = structType;
}

std::shared_ptr<Type> Sema::analyzeMemberAccess(MemberAccessNode* node) {
    auto objectType = analyzeExpression(node->getObject());

    if (!objectType->isStruct()) {
        throw std::runtime_error("Member access on non-struct type");
    }

    auto structType = std::static_pointer_cast<StructType>(objectType);
    auto memberType = structType->getMemberType(node->getMember());

    if (!memberType) {
        throw std::runtime_error("Unknown member: " + node->getMember());
    }

    node->setResolvedType(memberType);
    return memberType;
}
```

### 6. CodeGen 修改

```cpp
// codegen.cpp

void CodeGen::genVarDecl(VarDeclStmtNode* stmt) {
    auto type = stmt->getResolvedType();
    int slot_count = type->getSlotCount();

    allocLocal(stmt->getName(), slot_count);

    // 初始化为 0
    for (int i = 0; i < slot_count; i++) {
        code_.emit(OpCode::PUSH, 0);
    }
}

void CodeGen::genMemberAccess(MemberAccessNode* expr) {
    // 计算对象地址
    if (auto* var = dynamic_cast<VariableNode*>(expr->getObject())) {
        int base_offset = getLocal(var->getName());

        // 获取成员偏移
        auto structType = std::static_pointer_cast<StructType>(
            var->getResolvedType()
        );
        int member_offset = structType->getMemberOffset(expr->getMember());

        // 计算成员地址并加载
        code_.emit(OpCode::LEA, base_offset + member_offset);
        code_.emit(OpCode::LOADM);
    }
}

void CodeGen::genMemberAccessAddr(MemberAccessNode* expr) {
    // 类似 genMemberAccess，但不执行 LOADM
    if (auto* var = dynamic_cast<VariableNode*>(expr->getObject())) {
        int base_offset = getLocal(var->getName());
        auto structType = std::static_pointer_cast<StructType>(
            var->getResolvedType()
        );
        int member_offset = structType->getMemberOffset(expr->getMember());
        code_.emit(OpCode::LEA, base_offset + member_offset);
    }
}
```

---

## 测试用例

### 阶段一测试

```c
// test_struct_basic.c
struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    return p.x + p.y;  // 期望: 30
}
```

```c
// test_struct_pointer_member.c
struct Data {
    int value;
    int *ptr;
};

int main() {
    struct Data d;
    int x = 42;
    d.value = 10;
    d.ptr = &x;
    return d.value + *d.ptr;  // 期望: 52
}
```

```c
// test_struct_array_member.c
struct Container {
    int arr[3];
};

int main() {
    struct Container c;
    c.arr[0] = 1;
    c.arr[1] = 2;
    c.arr[2] = 3;
    return c.arr[0] + c.arr[1] + c.arr[2];  // 期望: 6
}
```

### 阶段二测试

```c
// test_struct_pointer.c
struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    struct Point *ptr = &p;
    ptr->x = 10;
    ptr->y = 20;
    return ptr->x + ptr->y;  // 期望: 30
}
```

```c
// test_struct_nested.c
struct Point {
    int x;
    int y;
};

struct Line {
    struct Point start;
    struct Point end;
};

int main() {
    struct Line line;
    line.start.x = 0;
    line.start.y = 0;
    line.end.x = 10;
    line.end.y = 10;
    return line.end.x + line.end.y;  // 期望: 20
}
```

```c
// test_struct_array.c
struct Point {
    int x;
    int y;
};

int main() {
    struct Point points[3];
    points[0].x = 1;
    points[1].x = 2;
    points[2].x = 3;
    return points[0].x + points[1].x + points[2].x;  // 期望: 6
}
```

### 阶段三测试

```c
// test_struct_function.c
struct Point {
    int x;
    int y;
};

struct Point createPoint(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

int getSum(struct Point p) {
    return p.x + p.y;
}

int main() {
    struct Point p = createPoint(10, 20);
    return getSum(p);  // 期望: 30
}
```

---

## 实现顺序

1. **Lexer**：添加 `struct` 关键字和 `.` 运算符
2. **Type 系统**：实现 `StructType` 类
3. **AST**：添加 `StructDeclNode` 和 `MemberAccessNode`
4. **Parser**：解析结构体定义和成员访问
5. **Sema**：结构体符号表和类型检查
6. **CodeGen**：结构体内存布局和成员访问代码生成
7. **测试**：编写测试用例验证功能

---

## 注意事项

1. **内存布局**：结构体成员按声明顺序连续存储
2. **对齐**：暂不考虑内存对齐，所有类型按 slot 计算
3. **作用域**：结构体定义只能在全局作用域
4. **命名**：结构体名字和变量名字在不同的命名空间
5. **类型检查**：严格检查成员访问的类型正确性

---

## 后续扩展（可选）

- 结构体初始化列表 `struct Point p = {10, 20};`
- 结构体整体赋值 `p2 = p1;`
- `typedef` 支持 `typedef struct Point Point;`
- 匿名结构体
- 位域（bit field）
