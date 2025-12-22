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

#include <QtDebug>
#include <QtEndian>
#include <QDateTime>
#include <cstring>

#include "framedreader.h"

FramedReader::FramedReader(QIODevice* device, QObject* parent) :
    AbstractReader(device, parent),
    _numChannels(1),
    hasSizeByte(true),
    isSizeField2B(false),
    frameSize(64),
    debugModeEnabled(false),
    _settingsValid(false),
    sync_i(0),
    gotSync(false),
    gotSize(false),
    _frameBuffer(nullptr),
    _frameBufferSize(0),
    _kmpMatcher(nullptr)
{
    paused = false;

    // Get channel mapping and checksum config first
    _channelMapping = _settingsWidget.channelMapping();
    _checksumConfig = _settingsWidget.checksumConfig();
    
    // initial settings
    _numChannels = _settingsWidget.numOfChannels();
    hasSizeByte = false;  // Size field removed - frame format is now fixed
    isSizeField2B = false;
    syncWord = _settingsWidget.syncWord();
    debugModeEnabled = _settingsWidget.isDebugModeEnabled();
    
    // Calculate frame size: Total Frame Length - sync word - checksum
    unsigned totalLength = _settingsWidget.totalFrameLength();
    unsigned frameStartLength = syncWord.size();
    unsigned checksumLength = _checksumConfig.enabled ? ChecksumCalculator::getOutputSize(_checksumConfig.algorithm) : 0;
    int calculatedFrameSize = totalLength - frameStartLength - checksumLength;
    frameSize = calculatedFrameSize > 0 ? calculatedFrameSize : 1;
    // Endianness is now per-channel

    // Allocate frame buffer
    _frameBuffer = new uint8_t[65535];
    _frameBufferSize = 65535;
    
    // Performance optimization: Initialize KMP matcher
    _kmpMatcher = new KMPMatcher(syncWord);
    _readBuffer.reserve(65536);  // Reserve 64KB for batch reading

    checkSettings();

    // init setting connections
    connect(&_settingsWidget, &FramedReaderSettings::numOfChannelsChanged,
            this, &FramedReader::onNumOfChannelsChanged);

    connect(&_settingsWidget, &FramedReaderSettings::syncWordChanged,
            this, &FramedReader::onSyncWordChanged);

    connect(&_settingsWidget, &FramedReaderSettings::checksumChanged,
            [this](bool enabled){ _checksumConfig.enabled = enabled; reset(); });

    connect(&_settingsWidget, &FramedReaderSettings::debugModeChanged,
            [this](bool enabled){ debugModeEnabled = enabled; });

    connect(&_settingsWidget, &FramedReaderSettings::channelMappingChanged,
            this, &FramedReader::onChannelMappingChanged);

    connect(&_settingsWidget, &FramedReaderSettings::checksumConfigChanged,
            this, &FramedReader::onChecksumConfigChanged);
    
    connect(&_settingsWidget, &FramedReaderSettings::totalFrameLengthChanged,
            this, &FramedReader::onTotalFrameLengthChanged);

    reset();
}

FramedReader::~FramedReader()
{
    // Clean up allocated resources
    if (_frameBuffer)
    {
        delete[] _frameBuffer;
        _frameBuffer = nullptr;
    }
    
    if (_kmpMatcher)
    {
        delete _kmpMatcher;
        _kmpMatcher = nullptr;
    }
}

QWidget* FramedReader::settingsWidget()
{
    return &_settingsWidget;
}

unsigned FramedReader::numChannels() const
{
    return _numChannels;
}

void FramedReader::checkSettings()
{
    if (debugModeEnabled)
        qDebug() << "checkSettings: syncWord =" << syncWord.toHex()
                 << "frameSize =" << frameSize;

    // Validate sync word
    if (syncWord.isEmpty())
    {
        _settingsValid = false;
        _lastErrorMessage = "Frame Start is invalid!";
        _settingsWidget.showMessage(_lastErrorMessage, true);
        if (debugModeEnabled)
            qDebug() << "Settings INVALID: Empty sync word";
        return;
    }

    // Validate channel mappings (use total frame size including sync word)
    QString errorMsg;
    unsigned totalFrameSize = syncWord.size() + frameSize;
    if (!_channelMapping.isValid(totalFrameSize, errorMsg))
    {
        _settingsValid = false;
        _lastErrorMessage = errorMsg;
        _settingsWidget.showMessage(_lastErrorMessage, true);
        if (debugModeEnabled)
            qDebug() << "Settings INVALID: Channel mapping error -" << errorMsg;
        return;
    }

    _settingsValid = true;
    _settingsWidget.showMessage("Settings are valid.");
    if (debugModeEnabled)
        qDebug() << "Settings are VALID";
}

void FramedReader::onNumOfChannelsChanged(unsigned value)
{
    _numChannels = value;
    _channelMapping.setNumChannels(value);
    checkSettings();
    reset();
    updateNumChannels();
    emit numOfChannelsChanged(value);
}

void FramedReader::onSyncWordChanged(QByteArray word)
{
    syncWord = word;
    // Update KMP matcher with new pattern
    if (_kmpMatcher)
    {
        _kmpMatcher->setPattern(word);
    }
    // Recalculate frameSize since sync word length may have changed
    recalculateFrameSize();
    checkSettings();
    reset();
}

void FramedReader::onChannelMappingChanged()
{
    _channelMapping = _settingsWidget.channelMapping();
    // Recalculate frameSize in case it was affected by other changes
    recalculateFrameSize();
    checkSettings();
    reset();
}

void FramedReader::onChecksumConfigChanged()
{
    _checksumConfig = _settingsWidget.checksumConfig();
    // Recalculate frameSize since checkCode size may have changed
    recalculateFrameSize();
    checkSettings();
    reset();
}

void FramedReader::onTotalFrameLengthChanged()
{
    // Recalculate frameSize when total frame length changes
    recalculateFrameSize();
    checkSettings();
    reset();
}

void FramedReader::recalculateFrameSize()
{
    // Calculate frame size: Total Frame Length - sync word - checkCode
    unsigned totalLength = _settingsWidget.totalFrameLength();
    unsigned frameStartLength = syncWord.size();
    unsigned checksumLength = _checksumConfig.enabled ? ChecksumCalculator::getOutputSize(_checksumConfig.algorithm) : 0;
    int calculatedFrameSize = totalLength - frameStartLength - checksumLength;
    frameSize = calculatedFrameSize > 0 ? calculatedFrameSize : 1;
}

void FramedReader::reset()
{
    if (debugModeEnabled)
        qDebug() << "reset() called: resetting sync state";
    sync_i = 0;
    gotSync = false;
    gotSize = false;
    if (hasSizeByte) frameSize = 0;
}

double FramedReader::extractChannelValue(const ChannelMapping& ch, const uint8_t* buffer)
{
    // Buffer now contains complete frame (sync word + payload)
    unsigned totalFrameSize = syncWord.size() + frameSize;
    if (ch.byteOffset + ch.byteLength > totalFrameSize)
        return 0.0;

    const uint8_t* data = buffer + ch.byteOffset;
    double value = 0.0;

    switch (ch.numberFormat)
    {
        case NumberFormat_uint8:
            value = double(*((quint8*)data));
            break;
        case NumberFormat_int8:
            value = double(*((qint8*)data));
            break;
        case NumberFormat_uint16:
        {
            quint16 v;
            std::memcpy(&v, data, 2);
            if (ch.endianness == LittleEndian)
                v = qFromLittleEndian(v);
            else
                v = qFromBigEndian(v);
            value = double(v);
            break;
        }
        case NumberFormat_int16:
        {
            qint16 v;
            std::memcpy(&v, data, 2);
            if (ch.endianness == LittleEndian)
                v = qFromLittleEndian(v);
            else
                v = qFromBigEndian(v);
            value = double(v);
            break;
        }
        case NumberFormat_uint24:
        {
            quint32 v = 0;
            if (ch.endianness == LittleEndian)
            {
                v = data[0] | (data[1] << 8) | (data[2] << 16);
            }
            else
            {
                v = (data[0] << 16) | (data[1] << 8) | data[2];
            }
            value = double(v);
            break;
        }
        case NumberFormat_int24:
        {
            qint32 v = 0;
            if (ch.endianness == LittleEndian)
            {
                v = data[0] | (data[1] << 8) | (data[2] << 16);
            }
            else
            {
                v = (data[0] << 16) | (data[1] << 8) | data[2];
            }
            // sign extend
            if ((v & 0x800000) == 0x800000)
                v = v | 0xFF000000;
            else
                v = v & 0x00FFFFFF;
            value = double(v);
            break;
        }
        case NumberFormat_uint32:
        {
            quint32 v;
            std::memcpy(&v, data, 4);
            if (ch.endianness == LittleEndian)
                v = qFromLittleEndian(v);
            else
                v = qFromBigEndian(v);
            value = double(v);
            break;
        }
        case NumberFormat_int32:
        {
            qint32 v;
            std::memcpy(&v, data, 4);
            if (ch.endianness == LittleEndian)
                v = qFromLittleEndian(v);
            else
                v = qFromBigEndian(v);
            value = double(v);
            break;
        }
        case NumberFormat_float:
        {
            float v;
            std::memcpy(&v, data, 4);
            if (ch.endianness == LittleEndian)
                v = qFromLittleEndian(v);
            else
                v = qFromBigEndian(v);
            value = double(v);
            break;
        }
        case NumberFormat_double:
        {
            double v;
            std::memcpy(&v, data, 8);
            if (ch.endianness == LittleEndian)
                v = qFromLittleEndian(v);
            else
                v = qFromBigEndian(v);
            value = v;
            break;
        }
        default:
            value = 0.0;
    }

    return value;
}

uint32_t FramedReader::calculateFrameChecksum(const uint8_t* data, unsigned dataLength)
{
    if (!_checksumConfig.enabled)
        return 0;

    // Build complete frame data (sync word + payload) for checksum calculation
    QByteArray completeFrame = syncWord;
    completeFrame.append((const char*)data, dataLength);
    
    // Now use 0-based indexing on the complete frame
    unsigned totalFrameLength = completeFrame.size();
    unsigned startByte = _checksumConfig.startByte;
    unsigned endByte = _checksumConfig.endByte;

    // Clamp byte range to actual complete frame data
    if (startByte >= totalFrameLength)
        startByte = 0;
    if (endByte >= totalFrameLength)
        endByte = totalFrameLength - 1;

    unsigned length = (endByte >= startByte) ? (endByte - startByte + 1) : 0;

    if (length == 0)
        return 0;

    return ChecksumCalculator::calculate(_checksumConfig.algorithm, 
                                       (const uint8_t*)completeFrame.data() + startByte, 
                                       length);
}

void FramedReader::readFrameDataAndExtractChannels()
{
    if (paused)
    {
        _device->read(_frameBufferSize);
        return;
    }

    // Read complete frame data including sync word
    // First, put sync word at the beginning of buffer
    std::memcpy(_frameBuffer, syncWord.data(), syncWord.size());
    
    // Then read payload data after sync word
    if (debugModeEnabled)
        qDebug() << "Reading frame data: frameSize=" << frameSize 
                 << "totalFrameLength=" << _settingsWidget.totalFrameLength()
                 << "syncWordSize=" << syncWord.size();
    _device->read((char*)_frameBuffer + syncWord.size(), frameSize);

    // Verify checksum if enabled
    if (_checksumConfig.enabled)
    {
        // Read checksum from device
        unsigned checksumSize = ChecksumCalculator::getOutputSize(_checksumConfig.algorithm);
        uint8_t receivedChecksum[4] = {0};
        _device->read((char*)receivedChecksum, checksumSize);

        // Calculate expected checksum (only for payload data, skip sync word)
        uint32_t expectedChecksum = calculateFrameChecksum(_frameBuffer + syncWord.size(), frameSize);

        // Compare (handle different sizes and endianness)
        bool checksumOk = true;
        for (unsigned i = 0; i < checksumSize; i++)
        {
            uint8_t expected;
            if (_checksumConfig.isLittleEndian)
            {
                // Little endian: LSB first
                expected = (expectedChecksum >> (i * 8)) & 0xFF;
            }
            else
            {
                // Big endian: MSB first
                expected = (expectedChecksum >> ((checksumSize - 1 - i) * 8)) & 0xFF;
            }
            
            if (receivedChecksum[i] != expected)
            {
                checksumOk = false;
                break;
            }
        }

        if (!checksumOk)
        {
            // Format timestamp as YYYY:MM:DD HH:MM:SS
            QString timestamp = QDateTime::currentDateTime().toString("yyyy:MM:dd HH:mm:ss");
            
            // Format received checksum value
            QString receivedStr;
            for (unsigned i = 0; i < checksumSize; i++)
            {
                if (i > 0) receivedStr += " ";
                receivedStr += QString("0x%1").arg(receivedChecksum[i], 2, 16, QChar('0')).toUpper();
            }
            
            // Format calculated checksum value (respecting configured endianness)
            QString calculatedStr;
            for (unsigned i = 0; i < checksumSize; i++)
            {
                if (i > 0) calculatedStr += " ";
                uint8_t calculatedByte;
                if (_checksumConfig.isLittleEndian)
                {
                    // Little endian: LSB first
                    calculatedByte = (expectedChecksum >> (i * 8)) & 0xFF;
                }
                else
                {
                    // Big endian: MSB first
                    calculatedByte = (expectedChecksum >> ((checksumSize - 1 - i) * 8)) & 0xFF;
                }
                calculatedStr += QString("0x%1").arg(calculatedByte, 2, 16, QChar('0')).toUpper();
            }
            
            if (debugModeEnabled)
            {
                qCritical() << timestamp << "CheckCode failed! Received:" << receivedStr 
                           << "Calculated:" << calculatedStr;
                
                // Provide additional diagnostic information
                QString endianStr = _checksumConfig.isLittleEndian ? "Little Endian" : "Big Endian";
                qCritical() << "Frame size:" << frameSize << "bytes, CheckCode algorithm:" 
                           << ChecksumCalculator::algorithmToString(_checksumConfig.algorithm)
                           << "Byte order:" << endianStr;
            }
            return;
        }
    }

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

    feedOut(samples);
}

unsigned FramedReader::readData()
{
    unsigned numBytesRead = 0;
    
    if (debugModeEnabled && _device->bytesAvailable() > 0)
        qDebug() << "readData: bytes available =" << _device->bytesAvailable() 
                 << "gotSync =" << gotSync << "frameSize =" << frameSize;

    if (!_settingsValid)
    {
        if (debugModeEnabled)
            qDebug() << "readData: Settings invalid, skipping data read";
        return numBytesRead;
    }

    // PERFORMANCE OPTIMIZATION: Batch read all available data instead of byte-by-byte
    // This reduces system calls from ~92,000/sec to ~100/sec at 921600 bps
    qint64 available = _device->bytesAvailable();
    if (available <= 0)
        return numBytesRead;
    
    // Read all available data at once
    QByteArray newData = _device->readAll();
    if (newData.isEmpty())
        return numBytesRead;
    
    numBytesRead = newData.size();
    _readBuffer.append(newData);
    
    if (debugModeEnabled)
        qDebug() << "Batch read" << numBytesRead << "bytes, buffer size now" << _readBuffer.size();
    
    // Process frames from buffer
    int searchPos = 0;
    unsigned checksumSize = _checksumConfig.enabled ? 
        ChecksumCalculator::getOutputSize(_checksumConfig.algorithm) : 0;
    unsigned totalFrameSize = syncWord.size() + frameSize + checksumSize;
    
    while (_readBuffer.size() >= (int)totalFrameSize)
    {
        // PERFORMANCE OPTIMIZATION: Use KMP algorithm for O(n+m) sync word search
        // instead of O(n*m) byte-by-byte comparison
        int syncPos = _kmpMatcher->search(_readBuffer, searchPos);
        
        if (syncPos == -1)
        {
            // No sync word found - keep last (syncWord.size() - 1) bytes for partial match
            int keepSize = syncWord.size() - 1;
            if (_readBuffer.size() > keepSize)
            {
                _readBuffer = _readBuffer.right(keepSize);
            }
            break;
        }
        
        // Check if we have a complete frame
        int frameStart = syncPos;
        int frameEnd = frameStart + totalFrameSize;
        
        if (frameEnd > _readBuffer.size())
        {
            // Incomplete frame - wait for more data
            break;
        }
        
        // Extract complete frame
        if (paused)
        {
            // Skip this frame
            _readBuffer.remove(0, frameEnd);
            searchPos = 0;
            continue;
        }
        
        // Copy frame data to buffer (including sync word)
        memcpy(_frameBuffer, _readBuffer.constData() + frameStart, syncWord.size() + frameSize);
        
        // Verify checksum if enabled
        bool checksumValid = true;
        if (_checksumConfig.enabled)
        {
            // Extract received checksum
            const uint8_t* receivedChecksum = 
                (const uint8_t*)_readBuffer.constData() + frameStart + syncWord.size() + frameSize;
            
            // Calculate expected checksum (only for payload data, skip sync word)
            uint32_t expectedChecksum = calculateFrameChecksum(_frameBuffer + syncWord.size(), frameSize);
            
            // Compare (handle different sizes and endianness)
            for (unsigned i = 0; i < checksumSize; i++)
            {
                uint8_t expected;
                if (_checksumConfig.isLittleEndian)
                {
                    // Little endian: LSB first
                    expected = (expectedChecksum >> (i * 8)) & 0xFF;
                }
                else
                {
                    // Big endian: MSB first
                    expected = (expectedChecksum >> ((checksumSize - 1 - i) * 8)) & 0xFF;
                }
                
                if (receivedChecksum[i] != expected)
                {
                    checksumValid = false;
                    break;
                }
            }
            
            if (!checksumValid && debugModeEnabled)
            {
                QString timestamp = QDateTime::currentDateTime().toString("yyyy:MM:dd HH:mm:ss");
                qCritical() << "[" << timestamp << "] Checksum mismatch at position" << syncPos;
            }
        }
        
        // Extract and feed channels if checksum is valid
        if (checksumValid)
        {
            SamplePack samples(_numChannels > 0 ? 1 : 0, _numChannels);
            
            for (unsigned i = 0; i < _numChannels; i++)
            {
                const ChannelMapping& ch = _channelMapping.channel(i);
                if (ch.enabled)
                {
                    samples.data(i)[0] = extractChannelValue(ch, _frameBuffer);
                }
            }
            
            feedOut(samples);
        }
        
        // Remove processed frame from buffer
        _readBuffer.remove(0, frameEnd);
        searchPos = 0;  // Start searching from beginning of remaining buffer
    }

    return numBytesRead;
}

void FramedReader::saveSettings(QSettings* settings)
{
    _settingsWidget.saveSettings(settings);
}

void FramedReader::loadSettings(QSettings* settings)
{
    _settingsWidget.loadSettings(settings);
    
    // Get channel mapping and checksum config first
    _channelMapping = _settingsWidget.channelMapping();
    _checksumConfig = _settingsWidget.checksumConfig();
    
    // Reload from settings widget
    _numChannels = _settingsWidget.numOfChannels();
    hasSizeByte = false;  // Size field removed - frame format is now fixed
    isSizeField2B = false;
    syncWord = _settingsWidget.syncWord();
    debugModeEnabled = _settingsWidget.isDebugModeEnabled();
    
    // Calculate frame size: Total Frame Length - sync word - checksum
    unsigned totalLength = _settingsWidget.totalFrameLength();
    unsigned frameStartLength = syncWord.size();
    unsigned checksumLength = _checksumConfig.enabled ? ChecksumCalculator::getOutputSize(_checksumConfig.algorithm) : 0;
    int calculatedFrameSize = totalLength - frameStartLength - checksumLength;
    frameSize = calculatedFrameSize > 0 ? calculatedFrameSize : 1;
    // Endianness is now per-channel
    
    checkSettings();
}
