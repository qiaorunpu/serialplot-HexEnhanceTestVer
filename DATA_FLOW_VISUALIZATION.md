# SerialPlot 数据流可视化 / Data Flow Visualization

## 简化数据流图 / Simplified Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         SerialPlot 应用程序                        │
│                        SerialPlot Application                     │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────┐
│   1. 数据入口层       │
│   Data Entry Layer   │
├─────────────────────┤
│  QSerialPort        │  ← 硬件串口 / Hardware Serial Port
│  (serialPort)       │
└──────────┬──────────┘
           │ readyRead() signal
           ↓
┌─────────────────────┐
│   2. 读取器层        │
│   Reader Layer      │
├─────────────────────┤
│ AbstractReader      │  ← 基类 / Base Class
│   ├─ BinaryStream   │  ← 二进制流 / Binary Stream
│   ├─ AsciiReader    │  ← ASCII/CSV
│   ├─ FramedReader   │  ← 帧格式 / Frame Format
│   └─ DemoReader     │  ← 演示 / Demo
└──────────┬──────────┘
           │ readData() → feedOut()
           ↓
┌─────────────────────┐
│   3. 分发层          │
│   Distribution      │
├─────────────────────┤
│ Source → Sink 机制   │
│ (Source-Sink)       │
└──────┬──────┬───────┘
       │      │
       │      └──────────────────┐
       │                         ↓
       │              ┌──────────────────┐
       │              │ SampleCounter    │ ← 采样率统计
       │              │ (sps display)    │   Sample Rate
       │              └──────────────────┘
       ↓
┌─────────────────────┐
│   4. 数据流层        │
│   Stream Layer      │
├─────────────────────┤
│ Stream              │  ← 主数据流 / Main Data Stream
│  ├─ gain/offset     │  ← 增益/偏移处理 / Gain/Offset
│  └─ StreamChannel[] │  ← 通道数组 / Channel Array
└──────────┬──────────┘
           │ feedIn() → addSamples()
           ↓
┌─────────────────────┐
│   5. 缓冲区层        │
│   Buffer Layer      │
├─────────────────────┤
│ RingBuffer[]        │  ← 环形缓冲区 / Circular Buffer
│ (per channel)       │     (每个通道 / per channel)
└──────────┬──────────┘
           │ sample(i)
           ↓
┌─────────────────────┐
│   6. 适配层          │
│   Adapter Layer     │
├─────────────────────┤
│ FrameBufferSeries   │  ← Qwt 接口适配 / Qwt Interface
└──────────┬──────────┘
           │ QPointF sample(i)
           ↓
┌─────────────────────┐
│   7. 绘图管理层      │
│   Plot Manager      │
├─────────────────────┤
│ PlotManager         │  ← 管理所有图表 / Manage All Plots
│   └─ Plot[]         │  ← QwtPlot 实例 / QwtPlot Instances
│       └─ Curve[]    │  ← QwtPlotCurve
└──────────┬──────────┘
           │ replot()
           ↓
┌─────────────────────┐
│   8. 显示层          │
│   Display Layer     │
├─────────────────────┤
│ QwtPlot (Qwt库)     │  ← 实际绘图 / Actual Drawing
│ 图表显示 / Chart    │
└─────────────────────┘
```

## 核心类关系图 / Core Class Relationships

```
┌──────────────────────────────────────────────────────────┐
│                    MainWindow                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │PortControl   │  │DataFormatPanel│  │PlotManager   │  │
│  └──────┬───────┘  └───────┬──────┘  └──────┬───────┘  │
└─────────┼──────────────────┼─────────────────┼──────────┘
          │                  │                 │
          │                  │                 │
    ┌─────▼──────┐    ┌─────▼──────┐    ┌────▼─────┐
    │QSerialPort │    │Reader      │    │Stream    │
    │            │    │(Source)    │    │(Sink)    │
    └────────────┘    └─────┬──────┘    └────┬─────┘
                            │                 │
                       connectSink()           │
                            └────────►────────┘
                                feedOut() / feedIn()

继承关系 / Inheritance:

AbstractReader ──┬──► BinaryStreamReader
(QObject,Source) ├──► AsciiReader
                 ├──► FramedReader
                 └──► DemoReader

Stream ───────────► (implements Sink interface)
```

## 关键接口 / Key Interfaces

### Source 接口 / Source Interface
```cpp
class Source {
    // 将数据传递给所有连接的 Sink
    // Pass data to all connected Sinks
    void feedOut(const SamplePack& data);
    
    // 连接一个 Sink
    // Connect a Sink
    void connectSink(Sink* sink);
};
```

### Sink 接口 / Sink Interface
```cpp
class Sink {
    // 接收来自 Source 的数据
    // Receive data from Source
    virtual void feedIn(const SamplePack& data);
    
    // 设置通道数
    // Set number of channels
    virtual void setNumChannels(unsigned nc, bool x);
};
```

### SamplePack 数据包 / SamplePack Data Package
```cpp
class SamplePack {
    unsigned _numSamples;    // 样本数 / Number of samples
    unsigned _numChannels;   // 通道数 / Number of channels
    double* _yData;          // Y轴数据 / Y-axis data
    
    // 访问通道数据 / Access channel data
    double* data(unsigned channel);
};
```

## 数据流时序图 / Data Flow Sequence

```
时间 │ QSerialPort │ Reader │ Source │ Sink(Stream) │ RingBuffer │ PlotManager
Time │             │        │        │              │            │
─────┼─────────────┼────────┼────────┼──────────────┼────────────┼─────────────
  1  │ readyRead() │        │        │              │            │
     │   signal    │        │        │              │            │
  2  │      ──────►│onData  │        │              │            │
     │             │Ready() │        │              │            │
  3  │             │readData│        │              │            │
     │             │   ()   │        │              │            │
  4  │             │───────►│feedOut │              │            │
     │             │        │  ()    │              │            │
  5  │             │        │───────►│feedIn()      │            │
     │             │        │        │              │            │
  6  │             │        │        │─────────────►│addSamples  │
     │             │        │        │              │    ()      │
  7  │             │        │        │ emit         │            │
     │             │        │        │ dataAdded()  │            │
  8  │             │        │        │─────────────────────────►│replot()
  9  │             │        │        │              │◄───────────│sample(i)
 10  │             │        │        │              │            │draw
```

## 性能关键点 / Performance Critical Points

```
┌────────────────────────────────────────────────────────┐
│ 性能优化点 / Performance Optimizations                   │
├────────────────────────────────────────────────────────┤
│                                                        │
│ 1. 批量读取 / Batch Reading                            │
│    FramedReader: readAll() 而非 read(1)                │
│    ⚡ 系统调用从 ~92,000/s 降至 ~100/s                  │
│                                                        │
│ 2. KMP 算法 / KMP Algorithm                            │
│    同步字搜索: O(n+m) 而非 O(n*m)                       │
│    ⚡ 更快的帧同步                                      │
│                                                        │
│ 3. 环形缓冲区 / Ring Buffer                            │
│    固定大小，避免动态分配                                │
│    ⚡ 减少内存碎片和分配开销                             │
│                                                        │
│ 4. 感兴趣区域 / Region of Interest                     │
│    FrameBufferSeries: 只绘制可见区域                   │
│    ⚡ 减少不必要的绘图操作                               │
│                                                        │
│ 5. 信号槽异步 / Async Signal-Slot                      │
│    UI 更新不阻塞数据读取                                │
│    ⚡ 更流畅的用户体验                                  │
│                                                        │
└────────────────────────────────────────────────────────┘
```

## 扩展指南 / Extension Guide

### 添加新的数据格式 / Adding New Data Format

```
1. 创建新的 Reader 类 / Create New Reader Class
   ↓
   class MyReader : public AbstractReader {
       unsigned readData() override { ... }
   };

2. 在 DataFormatPanel 中注册 / Register in DataFormatPanel
   ↓
   MyReader myReader(port, this);
   connect(ui->rbMyFormat, &QRadioButton::toggled,
           [this](bool checked) {
               if (checked) selectReader(&myReader);
           });

3. 完成！数据会自动流向 Stream 和 PlotManager
   Done! Data will automatically flow to Stream and PlotManager
```

### 添加数据处理步骤 / Adding Data Processing Step

```
方法 1: 在 Reader 中处理 / Method 1: Process in Reader
   readData() → 处理数据 / Process → feedOut()

方法 2: 创建中间 Sink / Method 2: Create Intermediate Sink
   Reader → MyProcessingSink → Stream
            (实现 Sink 和 Source)
```

## 故障排查 / Troubleshooting

```
问题 / Issue: 没有数据显示 / No Data Display
├─ 检查 / Check: serialPort.isOpen()
├─ 检查 / Check: currentReader->enable(true) 已调用 / called
├─ 检查 / Check: source->connectSink(&stream) 已连接 / connected
└─ 检查 / Check: stream.pause(false) 未暂停 / not paused

问题 / Issue: 数据显示错误 / Incorrect Data Display
├─ 检查 / Check: Reader 的 numChannels() 设置
├─ 检查 / Check: 数据格式配置（字节序、数据类型）/ format config
└─ 检查 / Check: Stream 的 gain/offset 设置

问题 / Issue: 性能问题 / Performance Issues
├─ 检查 / Check: FramedReader 使用 KMP 而非逐字节 / using KMP
├─ 检查 / Check: 批量读取而非单字节读取 / batch reading
└─ 检查 / Check: Plot 的可见区域优化 / visible region optimization
```

## 参考代码位置 / Reference Code Locations

| 组件 / Component | 头文件 / Header | 实现文件 / Implementation |
|-----------------|----------------|-------------------------|
| AbstractReader | src/abstractreader.h | src/abstractreader.cpp |
| BinaryStreamReader | src/binarystreamreader.h | src/binarystreamreader.cpp |
| FramedReader | src/framedreader.h | src/framedreader.cpp |
| Source | src/source.h | src/source.cpp |
| Sink | src/sink.h | src/sink.cpp |
| Stream | src/stream.h | src/stream.cpp |
| StreamChannel | src/streamchannel.h | src/streamchannel.cpp |
| RingBuffer | src/ringbuffer.h | src/ringbuffer.cpp |
| PlotManager | src/plotmanager.h | src/plotmanager.cpp |
| MainWindow | src/mainwindow.h | src/mainwindow.cpp |

---

详细文档 / Detailed Documentation:
- 中文版 / Chinese: `DATA_FLOW_ANALYSIS.md`
- English: `DATA_FLOW_ANALYSIS_EN.md`
