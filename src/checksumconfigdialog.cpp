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

#include "checksumconfigdialog.h"
#include "ui_checksumconfigdialog.h"

ChecksumConfigDialog::ChecksumConfigDialog(ChecksumConfig& config, unsigned maxFrameSize, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ChecksumConfigDialog),
      _config(config),
      _maxFrameSize(maxFrameSize)
{
    ui->setupUi(this);

    // Setup algorithm combo box
    ui->cbAlgorithm->addItem("None", (int)ChecksumAlgorithm::None);
    ui->cbAlgorithm->addItem("CRC-8", (int)ChecksumAlgorithm::CRC8);
    ui->cbAlgorithm->addItem("CRC-16", (int)ChecksumAlgorithm::CRC16);
    ui->cbAlgorithm->addItem("CRC-16-CCITT", (int)ChecksumAlgorithm::CRC16_CCITT);
    ui->cbAlgorithm->addItem("CRC-16-MODBUS", (int)ChecksumAlgorithm::CRC16_MODBUS);
    ui->cbAlgorithm->addItem("CRC-32", (int)ChecksumAlgorithm::CRC32);
    ui->cbAlgorithm->addItem("SUM-8", (int)ChecksumAlgorithm::SUM8);
    ui->cbAlgorithm->addItem("SUM-16", (int)ChecksumAlgorithm::SUM16);
    ui->cbAlgorithm->addItem("SUM-24", (int)ChecksumAlgorithm::SUM24);
    ui->cbAlgorithm->addItem("SUM-32", (int)ChecksumAlgorithm::SUM32);
    ui->cbAlgorithm->addItem("XOR-8", (int)ChecksumAlgorithm::XOR8);

    // Set current algorithm
    int idx = ui->cbAlgorithm->findData((int)_config.algorithm);
    if (idx >= 0) ui->cbAlgorithm->setCurrentIndex(idx);

    // Setup byte range spinboxes (1-based indexing for user display)
    // Range covers the complete frame (sync word + payload)
    // Checksum bytes are transmitted separately after the frame
    ui->spStartByte->setMinimum(1);
    ui->spStartByte->setMaximum(_maxFrameSize);
    ui->spEndByte->setMinimum(1);
    ui->spEndByte->setMaximum(_maxFrameSize);
    ui->spStartByte->setValue(_config.startByte + 1);  // Convert from 0-based to 1-based
    ui->spEndByte->setValue(_config.endByte + 1);      // Convert from 0-based to 1-based
    
    // Setup endianness combo box
    ui->cbEndianness->setCurrentIndex(_config.isLittleEndian ? 0 : 1);

    connect(ui->cbAlgorithm, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChecksumConfigDialog::onAlgorithmChanged);
    
    // Update info when byte range or endianness changes
    connect(ui->spStartByte, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ChecksumConfigDialog::updateAlgorithmInfo);
    connect(ui->spEndByte, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ChecksumConfigDialog::updateAlgorithmInfo);
    connect(ui->cbEndianness, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChecksumConfigDialog::updateAlgorithmInfo);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, [this]() {
        _config.algorithm = (ChecksumAlgorithm)ui->cbAlgorithm->currentData().toInt();
        _config.startByte = ui->spStartByte->value() - 1;  // Convert from 1-based to 0-based
        _config.endByte = ui->spEndByte->value() - 1;      // Convert from 1-based to 0-based
        _config.isLittleEndian = (ui->cbEndianness->currentIndex() == 0);  // 0 = Little Endian, 1 = Big Endian
        // enabled is controlled by main UI checkbox, not here
        accept();
    });

    updateAlgorithmInfo();
}

ChecksumConfigDialog::~ChecksumConfigDialog()
{
    delete ui;
}

void ChecksumConfigDialog::onAlgorithmChanged(int index)
{
    // Update maximum values when algorithm changes
    // The maximum should be based on total frame length (sync word + payload)
    // Checksum bytes are transmitted separately after the frame data
    ChecksumAlgorithm algo = (ChecksumAlgorithm)ui->cbAlgorithm->currentData().toInt();
    
    ui->spStartByte->setMaximum(_maxFrameSize);
    ui->spEndByte->setMaximum(_maxFrameSize);
    
    updateAlgorithmInfo();
}

void ChecksumConfigDialog::updateAlgorithmInfo()
{
    ChecksumAlgorithm algo = (ChecksumAlgorithm)ui->cbAlgorithm->currentData().toInt();
    unsigned size = ChecksumCalculator::getOutputSize(algo);
    
    QString info;
    if (algo == ChecksumAlgorithm::None)
    {
        info = "No checksum";
    }
    else
    {
        // Get current byte range (1-based display)
        unsigned startByte = ui->spStartByte->value();
        unsigned endByte = ui->spEndByte->value();
        unsigned totalBytes = (endByte >= startByte) ? (endByte - startByte + 1) : 0;
        
        QString endianStr = (ui->cbEndianness->currentIndex() == 0) ? "Little Endian" : "Big Endian";
        info = QString("Output size: %1 byte(s). Checksum range: bytes %2 to %3 (%4 bytes total). Byte order: %5")
                .arg(size)
                .arg(startByte)
                .arg(endByte)
                .arg(totalBytes)
                .arg(endianStr);
    }
    ui->lInfo->setText(info);
}
