# SerialPlot Data Flow Analysis

## Overview

This document provides a detailed analysis of the data flow in the SerialPlot application, from receiving data from the serial port through various processing stages to final display on the chart. The analysis is based on careful examination of the actual code implementation, not assumptions based on file names.

## Data Flow Architecture Diagram

```
Serial Port Data (QSerialPort)
    ↓
AbstractReader subclasses (Read and Parse)
    ├── BinaryStreamReader (Binary Stream)
    ├── AsciiReader (ASCII/CSV Format)
    ├── FramedReader (Frame Format)
    └── DemoReader (Demo Data)
    ↓
Source::feedOut() method
    ↓
Sink::feedIn() method
    ├→ Stream (Main Data Stream)
    │    ↓
    │   StreamChannel (Per Channel)
    │    ↓
    │   RingBuffer (Circular Buffer)
    │    ↓
    │   FrameBufferSeries (Qwt Interface)
    │    ↓
    │   PlotManager & Plot
    │    ↓
    │   Chart Display
    │
    └→ SampleCounter (Sampling Rate Counter)
```

## Detailed Data Flow Path

### 1. Data Entry: Serial Port Device (QSerialPort)

**File Location**: `src/mainwindow.cpp:76`, `src/mainwindow.h:76`

```cpp
QSerialPort serialPort;
```

- Serial port device object responsible for hardware communication
- Managed by `PortControl` class (open/close/configure)
- Emits `readyRead()` signal when data arrives

### 2. Data Readers: AbstractReader Series

#### 2.1 AbstractReader Base Class

**File Location**: `src/abstractreader.h`, `src/abstractreader.cpp`

**Key Functions**:
- `AbstractReader::onDataReady()` - Called when QIODevice emits `readyRead` signal
- `AbstractReader::readData()` - Pure virtual function, implemented by subclasses

**Code Flow** (`src/abstractreader.cpp:48-51`):
```cpp
void AbstractReader::onDataReady()
{
    bytesRead += readData();
}
```

**Inheritance**:
- AbstractReader inherits from `QObject` and `Source`
- As a Source, it can connect to multiple Sinks

#### 2.2 BinaryStreamReader (Binary Stream Reader)

**File Location**: `src/binarystreamreader.h`, `src/binarystreamreader.cpp`

**Key Function**: `BinaryStreamReader::readData()` (`src/binarystreamreader.cpp:121-173`)

**Data Processing Flow**:
1. Calculate package size: `packageSize = sampleSize * _numChannels`
2. Read available bytes and calculate number of complete packages
3. Create `SamplePack` object for each package
4. Loop to read sample data for each channel
5. **Call `feedOut(samples)` to pass data to connected Sinks** (line 170)

**Key Code** (`src/binarystreamreader.cpp:162-170`):
```cpp
// Actual reading
SamplePack samples(numOfPackagesToRead, _numChannels);
for (unsigned i = 0; i < numOfPackagesToRead; i++)
{
    for (unsigned ci = 0; ci < _numChannels; ci++)
    {
        samples.data(ci)[i] = (this->*readSample)();
    }
}
feedOut(samples);  // ← KEY: Data passed to Sink
```

#### 2.3 FramedReader (Frame Format Reader)

**File Location**: `src/framedreader.h`, `src/framedreader.cpp`

**Key Functions**: 
- `FramedReader::readData()` (`src/framedreader.cpp:499-630`)
- `FramedReader::readFrameDataAndExtractChannels()` (`src/framedreader.cpp:384-497`)

**Data Processing Flow**:
1. Use KMP algorithm to search for sync word
2. Read complete frame data (including sync word and payload)
3. Verify checksum if enabled
4. Extract channel data from frame buffer
5. **Call `feedOut(samples)` to pass data** (line 496)

**Key Code** (`src/framedreader.cpp:485-496`):
```cpp
// Extract channels
SamplePack samples(_numChannels > 0 ? 1 : 0, _numChannels);

for (unsigned i = 0; i < _numChannels; i++)
{
    const ChannelMapping& ch = _channelMapping.channel(i);
    if (ch.enabled)
    {
        samples.data(i)[0] = extractChannelValue(ch, _frameBuffer);
    }
}

feedOut(samples);  // ← KEY: Data passed to Sink
```

#### 2.4 AsciiReader (ASCII Reader)

**File Location**: `src/asciireader.h`, `src/asciireader.cpp`

**Key Function**: `AsciiReader::readData()` 

**Data Processing Flow**:
1. Read ASCII text data line by line
2. Parse each line using delimiter (comma, tab, etc.)
3. Support decimal and hexadecimal formats
4. **Call `feedOut(*samples)` to pass data** (line 155)

### 3. Source-Sink Architecture

#### 3.1 Source Class

**File Location**: `src/source.h`, `src/source.cpp`

**Key Function**: `Source::feedOut()` (`src/source.cpp:65-71`)

```cpp
void Source::feedOut(const SamplePack& data) const
{
    for (auto sink : sinks)
    {
        sink->feedIn(data);  // ← Pass data to all connected Sinks
    }
}
```

**Functionality**:
- Maintains a list of Sinks
- `feedOut()` method distributes data to all connected Sinks
- `connectSink()` method connects new Sink
- `disconnectSinks()` method disconnects all Sinks

#### 3.2 Sink Class

**File Location**: `src/sink.h`, `src/sink.cpp`

**Key Function**: `Sink::feedIn()` (`src/sink.cpp:38-44`)

```cpp
void Sink::feedIn(const SamplePack& data)
{
    for (auto sink : followers)
    {
        sink->feedIn(data);  // ← Pass to followers
    }
}
```

**Functionality**:
- Receives data from Source
- Supports "follower" mode, can pass data to other Sinks
- Stream class implements Sink interface

### 4. Data Format Panel: DataFormatPanel

**File Location**: `src/dataformatpanel.h`, `src/dataformatpanel.cpp`

**Key Functions**: 
- `DataFormatPanel::selectReader()` (`src/dataformatpanel.cpp:117-135`)
- `DataFormatPanel::activeSource()` (`src/dataformatpanel.cpp:80-83`)

**Functionality**:
1. Manages multiple Reader instances (BinaryStreamReader, AsciiReader, FramedReader, DemoReader)
2. Switches current active Reader based on user selection
3. Emits `sourceChanged` signal to notify main window

**Key Code** (`src/dataformatpanel.cpp:117-135`):
```cpp
void DataFormatPanel::selectReader(AbstractReader* reader)
{
    currentReader->enable(false);
    reader->enable();
    
    // Reconnect signals
    disconnect(currentReader, 0, this, 0);
    
    // Switch settings widget
    ui->horizontalLayout->removeWidget(currentReader->settingsWidget());
    currentReader->settingsWidget()->hide();
    ui->horizontalLayout->addWidget(reader->settingsWidget(), 1);
    reader->settingsWidget()->show();
    
    reader->pause(paused);
    
    currentReader = reader;
    emit sourceChanged(currentReader);  // ← Notify main window
}
```

### 5. Main Window Connection: MainWindow

**File Location**: `src/mainwindow.cpp`

**Key Function**: `MainWindow::onSourceChanged()` (`src/mainwindow.cpp:423-427`)

```cpp
void MainWindow::onSourceChanged(Source* source)
{
    source->connectSink(&stream);         // ← Connect to main data stream
    source->connectSink(&sampleCounter);  // ← Connect to sample rate counter
}
```

**Initialization Connection** (`src/mainwindow.cpp:274-277`):
```cpp
// Initialize stream connections
connect(&dataFormatPanel, &DataFormatPanel::sourceChanged,
        this, &MainWindow::onSourceChanged);
onSourceChanged(dataFormatPanel.activeSource());
```

**Functionality**:
- When user switches data format, reconnect Source and Sink
- Connect Reader (Source) to Stream (Sink)
- Also connect to SampleCounter for sampling rate statistics

### 6. Data Stream: Stream Class

**File Location**: `src/stream.h`, `src/stream.cpp`

**Key Function**: `Stream::feedIn()` (`src/stream.cpp:213-244`)

**Data Processing Flow**:
```cpp
void Stream::feedIn(const SamplePack& pack)
{
    Q_ASSERT(pack.numChannels() == numChannels() &&
             pack.hasX() == hasX());
    
    if (_paused) return;  // Skip when paused
    
    unsigned ns = pack.numSamples();
    
    // Apply gain and offset (if enabled)
    const SamplePack* mPack = nullptr;
    if (infoModel()->gainOrOffsetEn())
        mPack = applyGainOffset(pack);
    
    // Add data to ring buffer of each channel
    for (unsigned ci = 0; ci < numChannels(); ci++)
    {
        auto buf = static_cast<RingBuffer*>(channels[ci]->yData());
        double* data = (mPack == nullptr) ? pack.data(ci) : mPack->data(ci);
        buf->addSamples(data, ns);  // ← Data stored in ring buffer
    }
    
    Sink::feedIn((mPack == nullptr) ? pack : *mPack);  // Pass to followers
    
    if (mPack != nullptr) delete mPack;
    emit dataAdded();  // ← Trigger redraw signal
}
```

**Functionality**:
1. Implements Sink interface, receives data from Reader
2. Applies user-configured gain (division) and offset
3. Distributes data to each StreamChannel
4. Emits `dataAdded()` signal to trigger chart update

### 7. Channel: StreamChannel Class

**File Location**: `src/streamchannel.h`, `src/streamchannel.cpp`

**Data Members**:
```cpp
const XFrameBuffer* _x;  // X-axis buffer (index or linear)
FrameBuffer* _y;         // Y-axis data buffer (RingBuffer)
```

**Functionality**:
- Each channel holds its own Y-axis data buffer (RingBuffer)
- Shares X-axis buffer (IndexBuffer or LinIndexBuffer)
- Provides channel name, color, visibility and other properties

### 8. Buffer: RingBuffer Class

**File Location**: `src/ringbuffer.h`, `src/ringbuffer.cpp`

**Key Function**: `RingBuffer::addSamples()`

**Functionality**:
- Ring buffer implementation with fixed size
- When buffer is full, new data overwrites old data
- Supports batch adding of samples
- Implements `FrameBuffer` interface, provides `sample(i)` method to access data

### 9. Plot Manager: PlotManager Class

**File Location**: `src/plotmanager.h`, `src/plotmanager.cpp`

**Initialization Connection** (`src/plotmanager.cpp:30-58`):
```cpp
PlotManager::PlotManager(QWidget* plotArea, PlotMenu* menu,
                         const Stream* stream, QObject* parent)
{
    _stream = stream;
    construct(plotArea, menu);
    if (_stream == nullptr) return;
    
    // Connect to ChannelInfoModel
    infoModel = _stream->infoModel();
    connect(infoModel, &QAbstractItemModel::dataChanged,
            this, &PlotManager::onChannelInfoChanged);
    
    connect(stream, &Stream::numChannelsChanged, 
            this, &PlotManager::onNumChannelsChanged);
    connect(stream, &Stream::dataAdded, 
            this, &PlotManager::replot);  // ← Redraw when data updates
    
    // Add initial curves
    for (unsigned int i = 0; i < stream->numChannels(); i++)
    {
        addCurve(stream->channel(i)->name(), 
                stream->channel(i)->xData(), 
                stream->channel(i)->yData());
    }
}
```

**Key Function**: `PlotManager::replot()` 

**Functionality**:
1. Listens to Stream's `dataAdded()` signal
2. Creates QwtPlotCurve for each StreamChannel
3. Calls `replot()` to update all chart displays
4. Manages multi-chart display mode
5. Handles legend, grid, zoom and other UI features

### 10. Data Adapter: FrameBufferSeries Class

**File Location**: `src/framebufferseries.h`, `src/framebufferseries.cpp`

**Functionality**:
- Implements Qwt's `QwtSeriesData<QPointF>` interface
- Acts as bridge between FrameBuffer and Qwt plotting library
- Provides `sample(i)` method to return QPointF point data
- Supports "region of interest" optimization, only draws visible portion

**Key Method**:
```cpp
QPointF FrameBufferSeries::sample(size_t i) const
{
    return QPointF(_x->sample(i), _y->sample(i));
}
```

### 11. Chart Display: Plot Class

**File Location**: `src/plot.h`, `src/plot.cpp`

**Functionality**:
- Inherits from QwtPlot
- Displays one or more QwtPlotCurve
- Provides zoom, pan, grid and other features
- Actually renders graphics to screen

## Complete Data Flow Call Chain

### Typical Data Flow Call Sequence:

1. **Serial Port Receives Data**
   ```
   QSerialPort::readyRead() signal triggered
   ```

2. **Reader Reads Data**
   ```
   AbstractReader::onDataReady()
   → BinaryStreamReader::readData()  // or other Reader
   → Create SamplePack object
   → Source::feedOut(samples)
   ```

3. **Distribute to Sinks**
   ```
   Source::feedOut()
   → for each sink in sinks:
       → Sink::feedIn(data)
   ```

4. **Stream Processes Data**
   ```
   Stream::feedIn(pack)
   → Apply gain/offset (if enabled)
   → for each channel:
       → RingBuffer::addSamples(data, n)
   → emit dataAdded()
   ```

5. **Trigger Redraw**
   ```
   Stream::dataAdded() signal
   → PlotManager::replot() slot
   → for each Plot widget:
       → QwtPlot::replot()
   ```

6. **Qwt Drawing**
   ```
   QwtPlot::replot()
   → for each QwtPlotCurve:
       → FrameBufferSeries::sample(i)
       → RingBuffer::sample(i)
       → Render to screen
   ```

## Key Design Patterns

### 1. Source-Sink Pattern (Observer Pattern Variant)

- **Source**: Data producer (Reader classes)
- **Sink**: Data consumer (Stream, SampleCounter, etc.)
- **Advantage**: Decouples data generation and consumption, supports one-to-many distribution

### 2. Strategy Pattern

- **AbstractReader**: Abstract strategy
- **BinaryStreamReader, AsciiReader, FramedReader**: Concrete strategies
- **DataFormatPanel**: Strategy context, responsible for selection and switching

### 3. Adapter Pattern

- **FrameBufferSeries**: Adapter, adapts FrameBuffer to Qwt interface

## SamplePack Data Structure

**File Location**: `src/samplepack.h`, `src/samplepack.cpp`

**Data Members**:
```cpp
unsigned _numSamples;    // Number of samples
unsigned _numChannels;   // Number of channels
double* _xData;          // X-axis data (optional)
double* _yData;          // Y-axis data (all channels stored contiguously)
```

**Memory Layout**:
```
_yData: [CH0_S0, CH0_S1, ..., CH1_S0, CH1_S1, ..., CHn_S0, CHn_S1, ...]
        |------ channel 0 ------|------ channel 1 ------|--- channel n ---|
```

## Data Format Support

### 1. Binary Stream Format (BinaryStreamReader)
- Supported data types: uint8, int8, uint16, int16, uint24, int24, uint32, int32, float, double
- Byte order: little-endian or big-endian
- Data organization: continuous channel sample packages

### 2. ASCII Format (AsciiReader)
- Delimiters: comma, tab, space, etc.
- Number format: decimal or hexadecimal
- Line filtering: supports prefix filtering

### 3. Frame Format (FramedReader)
- Sync word: custom frame start marker
- Channel mapping: flexible byte offset and data type configuration
- Checksum: supports multiple checksum algorithms (CRC, checksum, etc.)
- Performance optimization: uses KMP algorithm for sync word search

## Performance Optimization Points

1. **Batch Reading**: Readers read multiple sample packages at once when possible
2. **Ring Buffer**: Avoids frequent memory allocation and data movement
3. **KMP Algorithm**: FramedReader uses KMP algorithm for O(n+m) sync word search
4. **Region of Interest**: FrameBufferSeries only processes data points in visible area
5. **Signal-Slot Optimization**: Uses Qt's signal-slot mechanism for asynchronous updates

## Threading Model

- **Main Thread**: All data processing and UI updates happen in main thread
- **No Additional Threads**: QSerialPort internally uses async I/O, but callbacks are in main thread
- **Signal-Driven**: Uses Qt signal-slot for event-driven data flow

## Legacy Code in Data Flow

After examining the actual code, the following points are confirmed:

1. **No unused data paths found**: All Readers use the same Source-Sink mechanism
2. **Stream is the only data storage**: No other parallel data flow paths
3. **Gain processing now uses division**: In `Stream::applyGainOffset()`, gain now uses division instead of multiplication (line 197)
4. **X data not yet implemented**: Code has reserved interface for X data, but all current Readers' `hasX()` return false

## Summary

SerialPlot's data flow uses a clear layered architecture:

1. **Hardware Layer**: QSerialPort handles serial port communication
2. **Parsing Layer**: AbstractReader subclasses handle different format parsing
3. **Distribution Layer**: Source-Sink mechanism implements data distribution
4. **Storage Layer**: Stream and RingBuffer handle data caching
5. **Display Layer**: PlotManager and Qwt handle data visualization

This design makes it easy to add new data formats - just implement a new AbstractReader subclass without modifying other parts of the code.
