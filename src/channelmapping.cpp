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

#include "channelmapping.h"

ChannelMappingConfig::ChannelMappingConfig()
{
}

void ChannelMappingConfig::setNumChannels(unsigned num)
{
    unsigned currentSize = _channels.size();
    
    if (num > currentSize)
    {
        // Add new channels, preserving existing ones
        for (unsigned i = currentSize; i < num; i++)
        {
            ChannelMapping ch;
            ch.byteOffset = i;
            ch.byteLength = 1;
            _channels.push_back(ch);
        }
    }
    else if (num < currentSize)
    {
        // Remove excess channels but preserve the first 'num' channels
        _channels.resize(num);
    }
    // If num == currentSize, do nothing to preserve all existing configs
}

unsigned ChannelMappingConfig::numChannels() const
{
    return _channels.size();
}

ChannelMapping& ChannelMappingConfig::channel(unsigned index)
{
    if (index >= _channels.size())
        throw std::out_of_range("Channel index out of range");
    return _channels[index];
}

const ChannelMapping& ChannelMappingConfig::channel(unsigned index) const
{
    if (index >= _channels.size())
        throw std::out_of_range("Channel index out of range");
    return _channels[index];
}

bool ChannelMappingConfig::isValid(unsigned payloadSize, QString& errorMsg) const
{
    // Check for overlapping byte ranges
    for (size_t i = 0; i < _channels.size(); i++)
    {
        for (size_t j = i + 1; j < _channels.size(); j++)
        {
            unsigned end_i = _channels[i].byteOffset + _channels[i].byteLength;
            unsigned end_j = _channels[j].byteOffset + _channels[j].byteLength;
            
            // Check overlap
            if ((_channels[i].byteOffset < end_j && end_i > _channels[j].byteOffset))
            {
                errorMsg = QString("Channel %1 and %2 have overlapping byte ranges!").arg(i).arg(j);
                return false;
            }
        }
    }
    
    // Check if all bytes are within payload size
    for (size_t i = 0; i < _channels.size(); i++)
    {
        unsigned end = _channels[i].byteOffset + _channels[i].byteLength;
        if (end > payloadSize)
        {
            errorMsg = QString("Channel %1 extends beyond payload size!").arg(i + 1);
            return false;
        }
    }
    
    return true;
}
