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

#include "rawdatarecorder.h"
#include <QDebug>

RawDataRecorder::RawDataRecorder(QObject *parent)
    : QObject(parent), disableBuffering(false), recording(false)
{
}

RawDataRecorder::~RawDataRecorder()
{
    if (recording)
    {
        stopRecording();
    }
}

bool RawDataRecorder::startRecording(const QString& fileName)
{
    if (recording)
    {
        qWarning() << "Already recording to" << file.fileName();
        return false;
    }

    file.setFileName(fileName);
    
    if (!file.open(QIODevice::WriteOnly))
    {
        qCritical() << "Failed to open file for raw recording:" << fileName << file.errorString();
        return false;
    }

    if (disableBuffering)
    {
        file.setTextModeEnabled(false);
    }

    recording = true;
    qDebug() << "Started raw data recording to" << fileName;
    return true;
}

void RawDataRecorder::stopRecording()
{
    if (!recording)
    {
        return;
    }

    file.close();
    recording = false;
    qDebug() << "Stopped raw data recording";
}

bool RawDataRecorder::isRecording() const
{
    return recording;
}

void RawDataRecorder::onDataReceived(const QByteArray& data)
{
    if (!recording || !file.isOpen())
    {
        return;
    }

    qint64 written = file.write(data);
    if (written == -1)
    {
        qCritical() << "Failed to write raw data to file:" << file.errorString();
        stopRecording();
    }
    else if (written != data.size())
    {
        qWarning() << "Partial write to raw data file. Expected:" << data.size() << "Actual:" << written;
    }

    if (disableBuffering)
    {
        file.flush();
    }
}