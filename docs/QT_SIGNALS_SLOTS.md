# Qt 信号与槽机制 (Qt Signals and Slots Mechanism)

## 什么是 private slots? (What are private slots?)

在 Qt 框架中，`private slots:` 是一个访问说明符，用于声明私有槽函数。槽函数是 Qt 信号与槽机制的重要组成部分。

In the Qt framework, `private slots:` is an access specifier used to declare private slot functions. Slot functions are an important component of Qt's signal-slot mechanism.

## Qt 信号与槽机制概述 (Overview of Qt Signals and Slots)

Qt 的信号与槽机制是一种用于对象间通信的强大工具。它是 Qt 框架的核心特性之一，提供了一种类型安全、松耦合的方式来处理事件和回调。

Qt's signal-slot mechanism is a powerful tool for inter-object communication. It is one of the core features of the Qt framework, providing a type-safe, loosely coupled way to handle events and callbacks.

### 基本概念 (Basic Concepts)

- **信号 (Signal)**: 当特定事件发生时，对象发出的通知
  - Notification emitted by an object when a specific event occurs
  
- **槽 (Slot)**: 响应信号的函数，当连接的信号发出时被调用
  - Function that responds to signals, called when a connected signal is emitted

- **连接 (Connection)**: 信号与槽之间的关联，通过 `connect()` 函数建立
  - Association between signals and slots, established through the `connect()` function

## 槽的访问级别 (Slot Access Levels)

Qt 支持三种访问级别的槽：

Qt supports three access levels for slots:

### 1. public slots:
- 可以被任何对象调用和连接
- Can be called and connected by any object
- 用于需要从外部访问的槽函数
- Used for slot functions that need to be accessed externally

### 2. protected slots:
- 可以被类本身和派生类访问
- Can be accessed by the class itself and derived classes
- 适合在继承层次结构中使用的槽
- Suitable for slots used within inheritance hierarchies

### 3. private slots:
- **只能被类本身访问**
- **Can only be accessed by the class itself**
- 不能被外部对象直接调用（但可以通过信号连接）
- Cannot be called directly by external objects (but can be connected via signals)
- **这是最常用的槽访问级别**
- **This is the most commonly used slot access level**

## private slots 的作用 (Purpose of private slots)

### 主要用途 (Primary Uses)

1. **封装内部实现 (Encapsulating Internal Implementation)**
   ```cpp
   private slots:
       void onButtonClicked();      // 处理内部按钮点击
       void updateInternalState();  // 更新内部状态
   ```
   这些槽函数处理类的内部逻辑，不应该被外部直接调用。
   
   These slot functions handle the class's internal logic and should not be called directly from outside.

2. **响应 UI 事件 (Responding to UI Events)**
   ```cpp
   private slots:
       void onPortToggled(bool open);       // 响应端口开关
       void onNumOfSamplesChanged(int value); // 响应采样数变化
   ```
   UI 组件发出的信号连接到这些私有槽，处理用户交互。
   
   Signals from UI components connect to these private slots to handle user interactions.

3. **处理定时器和异步操作 (Handling Timers and Async Operations)**
   ```cpp
   private slots:
       void updatePinLeds(void);   // 定期更新 LED 状态
       void onDataReady();         // 数据准备就绪时调用
   ```
   用于响应定时器超时或异步操作完成的信号。
   
   Used to respond to timer timeouts or async operation completion signals.

4. **对象间通信 (Inter-Object Communication)**
   ```cpp
   private slots:
       void onSourceChanged(Source* source);  // 响应数据源变化
       void onSpsChanged(float sps);          // 响应采样率变化
   ```
   接收来自其他对象的通知，但不暴露实现细节。
   
   Receive notifications from other objects without exposing implementation details.

## 代码示例 (Code Examples)

### 示例 1: MainWindow 中的私有槽 (Private slots in MainWindow)

```cpp
// 在 mainwindow.h 中
class MainWindow : public QMainWindow
{
    Q_OBJECT

private slots:
    void onPortToggled(bool open);
    void onSourceChanged(Source* source);
    void onNumOfSamplesChanged(int value);
    void clearPlot();
    void onSpsChanged(float sps);
    void enableDemo(bool enabled);
};
```

这些槽函数：
- 响应各种 UI 组件和内部对象发出的信号
- 保持为私有，因为它们处理 MainWindow 的内部状态
- 不需要被其他类直接调用

These slot functions:
- Respond to signals from various UI components and internal objects
- Remain private because they handle MainWindow's internal state
- Don't need to be called directly by other classes

### 示例 2: 连接信号到私有槽 (Connecting signals to private slots)

```cpp
// 在构造函数中建立连接
connect(&portControl, &PortControl::portToggled,
        this, &MainWindow::onPortToggled);

connect(&stream, &Stream::samplesPerSecondChanged,
        this, &MainWindow::onSpsChanged);
```

尽管槽是私有的，但 Qt 的信号槽机制允许通过 `connect()` 连接到它们。这提供了：
- **封装性**: 槽的实现细节对外部隐藏
- **灵活性**: 仍然可以通过信号机制调用
- **类型安全**: 编译时检查信号和槽的参数匹配

Although the slots are private, Qt's signal-slot mechanism allows connecting to them via `connect()`. This provides:
- **Encapsulation**: Slot implementation details are hidden from outside
- **Flexibility**: Can still be invoked through the signal mechanism
- **Type Safety**: Compile-time checking of signal-slot parameter matching

## 为什么使用 private slots 而不是 public slots? (Why use private slots instead of public slots?)

### 优点 (Advantages)

1. **更好的封装 (Better Encapsulation)**
   - 防止外部代码直接调用内部处理函数
   - Prevents external code from directly calling internal handler functions

2. **更清晰的接口 (Clearer Interface)**
   - 明确哪些方法是公共 API，哪些是内部实现
   - Clearly distinguishes public API from internal implementation

3. **更容易维护 (Easier Maintenance)**
   - 内部实现可以自由修改，不影响外部代码
   - Internal implementation can be freely modified without affecting external code

4. **防止误用 (Prevents Misuse)**
   - 避免其他开发者错误地直接调用槽函数
   - Avoids other developers incorrectly calling slot functions directly

### 使用建议 (Usage Guidelines)

- **默认使用 private slots**: 除非有明确理由需要公开
  - **Use private slots by default**: Unless there's a clear reason to make them public

- **public slots 仅用于**: 设计为公共 API 的槽函数
  - **Use public slots only for**: Slot functions designed as public API

- **protected slots 用于**: 需要在派生类中重写或访问的槽
  - **Use protected slots for**: Slots that need to be overridden or accessed in derived classes

## 常见模式 (Common Patterns)

### 模式 1: UI 事件处理 (UI Event Handling)

```cpp
class MyWidget : public QWidget
{
    Q_OBJECT

private:
    QPushButton* button;

private slots:
    void handleButtonClick();  // 处理按钮点击的私有槽
};

// 在构造函数中
connect(button, &QPushButton::clicked,
        this, &MyWidget::handleButtonClick);
```

### 模式 2: 定时器处理 (Timer Handling)

```cpp
class PortControl : public QWidget
{
    Q_OBJECT

private:
    QTimer pinUpdateTimer;

private slots:
    void updatePinLeds(void);  // 定期更新的私有槽
};

// 在构造函数中
connect(&pinUpdateTimer, &QTimer::timeout,
        this, &PortControl::updatePinLeds);
```

### 模式 3: 对象间通信 (Inter-Object Communication)

```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Stream stream;

private slots:
    void onSpsChanged(float sps);  // 响应流对象变化的私有槽
};

// 在构造函数中
connect(&stream, &Stream::samplesPerSecondChanged,
        this, &MainWindow::onSpsChanged);
```

## 总结 (Summary)

`private slots:` 是 Qt 中最常用的槽访问级别，它提供了：
- 良好的封装性
- 清晰的接口设计
- 灵活的信号槽连接
- 类型安全的事件处理

`private slots:` is the most commonly used slot access level in Qt, providing:
- Good encapsulation
- Clear interface design
- Flexible signal-slot connections
- Type-safe event handling

在 SerialPlot 项目中，你可以看到大量使用 `private slots:` 的例子，它们用于处理 UI 事件、定时器超时、数据更新等各种内部逻辑。

In the SerialPlot project, you can see many examples of `private slots:` usage for handling UI events, timer timeouts, data updates, and various internal logic.

## 参考资源 (References)

- [Qt 官方文档: Signals & Slots](https://doc.qt.io/qt-6/signalsandslots.html)
- [Qt 官方文档: Qt's Property System](https://doc.qt.io/qt-6/properties.html)
- SerialPlot 源代码中的实际使用示例
  - `src/mainwindow.h` - MainWindow 类的槽函数
  - `src/portcontrol.h` - PortControl 类的槽函数
  - `src/snapshot.h` - Snapshot 类的槽函数
