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

#ifndef RAWDATAVIEW_H
#define RAWDATAVIEW_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime>

/**
 * Widget for displaying raw serial data in ASCII or HEX format
 * with optional timestamp logging and freeze/clear functionality
 */
class RawDataView : public QWidget
{
    Q_OBJECT

public:
    explicit RawDataView(QWidget *parent = nullptr);
    
    /// Add received data to display
    void addReceivedData(const QByteArray& data);
    
    /// Add sent data to display (for future use when send functionality is added)
    void addSentData(const QByteArray& data);

public slots:
    /// Clear all displayed data
    void clearData();
    
    /// Toggle freeze mode
    void toggleFreeze(bool frozen);
    
private slots:
    void onDisplayModeChanged();
    void onLogModeChanged();
    void onWordWrapModeChanged();

private:
    void setupUI();
    void addDataToDisplay(const QByteArray& data, bool isReceived);
    QString formatDataAsHex(const QByteArray& data);
    QString formatDataAsAscii(const QByteArray& data);
    QString formatDataAsAsciiWithEscapes(const QByteArray& data);
    
    // UI components
    QPlainTextEdit* m_textDisplay;
    QRadioButton* m_asciiRadio;
    QRadioButton* m_hexRadio;
    QCheckBox* m_logModeCheck;
    QCheckBox* m_wordWrapCheck;
    QPushButton* m_clearButton;
    QPushButton* m_freezeButton;
    
    // State
    bool m_isHexMode;
    bool m_isLogMode;
    bool m_isFrozen;
    bool m_isWordWrapMode;
};

#endif // RAWDATAVIEW_H