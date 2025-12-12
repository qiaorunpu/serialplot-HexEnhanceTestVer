/*
  Copyright © 2021 Hasan Yavuz Özderya

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

#ifndef FRAMEDREADERSETTINGS_H
#define FRAMEDREADERSETTINGS_H

#include <QWidget>
#include <QByteArray>
#include <QSettings>
#include <QButtonGroup>

#include "numberformatbox.h"
#include "endiannessbox.h"
#include "channelmapping.h"
#include "checksumcalculator.h"

namespace Ui {
class FramedReaderSettings;
}

struct ChecksumConfig
{
    ChecksumAlgorithm algorithm;
    unsigned startByte;
    unsigned endByte;
    bool enabled;
    bool isLittleEndian;  // true for little endian, false for big endian

    ChecksumConfig() : algorithm(ChecksumAlgorithm::None), startByte(0), endByte(0), enabled(false), isLittleEndian(true) {}
};

class FramedReaderSettings : public QWidget
{
    Q_OBJECT

public:
    explicit FramedReaderSettings(QWidget *parent = 0);
    ~FramedReaderSettings();

    void showMessage(QString message, bool error = false);

    unsigned numOfChannels();
    QByteArray syncWord();
    unsigned fixedFrameSize() const;
    unsigned totalFrameLength() const;
    bool isChecksumEnabled();
    bool isDebugModeEnabled();
    
    ChannelMappingConfig& channelMapping();
    ChecksumConfig& checksumConfig();
    
    /// Save settings into a `QSettings`
    void saveSettings(QSettings* settings);
    /// Loads settings from a `QSettings`.
    void loadSettings(QSettings* settings);

signals:
    /// If sync word is invalid (empty or 1 nibble missing at the end)
    /// signaled with an empty array
    void syncWordChanged(QByteArray);
    void fixedFrameSizeChanged(unsigned);
    void totalFrameLengthChanged(unsigned);
    void checksumChanged(bool);
    void numOfChannelsChanged(unsigned);
    void debugModeChanged(bool);
    void channelMappingChanged();
    void checksumConfigChanged();

private:
    Ui::FramedReaderSettings *ui;
    QButtonGroup fbGroup;
    ChannelMappingConfig _channelMapping;
    ChecksumConfig _checksumConfig;
    
private:
    void updatePayloadSizeInternal();

private slots:
    void onSyncWordEdited();
    void onChannelMappingClicked();
    void onChecksumConfigClicked();
    void onTotalFrameLengthChanged();
    void updatePayloadSize();
};

#endif // FRAMEDREADERSETTINGS_H
