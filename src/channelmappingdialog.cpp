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

#include "channelmappingdialog.h"
#include "ui_channelmappingdialog.h"
#include "numberformat.h"
#include "endiannessbox.h"
#include <QMessageBox>
#include <QComboBox>
#include <QSpinBox>

ChannelMappingDialog::ChannelMappingDialog(ChannelMappingConfig& config, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ChannelMappingDialog),
      _config(config),
      _totalFrameSize(64),
      _updating(false)
{
    ui->setupUi(this);

    // Payload size is now read-only, calculated from main UI

    connect(ui->tableWidget, &QTableWidget::cellChanged,
            this, &ChannelMappingDialog::onTableCellChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &ChannelMappingDialog::onAccepted);

    loadFromConfig();
}

ChannelMappingDialog::~ChannelMappingDialog()
{
    delete ui;
}

void ChannelMappingDialog::loadFromConfig()
{
    _updating = true;

    updateTable();

    _updating = false;
}

void ChannelMappingDialog::updateTable()
{
    ui->tableWidget->setRowCount(_config.numChannels());
    
    for (unsigned i = 0; i < _config.numChannels(); i++)
    {
        const ChannelMapping& ch = _config.channel(i);

        // Channel number (start from 1)
        auto item = new QTableWidgetItem(QString::number(i + 1));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 0, item);

        // Byte Position (1-based display)
        auto offsetSpin = new QSpinBox();
        offsetSpin->setMinimum(1);
        offsetSpin->setMaximum(_totalFrameSize);
        offsetSpin->setValue(ch.byteOffset + 1); // Convert 0-based to 1-based for display
        ui->tableWidget->setCellWidget(i, 1, offsetSpin);

        // Byte Length (Data Length) - read-only, determined by format
        auto lengthSpin = new QSpinBox();
        lengthSpin->setMinimum(1);
        lengthSpin->setMaximum(_totalFrameSize);
        lengthSpin->setValue(numberFormatByteSize(ch.numberFormat));
        lengthSpin->setEnabled(false); // Make read-only
        ui->tableWidget->setCellWidget(i, 2, lengthSpin);

        // Format
        auto formatCombo = new QComboBox();
        formatCombo->addItem("uint8", NumberFormat_uint8);
        formatCombo->addItem("int8", NumberFormat_int8);
        formatCombo->addItem("uint16", NumberFormat_uint16);
        formatCombo->addItem("int16", NumberFormat_int16);
        formatCombo->addItem("uint24", NumberFormat_uint24);
        formatCombo->addItem("int24", NumberFormat_int24);
        formatCombo->addItem("uint32", NumberFormat_uint32);
        formatCombo->addItem("int32", NumberFormat_int32);
        formatCombo->addItem("float", NumberFormat_float);
        formatCombo->addItem("double", NumberFormat_double);
        formatCombo->setCurrentIndex(formatCombo->findData((int)ch.numberFormat));
        connect(formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, i]() { this->updateDataLengthForRow(i); });
        ui->tableWidget->setCellWidget(i, 3, formatCombo);

        // Endianness
        auto endiCombo = new QComboBox();
        endiCombo->addItem("Little Endian", LittleEndian);
        endiCombo->addItem("Big Endian", BigEndian);
        endiCombo->setCurrentIndex(ch.endianness == LittleEndian ? 0 : 1);
        ui->tableWidget->setCellWidget(i, 4, endiCombo);

        // Enabled
        auto enabledCheckBox = new QTableWidgetItem();
        enabledCheckBox->setCheckState(ch.enabled ? Qt::Checked : Qt::Unchecked);
        ui->tableWidget->setItem(i, 5, enabledCheckBox);
    }
}

void ChannelMappingDialog::setTotalFrameSize(unsigned totalFrameSize)
{
    _totalFrameSize = totalFrameSize;
}

void ChannelMappingDialog::onTableCellChanged(int row, int column)
{
    if (_updating) return;

    _updating = true;

    // When byte length changes, automatically update based on data type
    if (column == 2) // Byte Length column
    {
        auto lengthSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, column));
        if (lengthSpin)
        {
            // The length was manually changed - this is fine, just store it
        }
    }

    _updating = false;
}

bool ChannelMappingDialog::validateAndShowError()
{
    QString errorMsg;
    if (!_config.isValid(_totalFrameSize, errorMsg))
    {
        QMessageBox::critical(this, "Validation Error", errorMsg);
        return false;
    }
    return true;
}

void ChannelMappingDialog::onAccepted()
{
    saveToConfig();
    if (validateAndShowError())
    {
        accept();
    }
}

void ChannelMappingDialog::updateDataLengthForRow(int row)
{
    auto formatCombo = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(row, 3));
    auto lengthSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 2));
    
    if (formatCombo && lengthSpin) {
        NumberFormat format = (NumberFormat)formatCombo->currentData().toInt();
        lengthSpin->setValue(numberFormatByteSize(format));
    }
}

void ChannelMappingDialog::saveToConfig()
{
    for (unsigned i = 0; i < _config.numChannels(); i++)
    {
        ChannelMapping& ch = _config.channel(i);

        auto offsetSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(i, 1));
        auto lengthSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(i, 2));
        auto formatCombo = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(i, 3));
        auto endiCombo = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(i, 4));
        auto enabledItem = ui->tableWidget->item(i, 5);

        if (offsetSpin) ch.byteOffset = offsetSpin->value() - 1; // Convert 1-based input to 0-based storage
        if (lengthSpin) ch.byteLength = lengthSpin->value();
        if (formatCombo) {
            ch.numberFormat = (NumberFormat)formatCombo->currentData().toInt();
            ch.byteLength = numberFormatByteSize(ch.numberFormat); // Ensure consistency
        }
        if (endiCombo) ch.endianness = (Endianness)endiCombo->currentData().toInt();
        if (enabledItem) ch.enabled = (enabledItem->checkState() == Qt::Checked);
    }
}
