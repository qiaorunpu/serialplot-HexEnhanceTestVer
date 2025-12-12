/*
  Copyright © 2020 Hasan Yavuz Özderya

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

#ifndef RECORDPANEL_H
#define RECORDPANEL_H

#include <QWidget>
#include <QString>
#include <QToolBar>
#include <QAction>
#include <QTimer>
#include <QElapsedTimer>

#include "datarecorder.h"
#include "rawdatarecorder.h"
#include "stream.h"

namespace Ui {
class RecordPanel;
}

class RecordPanel : public QWidget
{
    Q_OBJECT

public:
    explicit RecordPanel(Stream* stream, QWidget* parent = 0);
    ~RecordPanel();

    QToolBar* toolbar();

    bool recordPaused();

    /// Stores settings into a `QSettings`
    void saveSettings(QSettings* settings);
    /// Loads settings from a `QSettings`.
    void loadSettings(QSettings* settings);

signals:
    void recordStarted();
    void recordStopped();
    void recordPausedChanged(bool enabled);
    void rawRecordingStarted(RawDataRecorder* recorder);
    void rawRecordingStopped(RawDataRecorder* recorder);

public slots:
    /// Must be called when port is closed
    void onPortClose();

    /// Start dual recording (raw + CSV)
    void startRecording();
    
    /// Stop all recording
    void stopRecording();

private:
    Ui::RecordPanel *ui;
    QToolBar recordToolBar;
    QAction recordAction;
    bool overwriteSelected;
    DataRecorder recorder;
    RawDataRecorder rawRecorder;
    Stream* _stream;
    
    // Timer functionality
    QTimer recordingTimer;
    QTimer progressTimer;
    QElapsedTimer elapsedTimer;
    int timerDuration; // in seconds, 0 = continuous
    bool isRecording;
    bool csvRecordingActive;

    /**
     * @brief Increments the file name.
     *
     * If file name doesn't have a number at the end of it, a number is appended
     * with underscore starting from 1.
     *
     * @return false if user cancels
     */
    bool incrementFileName(void);

    /**
     * @brief Used to ask user confirmation if auto generated file
     * name exists.
     *
     * If user confirms overwrite, `selectedFile` is set to
     * `fileName`. User is also given option to select file and is
     * shown a file select dialog in this case.
     *
     * @param fileName auto generated file name.
     * @return false if user cancels
     */
    bool confirmOverwrite(QString fileName);

    /// Returns filename in edit box. May be invalid!
    QString selectedFile() const;
    /// Sets the filename in edit box.
    void setSelectedFile(QString f);

    /**
     * Tries to get a valid file name by handling user interactions and
     * automatic naming (increment, timestamp etc).
     *
     * Returned file name can be used immediately. File name box should also be
     * set to selected file name.
     *
     * @return empty if failure otherwise valid filename
     */
    QString getSelectedFile();

    /// Formats timestamp in given text
    QString formatTimeStamp(QString t) const;

    bool startRecording(QString fileName);

    /// Returns separator text from ui. "\t" is converted to TAB
    /// character.
    QString getSeparator() const;

    DataRecorder::TimestampOption currentTimestampOption() const;

    /// Process filename with auto-increment and timestamp if needed
    QString processFileName(const QString& fileName, bool autoIncrement);

private slots:
    /**
     * @brief Opens up the file select dialog for raw data
     *
     * @return true if file selected, false if user cancels
     */
    bool selectRawFile();

    /**
     * @brief Opens up the file select dialog for CSV data
     *
     * @return true if file selected, false if user cancels
     */
    bool selectCsvFile();

    void onRecord(bool start);
    
    /// Timer timeout handler
    void onTimerTimeout();
    
    /// Timer duration changed
    void onTimerDurationChanged(int seconds);
    
    /// Update progress bar
    void updateProgress();

};

#endif // RECORDPANEL_H
