# 自动化与报告输出说明

## 脚本入口

项目提供统一脚本：

- [run_rpg_automation.bat](../../run_rpg_automation.bat)

默认推荐：

```bat
run_rpg_automation.bat autoplay
```

## 当前流程

当前最稳妥的流程是：

1. 运行脚本启动测试
2. 进入场景
3. 手动按一次 `BackSpace`
4. 让系统自动记录和输出报告

## 为什么仍保留手动 `BackSpace`

项目里虽然存在 `IA_Autoplay` 资源，但 AutoPlay 入口更多掌握在蓝图与输入资源侧，缺少稳定的 C++ 公开接口。

因此当前方案选择：

- 保留手动触发
- 自动化重点放在会话调度、性能采样和结果输出

## 输出内容

输出目录：

- [artifacts/perf-reports](../../artifacts/perf-reports)

主要产物包括：

- `summary.md`
- `autotest.md`
- 趋势图表

这套设计的核心优势是：

- 测试过程可重复
- 结果可沉淀
- 展示时可以直接给出真实报告样本
