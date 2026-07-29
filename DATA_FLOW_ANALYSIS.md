# SerialPlot 数据流分析文档

## 概述

本文档详细分析了 SerialPlot 应用程序中数据的流动路径，从串口接收数据开始，经过各种处理阶段，最终在图表上显示。文档严格对照了实际代码实现，而非基于文件名猜测。

## 数据流架构图

```
串口数据 (QSerialPort)
    ↓
AbstractReader 子类 (读取并解析)
    ├── BinaryStreamReader (二进制流)
    ├── AsciiReader (ASCII/CSV格式)
    ├── FramedReader (帧格式)
    └── DemoReader (演示数据)
    ↓
Source::feedOut() 方法
    ↓
Sink::feedIn() 方法
    ├→ Stream (主数据流)
    │    ↓
    │   StreamChannel (每个通道)
    │    ↓
    │   RingBuffer (环形缓冲区)
    │    ↓
    │   FrameBufferSeries (Qwt接口)
    │    ↓
    │   PlotManager & Plot
    │    ↓
    │   图表显示
    │
    └→ SampleCounter (采样率计数器)
```

## 详细数据流路径

### 1. 数据入口：串口设备 (QSerialPort)

**文件位置**: `src/mainwindow.cpp:76`, `src/mainwindow.h:76`

```cpp
QSerialPort serialPort;
```

- 串口设备对象，负责与硬件串口通信
- 由 `PortControl` 类管理（打开/关闭/配置）
- 当数据到达时发出 `readyRead()` 信号

### 2. 数据读取器：AbstractReader 系列

#### 2.1 AbstractReader 基类

**文件位置**: `src/abstractreader.h`, `src/abstractreader.cpp`

**关键函数**:
- `AbstractReader::onDataReady()` - 当 QIODevice 的 `readyRead` 信号触发时调用
- `AbstractReader::readData()` - 纯虚函数，由子类实现具体读取逻辑

**代码流程** (`src/abstractreader.cpp:48-51`):
```cpp
void AbstractReader::onDataReady()
{
    bytesRead += readData();
}
```

**数据继承关系**:
- AbstractReader 继承自 `QObject` 和 `Source`
- 作为 Source，可以连接到多个 Sink

#### 2.2 BinaryStreamReader (二进制流读取器)

**文件位置**: `src/binarystreamreader.h`, `src/binarystreamreader.cpp`

**关键函数**: `BinaryStreamReader::readData()` (`src/binarystreamreader.cpp:121-173`)

**数据处理流程**:
1. 计算包大小：`packageSize = sampleSize * _numChannels`
2. 读取可用字节数并计算完整包数量
3. 为每个包创建 `SamplePack` 对象
4. 循环读取每个通道的样本数据
5. **调用 `feedOut(samples)` 将数据传递给连接的 Sink** (line 170)

**关键代码** (`src/binarystreamreader.cpp:162-170`):
```cpp
// 实际读取
SamplePack samples(numOfPackagesToRead, _numChannels);
for (unsigned i = 0; i < numOfPackagesToRead; i++)
{
    for (unsigned ci = 0; ci < _numChannels; ci++)
    {
        samples.data(ci)[i] = (this->*readSample)();
    }
}
feedOut(samples);  // ← 关键：数据传递给 Sink
```

#### 2.3 FramedReader (帧格式读取器)

**文件位置**: `src/framedreader.h`, `src/framedreader.cpp`

**关键函数**: 
- `FramedReader::readData()` (`src/framedreader.cpp:499-630`)
- `FramedReader::readFrameDataAndExtractChannels()` (`src/framedreader.cpp:384-497`)

**数据处理流程**:
1. 使用 KMP 算法搜索同步字（sync word）
2. 读取完整帧数据（包括同步字和载荷）
3. 如果启用校验和，验证数据完整性
4. 从帧缓冲区提取各通道数据
5. **调用 `feedOut(samples)` 传递数据** (line 496)

**关键代码** (`src/framedreader.cpp:485-496`):
```cpp
// 提取通道
SamplePack samples(_numChannels > 0 ? 1 : 0, _numChannels);

for (unsigned i = 0; i < _numChannels; i++)
{
    const ChannelMapping& ch = _channelMapping.channel(i);
    if (ch.enabled)
    {
        samples.data(i)[0] = extractChannelValue(ch, _frameBuffer);
    }
}

feedOut(samples);  // ← 关键：数据传递给 Sink
```

#### 2.4 AsciiReader (ASCII读取器)

**文件位置**: `src/asciireader.h`, `src/asciireader.cpp`

**关键函数**: `AsciiReader::readData()` 

**数据处理流程**:
1. 按行读取ASCII文本数据
2. 使用分隔符（逗号、制表符等）解析每行
3. 支持十进制和十六进制格式
4. **调用 `feedOut(*samples)` 传递数据** (line 155)

### 3. Source-Sink 架构

#### 3.1 Source 类

**文件位置**: `src/source.h`, `src/source.cpp`

**关键函数**: `Source::feedOut()` (`src/source.cpp:65-71`)

```cpp
void Source::feedOut(const SamplePack& data) const
{
    for (auto sink : sinks)
    {
        sink->feedIn(data);  // ← 将数据传递给所有连接的 Sink
    }
}
```

**功能**:
- 维护一个 Sink 列表
- `feedOut()` 方法将数据分发给所有连接的 Sink
- `connectSink()` 方法连接新的 Sink
- `disconnectSinks()` 方法断开所有 Sink

#### 3.2 Sink 类

**文件位置**: `src/sink.h`, `src/sink.cpp`

**关键函数**: `Sink::feedIn()` (`src/sink.cpp:38-44`)

```cpp
void Sink::feedIn(const SamplePack& data)
{
    for (auto sink : followers)
    {
        sink->feedIn(data);  // ← 传递给追随者
    }
}
```

**功能**:
- 接收来自 Source 的数据
- 支持 "follower" 模式，可以将数据传递给其他 Sink
- Stream 类实现了 Sink 接口

### 4. 数据格式面板：DataFormatPanel

**文件位置**: `src/dataformatpanel.h`, `src/dataformatpanel.cpp`

**关键函数**: 
- `DataFormatPanel::selectReader()` (`src/dataformatpanel.cpp:117-135`)
- `DataFormatPanel::activeSource()` (`src/dataformatpanel.cpp:80-83`)

**功能**:
1. 管理多个 Reader 实例（BinaryStreamReader, AsciiReader, FramedReader, DemoReader）
2. 根据用户选择切换当前活动的 Reader
3. 发出 `sourceChanged` 信号通知主窗口

**关键代码** (`src/dataformatpanel.cpp:117-135`):
```cpp
void DataFormatPanel::selectReader(AbstractReader* reader)
{
    currentReader->enable(false);
    reader->enable();
    
    // 重新连接信号
    disconnect(currentReader, 0, this, 0);
    
    // 切换设置窗口部件
    ui->horizontalLayout->removeWidget(currentReader->settingsWidget());
    currentReader->settingsWidget()->hide();
    ui->horizontalLayout->addWidget(reader->settingsWidget(), 1);
    reader->settingsWidget()->show();
    
    reader->pause(paused);
    
    currentReader = reader;
    emit sourceChanged(currentReader);  // ← 通知主窗口
}
```

### 5. 主窗口连接：MainWindow

**文件位置**: `src/mainwindow.cpp`

**关键函数**: `MainWindow::onSourceChanged()` (`src/mainwindow.cpp:423-427`)

```cpp
void MainWindow::onSourceChanged(Source* source)
{
    source->connectSink(&stream);         // ← 连接到主数据流
    source->connectSink(&sampleCounter);  // ← 连接到采样率计数器
}
```

**初始化连接** (`src/mainwindow.cpp:274-277`):
```cpp
// 初始化流连接
connect(&dataFormatPanel, &DataFormatPanel::sourceChanged,
        this, &MainWindow::onSourceChanged);
onSourceChanged(dataFormatPanel.activeSource());
```

**功能**:
- 当用户切换数据格式时，重新连接 Source 和 Sink
- 将 Reader (Source) 连接到 Stream (Sink)
- 同时连接到 SampleCounter 用于统计采样率

### 6. 数据流：Stream 类

**文件位置**: `src/stream.h`, `src/stream.cpp`

**关键函数**: `Stream::feedIn()` (`src/stream.cpp:213-244`)

**数据处理流程**:
```cpp
void Stream::feedIn(const SamplePack& pack)
{
    Q_ASSERT(pack.numChannels() == numChannels() &&
             pack.hasX() == hasX());
    
    if (_paused) return;  // 暂停时跳过
    
    unsigned ns = pack.numSamples();
    
    // 应用增益和偏移（如果启用）
    const SamplePack* mPack = nullptr;
    if (infoModel()->gainOrOffsetEn())
        mPack = applyGainOffset(pack);
    
    // 将数据添加到每个通道的环形缓冲区
    for (unsigned ci = 0; ci < numChannels(); ci++)
    {
        auto buf = static_cast<RingBuffer*>(channels[ci]->yData());
        double* data = (mPack == nullptr) ? pack.data(ci) : mPack->data(ci);
        buf->addSamples(data, ns);  // ← 数据存入环形缓冲区
    }
    
    Sink::feedIn((mPack == nullptr) ? pack : *mPack);  // 传递给追随者
    
    if (mPack != nullptr) delete mPack;
    emit dataAdded();  // ← 触发重绘信号
}
```

**功能**:
1. 实现 Sink 接口，接收来自 Reader 的数据
2. 应用用户配置的增益（除法）和偏移
3. 将数据分发到各个 StreamChannel
4. 发出 `dataAdded()` 信号触发图表更新

### 7. 通道：StreamChannel 类

**文件位置**: `src/streamchannel.h`, `src/streamchannel.cpp`

**数据成员**:
```cpp
const XFrameBuffer* _x;  // X轴缓冲区（索引或线性）
FrameBuffer* _y;         // Y轴数据缓冲区（RingBuffer）
```

**功能**:
- 每个通道持有自己的 Y 轴数据缓冲区（RingBuffer）
- 共享 X 轴缓冲区（IndexBuffer 或 LinIndexBuffer）
- 提供通道名称、颜色、可见性等属性

### 8. 缓冲区：RingBuffer 类

**文件位置**: `src/ringbuffer.h`, `src/ringbuffer.cpp`

**关键函数**: `RingBuffer::addSamples()`

**功能**:
- 环形缓冲区实现，固定大小
- 当缓冲区满时，新数据覆盖旧数据
- 支持批量添加样本
- 实现 `FrameBuffer` 接口，提供 `sample(i)` 方法访问数据

### 9. 绘图管理器：PlotManager 类

**文件位置**: `src/plotmanager.h`, `src/plotmanager.cpp`

**初始化连接** (`src/plotmanager.cpp:30-58`):
```cpp
PlotManager::PlotManager(QWidget* plotArea, PlotMenu* menu,
                         const Stream* stream, QObject* parent)
{
    _stream = stream;
    construct(plotArea, menu);
    if (_stream == nullptr) return;
    
    // 连接到 ChannelInfoModel
    infoModel = _stream->infoModel();
    connect(infoModel, &QAbstractItemModel::dataChanged,
            this, &PlotManager::onChannelInfoChanged);
    
    connect(stream, &Stream::numChannelsChanged, 
            this, &PlotManager::onNumChannelsChanged);
    connect(stream, &Stream::dataAdded, 
            this, &PlotManager::replot);  // ← 数据更新时重绘
    
    // 添加初始曲线
    for (unsigned int i = 0; i < stream->numChannels(); i++)
    {
        addCurve(stream->channel(i)->name(), 
                stream->channel(i)->xData(), 
                stream->channel(i)->yData());
    }
}
```

**关键函数**: `PlotManager::replot()` 

**功能**:
1. 监听 Stream 的 `dataAdded()` 信号
2. 为每个 StreamChannel 创建 QwtPlotCurve
3. 调用 `replot()` 更新所有图表显示
4. 管理多图表显示模式
5. 处理图例、网格、缩放等 UI 功能

### 10. 数据适配器：FrameBufferSeries 类

**文件位置**: `src/framebufferseries.h`, `src/framebufferseries.cpp`

**功能**:
- 实现 Qwt 的 `QwtSeriesData<QPointF>` 接口
- 作为 FrameBuffer 和 Qwt 绘图库之间的桥梁
- 提供 `sample(i)` 方法返回 QPointF 点数据
- 支持"感兴趣区域"优化，只绘制可见部分

**关键方法**:
```cpp
QPointF FrameBufferSeries::sample(size_t i) const
{
    return QPointF(_x->sample(i), _y->sample(i));
}
```

### 11. 图表显示：Plot 类

**文件位置**: `src/plot.h`, `src/plot.cpp`

**功能**:
- 继承自 QwtPlot
- 显示一个或多个 QwtPlotCurve
- 提供缩放、平移、网格等功能
- 实际渲染图形到屏幕

## 完整数据流调用链

### 典型的数据流调用序列：

1. **串口接收数据**
   ```
   QSerialPort::readyRead() 信号触发
   ```

2. **Reader 读取数据**
   ```
   AbstractReader::onDataReady()
   → BinaryStreamReader::readData()  // 或其他 Reader
   → 创建 SamplePack 对象
   → Source::feedOut(samples)
   ```

3. **分发到 Sink**
   ```
   Source::feedOut()
   → for each sink in sinks:
       → Sink::feedIn(data)
   ```

4. **Stream 处理数据**
   ```
   Stream::feedIn(pack)
   → 应用增益/偏移（如果启用）
   → for each channel:
       → RingBuffer::addSamples(data, n)
   → emit dataAdded()
   ```

5. **触发重绘**
   ```
   Stream::dataAdded() 信号
   → PlotManager::replot() 槽函数
   → for each Plot widget:
       → QwtPlot::replot()
   ```

6. **Qwt 绘图**
   ```
   QwtPlot::replot()
   → for each QwtPlotCurve:
       → FrameBufferSeries::sample(i)
       → RingBuffer::sample(i)
       → 渲染到屏幕
   ```

## 关键设计模式

### 1. Source-Sink 模式 (观察者模式变体)

- **Source**: 数据生产者（Reader 类）
- **Sink**: 数据消费者（Stream、SampleCounter 等）
- **优势**: 解耦数据生成和消费，支持一对多分发

### 2. 策略模式

- **AbstractReader**: 抽象策略
- **BinaryStreamReader, AsciiReader, FramedReader**: 具体策略
- **DataFormatPanel**: 策略上下文，负责选择和切换

### 3. 适配器模式

- **FrameBufferSeries**: 适配器，将 FrameBuffer 适配到 Qwt 的接口

## SamplePack 数据结构

**文件位置**: `src/samplepack.h`, `src/samplepack.cpp`

**数据成员**:
```cpp
unsigned _numSamples;    // 样本数量
unsigned _numChannels;   // 通道数量
double* _xData;          // X轴数据（可选）
double* _yData;          // Y轴数据（所有通道连续存储）
```

**内存布局**:
```
_yData: [CH0_S0, CH0_S1, ..., CH1_S0, CH1_S1, ..., CHn_S0, CHn_S1, ...]
        |------ channel 0 ------|------ channel 1 ------|--- channel n ---|
```

## 数据格式支持

### 1. 二进制流格式 (BinaryStreamReader)
- 支持的数据类型：uint8, int8, uint16, int16, uint24, int24, uint32, int32, float, double
- 字节序：小端或大端
- 数据组织：连续的通道样本包

### 2. ASCII 格式 (AsciiReader)
- 分隔符：逗号、制表符、空格等
- 数字格式：十进制或十六进制
- 行过滤：支持前缀过滤

### 3. 帧格式 (FramedReader)
- 同步字：自定义帧起始标记
- 通道映射：灵活的字节偏移和数据类型配置
- 校验和：支持多种校验算法（CRC、校验和等）
- 性能优化：使用 KMP 算法进行同步字搜索

## 性能优化点

1. **批量读取**: Reader 尽可能一次读取多个样本包
2. **环形缓冲区**: 避免频繁的内存分配和数据移动
3. **KMP 算法**: FramedReader 使用 KMP 算法进行 O(n+m) 同步字搜索
4. **感兴趣区域**: FrameBufferSeries 只处理可见区域的数据点
5. **信号槽优化**: 使用 Qt 的信号槽机制实现异步更新

## 线程模型

- **主线程**: 所有数据处理和 UI 更新都在主线程
- **无额外线程**: QSerialPort 内部使用异步 I/O，但回调在主线程
- **信号驱动**: 使用 Qt 信号槽实现事件驱动的数据流

## 数据流中的历史遗留代码

通过对照实际代码，确认了以下要点：

1. **没有发现未使用的数据路径**: 所有的 Reader 都通过相同的 Source-Sink 机制
2. **Stream 是唯一的数据存储**: 没有其他并行的数据流路径
3. **增益处理已改为除法**: 在 `Stream::applyGainOffset()` 中，增益现在使用除法而非乘法 (line 197)
4. **X 数据暂未实现**: 代码中预留了 X 数据的接口，但当前所有 Reader 的 `hasX()` 都返回 false

## 总结

SerialPlot 的数据流采用了清晰的分层架构：

1. **硬件层**: QSerialPort 负责串口通信
2. **解析层**: AbstractReader 子类负责不同格式的数据解析
3. **分发层**: Source-Sink 机制实现数据分发
4. **存储层**: Stream 和 RingBuffer 负责数据缓存
5. **显示层**: PlotManager 和 Qwt 负责数据可视化

这种设计使得添加新的数据格式变得容易，只需要实现一个新的 AbstractReader 子类即可，无需修改其他部分的代码。
