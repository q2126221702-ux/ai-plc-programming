# TIA导出 vs 我们生成 — LAD FlgNet 结构对比

> 数据来源：TIA Portal V20 导出的 `Main_template.xml` / `Main_FlgNet_diag.xml`，以及通过 AI Code Generation 从 `Main.lad.json` 生成的 `Main_generated.xml`

## 整体结构差异

| 项目 | TIA导出 | 我们生成 | 说明 |
|------|---------|---------|------|
| CompileUnit数量 | 1个（所有网络合并在一个FlgNet中） | 每个网络独立一个CompileUnit | TIA将所有网络放在单个CompileUnit中，我们每个网络独立生成。这是架构差异，不影响功能正确性 |
| FlgNet结构 | 单个FlgNet包含所有Parts和Wires | 每个CompileUnit各自包含独立FlgNet | 同上 |
| Powerrail Wire | 一条Wire(148)连接所有网络的电源流入口 | 每个网络各自一条Powerrail Wire | 同上 |
| UId编号 | 全局递增（21~147） | 每个CU从21开始重新编号 | 同上 |

---

## 指令级对比结果

所有指令类型的 Part 属性、Wire 连接方式、Access 结构均已与 TIA 导出一致：

| # | 指令类型 | TIA Part Name | 电源流入口pin | 数据pin | 输出pin | 状态 |
|---|---------|--------------|-------------|---------|---------|------|
| 1 | 常开触点 | Contact | in | operand | out | ✅ |
| 2 | 线圈 | Coil | in | operand | — | ✅ |
| 3 | 置位线圈 | SCoil | in | operand | — | ✅ |
| 4 | 复位线圈 | RCoil | in | operand | — | ✅ |
| 5 | 上升沿触点 | PContact | pre | operand, bit | out | ✅ |
| 6 | 下降沿触点 | NContact | pre | operand, bit | out | ✅ |
| 7 | 置位位域 | SBitfield | en | n, operand | — | ✅ |
| 8 | 复位位域 | RBitfield | en | n, operand | — | ✅ |
| 9 | 接通延时定时器 | TON | IN | PT | Q, ET | ✅ |
| 10 | 断开延时定时器 | TOF | IN | PT | Q, ET | ✅ |
| 11 | 脉冲定时器 | TP | IN | PT | Q, ET | ✅ |
| 12 | 加计数器 | CTU | CU | R(OpenCon), PV | Q, CV | ✅ |
| 13 | 减计数器 | CTD | CD | LD(OpenCon), PV | Q, CV | ✅ |
| 14 | 加减计数器 | CTUD | CU | CD,R,LD(OpenCon), PV | QU, QD, CV | ✅ |
| 15 | 等于 | Eq | pre | in1, in2 | out | ✅ |
| 16 | 不等于 | Ne | pre | in1, in2 | out | ✅ |
| 17 | 大于等于 | Ge | pre | in1, in2 | out | ✅ |
| 18 | 小于等于 | Le | pre | in1, in2 | out | ✅ |
| 19 | 大于 | Gt | pre | in1, in2 | out | ✅ |
| 20 | 小于 | Lt | pre | in1, in2 | out | ✅ |
| 21 | 加法 | Add | en | in1, in2 | eno, out | ✅ |
| 22 | 减法 | Sub | en | in1, in2 | eno, out | ✅ |
| 23 | 乘法 | Mul | en | in1, in2 | eno, out | ✅ |
| 24 | 除法 | Div | en | in1, in2 | eno, out | ✅ |
| 25 | 取模 | Mod | en | in1, in2 | eno, out | ✅ |
| 26 | 移动 | Move | en | in | eno, out1 | ✅ |
| 27 | 跳转 | JMP | in | operand | — | ✅ |
| 28 | 标号 | LABEL | in | operand | — | ✅ |
| 29 | 返回 | RET | in | — | — | ✅ |
| 30 | 空操作 | NOP | in | — | out | ✅ |
| 31 | 或运算 | O | in1, in2 | — | out | ✅ |
| 32 | 分支汇合 | Junction | in | — | out | ✅ |

### 关键属性对照

| 指令类型 | 特殊属性 | TIA值 | 我们值 | 状态 |
|---------|---------|-------|-------|------|
| TON/TOF/TP | Version | 1.0 | 1.0 | ✅ |
| TON/TOF/TP | Instance | GlobalVariable + Component | GlobalVariable + Component | ✅ |
| TON/TOF/TP | TemplateValue time_type | Time | Time | ✅ |
| CTU/CTD/CTUD | TemplateValue value_type | Int | Int | ✅ |
| CTU/CTD | 未连接pin (R/LD) | OpenCon | OpenCon | ✅ |
| CTUD | 未连接pin (CD/R/LD) | OpenCon | OpenCon | ✅ |
| TON/TOF/TP | ET输出 | →OpenCon Wire | →OpenCon Wire | ✅ |
| CTU/CTD | CV输出 | →OpenCon Wire | →OpenCon Wire | ✅ |
| CTUD | QD/CV输出 | →OpenCon Wire | →OpenCon Wire | ✅ |
| Eq/Ne/... | TemplateValue SrcType | Variant/Int | Variant/Int | ✅ |
| Add/Sub/... | DisabledENO | true | true | ✅ |
| Add/Sub/... | AutomaticTyped SrcType | 有 | 有 | ✅ |
| Add/Mul | TemplateValue Card | 2 | 2 | ✅ |
| Move | DisabledENO | true | true | ✅ |
| Move | TemplateValue Card | 1 | 1 | ✅ |
| Move | 输出pin名 | out1 | out1 | ✅ |

---

## 已修复问题清单

| # | 问题 | 修复内容 |
|---|------|---------|
| 1 | 比较指令Part Name | 从CmpEQ改为Eq，与TIA一致 |
| 2 | 比较指令TemplateValue | 从data_type改为SrcType |
| 3 | 数学指令Part Name | 从ADD改为Add等，与TIA一致 |
| 4 | 数学指令TemplateValue | 添加AutomaticTyped SrcType |
| 5 | 数学指令DisabledENO | 添加DisabledENO="true" |
| 6 | Move输出pin名 | 从out改为out1 |
| 7 | Move DisabledENO | 添加 |
| 8 | Move TemplateValue | 从data_type改为Card |
| 9 | 未连接pin处理 | 从LiteralConstant改为OpenCon（R/LD/CD pin） |
| 10 | 未使用的输出Wire | Q/QU/ENO不再生成OpenCon Wire |
| 11 | SBitfield/RBitfield解析 | 电源流入口识别添加"en" pin，nextMap构建添加"eno"→"en"连接 |
| 12 | 比较/数学/Move解析 | 同上，电源流入口识别添加"en" pin |
| 13 | CTUD CD/LD pin | 从完全缺失改为OpenCon连接 |
| 14 | ET/CV/QD→OpenCon Wire | 从skipOpenCon列表中移除ET/CV/QD |
| 15 | 比较指令PartSchema缺少pre pin | InputPins从["in1","in2"]改为["pre","in1","in2"]，电源流Wire正确连接到pre pin |
| 16 | 分支中比较指令只识别旧名 | 添加Eq/Ne/Gt/Lt/Ge/Le新名称识别 |
| 17 | 分支中数学指令只识别旧名 | 添加Add/Sub/Mul/Div/Mod新名称识别 |
