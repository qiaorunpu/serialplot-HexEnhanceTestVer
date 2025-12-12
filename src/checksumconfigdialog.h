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

#ifndef CHECKSUMCONFIGDIALOG_H
#define CHECKSUMCONFIGDIALOG_H

#include <QDialog>
#include "checksumcalculator.h"
#include "framedreadersettings.h"

namespace Ui {
class ChecksumConfigDialog;
}

class ChecksumConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChecksumConfigDialog(ChecksumConfig& config, unsigned maxFrameSize, QWidget *parent = nullptr);
    ~ChecksumConfigDialog();

private slots:
    void onAlgorithmChanged(int index);

private:
    void updateAlgorithmInfo();

    Ui::ChecksumConfigDialog *ui;
    ChecksumConfig& _config;
    unsigned _maxFrameSize;
};

#endif // CHECKSUMCONFIGDIALOG_H
