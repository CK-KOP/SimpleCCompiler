# SimpleC 编译器 - 文档索引

## 项目概述

SimpleC 是一个 C 语言子集编译器，支持结构体、指针、数组、全局变量、初始化列表等核心特性。本文档目录包含了项目的设计、开发和优化文档。

---

## 📚 文档结构

```
docs/
├── README.md                    # 本文件 - 文档导航
├── compilation-flow.md          # 编译流程说明（总文档）
├── development-plan.md          # 开发计划与进度（总文档）
├── dev-notes.md                 # 开发笔记与问题记录（总文档）
└── phases/                      # 具体开发阶段的文档
    ├── codegen-optimization.md
    ├── phase6_global_variables.md
    └── phase7_initialization_lists.md
```

---

## 🎯 总文档（核心文档）

### 1. 📋 [开发计划与进度](development-plan.md)
**项目总体规划和技术路线图**

**当前进度**（2026-02-01）：
- ✅ 阶段一：基础结构体
- ✅ 阶段二：结构体指针、嵌套、数组
- ✅ 阶段三：函数参数和返回值
- ✅ 阶段四：结构体整体赋值
- ✅ 阶段五：初始化列表（Phase 1 平面初始化）
- ✅ 阶段六：全局变量
- ⏳ 阶段七：自引用结构体（可选）

**适合人群**：了解项目整体规划的技术决策者

---

### 2. 🔧 [编译流程说明](compilation-flow.md)
**编译器各阶段的详细说明**

**内容**：
- Lexer（词法分析）
- Parser（语法分析）
- Sema（语义分析）
- CodeGen（代码生成）
- VM（虚拟机执行）

**适合人群**：想要了解编译器原理的学习者

---

### 3. 📝 [开发笔记](dev-notes.md)
**开发过程中的问题记录和解决方案**

**内容**：
- 遇到的技术问题
- 解决方案和经验总结
- 待完成的任务清单
- 临时笔记和想法

**适合人群**：参与日常开发的维护者，想要学习问题解决思路的开发者

---

## 📂 具体开发阶段文档

### Phase 6: 全局变量

#### [全局变量实现报告](phases/phase6_global_variables.md)
**完整的实现文档**

**实现内容**：
- 全局变量内存布局
- VM 架构调整（全局数据区）
- 新增指令（LOADG, STOREG, LEAG）
- 符号表修改
- 完整的测试验证

**状态**：✅ 已完成（2026-01-31）

---

### Phase 7: 初始化列表

#### [初始化列表实现报告](phases/phase7_initialization_lists.md)
**Phase 1 平面初始化列表的完整实现**

**实现内容**：
- 数组初始化：`int arr[3] = {1, 2, 3};`
- 结构体初始化：`struct Point p = {10, 20};`
- 标量初始化：`int x = {42};`
- 部分初始化（剩余元素补 0）
- 全局/局部变量初始化
- 完整的测试验证

**状态**：✅ 已完成（2026-02-01）

---

### 代码生成器优化

#### [CodeGen 优化完成总结](phases/codegen-optimization.md)
**代码生成器优化的完整记录**

**优化内容**：
- ✅ Phase 1: 类型判断辅助函数（8个辅助函数）
- ✅ Phase 2: 统一变量分配接口（allocateVariable）
- ✅ Phase 3: 统一变量管理系统（VariableInfo）

**核心成果**：
- 消除了 20+ 处重复的类型检查代码
- 统一了变量分配接口（genVarDecl: 42行→26行）
- 完全删除了旧的分散式变量管理系统
- 支持嵌套作用域
- 所有测试用例通过

---

## 🚀 快速导航

### 按角色查找文档

**项目新人**：
1. [development-plan.md](development-plan.md) - 了解项目规划
2. [compilation-flow.md](compilation-flow.md) - 理解编译流程
3. [examples/](../examples/README.md) - 查看测试用例

**开发人员**：
1. [development-plan.md](development-plan.md) - 查看当前阶段
2. [phases/codegen-optimization.md](phases/codegen-optimization.md) - 了解代码结构
3. [dev-notes.md](dev-notes.md) - 了解当前工作和问题

**架构师**：
1. [development-plan.md](development-plan.md) - 技术路线
2. [phases/phase6_global_variables.md](phases/phase6_global_variables.md) - 设计方案
3. [phases/codegen-optimization.md](phases/codegen-optimization.md) - 优化策略

### 按主题查找文档

**结构体实现**：
- [development-plan.md](development-plan.md) - 阶段一到阶段七的完整规划

**初始化列表**：
- [phases/phase7_initialization_lists.md](phases/phase7_initialization_lists.md) - 完整实现报告

**全局变量**：
- [phases/phase6_global_variables.md](phases/phase6_global_variables.md) - 完整实现报告

**代码生成优化**：
- [phases/codegen-optimization.md](phases/codegen-optimization.md) - 完整优化录

**编译器原理**：
- [compilation-flow.md](compilation-flow.md) - 各阶段说明

---

## 📊 当前项目状态 (2026-02-01)

### ✅ 已完成
- ✅ 基础结构体（阶段一）
- ✅ 结构体指针、嵌套、数组（阶段二）
- ✅ 结构体函数参数和返回值（阶段三）
- ✅ 结构体整体赋值（阶段四）
- ✅ 初始化列表 - Phase 1 平面初始化（阶段五）
- ✅ 全局变量支持（阶段六）
- ✅ CodeGen 优化（类型辅助函数、统一变量管理）

### ⏳ 待实现
- ⏳ 初始化列表 - Phase 2 嵌套初始化（可选）
- ⏳ 自引用结构体（阶段七，可选）

---

## 📝 文档维护规范

### 文档分类

**总文档**（放在 docs/ 根目录）：
- `compilation-flow.md` - 程序运行流程
- `development-plan.md` - 开发计划与进度
- `dev-notes.md` - 开发笔记与问题记录

**具体开发文档**（放在 docs/phases/ 目录）：
- Phase 相关的设计文档
- 具体功能的实现报告
- 优化和重构记录

### 新增文档
1. 确定文档类型（总文档 vs 具体开发文档）
2. 放在对应目录（docs/ vs docs/phases/）
3. 使用清晰的文件名（小写英文，连字符分隔）
4. 在本 README 中添加索引
5. 更新相关文档的交叉引用

### 更新文档
1. 在文档顶部标注最后更新日期
2. 重大变更添加变更日志
3. 同步更新 development-plan.md 中的进度

### 废弃文档
1. 将内容合并到其他文档
2. 删除原文件
3. 更新所有引用

---

**最后更新**: 2026-02-01
**维护者**: KOP
