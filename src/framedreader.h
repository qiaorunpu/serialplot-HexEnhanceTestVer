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

#ifndef FRAMEDREADER_H
#define FRAMEDREADER_H

#include <QSettings>
#include <map>

#include "abstractreader.h"
#include "framedreadersettings.h"
#include "channelmapping.h"
#include "checksumcalculator.h"

/**
 * Reads data in a customizable framed format with flexible channel mapping
 * and multiple checksum algorithms.
 */
class FramedReader : public AbstractReader
{
    Q_OBJECT

public:
    explicit FramedReader(QIODevice* device, QObject *parent = 0);
    QWidget* settingsWidget();
    unsigned numChannels() const;
    /// Stores settings into a `QSettings`
    void saveSettings(QSettings* settings);
    /// Loads settings from a `QSettings`.
    void loadSettings(QSettings* settings);

private:
    // settings related members
    FramedReaderSettings _settingsWidget;
    unsigned _numChannels;
    QByteArray syncWord;
    bool hasSizeByte;
    bool isSizeField2B;
    unsigned frameSize;
    bool debugModeEnabled;
    
    // Channel mapping and checksum
    ChannelMappingConfig _channelMapping;
    ChecksumConfig _checksumConfig;

    /// Checks the validity of settings
    void checkSettings();
    QString getLastErrorMessage() const;
    bool _settingsValid;
    QString _lastErrorMessage;

    // read state related members
    unsigned sync_i;
    bool gotSync;
    bool gotSize;
    uint8_t* _frameBuffer;
    unsigned _frameBufferSize;

    void reset();
    void readFrameDataAndExtractChannels();
    
    /// Extract a single value from buffer according to channel mapping
    double extractChannelValue(const ChannelMapping& ch, const uint8_t* buffer);
    
    /// Calculate checksum based on configuration
    uint32_t calculateFrameChecksum(const uint8_t* data, unsigned dataLength);

    unsigned readData() override;

private slots:
    void onNumOfChannelsChanged(unsigned value);
    void onChannelMappingChanged();
    void onChecksumConfigChanged();
    void onTotalFrameLengthChanged();
    void onSyncWordChanged(QByteArray);

private:
    void recalculateFrameSize();
};

#endif // FRAMEDREADER_H
