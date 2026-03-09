# UE5 ActionRPG Demo 游戏系统测试项目

## 项目简介

本项目基于开源 **UE5 ActionRPG Demo**，围绕核心玩法系统开展系统化测试与分析，覆盖角色控制、战斗、AI、掉落拾取、背包商店及性能表现等模块。

在原始 Demo 基础上，还进行了部分测试场景扩展，包括：

- [Jump功能扩展](docs/JUMP_EXTENSION.md)
- [AI Stress Test构建](docs/AI_STRESS_SPAWNER.md)
- [角色资源替换](docs/RESOURCE_REPLACEMENT.md)

相关扩展主要用于支撑兼容性验证、压力测试与系统依赖关系分析

本仓库主要展示 **游戏测试流程、测试用例设计、缺陷记录与性能测试结果**。

---

## 测试目标

本项目重点验证以下内容：

- 核心玩法系统的功能稳定性
- 多系统交互下的行为正确性
- 战斗与 AI 在复杂场景下的表现
- 高密度 AI 场景下的性能变化
- UE 动画系统与角色资源之间的依赖关系

---

## 测试范围

测试主要覆盖以下模块：

- **角色控制系统**
- **战斗系统**
- **AI 系统**
- **掉落与拾取系统**
- **背包与物品管理系统**
- **商店交易系统**
- **数据存储与恢复机制**
- **性能与稳定性表现**

---

## 测试方法

测试用例设计主要采用以下方法：

- 等价类划分
- 边界值分析
- 场景法
- 灰盒测试

灰盒测试主要通过以下方式构造异常或边界场景：

- 调整 AI 刷新数量
- 修改蓝图参数
- 构造特殊测试场景

---

## 核心功能测试场景

| 场景 | 覆盖系统 | 描述 |
|------|----------|------|
| 战斗 → 掉落 → 拾取 | 战斗 / 掉落 / 背包 | 验证敌人击杀后的掉落生成与背包更新 |
| 商店购买流程 | 商店 / 背包 / 数据存储 | 验证金币扣减、物品入包与数据保存 |
| Jump 功能兼容性测试 | 角色控制 / 战斗 | 验证 Jump 启用后与攻击、落地逻辑的兼容性 |
| AI 寻路测试 | AI / 导航系统 | 验证玩家处于高地时 AI 的追击行为 |
| 角色资源替换实验 | 动画系统 / Skeleton | 分析动画蓝图与角色资源的依赖关系 |

---

## 性能测试

通过构建 **AI Stress Test 场景** 进行压力测试。

AI 数量测试梯度：

`0 → 10 → 50 → 100 → 500 → 1000`

使用 UE 实时性能监控工具：

`stat fps` / `stat unit` / `stat unitgraph`

主要关注指标：

- FPS 变化
- CPU / GPU 帧时间
- Game Thread 开销
- AI Tick 开销
- Actor 数量变化

详细结果见：[性能与压力测试报告](docs/PERFORMANCE_TEST.md)

---

## 主要测试产出

本项目包含以下测试文档：

- [测试用例集](docs/TEST_CASES.md)
- [缺陷与风险记录](docs/BUG_REPORT.md)
- [性能与压力测试报告](docs/PERFORMANCE_TEST.md)
- [Jump功能扩展说明](docs/JUMP_EXTENSION.md)
- [AI Stress Test构建说明](docs/AI_STRESS_SPAWNER.md)
- [角色资源替换说明](docs/RESOURCE_REPLACEMENT.md)

---

## 项目结构

```text
docs/
├── TEST_CASES.md
├── BUG_REPORT.md
├── PERFORMANCE_TEST.md
├── JUMP_EXTENSION.md
├── AI_STRESS_SPAWNER.md
└── RESOURCE_REPLACEMENT.md.md

images/
├── ai_spawner_blueprint.png
├── ai_performance_chart.png

```

---

# 参考项目

本仓库基于以下开源项目进行测试分析：

Original Project:  
https://github.com/vahabahmadvand/ActionRPG_UE5

本仓库仅用于 **测试分析与实验记录展示**，不作为原项目源码的再发布版本。
