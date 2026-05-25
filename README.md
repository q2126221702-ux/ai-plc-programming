# AI PLC 编程工具

基于 AI 大语言模型的西门子 PLC 自动编程工具，支持从自然语言需求描述自动生成完整的 TIA Portal PLC 项目。

## 功能特性

- **自然语言输入**：用中文描述控制需求，AI 自动理解并生成 PLC 程序
- **完整工程生成**：生成可直接导入 TIA Portal 的 XML 工程文件
- **多模式支持**：
  - 星三角降压启动
  - 自动往返控制
  - 顺序控制
  - 手动/自动混合控制
  - 定时/计数控制
  - 互锁/安全控制
- **状态机生成**：复杂顺序控制自动生成状态机程序
- **安全规则检查**：自动添加急停、互锁、超时保护等工业安全规则
- **HMI 变量生成**：自动生成触摸屏变量和报警逻辑
- **仿真验证**：内置仿真引擎验证程序逻辑

## 技术架构

```
用户需求（自然语言）
    ↓
任务规划（Task Planning）
    ↓
语义解析（Semantic Parsing）
    ↓
控制推理（Control Reasoning）
    ↓
程序合成（Program Synthesis）
    ↓
工业规则引擎（Industrial Rule Engine）
    ↓
自修复（Self-Repair）
    ↓
验证（Validation）
    ↓
CFG 分析 + IR 转换 + 优化
    ↓
仿真（Simulation）
    ↓
多后端生成（SCL / STL / XML）
    ↓
TIA Portal 导入
```

## 环境要求

- Windows 10/11
- .NET Framework 4.8
- Visual Studio 2022（编译用）
- Siemens TIA Portal V20（导入生成的 PLC 项目）

## 编译

```powershell
# 使用 MSBuild 编译
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
    "博图ai自动.vcxproj" /p:Configuration=Debug /p:Platform=x64 /t:Build
```

或在 Visual Studio 中打开 `博图ai自动.slnx` 直接编译。

## 使用

1. 运行 `博图ai自动.exe`
2. 配置 API 密钥（菜单 → API → 配置API密钥）
3. 在输入框中用中文描述控制需求
4. 选择生成模式（完整工程 / 仅程序段）
5. 点击生成，等待 AI 完成
6. 生成的 XML 文件可导入 TIA Portal

## 支持的 AI 模型

- DeepSeek API（推荐）
- 兼容 OpenAI 格式的 API
- 兼容 Anthropic 格式的 API

## 支持的 LAD 指令

| 类型 | 指令 |
|------|------|
| 位逻辑 | 常开/常闭触点、线圈、置位/复位线圈、上升沿/下降沿触点 |
| 定时器 | TON（接通延时）、TOF（断开延时）、TP（脉冲） |
| 计数器 | CTU（加计数）、CTD（减计数）、CTUD（加减计数） |
| 比较器 | 等于、不等于、大于、小于、大于等于、小于等于 |
| 数学运算 | 加、减、乘、除、取模 |
| 数据处理 | 移动 |
| 程序控制 | 跳转、标号、返回 |

## 经典题目示例

项目内置经典 PLC 编程题目库，涵盖：

- 基础逻辑控制（点动、自锁、互锁、多地控制）
- 电机正反转控制（互锁、限位、自动往返）
- 星三角降压启动
- 顺序控制（顺序启动、逆序停止）
- 定时/计数控制
- 运料小车控制
- 液体混合控制
- 交通灯控制

## 导入 TIA Portal

1. 打开 TIA Portal，创建或打开项目
2. 右键 PLC 变量表 → 导入 → 选择生成的变量表 XML
3. 右键程序块 → 外部源 → 选择生成的源 XML
4. 编译并下载到 PLC

## 项目结构

```
├── AiPipeline.h          # AI 生成流水线（核心）
├── AgentSystem.h         # Agent 系统
├── ApiConfigDialog.h     # API 配置界面
├── DataTypes.h           # 数据类型定义
├── Dsl.h                 # DSL 数据结构
├── IdeController.h       # IDE 控制器
├── JsonSerializer.h      # JSON 序列化
├── LadConversion.h       # LAD 转换
├── LadXmlParser.h        # LAD XML 解析
├── MainForm.h            # 主界面
├── ProjectContext.h      # 项目上下文
├── RoundTrip.h           # 往返控制
├── TiaBridge.h/cpp       # TIA Portal 桥接
├── TiaExport.h           # TIA 导出
├── TiaImport.h           # TIA 导入
├── TiaUtils.h            # TIA 工具
└── main.cpp              # 入口
```

## 安全说明

- API 密钥存储在 `phase3_config.json` 中，该文件已在 `.gitignore` 中排除
- 请勿将包含 API 密钥的配置文件提交到版本控制系统

## License

MIT
