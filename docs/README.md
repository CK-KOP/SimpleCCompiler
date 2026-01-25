# SimpleC 编译器 - 文档索引

## 项目概述

SimpleC 是一个 C 语言子��编译器，支持结构体、指针、数组等核心特性。本文档目录包含了项目的设计、开发和优化文档。

---

## 核心文档

### 📋 [开发计划](development-plan.md)
**项目总体规划和技术路线图**

- **关键技术挑战**：语法歧义问题、结构体多义性等
- **实现分阶段**：从基础结���体到自引用的完整路线
- **当前进度**：
  - ✅ 阶段一：基础结构体（已完成）
  - ✅ 阶段二：结构体指针、嵌套、数组（已完成）
  - ✅ 阶段三：函数参数和返回值（已完成）
  - ✅ 阶段四：结构体整体赋值（已完成）
  - ⏳ 阶段五：初始化列表（待实现）
  - 🔄 阶段六：全局变量（实现中）
  - ⏳ 阶段七：自引用结构体（可选）

**适合人群**：了解项目整体规划的技术决策者

---

### 🚀 [CodeGen 优化完成总结](codegen-optimization.md)
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
- 所有 13 个测试用例通过

**适合人群**：想要了解代码优化细节的开发者

---

### 🌍 [Phase 6: 全局变量设计](phase6-global-variables-design.md)
**全局变量实现的完整设计方案**

**设计内容**：
- 全局变量内存布局
- VM 架构调整（全局数据区）
- 新增指令（LOADG, STOREG, LEAG）
- 符号表修改
- 实现步骤和测试方案

**前置条件**：
- 已有 VariableInfo 结构（is_global 标记）
- 已有 next_global_offset_ 计数器
- 已有 allocateGlobalVariable 接口框架

**适合人群**：准备实现全局变量功能的开发者

---

### 📝 [开发笔记](dev-notes.md)
**开发过程中的临时笔记和TODO**

**内容**：
- 当前正在进行的工作
- 发现的问题和临时解决方案
- 待完成的任务清单

**适合人群**：参与日常开发的维护者

---

### 🔧 [编译流程](compilation-flow.md)
**编译器各阶段的详细说明**

**内容**：
- Lexer（词法分析）
- Parser（语法分析）
- Sema（语义分析）
- CodeGen（代码生成）
- VM（虚拟机执行）

**适合人群**：想要了解编译器原理的学习者

---

## 文档关系图

```
docs/
├── README.md (本文件) - 文档导航
├── development-plan.md - 项目总体规划
├── codegen-optimization.md - 代码生成器优化记录
├── phase6-global-variables-design.md - Phase 6 设计方案
├── dev-notes.md - 开发笔记
└── compilation-flow.md - 编译流程说明
```

---

## 快速导航

### 按角色查找文档

**项目新人**：
1. 先读 [development-plan.md](development-plan.md) - 了解项目规划
2. 再读 [compilation-flow.md](compilation-flow.md) - 理解编译流程
3. 查看 [examples/](../examples/README.md) - 看测试用例

**开发人员**：
1. 参考 [development-plan.md](development-plan.md) - 查看当前阶段
2. 阅读 [codegen-optimization.md](codegen-optimization.md) - 了解代码结构
3. 查看 [dev-notes.md](dev-notes.md) - 了解当前工作

**架构师**：
1. [development-plan.md](development-plan.md) - 技术路线
2. [phase6-global-variables-design.md](phase6-global-variables-design.md) - 设计方案
3. [codegen-optimization.md](codegen-optimization.md) - 优化策略

### 按主题查找文档

**结构体实现**：
- [development-plan.md](development-plan.md) - 阶段一到阶段七

**代码生成优化**：
- [codegen-optimization.md](codegen-optimization.md) - 完整优化记录

**全局变量**：
- [phase6-global-variables-design.md](phase6-global-variables-design.md) - 设计方案
- [codegen-optimization.md](codegen-optimization.md) - 基础设施准备

**编译器原理**：
- [compilation-flow.md](compilation-flow.md) - 各阶段说明

---

## 当前项目状态 (2026-01-21)

### ✅ 已完成
- 基础结构体（阶段一）
- 结构体指针、嵌套、数组（阶段二）
- 结构体函数参数和返回值（阶段三）
- 结构体整体赋值（阶段四）
- CodeGen 优化（类型辅助函数、统一变量管理）

### 🔄 进行中
- 全局变量支持（阶段六）

### ⏳ 待实现
- 初始化列表（阶段五）
- 自引用结构体（阶段七，可选）

---

## 文档维护规范

### 新增文档
1. 使用清晰的文件名（小写英文，连字符分隔）
2. 在本 README 中添加索引
3. 更新相关文档的交叉引用

### 更新文档
1. 在文档顶部标注最后更新日期
2. 重大变更添加变更日志
3. 同步更新 development-plan.md 中的进度

### 废弃文档
1. 将内容合并到其他文档
2. 删除原文件
3. 更新所有引用

---

**最后更新**: 2026-01-21
**维护者**: KOP
