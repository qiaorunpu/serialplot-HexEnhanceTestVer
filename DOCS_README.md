# 数据流分析文档 / Data Flow Analysis Documentation

## 文档列表 / Document List

本仓库包含以下数据流分析文档：

This repository contains the following data flow analysis documents:

### 1. DATA_FLOW_VISUALIZATION.md (可视化图表 / Visual Diagrams) ⭐ 推荐开始 / Recommended Start
数据流可视化图表、架构图、时序图、故障排查指南

Visual diagrams, architecture charts, sequence diagrams, troubleshooting guide

### 2. DATA_FLOW_ANALYSIS.md (中文详细版 / Chinese Detailed)
SerialPlot 数据流详细分析文档（中文）

Comprehensive data flow analysis for SerialPlot (Chinese)

### 3. DATA_FLOW_ANALYSIS_EN.md (English Detailed)
SerialPlot Data Flow Analysis (English)

Comprehensive data flow analysis for SerialPlot (English)

## 文档内容 / Document Content

### 可视化文档 (Visualization Document)
**DATA_FLOW_VISUALIZATION.md** 包含：

Contains:
- 📊 **8层架构图** / 8-Layer architecture diagram
- 🔗 **类关系图** / Class relationship diagram
- ⏱️ **时序图** / Sequence diagram
- 🎯 **接口定义** / Interface definitions
- 🚀 **扩展指南** / Extension guide
- 🐛 **故障排查** / Troubleshooting

### 详细分析文档 (Detailed Analysis Documents)
**DATA_FLOW_ANALYSIS.md** 和 **DATA_FLOW_ANALYSIS_EN.md** 包含相同内容：

Both contain the same content:
- 📊 **完整的数据流架构图** / Complete data flow architecture diagram
- 🔍 **11个核心组件的详细分析** / Detailed analysis of 11 core components
- 🔗 **完整的函数调用链** / Complete function call chain
- 📝 **代码位置和行号** / Code locations and line numbers
- 💻 **关键代码片段** / Key code snippets
- 🎨 **设计模式分析** / Design pattern analysis
- ⚡ **性能优化点** / Performance optimization points

## 验证方法 / Verification Method

文档内容经过严格验证：

The documentation has been thoroughly verified:

- ✅ 严格对照实际代码实现 / Strictly verified against actual code
- ✅ 标注了每个类和函数的源文件位置 / Annotated source file locations
- ✅ 引用了具体的代码行号 / Referenced specific line numbers
- ✅ 没有基于文件名猜测 / No assumptions based on file names
- ✅ 追踪了实际的函数调用 / Traced actual function calls

## 主要数据流路径 / Main Data Flow Path

```
QSerialPort → AbstractReader → Source::feedOut() → Sink::feedIn() 
→ Stream → RingBuffer → FrameBufferSeries → PlotManager → Plot
```

## 使用建议 / Usage Recommendations

### 🚀 快速开始 (Quick Start)
**推荐从这里开始** / **Start here**: `DATA_FLOW_VISUALIZATION.md`
- 快速了解整体架构 / Quick overview of architecture
- 清晰的可视化图表 / Clear visual diagrams

### 📚 根据需求选择 (Choose by Need)

如果您想 / If you want to:

- 📖 **快速理解整体流程** / Quick understanding of overall flow
  - 👉 查看 `DATA_FLOW_VISUALIZATION.md` 的简化数据流图
  - 👉 View the simplified flow diagram in `DATA_FLOW_VISUALIZATION.md`

- 🔍 **查找具体代码位置** / Find specific code locations
  - 👉 查看 `DATA_FLOW_ANALYSIS.md` (中文) 或 `DATA_FLOW_ANALYSIS_EN.md` (English)
  - 👉 Contains exact file paths and line numbers

- 🔧 **添加新的数据格式** / Add new data formats
  - 👉 查看 `DATA_FLOW_VISUALIZATION.md` 的扩展指南
  - 👉 Check extension guide in `DATA_FLOW_VISUALIZATION.md`
  - 👉 参考 `DATA_FLOW_ANALYSIS.md` 的"AbstractReader系列"
  - 👉 Refer to "AbstractReader Series" in detailed docs

- 🚀 **优化性能** / Optimize performance
  - 👉 查看 `DATA_FLOW_VISUALIZATION.md` 的性能关键点
  - 👉 Check performance critical points
  - 👉 查看详细文档的"性能优化点"部分
  - 👉 See "Performance Optimization Points" in detailed docs

- 🐛 **排查问题** / Troubleshoot issues
  - 👉 查看 `DATA_FLOW_VISUALIZATION.md` 的故障排查部分
  - 👉 Check troubleshooting section in visualization doc

- 🏗️ **深入理解架构** / Deep dive into architecture
  - 👉 阅读 `DATA_FLOW_ANALYSIS.md` 或 `DATA_FLOW_ANALYSIS_EN.md`
  - 👉 Read the detailed analysis documents
  - 👉 包含设计模式、完整调用链等
  - 👉 Contains design patterns, complete call chains, etc.

## 技术栈 / Technology Stack

- **Qt 6** - Application framework
- **Qwt 6.3** - Plotting library
- **C++** - Programming language

## 贡献 / Contributing

如果您发现文档有任何错误或需要改进的地方，请提交 Issue 或 Pull Request。

If you find any errors or areas for improvement in the documentation, please submit an Issue or Pull Request.

## 许可证 / License

本文档遵循与 SerialPlot 项目相同的许可证 (GPLv3)。

This documentation follows the same license as the SerialPlot project (GPLv3).
