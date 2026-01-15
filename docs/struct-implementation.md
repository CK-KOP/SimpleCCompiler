# 结构体功能实现计划

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
- ❌ 结构体整体赋值 `p2 = p1;`
- ❌ 结构体指针 `struct Point *ptr;`
- ❌ 嵌套结构体
- ❌ 结构体数组
- ❌ 结构体作为函数参数/返回值

---

### 阶段二：结构体指针、嵌套、数组

**目标**：扩展结构体的组合能力。

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

  // ❌ 不支持自引用
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

---

### 阶段三：函数参数和返回值

**目标**：支持结构体在函数间传递。

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

- ✅ 结构体作为函数参数（值传递）
- ✅ 结构体作为返回值
- ✅ 结构体指针作为参数（引用传递）

#### 实现挑战

- 需要修改调用约定（多 slot 参数和返回值）
- 栈帧布局需要调整
- 参数压栈和返回值处理

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
