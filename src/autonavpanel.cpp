#include "autonavpanel.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

AutoNavPanel::AutoNavPanel(QSerialPort* port, QWidget* parent)
    : QWidget(parent), m_port(port)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->addWidget(buildControls());

    connect(&m_timer, &QTimer::timeout, this, &AutoNavPanel::onTimerTimeout);
}

AutoNavPanel::~AutoNavPanel()
{
    m_timer.stop();
}

void AutoNavPanel::setSerialPort(QSerialPort* port)
{
    m_port = port;
}

QWidget* AutoNavPanel::buildControls()
{
    auto* panel = new QWidget(this);
    auto* layout = new QVBoxLayout(panel);
    auto* formLayout = new QFormLayout();

    auto* intervalSpin = new QSpinBox(panel);
    intervalSpin->setRange(50, 60 * 1000);
    intervalSpin->setValue(m_intervalMs);
    formLayout->addRow(tr("Interval (ms)"), intervalSpin);
    layout->addLayout(formLayout);

    auto* buttonLayout = new QHBoxLayout();
    auto* startButton = new QPushButton(tr("Start"), panel);
    auto* stopButton = new QPushButton(tr("Stop"), panel);
    auto* sendOnceButton = new QPushButton(tr("Send once"), panel);

    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(sendOnceButton);
    layout->addLayout(buttonLayout);
    layout->addStretch(1);

    connect(intervalSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        m_intervalMs = value;
        if (m_timer.isActive())
            m_timer.start(m_intervalMs);
    });
    connect(startButton, &QPushButton::clicked, this, &AutoNavPanel::onStartClicked);
    connect(stopButton, &QPushButton::clicked, this, &AutoNavPanel::onStopClicked);
    connect(sendOnceButton, &QPushButton::clicked, this, &AutoNavPanel::onSendOnceClicked);

    return panel;
}

void AutoNavPanel::onStartClicked()
{
    m_timer.start(m_intervalMs);
}

void AutoNavPanel::onStopClicked()
{
    m_timer.stop();
}

void AutoNavPanel::onSendOnceClicked()
{
    assembleAndSendFrame();
}

void AutoNavPanel::onTimerTimeout()
{
    assembleAndSendFrame();
}

void AutoNavPanel::assembleAndSendFrame()
{
    if (!m_port || !m_port->isOpen())
        return;

    QByteArray frame;
    frame.reserve(4);
    frame.push_back(static_cast<char>(0xAA));
    frame.push_back(static_cast<char>(0x55));
    frame.push_back(static_cast<char>(m_frameCnt++));

    uint8_t checksum = static_cast<uint8_t>(
        static_cast<uint8_t>(frame[0]) +
        static_cast<uint8_t>(frame[1]) +
        static_cast<uint8_t>(frame[2]));
    frame.push_back(static_cast<char>(checksum));

    m_port->write(frame);
}
