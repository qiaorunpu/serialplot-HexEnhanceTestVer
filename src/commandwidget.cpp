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

#include "commandwidget.h"
#include "ui_commandwidget.h"

#include <QtDebug>
#include <QIcon>

CommandWidget::CommandWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommandWidget),
    _sendAction(this)
{
    qDebug() << "CommandWidget: Constructor started";
    qDebug() << "  - parent=" << parent;
    qDebug() << "  - parent valid?" << (parent != nullptr);
    qDebug() << "  - this=" << this;
    qDebug() << "  - ui ptr=" << ui;
    qDebug() << "  - _sendAction ptr=" << &_sendAction;
    
    qDebug() << "CommandWidget: About to call setupUi...";
    try {
        ui->setupUi(this);
        qDebug() << "CommandWidget: setupUi completed successfully";
    } catch (const std::exception& e) {
        qCritical() << "CommandWidget: Exception in setupUi:" << e.what();
        throw;
    } catch (...) {
        qCritical() << "CommandWidget: Unknown exception in setupUi";
        throw;
    }
    
    qDebug() << "CommandWidget: Connecting signals...";
    qDebug() << "  - pbDelete valid?" << (ui->pbDelete != nullptr);
    qDebug() << "  - pbSend valid?" << (ui->pbSend != nullptr);
    qDebug() << "  - pbASCII valid?" << (ui->pbASCII != nullptr);
    qDebug() << "  - leName valid?" << (ui->leName != nullptr);

    connect(ui->pbDelete, &QPushButton::clicked, this, &CommandWidget::onDeleteClicked);
    qDebug() << "  - pbDelete connected";
    connect(ui->pbSend, &QPushButton::clicked, this, &CommandWidget::onSendClicked);
    qDebug() << "  - pbSend connected";
    connect(ui->pbASCII, &QPushButton::toggled, this, &CommandWidget::onASCIIToggled);
    qDebug() << "  - pbASCII connected";
    connect(ui->leName, &QLineEdit::textChanged, [this](QString text)
            {
                this->_sendAction.setText(text);
            });
    qDebug() << "  - leName connected";
    connect(&_sendAction, &QAction::triggered, this, &CommandWidget::onSendClicked);
    qDebug() << "  - sendAction connected";
    
    qDebug() << "CommandWidget: Constructor finished successfully";
}

CommandWidget::~CommandWidget()
{
    qDebug() << "CommandWidget: Destructor called for" << this;
    delete ui;
    qDebug() << "CommandWidget: Destructor finished";
}

void CommandWidget::onDeleteClicked()
{
    this->deleteLater();
}

void CommandWidget::onSendClicked()
{
    auto command = ui->leCommand->text();

    if (command.isEmpty())
    {
        qWarning() << "Enter a command to send!";
        ui->leCommand->setFocus(Qt::OtherFocusReason);
        emit focusRequested();
        return;
    }

    if (isASCIIMode())
    {
        qDebug() << "Sending " << name() << ":" << command;
        emit sendCommand(ui->leCommand->unEscapedText().toLatin1());
    }
    else // hex mode
    {
        command = command.remove(' ');
        // check if nibbles are missing
        if (command.size() % 2 == 1)
        {
            qWarning() << "HEX command is missing a nibble at the end!";
            ui->leCommand->setFocus(Qt::OtherFocusReason);
            // highlight the byte that is missing a nibble (last byte obviously)
            int textSize = ui->leCommand->text().size();
            ui->leCommand->setSelection(textSize-1, textSize);
            return;
        }
        qDebug() << "Sending HEX:" << command;
        emit sendCommand(QByteArray::fromHex(command.toLatin1()));
    }
}

void CommandWidget::onASCIIToggled(bool checked)
{
    ui->leCommand->setMode(checked);
}

bool CommandWidget::isASCIIMode()
{
    return ui->pbASCII->isChecked();
}

void CommandWidget::setASCIIMode(bool enabled)
{
    qDebug() << "CommandWidget::setASCIIMode: this=" << this << ", enabled=" << enabled;
    if (enabled)
    {
        qDebug() << "  - Checking pbASCII...";
        ui->pbASCII->setChecked(true);
        qDebug() << "  - pbASCII checked";
    }
    else
    {
        qDebug() << "  - Checking pbHEX...";
        ui->pbHEX->setChecked(true);
        qDebug() << "  - pbHEX checked";
    }
}

void CommandWidget::setName(QString name)
{
    qDebug() << "CommandWidget::setName: this=" << this << ", name=" << name;
    qDebug() << "  - leName ptr=" << ui->leName;
    ui->leName->setText(name);
    qDebug() << "  - setText completed";
}

QString CommandWidget::name()
{
    return ui->leName->text();
}

void CommandWidget::setFocusToEdit()
{
    ui->leCommand->setFocus(Qt::OtherFocusReason);
}

QAction* CommandWidget::sendAction()
{
    return &_sendAction;
}

QString CommandWidget::commandText()
{
    return ui->leCommand->text();
}

void CommandWidget::setCommandText(QString str)
{
    qDebug() << "CommandWidget::setCommandText: this=" << this << ", length=" << str.length();
    qDebug() << "  - leCommand ptr=" << ui->leCommand;
    ui->leCommand->selectAll();
    ui->leCommand->insert(str);
    qDebug() << "  - Text inserted";
}
