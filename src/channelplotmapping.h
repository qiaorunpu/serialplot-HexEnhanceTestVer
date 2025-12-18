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

#ifndef CHANNELPLOTMAPPING_H
#define CHANNELPLOTMAPPING_H

#include <QObject>
#include <QVector>
#include <QList>
#include <QString>
#include <QSettings>

/**
 * Manages the mapping between channels and plots, allowing flexible
 * assignment of channels to multiple plots.
 */
class ChannelPlotMapping : public QObject
{
    Q_OBJECT

public:
    enum MappingMode {
        SinglePlot,   ///< All channels in one plot (legacy mode)
        MultiPlot,    ///< Each channel in its own plot (legacy mode)
        CustomPlot    ///< Custom mapping of channels to plots
    };

    explicit ChannelPlotMapping(QObject* parent = nullptr);

    /// Set the number of channels available for mapping
    void setNumChannels(unsigned numChannels);
    
    /// Get the current mapping mode
    MappingMode mode() const { return _mode; }
    
    /// Set the mapping mode
    void setMode(MappingMode mode);
    
    /// Set the number of plots to use (only relevant in CustomPlot mode)
    void setNumPlots(unsigned numPlots);
    
    /// Get the number of plots currently configured
    unsigned getNumPlotsNeeded() const;
    
    /// Get which plot a specific channel is assigned to
    unsigned getPlotForChannel(unsigned channelIndex) const;
    
    /// Set which plot a specific channel should be assigned to
    void setPlotForChannel(unsigned channelIndex, unsigned plotIndex);
    
    /// Get all channels assigned to a specific plot
    QList<unsigned> getChannelsForPlot(unsigned plotIndex) const;
    
    /// Get the name of a specific plot
    QString getPlotName(unsigned plotIndex) const;
    
    /// Set the name of a specific plot
    void setPlotName(unsigned plotIndex, const QString& name);
    
    /// Reset all mappings to default (each channel to its own plot)
    void resetToDefault();
    
    /// Save mapping configuration to settings
    void saveSettings(QSettings* settings);
    
    /// Load mapping configuration from settings
    void loadSettings(QSettings* settings);

signals:
    /// Emitted when the mapping configuration changes
    void mappingChanged();

private:
    MappingMode _mode;
    unsigned _numChannels;
    unsigned _numPlots;
    QVector<unsigned> _channelToPlot;  ///< Maps channel index to plot index
    QVector<QString> _plotNames;       ///< Names for each plot
    
    /// Update internal state after mode change
    void updateMappingForMode();
};

#endif // CHANNELPLOTMAPPING_H