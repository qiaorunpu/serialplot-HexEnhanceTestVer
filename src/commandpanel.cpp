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

#include <QByteArray>
#include <QtDebug>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>

#include "commandpanel.h"
#include "ui_commandpanel.h"
#include "setting_defines.h"

CommandPanel::CommandPanel(QSerialPort* port, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommandPanel),
    _menu(tr("&Commands")), _newCommandAction(tr("&New Command"), this)
{
    serialPort = port;

    ui->setupUi(this);
    
    // Setup commands area
    auto layout = new QVBoxLayout();
    layout->setSpacing(0);
    ui->scrollAreaWidgetContents->setLayout(layout);

    // Initialize and setup raw data view
    rawDataView = new RawDataView(this);
    
    // Add raw data view to the right panel
    auto rawDataLayout = new QVBoxLayout(ui->rawDataWidget);
    
    // Add title for raw data
    auto rawDataLabel = new QLabel("Raw Data");
    rawDataLabel->setFont(QFont("", -1, QFont::Bold));
    rawDataLayout->addWidget(rawDataLabel);
    rawDataLayout->addWidget(rawDataView);

    connect(ui->pbNew, &QPushButton::clicked, this, &CommandPanel::newCommand);
    connect(&_newCommandAction, &QAction::triggered, this, &CommandPanel::newCommand);

    _menu.addAction(&_newCommandAction);
    _menu.addSeparator();

    command_name_counter = 0;
}

CommandPanel::~CommandPanel()
{
    commands.clear(); // UI will 'delete' actual objects
    delete ui;
}

CommandWidget* CommandPanel::newCommand()
{
    qDebug() << "\n>>> CommandPanel::newCommand: ENTER";
    qDebug() << "  - ui ptr=" << ui;
    qDebug() << "  - ui valid?" << (ui != nullptr);
    
    if (ui && ui->scrollAreaWidgetContents) {
        qDebug() << "  - scrollAreaWidgetContents ptr=" << ui->scrollAreaWidgetContents;
        qDebug() << "  - scrollAreaWidgetContents layout=" << ui->scrollAreaWidgetContents->layout();
    } else {
        qCritical() << "  ERROR: UI not initialized!";
        return nullptr;
    }
    
    qDebug() << "  Creating CommandWidget with parent...";
    CommandWidget* command = nullptr;
    try {
        // 修复：传入父窗口避免辅助功能系统问题
        command = new CommandWidget(ui->scrollAreaWidgetContents);
        qDebug() << "  CommandWidget created successfully, ptr=" << command;
    } catch (const std::exception& e) {
        qCritical() << "  EXCEPTION during CommandWidget creation:" << e.what();
        return nullptr;
    } catch (...) {
        qCritical() << "  UNKNOWN EXCEPTION during CommandWidget creation";
        return nullptr;
    }
    
    if (!command) {
        qCritical() << "  ERROR: CommandWidget is nullptr after creation!";
        return nullptr;
    }
    
    qDebug() << "  Widget created, setting name...";
    qDebug() << "  Current command_name_counter=" << command_name_counter;
    command_name_counter++;
    command->setName(tr("Command ") + QString::number(command_name_counter));
    qDebug() << "CommandPanel::newCommand: Name set";
    ui->scrollAreaWidgetContents->layout()->addWidget(command);
    command->setFocusToEdit();
    connect(command, &CommandWidget::sendCommand, this, &CommandPanel::sendCommand);
    connect(command, &CommandWidget::focusRequested, this, &CommandPanel::focusRequested);
    _menu.addAction(command->sendAction());

    // add to command list and remove on destroy
    commands << command;
    connect(command, &QObject::destroyed, [this](QObject* obj)
            {
                commands.removeOne(static_cast<CommandWidget*>(obj));
                reAssignShortcuts();
            });

    reAssignShortcuts();
    return command;
}

void CommandPanel::reAssignShortcuts()
{
    // can assign shortcuts to first 12 commands
    for (int i = 0; i < std::min(12, (int)commands.size()); i++)
    {
        auto cmd = commands[i];
        cmd->sendAction()->setShortcut(QKeySequence(Qt::Key_F1 + i));
    }
}

void CommandPanel::sendCommand(QByteArray command)
{
    if (!serialPort->isOpen())
    {
        qCritical() << "Port is not open!";
        return;
    }

    if (serialPort->write(command) >= 0)
    {
        // Add sent data to raw data view
        rawDataView->addSentData(command);
    }
    else
    {
        qCritical() << "Send command failed!";
    }
}

QMenu* CommandPanel::menu()
{
    return &_menu;
}

QAction* CommandPanel::newCommandAction()
{
    return &_newCommandAction;
}

RawDataView* CommandPanel::getRawDataView() const
{
    return rawDataView;
}

unsigned CommandPanel::numOfCommands()
{
    return commands.size();
}

void CommandPanel::saveSettings(QSettings* settings)
{
    settings->beginGroup(SettingGroup_Commands);
    settings->beginWriteArray(SG_Commands_Command);
    for (int i = 0; i < commands.size(); i ++)
    {
        settings->setArrayIndex(i);
        auto command = commands[i];
        settings->setValue(SG_Commands_Name, command->name());
        settings->setValue(SG_Commands_Type, command->isASCIIMode() ? "ascii" : "hex");
        settings->setValue(SG_Commands_Data, command->commandText());
    }
    settings->endArray();
    settings->endGroup();
}

void CommandPanel::loadSettings(QSettings* settings)
{
    // clear all commands
    while (commands.size())
    {
        auto command = commands.takeLast();
        command->disconnect();
        delete command;
    }

    // load commands
    settings->beginGroup(SettingGroup_Commands);
    
    // 先检查实际有多少个命令条目
    qDebug() << "\n=== CommandPanel::loadSettings: Checking INI file consistency ===";
    qDebug() << "  - All keys in Commands group:" << settings->childKeys();
    qDebug() << "  - All groups in Commands:" << settings->childGroups();
    
    unsigned size = settings->beginReadArray(SG_Commands_Command);
    qDebug() << "\nCommandPanel::loadSettings: INI says" << size << "commands to load";
    
    // 检查实际有多少个条目存在
    unsigned actualCount = 0;
    for (unsigned i = 1; i <= 20; i++) {
        settings->setArrayIndex(i-1);
        if (settings->contains(SG_Commands_Name) || settings->contains(SG_Commands_Command)) {
            actualCount = i;
        }
    }
    qDebug() << "CommandPanel::loadSettings: Actually found" << actualCount << "command entries in INI";
    
    if (size != actualCount) {
        qWarning() << "\n!!! DATA INCONSISTENCY DETECTED !!!";
        qWarning() << "  - 'size' field in INI says:" << size;
        qWarning() << "  - Actual entries found:" << actualCount;
        qWarning() << "  - Will only load" << size << "commands (extra" << (actualCount - size) << "entries will be ignored)";
        qWarning() << "  - This may cause unexpected behavior!\n";
    }
    
    qDebug() << "CommandPanel::loadSettings: Loading" << size << "commands";
    for (unsigned i = 0; i < size; i ++)
    {
        qDebug() << "\n=== CommandPanel::loadSettings: Processing command index" << i << "===";
        settings->setArrayIndex(i);
        
        // 显示这个条目的所有数据
        qDebug() << "  INI data for index" << i << ":";
        QStringList keys = settings->childKeys();
        for (const QString& key : keys) {
            qDebug() << "    -" << key << "=" << settings->value(key);
        }
        
        qDebug() << "  Creating widget...";
        auto command = newCommand();
        if (!command) {
            qCritical() << "  ERROR: Failed to create command widget!";
            continue;
        }
        qDebug() << "  Widget created successfully, ptr=" << command;

        // load command name
        QString name = settings->value(SG_Commands_Name, "").toString();
        qDebug() << "  Setting name:" << name;
        if (!name.isEmpty()) {
            command->setName(name);
            qDebug() << "  Name set successfully";
        } else {
            qWarning() << "  WARNING: Empty command name for index" << i;
        }

        // Important: type should be set before command data for correct validation
        QString type = settings->value(SG_Commands_Type, "").toString();
        qDebug() << "  Command type:" << type;
        if (type == "ascii")
        {
            qDebug() << "  Setting ASCII mode...";
            command->setASCIIMode(true);
            qDebug() << "  ASCII mode set";
        }
        else if (type == "hex")
        {
            qDebug() << "  Setting HEX mode...";
            command->setASCIIMode(false);
            qDebug() << "  HEX mode set";
        }
        else
        {
            qDebug() << "  Mode unchanged (type was:" << type << ")";
        }

        // load command data
        QString cmdData = settings->value(SG_Commands_Data, "").toString();
        qDebug() << "  Command data length:" << cmdData.length();
        qDebug() << "  Command data (first 50 chars):" << cmdData.left(50);
        command->setCommandText(cmdData);
        qDebug() << "  Command data set";
        qDebug() << "=== Command" << i << "loaded successfully ===\n";
    }

    settings->endArray();
    settings->endGroup();
    
    qDebug() << "\nCommandPanel::loadSettings: Completed successfully";
    qDebug() << "  - Total commands loaded:" << commands.size();
    qDebug() << "  - Expected:" << size;
    if (commands.size() != size) {
        qWarning() << "  WARNING: Mismatch between expected (" << size << ") and actual (" << commands.size() << ") loaded commands!";
    }
}
