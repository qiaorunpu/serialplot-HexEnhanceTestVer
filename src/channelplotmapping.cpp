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

#include "channelplotmapping.h"
#include <algorithm>

ChannelPlotMapping::ChannelPlotMapping(QObject* parent)
    : QObject(parent)
    , _mode(SinglePlot)
    , _numChannels(0)
    , _numPlots(1)
{
}

void ChannelPlotMapping::setNumChannels(unsigned numChannels)
{
    if (_numChannels == numChannels) return;
    
    _numChannels = numChannels;
    _channelToPlot.resize(numChannels);
    
    updateMappingForMode();
    emit mappingChanged();
}

void ChannelPlotMapping::setMode(MappingMode mode)
{
    if (_mode == mode) return;
    
    _mode = mode;
    updateMappingForMode();
    emit mappingChanged();
}

void ChannelPlotMapping::setNumPlots(unsigned numPlots)
{
    if (numPlots == 0) numPlots = 1; // At least one plot
    if (_numPlots == numPlots) return;
    
    _numPlots = numPlots;
    _plotNames.resize(numPlots);
    
    // Set default names for new plots
    for (unsigned i = 0; i < numPlots; ++i) {
        if (_plotNames[i].isEmpty()) {
            _plotNames[i] = QString("Plot %1").arg(i + 1);
        }
    }
    
    // Update channel mapping if in custom mode
    if (_mode == CustomPlot) {
        // Reassign channels that were mapped to non-existent plots
        for (unsigned ch = 0; ch < _numChannels; ++ch) {
            if (_channelToPlot[ch] >= numPlots) {
                _channelToPlot[ch] = 0; // Assign to first plot
            }
        }
    }
    
    updateMappingForMode();
    emit mappingChanged();
}

unsigned ChannelPlotMapping::getNumPlotsNeeded() const
{
    switch (_mode) {
        case SinglePlot:
            return 1;
        case MultiPlot:
            return _numChannels;
        case CustomPlot:
            return _numPlots;
    }
    return 1;
}

unsigned ChannelPlotMapping::getPlotForChannel(unsigned channelIndex) const
{
    if (channelIndex >= _numChannels) return 0;
    
    unsigned plotIndex;
    switch (_mode) {
        case SinglePlot:
            plotIndex = 0; // All channels in plot 0
            break;
        case MultiPlot:
            plotIndex = channelIndex; // Each channel in its own plot
            break;
        case CustomPlot:
            plotIndex = _channelToPlot[channelIndex];
            break;
        default:
            plotIndex = 0;
            break;
    }
    
    // Ensure plotIndex is within valid range
    if (plotIndex >= _numPlots) {
        return 0; // Default to plot 0 if invalid
    }
    
    return plotIndex;
}

void ChannelPlotMapping::setPlotForChannel(unsigned channelIndex, unsigned plotIndex)
{
    if (channelIndex >= _numChannels || plotIndex >= _numPlots) return;
    if (_mode != CustomPlot) return; // Only allow in custom mode
    
    _channelToPlot[channelIndex] = plotIndex;
    emit mappingChanged();
}

QList<unsigned> ChannelPlotMapping::getChannelsForPlot(unsigned plotIndex) const
{
    QList<unsigned> channels;
    
    switch (_mode) {
        case SinglePlot:
            if (plotIndex == 0) {
                for (unsigned i = 0; i < _numChannels; ++i) {
                    channels.append(i);
                }
            }
            break;
        case MultiPlot:
            if (plotIndex < _numChannels) {
                channels.append(plotIndex);
            }
            break;
        case CustomPlot:
            for (unsigned i = 0; i < _numChannels; ++i) {
                if (_channelToPlot[i] == plotIndex) {
                    channels.append(i);
                }
            }
            break;
    }
    
    return channels;
}

QString ChannelPlotMapping::getPlotName(unsigned plotIndex) const
{
    if (plotIndex >= _plotNames.size()) {
        return QString("Plot %1").arg(plotIndex + 1);
    }
    return _plotNames[plotIndex];
}

void ChannelPlotMapping::setPlotName(unsigned plotIndex, const QString& name)
{
    if (plotIndex >= _plotNames.size()) {
        _plotNames.resize(plotIndex + 1);
    }
    _plotNames[plotIndex] = name;
    emit mappingChanged();
}

void ChannelPlotMapping::resetToDefault()
{
    setMode(MultiPlot);
}

void ChannelPlotMapping::updateMappingForMode()
{
    switch (_mode) {
        case SinglePlot:
            // All channels map to plot 0
            for (unsigned i = 0; i < _numChannels; ++i) {
                if (i < _channelToPlot.size()) {
                    _channelToPlot[i] = 0;
                }
            }
            _numPlots = 1;
            break;
            
        case MultiPlot:
            // Each channel maps to its own plot
            _numPlots = _numChannels;
            _plotNames.resize(_numPlots);
            for (unsigned i = 0; i < _numChannels; ++i) {
                if (i < _channelToPlot.size()) {
                    _channelToPlot[i] = i;
                }
                if (i < _plotNames.size() && _plotNames[i].isEmpty()) {
                    _plotNames[i] = QString("Channel %1").arg(i + 1);
                }
            }
            break;
            
        case CustomPlot:
            // Keep existing mapping, but ensure all channels have valid plots
            _plotNames.resize(_numPlots);
            for (unsigned i = 0; i < _numChannels; ++i) {
                if (i >= _channelToPlot.size() || _channelToPlot[i] >= _numPlots) {
                    if (i < _channelToPlot.size()) {
                        _channelToPlot[i] = 0; // Default to first plot
                    }
                }
            }
            for (unsigned i = 0; i < _numPlots; ++i) {
                if (_plotNames[i].isEmpty()) {
                    _plotNames[i] = QString("Plot %1").arg(i + 1);
                }
            }
            break;
    }
}