# AI Stress Spawner 说明

## 1. 扩展背景

为了支撑性能与压力测试，我实现了一个 **AI Stress Spawner（AI 批量生成器）**，用于在游戏开始时自动生成指定数量的 AI，从而快速构造不同规模的战斗压力场景。

## 2. 蓝图实现

![AI Stress Spawner](../images/ai_spawner_blueprint.png)

通过该工具可以快速构建高密度 AI 场景，从而进行性能压力测试。

## 3. 可配置参数

- **SpawnCount**：生成 AI 数量
- **SpawnRadius**：AI 生成范围半径

## 4. 核心逻辑

`BeginPlay → ForLoop → 随机生成偏移坐标 → SpawnActor (NPC)`

## 5. 扩展目标

该扩展主要用于支撑以下测试目标：

- 不同 AI 数量下的性能表现观察
- 高密度战斗场景下的稳定性验证
- AI 批量生成阶段与稳定运行阶段的性能对比
- FPS、Frame Time、Game Thread 与 GPU Time 的趋势分析

## 6. 扩展意义

相较于依赖关卡中手动摆放的固定敌人，AI Stress Spawner 提升了压力测试场景的可配置性、可重复性与测试效率，使性能测试具备更明确的实验条件。

该扩展使项目从基础玩法验证进一步延伸到结构化性能测试场景构建。
