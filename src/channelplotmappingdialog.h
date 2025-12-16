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

#ifndef CHANNELPLOTMAPPINGDIALOG_H
#define CHANNELPLOTMAPPINGDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>

#include "channelplotmapping.h"
#include "channelinfomodel.h"

/**
 * Dialog for configuring channel to plot mappings.
 */
class ChannelPlotMappingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChannelPlotMappingDialog(ChannelPlotMapping* mapping,
                                    const ChannelInfoModel* channelInfo,
                                    QWidget *parent = nullptr);

private slots:
    void onNumPlotsChanged(int numPlots);
    void onPlotNameChanged();
    void onChannelMappingChanged();
    void resetToDefault();

private:
    void setupUI();
    void updatePlotNamesTable();
    void updateChannelMappingTable();
    void updateFromMapping();

    ChannelPlotMapping* _mapping;
    const ChannelInfoModel* _channelInfo;
    
    QSpinBox* spNumPlots;
    QTableWidget* plotNamesTable;
    QTableWidget* channelMappingTable;
    QPushButton* resetButton;
};

#endif // CHANNELPLOTMAPPINGDIALOG_H