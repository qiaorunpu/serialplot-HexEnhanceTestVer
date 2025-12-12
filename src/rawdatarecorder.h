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

#ifndef RAWDATARECORDER_H
#define RAWDATARECORDER_H

#include <QObject>
#include <QFile>

/**
 * Records raw binary data directly to file without any formatting.
 * Used for capturing the raw serial stream without timestamps or parsing.
 */
class RawDataRecorder : public QObject
{
    Q_OBJECT
public:
    explicit RawDataRecorder(QObject *parent = 0);
    ~RawDataRecorder();

    /// Disables file buffering
    bool disableBuffering = false;

    /// Start recording raw data to the specified file
    bool startRecording(const QString& fileName);
    
    /// Stop recording and close the file
    void stopRecording();
    
    /// Check if currently recording
    bool isRecording() const;

public slots:
    /// Write raw data to file
    void onDataReceived(const QByteArray& data);

private:
    QFile file;
    bool recording;
};

#endif // RAWDATARECORDER_H