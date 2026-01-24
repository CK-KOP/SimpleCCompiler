# SimpleC 测试样例说明

本目录包含 SimpleC 编译器的所有测试样例，按功能特性分类组织。

## 目录结构

```
examples/
├── control/           # 控制流测试
├── pointer/           # 指针测试
├── array/             # 数组测试
├── recursive/         # 递归测试
├── scope/             # 作用域测试
├── struct/            # 结构体测试
├── comprehensive/      # 综合测试
└── error/             # 错误检测测试
```

## 各分类说明

### 1. control/ - 控制流测试

测试 if-else、for、while、do-while、break、continue 等控制流语句。

**样例文件**：
- `control_flow.c` - 控制流综合测试，包含多种控制流语句的嵌套使用

**运行测试**：
```bash
./build/simplec examples/control/control_flow.c
# 预期返回值: 23
```

---

### 2. pointer/ - 指针测试

测试基本指针、多级指针、指针参数、指针运算、指针与数组等。

**样例文件**：
- `pointer_comprehensive.c` - 指针综合测试，包含多级指针、指针参数、指针与数组

**运行测试**：
```bash
./build/simplec examples/pointer/pointer_comprehensive.c
# 预期返回值: 0
```

---

### 3. array/ - 数组测试

测试数组声明、访问、遍历、排序等操作。

**样例文件**：
- `array_comprehensive.c` - 数组综合测试，包含数组访问、排序、求和、最大值

**运行测试**：
```bash
./build/simplec examples/array/array_comprehensive.c
# 预期返回值: 34
```

---

### 4. recursive/ - 递归测试

测试各种递归算法：阶乘、斐波那契、最大公约数、数组递归求和等。

**样例文件**：
- `recursive_algorithms.c` - 递归算法综合测试

**运行测试**：
```bash
./build/simplec examples/recursive/recursive_algorithms.c
# 预期返回值: 196
```

---

### 5. scope/ - 作用域测试

测试嵌套作用域、变量遮蔽等作用域相关功能。

**样例文件**：
- `scope_nested.c` - 多层嵌套作用域测试

**运行测试**：
```bash
./build/simplec examples/scope/scope_nested.c
# 预期返回值: 1375
```

---

### 6. struct/ - 结构体测试

测试结构体的定义、成员访问、嵌套结构体、结构体数组、结构体指针等。

**样例文件**：
- `struct_basic.c` - 结构体基础功能（Phase 1）
- `struct_comprehensive.c` - 结构体综合功能（Phase 2）
- `struct_advanced.c` - 高级结构体功能
- `struct_assign.c` - 结构体赋值测试

**运行测试**：
```bash
./build/simplec examples/struct/struct_basic.c
# 预期返回值: 350

./build/simplec examples/struct/struct_comprehensive.c
# 预期返回值: 610

./build/simplec examples/struct/struct_advanced.c
# 预期返回值: 995
```

---

### 7. comprehensive/ - 综合测试

测试多种功能的综合运用，模拟实际应用场景。

**样例文件**：
- `shopping_cart.c` - 购物车系统，综合运用数组、指针、递归、控制流、作用域

**运行测试**：
```bash
./build/simplec examples/comprehensive/shopping_cart.c
# 预期返回值: 849
```

---

### 8. error/ - 错误检测测试

测试编译器的错误检测能力，验证类型检查、语义分析是否正确。

**样例文件**：
- `basic_errors.c` - 基础错误测试
- `type_errors.c` - 类型错误测试

**运行测试**：
```bash
./build/simplec examples/error/basic_errors.c
# 预期: 检测到语义错误

./build/simplec examples/error/type_errors.c
# 预期: 检测到多个类型错误
```

---

## 快速测试

### 测试所有样例

```bash
# 测试所有正确用例
for dir in control pointer array recursive scope struct comprehensive; do
    echo "=== Testing $dir ==="
    for file in examples/$dir/*.c; do
        echo "Testing: $file"
        ./build/simplec "$file"
    done
    echo ""
done
```

### 测试特定分类

```bash
# 只测试控制流
./build/simplec examples/control/*.c

# 只测试指针
./build/simplec examples/pointer/*.c

# 只测试结构体
./build/simplec examples/struct/*.c
```

---

## 测试覆盖的功能点

### ✅ 已支持的功能

- [x] 基本数据类型
- [x] 控制流语句
- [x] 函数定义和调用
- [x] 递归
- [x] 指针（包括多级指针）
- [x] 数组
- [x] 嵌套作用域
- [x] 结构体定义和使用
- [x] 结构体指针和箭头操作符
- [x] 结构体数组
- [x] 嵌套结构体
- [x] 类型检查和错误检测

### ⏳ 待实现的功能

- [ ] 全局变量（Phase 6）
- [ ] 结构体初始化列表
- [ ] 自引用结构体
- [ ] 枚举类型
- [ ] 联合体

---

## 贡献指南

如果添加新的测试样例：

1. **选择合适的分类**：根据测试内容选择对应的子目录
2. **命名规范**：使用描述性的文件名，如 `pointer_comprehensive.c`
3. **注释说明**：在文件开头添加注释说明测试目的和预期结果
4. **测试验证**：确保测试能够正确编译并返回预期值
5. **更新文档**：在本文档中添加新测试的说明

---

## 总结

本测试套件覆盖了 SimpleC 编译器的所有核心功能，每个测试都有明确的预期结果，便于验证编译器的正确性。通过这些测试，可以全面评估编译器的功能和稳定性。
