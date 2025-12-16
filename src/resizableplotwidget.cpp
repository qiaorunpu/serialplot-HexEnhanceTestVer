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

#include "resizableplotwidget.h"
#include <QMouseEvent>
#include <QApplication>
#include <QResizeEvent>
#include <QSplitter>

ResizablePlotWidget::ResizablePlotWidget(Plot* plot, const QString& title, QWidget* parent)
    : QFrame(parent)
    , _plot(plot)
    , titleEditable(true)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    setLineWidth(1);
    
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    
    // Create title bar
    titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(4, 2, 4, 2);
    
    titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-weight: bold; color: #333;");
    
    titleEdit = new QLineEdit(title);
    titleEdit->setFrame(false);
    titleEdit->setStyleSheet("font-weight: bold; color: #333; background: transparent;");
    titleEdit->hide();
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(titleEdit);
    titleLayout->addStretch();
    
    layout->addLayout(titleLayout);
    layout->addWidget(_plot, 1);
    
    // Connect signals
    connect(titleEdit, &QLineEdit::editingFinished, this, &ResizablePlotWidget::onTitleEditFinished);
    
    // Enable double-click to edit title
    titleLabel->installEventFilter(this);
}

ResizablePlotWidget::~ResizablePlotWidget()
{
}

void ResizablePlotWidget::setTitle(const QString& title)
{
    titleLabel->setText(title);
    titleEdit->setText(title);
}

QString ResizablePlotWidget::title() const
{
    return titleLabel->text();
}

void ResizablePlotWidget::setTitleEditable(bool editable)
{
    titleEditable = editable;
}

void ResizablePlotWidget::onTitleEditFinished()
{
    QString newTitle = titleEdit->text();
    titleLabel->setText(newTitle);
    titleLabel->show();
    titleEdit->hide();
    
    emit titleChanged(newTitle);
}

bool ResizablePlotWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == titleLabel && event->type() == QEvent::MouseButtonDblClick && titleEditable) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            titleLabel->hide();
            titleEdit->show();
            titleEdit->selectAll();
            titleEdit->setFocus();
            return true;
        }
    }
    return QFrame::eventFilter(obj, event);
}

// PlotLayoutContainer implementation

PlotLayoutContainer::PlotLayoutContainer(QWidget* parent)
    : QWidget(parent)
    , _columns(2)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);
}

PlotLayoutContainer::~PlotLayoutContainer()
{
    clearPlots();
}

void PlotLayoutContainer::addPlot(ResizablePlotWidget* plotWidget)
{
    if (!plotWidget || _plots.contains(plotWidget)) return;
    
    _plots.append(plotWidget);
    plotWidget->setParent(this);
    rebuildLayout();
}

void PlotLayoutContainer::removePlot(ResizablePlotWidget* plotWidget)
{
    if (!plotWidget || !_plots.contains(plotWidget)) return;
    
    _plots.removeAll(plotWidget);
    plotWidget->setParent(nullptr);
    rebuildLayout();
}

void PlotLayoutContainer::clearPlots()
{
    for (auto* plotWidget : _plots) {
        plotWidget->setParent(nullptr);
    }
    _plots.clear();
    rebuildLayout();
}

void PlotLayoutContainer::setColumns(int columns)
{
    if (columns < 1) columns = 1;
    if (_columns == columns) return;
    
    _columns = columns;
    rebuildLayout();
}

void PlotLayoutContainer::rebuildLayout()
{
    // Clear existing layout
    while (mainLayout->count()) {
        QLayoutItem* item = mainLayout->takeAt(0);
        if (QWidget* widget = item->widget()) {
            widget->setParent(nullptr);
        }
        delete item;
    }
    qDeleteAll(rowSplitters);
    rowSplitters.clear();
    
    if (_plots.isEmpty()) return;
    
    // Calculate number of rows needed
    int rows = (_plots.size() + _columns - 1) / _columns;
    
    // Create row splitters
    for (int row = 0; row < rows; ++row) {
        QSplitter* rowSplitter = new QSplitter(Qt::Horizontal, this);
        rowSplitters.append(rowSplitter);
        
        // Add plots to this row
        for (int col = 0; col < _columns; ++col) {
            int plotIndex = row * _columns + col;
            if (plotIndex < _plots.size()) {
                rowSplitter->addWidget(_plots[plotIndex]);
            } else {
                // Add empty widget to maintain layout
                QWidget* emptyWidget = new QWidget();
                emptyWidget->setMinimumSize(100, 50);
                emptyWidget->hide();
                rowSplitter->addWidget(emptyWidget);
            }
        }
        
        // Set equal sizes for all widgets in the row
        QList<int> sizes;
        int widgetWidth = width() / _columns;
        for (int i = 0; i < rowSplitter->count(); ++i) {
            sizes << widgetWidth;
        }
        rowSplitter->setSizes(sizes);
        
        mainLayout->addWidget(rowSplitter);
    }
    
    // If we have multiple rows, create a vertical splitter to manage them
    if (rowSplitters.size() > 1) {
        QSplitter* mainSplitter = new QSplitter(Qt::Vertical, this);
        
        // Move all row splitters to the main splitter
        for (QSplitter* rowSplitter : rowSplitters) {
            mainLayout->removeWidget(rowSplitter);
            mainSplitter->addWidget(rowSplitter);
        }
        
        // Set equal heights for all rows
        QList<int> heights;
        int rowHeight = height() / rowSplitters.size();
        for (int i = 0; i < rowSplitters.size(); ++i) {
            heights << rowHeight;
        }
        mainSplitter->setSizes(heights);
        
        mainLayout->addWidget(mainSplitter);
    }
}

void PlotLayoutContainer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateSplitterSizes();
}

void PlotLayoutContainer::updateSplitterSizes()
{
    if (rowSplitters.isEmpty()) return;
    
    // Update row splitter sizes to maintain equal widths
    for (QSplitter* rowSplitter : rowSplitters) {
        if (rowSplitter->count() > 0) {
            QList<int> sizes;
            int widgetWidth = rowSplitter->width() / rowSplitter->count();
            for (int i = 0; i < rowSplitter->count(); ++i) {
                sizes << widgetWidth;
            }
            rowSplitter->setSizes(sizes);
        }
    }
}