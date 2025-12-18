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

#include "dataformatpanel.h"
#include "ui_dataformatpanel.h"

#include <QRadioButton>
#include <QtDebug>

#include "setting_defines.h"

DataFormatPanel::DataFormatPanel(QSerialPort* port, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataFormatPanel),
    framedReader(port, this),
    demoReader(port, this)
{
    ui->setupUi(this);

    serialPort = port;
    paused = false;
    readerBeforeDemo = nullptr;
    _bytesRead = 0;

    // Initialize with Custom Frame reader (only available option now)
    currentReader = &framedReader;
    framedReader.enable();
    ui->horizontalLayout->addWidget(framedReader.settingsWidget(), 1);
}

DataFormatPanel::~DataFormatPanel()
{
    delete ui;
}

unsigned DataFormatPanel::numChannels() const
{
    return currentReader->numChannels();
}

Source* DataFormatPanel::activeSource()
{
    return currentReader;
}

void DataFormatPanel::pause(bool enabled)
{
    paused = enabled;
    currentReader->pause(enabled);
    demoReader.pause(enabled);
}

void DataFormatPanel::enableDemo(bool demoEnabled)
{
    if (demoEnabled)
    {
        readerBeforeDemo = currentReader;
        demoReader.setNumChannels(readerBeforeDemo->numChannels());
        selectReader(&demoReader);
    }
    else
    {
        Q_ASSERT(readerBeforeDemo != nullptr);
        selectReader(readerBeforeDemo);
    }

    // disable/enable reader selection buttons during/after demo
    // Custom Frame is always enabled (demo uses it too)
}

bool DataFormatPanel::isDemoEnabled() const
{
    return currentReader == &demoReader;
}

void DataFormatPanel::selectReader(AbstractReader* reader)
{
    currentReader->enable(false);
    reader->enable();

    // re-connect signals
    disconnect(currentReader, 0, this, 0);

    // switch the settings widget
    ui->horizontalLayout->removeWidget(currentReader->settingsWidget());
    currentReader->settingsWidget()->hide();
    ui->horizontalLayout->addWidget(reader->settingsWidget(), 1);
    reader->settingsWidget()->show();

    reader->pause(paused);

    currentReader = reader;
    emit sourceChanged(currentReader);
}

uint64_t DataFormatPanel::bytesRead()
{
    _bytesRead += currentReader->getBytesRead();
    return _bytesRead;
}

void DataFormatPanel::saveSettings(QSettings* settings)
{
    settings->beginGroup(SettingGroup_DataFormat);

    // save selected data format (always custom frame now)
    settings->setValue(SG_DataFormat_Format, "custom");

    settings->endGroup();

    // save reader settings
    framedReader.saveSettings(settings);
}

void DataFormatPanel::loadSettings(QSettings* settings)
{
    settings->beginGroup(SettingGroup_DataFormat);

    // Always use custom frame format
    selectReader(&framedReader);

    settings->endGroup();

    // load reader settings
    framedReader.loadSettings(settings);
}
