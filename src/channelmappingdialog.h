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

#ifndef CHANNELMAPPINGDIALOG_H
#define CHANNELMAPPINGDIALOG_H

#include <QDialog>
#include <QTableWidgetItem>
#include "channelmapping.h"

namespace Ui {
class ChannelMappingDialog;
}

class ChannelMappingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChannelMappingDialog(ChannelMappingConfig& config, QWidget *parent = nullptr);
    ~ChannelMappingDialog();
    
    void setTotalFrameSize(unsigned totalFrameSize);

private slots:
    void onAccepted();
    void onTableCellChanged(int row, int column);

private:
    void updateTable();
    void loadFromConfig();
    void saveToConfig();
    bool validateAndShowError();
    void updateDataLengthForRow(int row);

    Ui::ChannelMappingDialog *ui;
    ChannelMappingConfig& _config;
    unsigned _totalFrameSize;
    bool _updating; // flag to prevent recursion during updates
};

#endif // CHANNELMAPPINGDIALOG_H
