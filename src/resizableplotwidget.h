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

#ifndef RESIZABLEPLOTWIDGET_H
#define RESIZABLEPLOTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QList>

#include "plot.h"

/**
 * A resizable plot widget container that includes a title bar
 * and can be arranged in a flexible layout with other plot widgets.
 */
class ResizablePlotWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ResizablePlotWidget(Plot* plot, const QString& title = "", QWidget* parent = nullptr);
    ~ResizablePlotWidget();

    /// Get the contained plot widget
    Plot* plot() const { return _plot; }
    
    /// Set the plot title
    void setTitle(const QString& title);
    
    /// Get the current title
    QString title() const;
    
    /// Enable/disable title editing
    void setTitleEditable(bool editable);

signals:
    /// Emitted when the title is changed
    void titleChanged(const QString& newTitle);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onTitleEditFinished();

private:
    Plot* _plot;
    QVBoxLayout* layout;
    QHBoxLayout* titleLayout;
    QLabel* titleLabel;
    QLineEdit* titleEdit;
    bool titleEditable;
};

/**
 * A container that manages multiple resizable plot widgets
 * with automatic layout arrangement.
 */
class PlotLayoutContainer : public QWidget
{
    Q_OBJECT

public:
    explicit PlotLayoutContainer(QWidget* parent = nullptr);
    ~PlotLayoutContainer();

    /// Add a new plot widget to the container
    void addPlot(ResizablePlotWidget* plotWidget);
    
    /// Remove a plot widget from the container
    void removePlot(ResizablePlotWidget* plotWidget);
    
    /// Clear all plot widgets
    void clearPlots();
    
    /// Get all plot widgets
    QList<ResizablePlotWidget*> plots() const { return _plots; }
    
    /// Set the number of columns for automatic layout
    void setColumns(int columns);
    
    /// Get the current number of columns
    int columns() const { return _columns; }

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void rebuildLayout();
    void updateSplitterSizes();

    QList<ResizablePlotWidget*> _plots;
    QVBoxLayout* mainLayout;
    QList<QSplitter*> rowSplitters;
    int _columns;
};

#endif // RESIZABLEPLOTWIDGET_H