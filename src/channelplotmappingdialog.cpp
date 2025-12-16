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

#include "channelplotmappingdialog.h"
#include <QHeaderView>
#include <QComboBox>
#include <QDialogButtonBox>

ChannelPlotMappingDialog::ChannelPlotMappingDialog(ChannelPlotMapping* mapping,
                                                 const ChannelInfoModel* channelInfo,
                                                 QWidget *parent)
    : QDialog(parent)
    , _mapping(mapping)
    , _channelInfo(channelInfo)
{
    setWindowTitle(tr("Configure Channel Plot Mapping"));
    setModal(true);
    resize(600, 400);
    
    setupUI();
    updateFromMapping();
    
    // Switch to custom mode when this dialog is opened
    _mapping->setMode(ChannelPlotMapping::CustomPlot);
}

void ChannelPlotMappingDialog::setupUI()
{
    auto layout = new QVBoxLayout(this);
    
    // Number of plots section
    auto plotsGroup = new QGroupBox(tr("Plot Configuration"), this);
    auto plotsLayout = new QHBoxLayout(plotsGroup);
    
    plotsLayout->addWidget(new QLabel(tr("Number of Plots:")));
    spNumPlots = new QSpinBox();
    spNumPlots->setMinimum(1);
    spNumPlots->setMaximum(32);
    spNumPlots->setValue(_mapping->getNumPlotsNeeded());
    plotsLayout->addWidget(spNumPlots);
    
    resetButton = new QPushButton(tr("Reset to Default"));
    plotsLayout->addWidget(resetButton);
    plotsLayout->addStretch();
    
    layout->addWidget(plotsGroup);
    
    // Plot names table
    auto plotNamesGroup = new QGroupBox(tr("Plot Names"), this);
    auto plotNamesLayout = new QVBoxLayout(plotNamesGroup);
    
    plotNamesTable = new QTableWidget();
    plotNamesTable->setColumnCount(2);
    plotNamesTable->setHorizontalHeaderLabels({tr("Plot"), tr("Name")});
    plotNamesTable->horizontalHeader()->setStretchLastSection(true);
    plotNamesTable->verticalHeader()->setVisible(false);
    plotNamesLayout->addWidget(plotNamesTable);
    
    layout->addWidget(plotNamesGroup);
    
    // Channel mapping table
    auto channelGroup = new QGroupBox(tr("Channel Assignments"), this);
    auto channelLayout = new QVBoxLayout(channelGroup);
    
    channelMappingTable = new QTableWidget();
    channelMappingTable->setColumnCount(3);
    channelMappingTable->setHorizontalHeaderLabels({tr("Channel"), tr("Name"), tr("Assigned Plot")});
    channelMappingTable->horizontalHeader()->setStretchLastSection(true);
    channelMappingTable->verticalHeader()->setVisible(false);
    channelLayout->addWidget(channelMappingTable);
    
    layout->addWidget(channelGroup);
    
    // Dialog buttons
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    
    // Connect signals
    connect(spNumPlots, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ChannelPlotMappingDialog::onNumPlotsChanged);
    connect(resetButton, &QPushButton::clicked,
            this, &ChannelPlotMappingDialog::resetToDefault);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ChannelPlotMappingDialog::onNumPlotsChanged(int numPlots)
{
    _mapping->setNumPlots(numPlots);
    updatePlotNamesTable();
    updateChannelMappingTable();
}

void ChannelPlotMappingDialog::onPlotNameChanged()
{
    auto lineEdit = qobject_cast<QLineEdit*>(sender());
    if (!lineEdit) return;
    
    bool ok;
    int plotIndex = lineEdit->property("plotIndex").toInt(&ok);
    if (ok) {
        _mapping->setPlotName(plotIndex, lineEdit->text());
    }
}

void ChannelPlotMappingDialog::onChannelMappingChanged()
{
    auto comboBox = qobject_cast<QComboBox*>(sender());
    if (!comboBox) return;
    
    bool ok;
    int channelIndex = comboBox->property("channelIndex").toInt(&ok);
    if (ok) {
        _mapping->setPlotForChannel(channelIndex, comboBox->currentIndex());
    }
}

void ChannelPlotMappingDialog::resetToDefault()
{
    _mapping->setMode(ChannelPlotMapping::MultiPlot);
    spNumPlots->setValue(_mapping->getNumPlotsNeeded());
    updateFromMapping();
}

void ChannelPlotMappingDialog::updatePlotNamesTable()
{
    unsigned numPlots = _mapping->getNumPlotsNeeded();
    plotNamesTable->setRowCount(numPlots);
    
    for (unsigned i = 0; i < numPlots; ++i) {
        // Plot index column
        auto indexItem = new QTableWidgetItem(QString::number(i + 1));
        indexItem->setFlags(Qt::ItemIsEnabled);
        plotNamesTable->setItem(i, 0, indexItem);
        
        // Plot name column
        auto nameEdit = new QLineEdit(_mapping->getPlotName(i));
        nameEdit->setProperty("plotIndex", i);
        connect(nameEdit, &QLineEdit::textChanged,
                this, &ChannelPlotMappingDialog::onPlotNameChanged);
        plotNamesTable->setCellWidget(i, 1, nameEdit);
    }
}

void ChannelPlotMappingDialog::updateChannelMappingTable()
{
    if (!_channelInfo) return;
    
    int numChannels = _channelInfo->rowCount();
    unsigned numPlots = _mapping->getNumPlotsNeeded();
    
    channelMappingTable->setRowCount(numChannels);
    
    for (int ch = 0; ch < numChannels; ++ch) {
        // Channel index column
        auto indexItem = new QTableWidgetItem(QString::number(ch + 1));
        indexItem->setFlags(Qt::ItemIsEnabled);
        channelMappingTable->setItem(ch, 0, indexItem);
        
        // Channel name column
        QString channelName = _channelInfo->data(_channelInfo->index(ch, ChannelInfoModel::COLUMN_NAME)).toString();
        auto nameItem = new QTableWidgetItem(channelName);
        nameItem->setFlags(Qt::ItemIsEnabled);
        channelMappingTable->setItem(ch, 1, nameItem);
        
        // Plot assignment combo box
        auto plotCombo = new QComboBox();
        for (unsigned p = 0; p < numPlots; ++p) {
            plotCombo->addItem(QString("Plot %1").arg(p + 1));
        }
        plotCombo->setCurrentIndex(_mapping->getPlotForChannel(ch));
        plotCombo->setProperty("channelIndex", ch);
        connect(plotCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ChannelPlotMappingDialog::onChannelMappingChanged);
        channelMappingTable->setCellWidget(ch, 2, plotCombo);
    }
}

void ChannelPlotMappingDialog::updateFromMapping()
{
    spNumPlots->setValue(_mapping->getNumPlotsNeeded());
    updatePlotNamesTable();
    updateChannelMappingTable();
}