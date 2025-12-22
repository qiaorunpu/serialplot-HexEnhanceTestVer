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

#include "defines.h"
#include "setting_defines.h"
#include "framedreadersettings.h"
#include "ui_framedreadersettings.h"
#include "channelmappingdialog.h"
#include "checksumconfigdialog.h"
#include "checksumcalculator.h"

FramedReaderSettings::FramedReaderSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FramedReaderSettings),
    fbGroup(this)
{
    ui->setupUi(this);

    ui->leSyncWord->setMode(false); // hex mode
    ui->leSyncWord->setText("AA BB");
    ui->spNumOfChannels->setMaximum(MAX_NUM_CHANNELS);

    // Setup channel mapping
    _channelMapping.setNumChannels(ui->spNumOfChannels->value());

    // Setup checkCode config
    _checksumConfig.enabled = false;
    _checksumConfig.algorithm = ChecksumAlgorithm::None;

    connect(ui->cbChecksum, &QCheckBox::toggled,
            [this](bool enabled)
            {
                _checksumConfig.enabled = enabled;
                emit checksumChanged(enabled);
                updatePayloadSize();  // Update payload size when checkCode is toggled
            });

    connect(ui->cbDebugMode, &QCheckBox::toggled,
            this, &FramedReaderSettings::debugModeChanged);

    connect(ui->pbChannelMapping, &QPushButton::clicked,
            this, &FramedReaderSettings::onChannelMappingClicked);

    connect(ui->pbChecksumConfig, &QPushButton::clicked,
            this, &FramedReaderSettings::onChecksumConfigClicked);

    // Size field is now fixed (no dynamic sizing)

    connect(ui->spNumOfChannels, &QSpinBox::valueChanged,
            [this](int value)
            {
                _channelMapping.setNumChannels(value);
                emit numOfChannelsChanged(value);
            });

    connect(ui->spTotalFrameLength, &QSpinBox::valueChanged,
            this, &FramedReaderSettings::onTotalFrameLengthChanged);

    connect(ui->leSyncWord, &QLineEdit::textChanged,
            this, &FramedReaderSettings::onSyncWordEdited);
    connect(ui->leSyncWord, &QLineEdit::textChanged,
            this, &FramedReaderSettings::updatePayloadSize);

    // Update payload size when checkCode config changes
    connect(this, &FramedReaderSettings::checksumConfigChanged,
            this, &FramedReaderSettings::updatePayloadSize);
}

FramedReaderSettings::~FramedReaderSettings()
{
    delete ui;
}

void FramedReaderSettings::showMessage(QString message, bool error)
{
    ui->lMessage->setText(message);
    if (error)
    {
        ui->lMessage->setStyleSheet("color: red;");
    }
    else
    {
        ui->lMessage->setStyleSheet("");
    }
}

unsigned FramedReaderSettings::numOfChannels()
{
    return ui->spNumOfChannels->value();
}

// NumberFormat method removed - each channel now has its own format

QByteArray FramedReaderSettings::syncWord()
{
    QString text = ui->leSyncWord->text().remove(' ');

    // check if nibble is missing
    if (text.size() % 2 == 1)
    {
        return QByteArray();
    }
    else
    {
        return QByteArray::fromHex(text.toLatin1());
    }
}

void FramedReaderSettings::onSyncWordEdited()
{
    // TODO: emit with a delay so that error message doesn't flash!
    emit syncWordChanged(syncWord());
}

unsigned FramedReaderSettings::fixedFrameSize() const
{
    return ui->spSize->value();
}

unsigned FramedReaderSettings::totalFrameLength() const
{
    return ui->spTotalFrameLength->value();
}

bool FramedReaderSettings::isChecksumEnabled()
{
    return ui->cbChecksum->isChecked();
}

bool FramedReaderSettings::isDebugModeEnabled()
{
    return ui->cbDebugMode->isChecked();
}

ChannelMappingConfig& FramedReaderSettings::channelMapping()
{
    return _channelMapping;
}

ChecksumConfig& FramedReaderSettings::checksumConfig()
{
    return _checksumConfig;
}

void FramedReaderSettings::onChannelMappingClicked()
{
    ChannelMappingDialog dialog(_channelMapping, this);
    // Pass total frame size (sync word + payload) since positions are now absolute
    unsigned totalFrameSize = syncWord().size() + ui->spSize->value();
    dialog.setTotalFrameSize(totalFrameSize);
    if (dialog.exec() == QDialog::Accepted)
    {
        emit channelMappingChanged();
    }
}

void FramedReaderSettings::onChecksumConfigClicked()
{
    ChecksumConfigDialog dialog(_checksumConfig, totalFrameLength(), this);
    if (dialog.exec() == QDialog::Accepted)
    {
        emit checksumConfigChanged();
    }
}

void FramedReaderSettings::saveSettings(QSettings* settings)
{
    settings->beginGroup(SettingGroup_CustomFrame);
    settings->setValue(SG_CustomFrame_NumOfChannels, numOfChannels());
    settings->setValue(SG_CustomFrame_TotalFrameLength, totalFrameLength());
    settings->setValue(SG_CustomFrame_FrameStart, ui->leSyncWord->text());
    settings->setValue(SG_CustomFrame_FixedFrameSize, fixedFrameSize());
    settings->setValue(SG_CustomFrame_Checksum, ui->cbChecksum->isChecked());
    settings->setValue(SG_CustomFrame_DebugMode, ui->cbDebugMode->isChecked());

    // Save checksum configuration - 添加条件判断
    if (_checksumConfig.enabled && _checksumConfig.algorithm != ChecksumAlgorithm::None)
    {
        settings->setValue(SG_CustomFrame_ChecksumAlgorithm, ChecksumCalculator::algorithmToString(_checksumConfig.algorithm));
        settings->setValue(SG_CustomFrame_ChecksumStartByte, _checksumConfig.startByte);
        settings->setValue(SG_CustomFrame_ChecksumEndByte, _checksumConfig.endByte);
        settings->setValue(SG_CustomFrame_ChecksumEndianness, _checksumConfig.isLittleEndian ? "little" : "big");
    }
    else
    {
        // 保存 None 以明确表示未启用
        settings->setValue(SG_CustomFrame_ChecksumAlgorithm, "None");
    }

    // Save channel mapping
    settings->beginGroup(SG_CustomFrame_ChannelMapping);
    for (unsigned i = 0; i < _channelMapping.numChannels(); i++)
    {
        const ChannelMapping& ch = _channelMapping.channel(i);
        QString chKey = QString("%1_%2").arg(SG_CustomFrame_Channel).arg(i);
        settings->beginGroup(chKey);
        settings->setValue(SG_CustomFrame_ChannelByteOffset, ch.byteOffset);
        settings->setValue(SG_CustomFrame_ChannelByteLength, ch.byteLength);
        settings->setValue(SG_CustomFrame_ChannelFormat, numberFormatToStr(ch.numberFormat));
        settings->setValue(SG_CustomFrame_ChannelEndianness, ch.endianness == LittleEndian ? "little" : "big");
        settings->setValue(SG_CustomFrame_ChannelEnabled, ch.enabled);
        settings->endGroup();
    }
    settings->endGroup();

    settings->endGroup();
}

void FramedReaderSettings::loadSettings(QSettings* settings)
{
    settings->beginGroup(SettingGroup_CustomFrame);

    // load number of channels
    ui->spNumOfChannels->setValue(
        settings->value(SG_CustomFrame_NumOfChannels, numOfChannels()).toInt());

    // load total frame length
    ui->spTotalFrameLength->setValue(
        settings->value(SG_CustomFrame_TotalFrameLength, ui->spTotalFrameLength->value()).toInt());

    // load frame start
    QString frameStartSetting =
        settings->value(SG_CustomFrame_FrameStart, ui->leSyncWord->text()).toString();
    auto validator = ui->leSyncWord->validator();
    validator->fixup(frameStartSetting);
    int pos = 0;
    if (validator->validate(frameStartSetting, pos) != QValidator::Invalid)
    {
        ui->leSyncWord->setText(frameStartSetting);
    }

    // load fixed frame size
    ui->spSize->setValue(
        settings->value(SG_CustomFrame_FixedFrameSize, ui->spSize->value()).toInt());

    // load checksum
    ui->cbChecksum->setChecked(
        settings->value(SG_CustomFrame_Checksum, ui->cbChecksum->isChecked()).toBool());

    // load debug mode
    ui->cbDebugMode->setChecked(
        settings->value(SG_CustomFrame_DebugMode, ui->cbDebugMode->isChecked()).toBool());

    // Load checksum configuration - 添加验证
    QString algoStr = settings->value(SG_CustomFrame_ChecksumAlgorithm, "None").toString();
    _checksumConfig.algorithm = ChecksumCalculator::stringToAlgorithm(algoStr);
    
    // 只在算法有效时加载其他配置
    if (_checksumConfig.algorithm != ChecksumAlgorithm::None)
    {
        _checksumConfig.startByte = settings->value(SG_CustomFrame_ChecksumStartByte, 0).toInt();
        _checksumConfig.endByte = settings->value(SG_CustomFrame_ChecksumEndByte, 0).toInt();
        QString endiannessStr = settings->value(SG_CustomFrame_ChecksumEndianness, "little").toString();
        _checksumConfig.isLittleEndian = (endiannessStr == "little");
    }
    else
    {
        // 重置为默认值
        _checksumConfig.startByte = 0;
        _checksumConfig.endByte = 0;
        _checksumConfig.isLittleEndian = true;
    }

    // Load channel mapping
    settings->beginGroup(SG_CustomFrame_ChannelMapping);
    for (unsigned i = 0; i < _channelMapping.numChannels(); i++)
    {
        ChannelMapping& ch = _channelMapping.channel(i);
        QString chKey = QString("%1_%2").arg(SG_CustomFrame_Channel).arg(i);
        if (settings->childGroups().contains(chKey))
        {
            settings->beginGroup(chKey);
            ch.byteOffset = settings->value(SG_CustomFrame_ChannelByteOffset, ch.byteOffset).toInt();
            ch.byteLength = settings->value(SG_CustomFrame_ChannelByteLength, ch.byteLength).toInt();
            ch.numberFormat = strToNumberFormat(settings->value(SG_CustomFrame_ChannelFormat, "uint8").toString());
            QString endiStr = settings->value(SG_CustomFrame_ChannelEndianness, "little").toString();
            ch.endianness = (endiStr == "little") ? LittleEndian : BigEndian;
            ch.enabled = settings->value(SG_CustomFrame_ChannelEnabled, true).toBool();
            settings->endGroup();
        }
    }
    settings->endGroup();

    settings->endGroup();
    updatePayloadSize();
}

void FramedReaderSettings::onTotalFrameLengthChanged()
{
    emit totalFrameLengthChanged(totalFrameLength());
    updatePayloadSize();
}

void FramedReaderSettings::updatePayloadSize()
{
    updatePayloadSizeInternal();
}

void FramedReaderSettings::updatePayloadSizeInternal()
{
    unsigned totalLength = totalFrameLength();
    unsigned frameStartLength = syncWord().size();
    unsigned checksumLength = _checksumConfig.enabled ? ChecksumCalculator::getOutputSize(_checksumConfig.algorithm) : 0;
    
    // No size field length - frame format is now fixed
    int payloadSize = totalLength - frameStartLength - checksumLength;
    
    if (payloadSize < 1) payloadSize = 1;
    
    ui->spSize->setValue(payloadSize);
}
