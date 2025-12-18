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

#include <QIcon>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>
#include <QDir>
#include <QRegularExpression>
#include <QCompleter>
#include <QFileSystemModel>
#include <QFileSystemModel>
#include <QTimer>
#include <QElapsedTimer>
#include <QtDebug>
#include <ctime>

#include "recordpanel.h"
#include "ui_recordpanel.h"
#include "setting_defines.h"
#include "rawdatarecorder.h"

RecordPanel::RecordPanel(Stream* stream, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RecordPanel),
    recorder(this),
    rawRecorder(this),
    recordingTimer(this)
{
    overwriteSelected = false;
    _stream = stream;
    isRecording = false;
    timerDuration = 0;
    csvRecordingActive = false;

    ui->setupUi(this);

    // Connect new UI controls
    connect(ui->pbStartOverwrite, &QPushButton::clicked,
            this, [this]() { startRecording(); });
    connect(ui->pbStopCapture, &QPushButton::clicked,
            this, [this]() { stopRecording(); });

    // Connect timer
    recordingTimer.setSingleShot(true);
    connect(&recordingTimer, &QTimer::timeout,
            this, &RecordPanel::onTimerTimeout);

    // Timer duration change
    connect(ui->spTimerSeconds, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RecordPanel::onTimerDurationChanged);

    // Setup progress timer
    progressTimer.setInterval(1000); // Update every second
    connect(&progressTimer, &QTimer::timeout, this, &RecordPanel::updateProgress);

    connect(ui->pbBrowseRaw, &QPushButton::clicked,
            this, &RecordPanel::selectRawFile);
    connect(ui->pbBrowseCsv, &QPushButton::clicked,
            this, &RecordPanel::selectCsvFile);

    connect(ui->cbRecordWhilePaused, SIGNAL(toggled(bool)),
            this, SIGNAL(recordPausedChanged(bool)));

    connect(ui->cbCsvDisableBuffering, &QCheckBox::toggled,
            [this](bool enabled)
            {
                recorder.disableBuffering = enabled;
            });
    
    connect(ui->cbRawDisableBuffering, &QCheckBox::toggled,
            [this](bool enabled)
            {
                rawRecorder.disableBuffering = enabled;
            });

    connect(ui->cbWindowsLineEnding, &QCheckBox::toggled,
            [this](bool enabled)
            {
                recorder.windowsLE = enabled;
            });

    connect(ui->spDecimals, &QSpinBox::valueChanged,
            [this](int decimals)
            {
                recorder.setDecimals(decimals);
            });


    // Setup completers for file paths
    QCompleter *rawCompleter = new QCompleter(this);
    auto rawFileSystemModel = new QFileSystemModel(rawCompleter);
    rawFileSystemModel->setRootPath(QDir::currentPath());
    rawCompleter->setModel(rawFileSystemModel);
    rawCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->leRawFilename->setCompleter(rawCompleter);

    QCompleter *csvCompleter = new QCompleter(this);
    auto csvFileSystemModel = new QFileSystemModel(csvCompleter);
    csvFileSystemModel->setRootPath(QDir::currentPath());
    csvCompleter->setModel(csvFileSystemModel);
    csvCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->leCsvFilename->setCompleter(csvCompleter);

    // setup timestamp selection
    ui->cbTimestampFormat->addItem(tr("seconds only"),
                                (int) DataRecorder::TimestampOption::seconds);
    ui->cbTimestampFormat->addItem(tr("seconds with precision"),
                                (int) DataRecorder::TimestampOption::seconds_precision);
    ui->cbTimestampFormat->addItem(tr("milliseconds"),
                                (int) DataRecorder::TimestampOption::milliseconds);
}

RecordPanel::~RecordPanel()
{
    delete ui;
}

bool RecordPanel::recordPaused()
{
    return ui->cbRecordWhilePaused->isChecked();
}

bool RecordPanel::selectRawFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        parentWidget(), tr("Select raw data file"), "", tr("Binary files (*.bin);;All files (*)"));

    if (fileName.isEmpty())
    {
        return false;
    }
    else
    {
        ui->leRawFilename->setText(fileName);
        return true;
    }
}

bool RecordPanel::selectCsvFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        parentWidget(), tr("Select CSV file"), "", tr("CSV files (*.csv);;All files (*)"));

    if (fileName.isEmpty())
    {
        return false;
    }
    else
    {
        ui->leCsvFilename->setText(fileName);
        return true;
    }
}

QString RecordPanel::selectedFile() const
{
    return ui->leCsvFilename->text();
}

void RecordPanel::setSelectedFile(QString f)
{
    ui->leCsvFilename->setText(f);
}

QString RecordPanel::getSelectedFile()
{
    if (selectedFile().isEmpty())
    {
        if (!selectCsvFile()) return QString();
    }

    // assume that file name contains a time format specifier
    if (selectedFile().contains("%"))
    {
        auto ts = formatTimeStamp(selectedFile());
        if (!QFile::exists(ts) || // file doesn't exists
            confirmOverwrite(ts)) // exists but user accepted overwrite
        {
            return ts;
        }
        return QString();
    }

    // if no timestamp and file exists try autoincrement option
    if (!overwriteSelected && QFile::exists(selectedFile()))
    {
        if (ui->cbCsvAutoInc->isChecked())
        {
            if (!incrementFileName()) return QString();
        }
        else
        {
            if (!confirmOverwrite(selectedFile()))
                return QString();
        }
    }

    return selectedFile();
}

QString RecordPanel::formatTimeStamp(QString t) const
{
    auto maxSize = t.size() + 1024;
    auto r = new char[maxSize];

    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(r, maxSize, t.toLatin1().data(), timeinfo);

    auto rs = QString(r);
    delete r;
    return rs;
}

void RecordPanel::onRecord(bool start)
{
    if (!start)
    {
        stopRecording();
        return;
    }

    bool canceled = false;
    if (ui->leSeparator->text().isEmpty())
    {
        QMessageBox::critical(this, "Error",
                              "Column separator cannot be empty! Please select a separator.");
        ui->leSeparator->setFocus(Qt::OtherFocusReason);
        canceled = true;
    }

    // check file name
    QString fn;
    if (!canceled)
    {
        fn = getSelectedFile();
        canceled = fn.isEmpty();
    }

    if (!canceled)
    {
        overwriteSelected = false;
        // TODO: show more visible error message when recording fails
        startRecording(fn);
    }
}

bool RecordPanel::incrementFileName(void)
{
    QFileInfo fileInfo(selectedFile());

    QString base = fileInfo.completeBaseName();
    QRegularExpression regex("(.*?)(\\d+)(?!.*\\d)(.*)");
    auto match = regex.match(base);

    if (match.hasMatch())
    {
        bool ok;
        int fileNum = match.captured(2).toInt(&ok);
        base = match.captured(1) + QString::number(fileNum + 1) + match.captured(3);
    }
    else
    {
        base += "_1";
    }

    QString suffix = fileInfo.suffix();;
    if (!suffix.isEmpty())
    {
        suffix = "." + suffix;
    }

    QString autoFileName = fileInfo.path() + "/" + base + suffix;

    // check if auto generated file name exists, ask user another name
    if (QFile::exists(autoFileName))
    {
        if (!confirmOverwrite(autoFileName))
        {
            return false;
        }
    }
    else
    {
        setSelectedFile(autoFileName);
    }

    return true;
}

bool RecordPanel::confirmOverwrite(QString fileName)
{
    // prepare message box
    QMessageBox mb(parentWidget());
    mb.setWindowTitle(tr("File Already Exists"));
    mb.setIcon(QMessageBox::Warning);
    mb.setText(tr("File (%1) already exists. How to continue?").arg(fileName));

    QAbstractButton* bCancel    = mb.addButton(QMessageBox::Cancel);
    QAbstractButton* bOverwrite = mb.addButton(tr("Overwrite"), QMessageBox::DestructiveRole);
    mb.addButton(tr("Select Another File"), QMessageBox::YesRole);

    mb.setEscapeButton(bCancel);

    // show message box
    mb.exec();

    if (mb.clickedButton() == bCancel)
    {
        return false;
    }
    else if (mb.clickedButton() == bOverwrite)
    {
        setSelectedFile(fileName);
        return true;
    }
    else                    // select button
    {
        return selectCsvFile();
    }
}

bool RecordPanel::startRecording(QString fileName)
{
    QStringList channelNames;

    if (ui->cbWriteHeader->isChecked())
    {
        channelNames = _stream->infoModel()->channelNames();
    }

    if (recorder.startRecording(fileName, getSeparator(), channelNames, currentTimestampOption()))
    {
        _stream->connectFollower(&recorder);
        return true;
    }
    else
    {
        return false;
    }
}



void RecordPanel::startRecording()
{
    if (isRecording)
    {
        return;
    }

    // Check which capture modes are enabled
    bool rawEnabled = ui->groupBoxRawData->isChecked();
    bool csvEnabled = ui->groupBoxParsedData->isChecked();
    
    if (!rawEnabled && !csvEnabled)
    {
        QMessageBox::warning(this, tr("Warning"), 
                           tr("Please enable at least one capture mode (Raw Binary Data or Parsed CSV Data)."));
        return;
    }
    
    // Validate file paths for enabled modes
    QString rawFile = rawEnabled ? ui->leRawFilename->text() : QString();
    QString csvFile = csvEnabled ? ui->leCsvFilename->text() : QString();
    
    if (rawEnabled && rawFile.isEmpty())
    {
        QMessageBox::warning(this, tr("Warning"), 
                           tr("Please specify a file path for Raw Binary Data recording."));
        return;
    }
    
    if (csvEnabled && csvFile.isEmpty())
    {
        QMessageBox::warning(this, tr("Warning"), 
                           tr("Please specify a file path for Parsed CSV Data recording."));
        return;
    }

    bool rawStarted = false;
    bool csvStarted = false;

    // Start raw data recording if enabled and file specified
    if (rawEnabled && !rawFile.isEmpty())
    {
        QString finalRawFile = processFileName(rawFile, ui->cbRawAutoInc->isChecked());
        if (!finalRawFile.isEmpty())
        {
            // Update UI with the actual filename being used
            if (finalRawFile != rawFile)
            {
                ui->leRawFilename->setText(finalRawFile);
            }
            
            rawStarted = rawRecorder.startRecording(finalRawFile);
            if (rawStarted)
            {
                // Raw data connection will be made externally through MainWindow
                emit rawRecordingStarted(&rawRecorder);
            }
        }
    }

    // Start CSV recording if enabled and file specified
    if (csvEnabled && !csvFile.isEmpty())
    {
        QString finalCsvFile = processFileName(csvFile, ui->cbCsvAutoInc->isChecked());
        if (!finalCsvFile.isEmpty())
        {
            // Update UI with the actual filename being used
            if (finalCsvFile != csvFile)
            {
                ui->leCsvFilename->setText(finalCsvFile);
            }
            
            QStringList channelNames;
            if (ui->cbWriteHeader->isChecked())
            {
                channelNames = _stream->infoModel()->channelNames();
            }

            csvStarted = recorder.startRecording(finalCsvFile, getSeparator(), channelNames, currentTimestampOption());
            if (csvStarted)
            {
                _stream->connectFollower(&recorder);
                csvRecordingActive = true;
            }
        }
    }

    if ((rawEnabled && !rawStarted) || (csvEnabled && !csvStarted))
    {
        QMessageBox::critical(this, tr("Error"), 
                            tr("Failed to start recording. Please check file permissions and paths."));
        
        // Clean up any partially started recording
        if (rawStarted)
        {
            emit rawRecordingStopped(&rawRecorder);
            rawRecorder.stopRecording();
        }
        if (csvStarted)
        {
            _stream->disconnectFollower(&recorder);
            recorder.stopRecording();
            csvRecordingActive = false;
        }
        return;
    }

    // Update UI
    isRecording = true;
    ui->pbStartOverwrite->setEnabled(false);
    ui->pbStopCapture->setEnabled(true);

    // Setup timer and progress bar
    timerDuration = ui->spTimerSeconds->value();
    if (timerDuration > 0)
    {
        ui->progressBar->setRange(0, timerDuration);
        ui->progressBar->setValue(0);
        ui->progressBar->setFormat(QString("%v / %m seconds"));
        
        recordingTimer.start(timerDuration * 1000);
        progressTimer.start();
        elapsedTimer.start();
    }
    else
    {
        ui->progressBar->setRange(0, 100);
        ui->progressBar->setValue(0);
        ui->progressBar->setFormat(QString("%v seconds (continuous)"));
        
        progressTimer.start();
        elapsedTimer.start();
    }

    emit recordStarted();
}

void RecordPanel::stopRecording()
{
    if (!isRecording)
    {
        return;
    }

    // Stop timers
    recordingTimer.stop();
    progressTimer.stop();

    // Stop raw recording if it was started
    if (rawRecorder.isRecording())
    {
        emit rawRecordingStopped(&rawRecorder);
        rawRecorder.stopRecording();
    }

    // Stop CSV recording if it was started
    if (csvRecordingActive)
    {
        _stream->disconnectFollower(&recorder);
        csvRecordingActive = false;
    }
    recorder.stopRecording();

    // Update UI
    isRecording = false;
    ui->pbStartOverwrite->setEnabled(true);
    ui->pbStopCapture->setEnabled(false);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat(QString("0 seconds"));

    emit recordStopped();
}

void RecordPanel::onTimerTimeout()
{
    stopRecording();
}

void RecordPanel::onTimerDurationChanged(int seconds)
{
    timerDuration = seconds;
    
    if (seconds == 0)
    {
        ui->labelStatus->setText(tr("(0 = continuous)"));
    }
    else
    {
        ui->labelStatus->setText(tr("(%1 seconds)").arg(seconds));
    }
}

void RecordPanel::updateProgress()
{
    if (!isRecording)
    {
        return;
    }

    int elapsed = elapsedTimer.elapsed() / 1000;
    
    if (timerDuration > 0)
    {
        // Timed recording - show progress toward completion
        ui->progressBar->setValue(elapsed);
    }
    else
    {
        // Continuous recording - show elapsed time without limit
        ui->progressBar->setValue(elapsed);
        ui->progressBar->setMaximum(elapsed + 1); // Always stay below 100%
    }
}

void RecordPanel::onPortClose()
{
    if (isRecording && ui->cbStopOnClose->isChecked())
    {
        stopRecording();
    }
}

QString RecordPanel::getSeparator() const
{
    QString sep = ui->leSeparator->text();
    sep.replace("\\t", "\t");
    return sep;
}

DataRecorder::TimestampOption RecordPanel::currentTimestampOption() const
{
    if (ui->cbInsertTimestamp->isChecked())
    {
        return static_cast<DataRecorder::TimestampOption>(ui->cbTimestampFormat->currentData().toInt());
    }
    else
    {
        return DataRecorder::TimestampOption::disabled;
    }
}

QString RecordPanel::processFileName(const QString& fileName, bool autoIncrement)
{
    if (fileName.isEmpty()) return QString();

    QString result = fileName;

    // Handle timestamp formatting
    if (fileName.contains("%"))
    {
        result = formatTimeStamp(fileName);
    }

    // Ensure directory exists
    QFileInfo fileInfo(result);
    QDir dir = fileInfo.dir();
    if (!dir.exists())
    {
        if (!dir.mkpath(dir.absolutePath()))
        {
            QMessageBox::critical(this, tr("Error"), 
                                tr("Failed to create directory: %1").arg(dir.absolutePath()));
            return QString();
        }
    }

    // Handle auto-increment if file exists
    if (autoIncrement && QFile::exists(result))
    {
        QFileInfo originalFileInfo(result);
        QString base = originalFileInfo.completeBaseName();
        QString suffix = originalFileInfo.suffix();
        QString path = originalFileInfo.path();

        // Find existing number at end of filename
        QRegularExpression regex("^(.*)_(\\d+)$");
        auto match = regex.match(base);

        int fileNum = 1;
        QString baseName = base;
        
        if (match.hasMatch())
        {
            // File already has _number format
            bool ok;
            fileNum = match.captured(2).toInt(&ok) + 1;
            if (!ok) fileNum = 1;
            baseName = match.captured(1);
        }
        else
        {
            // File doesn't have _number format, use original name as base
            baseName = base;
            fileNum = 1;
        }

        // Generate unique filename
        do {
            result = path + "/" + baseName + "_" + QString::number(fileNum);
            if (!suffix.isEmpty())
            {
                result += "." + suffix;
            }
            fileNum++;
        } while (QFile::exists(result));
    }
    else if (QFile::exists(result))
    {
        // Ask for overwrite confirmation
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("File Exists"), 
            tr("File '%1' already exists. Do you want to overwrite it?").arg(result),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply != QMessageBox::Yes)
        {
            return QString(); // User declined to overwrite
        }
    }

    return result;
}

void RecordPanel::saveSettings(QSettings* settings)
{
    settings->beginGroup(SettingGroup_Record);
    
    // Timer settings
    settings->setValue("timerSeconds", ui->spTimerSeconds->value());
    
    // Raw file settings  
    settings->setValue("rawEnabled", ui->groupBoxRawData->isChecked());
    settings->setValue("rawAutoIncrement", ui->cbRawAutoInc->isChecked());
    settings->setValue("rawDisableBuffering", ui->cbRawDisableBuffering->isChecked());
    settings->setValue("rawFilename", ui->leRawFilename->text());
    
    // CSV file settings
    settings->setValue("csvEnabled", ui->groupBoxParsedData->isChecked());
    settings->setValue(SG_Record_AutoIncrement, ui->cbCsvAutoInc->isChecked());
    settings->setValue(SG_Record_RecordPaused, ui->cbRecordWhilePaused->isChecked());
    settings->setValue(SG_Record_StopOnClose, ui->cbStopOnClose->isChecked());
    settings->setValue(SG_Record_Header, ui->cbWriteHeader->isChecked());
    settings->setValue(SG_Record_DisableBuffering, ui->cbCsvDisableBuffering->isChecked());
    settings->setValue(SG_Record_Separator, ui->leSeparator->text());
    settings->setValue(SG_Record_Decimals, ui->spDecimals->value());
    settings->setValue(SG_Record_Timestamp, ui->cbInsertTimestamp->isChecked());
    settings->setValue("csvFilename", ui->leCsvFilename->text());
    settings->setValue("windowsLineEnding", ui->cbWindowsLineEnding->isChecked());

    QString tsFormatStr;
    auto tsOpt = static_cast<DataRecorder::TimestampOption>(ui->cbTimestampFormat->currentData().toInt());
    switch (tsOpt)
    {
        case DataRecorder::TimestampOption::seconds:
            tsFormatStr = "seconds";
            break;
        case DataRecorder::TimestampOption::seconds_precision:
            tsFormatStr = "seconds_with_precision";
            break;
        case DataRecorder::TimestampOption::milliseconds:
            tsFormatStr = "milliseconds";
            break;
        default:
            Q_ASSERT(false);
    }
    settings->setValue(SG_Record_TimestampFormat, tsFormatStr);

    settings->endGroup();
}

void RecordPanel::loadSettings(QSettings* settings)
{
    settings->beginGroup(SettingGroup_Record);
    
    // Timer settings
    ui->spTimerSeconds->setValue(settings->value("timerSeconds", 0).toInt());
    
    // Raw file settings
    ui->groupBoxRawData->setChecked(settings->value("rawEnabled", true).toBool());
    ui->cbRawAutoInc->setChecked(settings->value("rawAutoIncrement", false).toBool());
    ui->cbRawDisableBuffering->setChecked(settings->value("rawDisableBuffering", false).toBool());
    ui->leRawFilename->setText(settings->value("rawFilename", "").toString());
    
    // CSV file settings
    ui->groupBoxParsedData->setChecked(settings->value("csvEnabled", true).toBool());
    ui->cbCsvAutoInc->setChecked(
        settings->value(SG_Record_AutoIncrement, ui->cbCsvAutoInc->isChecked()).toBool());
    ui->cbRecordWhilePaused->setChecked(
        settings->value(SG_Record_RecordPaused, ui->cbRecordWhilePaused->isChecked()).toBool());
    ui->cbStopOnClose->setChecked(
        settings->value(SG_Record_StopOnClose, ui->cbStopOnClose->isChecked()).toBool());
    ui->cbWriteHeader->setChecked(
        settings->value(SG_Record_Header, ui->cbWriteHeader->isChecked()).toBool());
    ui->cbCsvDisableBuffering->setChecked(
        settings->value(SG_Record_DisableBuffering, ui->cbCsvDisableBuffering->isChecked()).toBool());
    ui->leSeparator->setText(settings->value(SG_Record_Separator, ui->leSeparator->text()).toString());
    ui->spDecimals->setValue(settings->value(SG_Record_Decimals, ui->spDecimals->value()).toInt());
    ui->cbInsertTimestamp->setChecked(
        settings->value(SG_Record_Timestamp, ui->cbInsertTimestamp->isChecked()).toBool());
    ui->leCsvFilename->setText(settings->value("csvFilename", "").toString());
    ui->cbWindowsLineEnding->setChecked(settings->value("windowsLineEnding", false).toBool());

    // load timestamp format
    QString tsFormatStr = settings->value(SG_Record_TimestampFormat, "").toString();
    // get current timestamp option
    DataRecorder::TimestampOption tsOpt = static_cast<DataRecorder::TimestampOption>(ui->cbTimestampFormat->currentData().toInt());
    if (tsFormatStr == "seconds")
    {
        tsOpt = DataRecorder::TimestampOption::seconds;
    }
    else if (tsFormatStr == "seconds_with_precision")
    {
        tsOpt = DataRecorder::TimestampOption::seconds_precision;
    }
    else if (tsFormatStr == "milliseconds")
    {
        tsOpt = DataRecorder::TimestampOption::milliseconds;
    }
    else if (!tsFormatStr.isEmpty())
    {
        qCritical() << "Invalid timestamp format option:" << tsFormatStr;
    }

    // find index of loaded option from combobox and select it
    int i = ui->cbTimestampFormat->findData((int) tsOpt);
    if (i >= 0)
    {
        ui->cbTimestampFormat->setCurrentIndex(i);
    }

    settings->endGroup();
}
