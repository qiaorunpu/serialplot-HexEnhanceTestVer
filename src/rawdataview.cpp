/*
  Copyright © 2025 Hasan Yavuz Özderya

  This file is part of serialplot.

  serialplot is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  serialplot is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with serialplot.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rawdataview.h"
#include <QGroupBox>
#include <QButtonGroup>
#include <QScrollBar>

RawDataView::RawDataView(QWidget *parent)
    : QWidget(parent),
      m_isHexMode(false),
      m_isLogMode(false),
      m_isFrozen(false),
      m_isWordWrapMode(false)
{
    setupUI();
}

void RawDataView::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    
    // Control panel
    auto controlPanel = new QGroupBox("Display Options");
    auto controlLayout = new QHBoxLayout(controlPanel);
    
    // ASCII/HEX radio buttons
    auto formatGroup = new QButtonGroup(this);
    m_asciiRadio = new QRadioButton("ASCII");
    m_hexRadio = new QRadioButton("HEX");
    m_asciiRadio->setChecked(true); // Default to ASCII
    
    formatGroup->addButton(m_asciiRadio);
    formatGroup->addButton(m_hexRadio);
    
    connect(m_asciiRadio, &QRadioButton::toggled, 
            this, &RawDataView::onDisplayModeChanged);
    connect(m_hexRadio, &QRadioButton::toggled, 
            this, &RawDataView::onDisplayModeChanged);
    
    // Log mode checkbox
    m_logModeCheck = new QCheckBox("Log Display Mode");
    connect(m_logModeCheck, &QCheckBox::toggled,
            this, &RawDataView::onLogModeChanged);
    
    // Word wrap checkbox (only for ASCII mode)
    m_wordWrapCheck = new QCheckBox("Auto Word Wrap");
    m_wordWrapCheck->setToolTip("Parse escape characters (\\n, \\r, \\t) in ASCII mode");
    connect(m_wordWrapCheck, &QCheckBox::toggled,
            this, &RawDataView::onWordWrapModeChanged);
    
    // Control buttons
    m_clearButton = new QPushButton("Clear");
    m_freezeButton = new QPushButton("Freeze");
    m_freezeButton->setCheckable(true);
    
    connect(m_clearButton, &QPushButton::clicked,
            this, &RawDataView::clearData);
    connect(m_freezeButton, &QPushButton::toggled,
            this, &RawDataView::toggleFreeze);
    
    // Add controls to layout
    controlLayout->addWidget(m_asciiRadio);
    controlLayout->addWidget(m_hexRadio);
    controlLayout->addWidget(m_logModeCheck);
    controlLayout->addWidget(m_wordWrapCheck);
    controlLayout->addStretch();
    controlLayout->addWidget(m_clearButton);
    controlLayout->addWidget(m_freezeButton);
    
    // Text display area
    m_textDisplay = new QPlainTextEdit();
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setFont(QFont("Consolas", 10)); // Monospace font for better alignment
    
    // Add to main layout
    mainLayout->addWidget(controlPanel);
    mainLayout->addWidget(m_textDisplay);
    
    // Set initial state
    onDisplayModeChanged();
    onLogModeChanged();
    onWordWrapModeChanged();
}

void RawDataView::addReceivedData(const QByteArray& data)
{
    if (!m_isFrozen && !data.isEmpty())
    {
        addDataToDisplay(data, true);
    }
}

void RawDataView::addSentData(const QByteArray& data)
{
    if (!m_isFrozen && !data.isEmpty())
    {
        addDataToDisplay(data, false);
    }
}

void RawDataView::addDataToDisplay(const QByteArray& data, bool isReceived)
{
    QString displayText;
    
    if (m_isLogMode)
    {
        // Log format: [2025-12-12 11:12:33.490]# RECV ASCII/21 <<<
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString direction = isReceived ? "RECV" : "SEND";
        QString format = m_isHexMode ? "HEX" : "ASCII";
        QString arrow = isReceived ? "<<<" : ">>>";
        
        displayText = QString("[%1]# %2 %3/%4 %5\n")
                      .arg(timestamp)
                      .arg(direction)
                      .arg(format)
                      .arg(data.size())
                      .arg(arrow);
        
        if (m_isHexMode)
        {
            displayText += formatDataAsHex(data);
        }
        else
        {
            displayText += m_isWordWrapMode ? formatDataAsAsciiWithEscapes(data) : formatDataAsAscii(data);
        }
        displayText += "\n";
    }
    else
    {
        // Simple format without timestamp
        if (m_isHexMode)
        {
            displayText = formatDataAsHex(data);
        }
        else
        {
            displayText = m_isWordWrapMode ? formatDataAsAsciiWithEscapes(data) : formatDataAsAscii(data);
        }
    }
    
    // Add to text display
    m_textDisplay->appendPlainText(displayText.trimmed());
    
    // Auto-scroll to bottom
    QScrollBar* scrollBar = m_textDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

QString RawDataView::formatDataAsHex(const QByteArray& data)
{
    QString result;
    for (int i = 0; i < data.size(); ++i)
    {
        if (i > 0) result += " ";
        result += QString("%1").arg((uint8_t)data[i], 2, 16, QChar('0')).toUpper();
    }
    return result;
}

QString RawDataView::formatDataAsAscii(const QByteArray& data)
{
    QString result;
    for (char c : data)
    {
        if (c >= 32 && c <= 126) // Printable ASCII
        {
            result += c;
        }
        else if (c == '\n')
        {
            result += "\\n";
        }
        else if (c == '\r')
        {
            result += "\\r";
        }
        else if (c == '\t')
        {
            result += "\\t";
        }
        else
        {
            // Non-printable character, show as hex
            result += QString("\\x%1").arg((uint8_t)c, 2, 16, QChar('0')).toUpper();
        }
    }
    return result;
}

void RawDataView::clearData()
{
    m_textDisplay->clear();
}

void RawDataView::toggleFreeze(bool frozen)
{
    m_isFrozen = frozen;
    m_freezeButton->setText(frozen ? "Unfreeze" : "Freeze");
}

void RawDataView::onDisplayModeChanged()
{
    m_isHexMode = m_hexRadio->isChecked();
    // Enable/disable the word wrap checkbox only when in ASCII mode
    m_wordWrapCheck->setEnabled(!m_isHexMode);
    if (m_isHexMode) {
        // Disable word wrap when switching to HEX mode
        m_wordWrapCheck->setChecked(false);
    }
}

void RawDataView::onLogModeChanged()
{
    m_isLogMode = m_logModeCheck->isChecked();
}

void RawDataView::onWordWrapModeChanged()
{
    m_isWordWrapMode = m_wordWrapCheck->isChecked();
    // Enable/disable the word wrap checkbox only when in ASCII mode
    m_wordWrapCheck->setEnabled(!m_isHexMode);
}

QString RawDataView::formatDataAsAsciiWithEscapes(const QByteArray& data)
{
    QString result;
    for (char c : data)
    {
        if (c >= 32 && c <= 126) // Printable ASCII
        {
            result += c;
        }
        else if (c == '\n')
        {
            result += '\n'; // Actual newline character
        }
        else if (c == '\r')
        {
            result += '\r'; // Actual carriage return
        }
        else if (c == '\t')
        {
            result += '\t'; // Actual tab character
        }
        else
        {
            // Non-printable character, show as hex
            result += QString("\\x%1").arg((uint8_t)c, 2, 16, QChar('0')).toUpper();
        }
    }
    return result;
}