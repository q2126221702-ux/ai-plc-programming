# Debug: plc-two-networks

## Status: [OPEN]

## Problem
不管选择哪个生成模式（简单/完整），运料小车题目都只能生成2个程序段。应该生成20+个程序段（含模式切换、顺序步骤、定时器、手动控制等）。

## Reproduction
1. 输入运料小车题目
2. 选择任意生成模式
3. 生成后只有2个程序段

## Hypotheses
| ID | Hypothesis | Likelihood | Effort | Expected Signal |
|----|------------|------------|--------|-----------------|
| A | HasAutoRoundTrip=true但IsSequential=false，仍走AutoRoundTrip路径 | High | Low | semanticPlan->IsSequential值为false |
| B | AI语义解析返回States为空或<=2个 | High | Low | States->Count值很小 |
| C | AI需求解析返回的Outputs只有2个 | Medium | Low | requirement->Outputs->Count=2 |
| D | 管线在某步骤抛异常，fallback只生成2个网络 | Medium | Low | 异常日志 |
| E | DetermineOutputActions匹配失败，大部分步骤被跳过 | Low | Medium | outputActions为空的步骤数 |

## Instrumentation Points
- P4LogicGraphGenerator::Generate: 分支决策前的关键字段值
- P6Pipeline::RunPhase6: 每步结果
- P4SemanticPlanParser: 解析后的字段值
