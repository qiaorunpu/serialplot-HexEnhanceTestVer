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

#ifndef CHANNELMAPPING_H
#define CHANNELMAPPING_H

#include <vector>
#include "numberformat.h"
#include "endiannessbox.h"

/**
 * Describes the mapping of a single channel in the frame.
 */
struct ChannelMapping
{
    unsigned byteOffset;      ///< Starting byte position from sync word (0-based internally, 1-based in UI)
    unsigned byteLength;      ///< Number of bytes for this channel
    NumberFormat numberFormat; ///< Data format (uint8, int24, etc.)
    Endianness endianness;    ///< Byte order for this channel
    bool enabled;             ///< Whether this channel is active

    ChannelMapping() : byteOffset(0), byteLength(1), numberFormat(NumberFormat_uint8), endianness(LittleEndian), enabled(true) {}
};

/**
 * Container for all channel mappings.
 */
class ChannelMappingConfig
{
public:
    ChannelMappingConfig();
    
    void setNumChannels(unsigned num);
    unsigned numChannels() const;
    
    ChannelMapping& channel(unsigned index);
    const ChannelMapping& channel(unsigned index) const;
    
    /// Returns true if all mappings are valid (no overlaps, contiguous, etc.)
    bool isValid(unsigned payloadSize, QString& errorMsg) const;
    
private:
    std::vector<ChannelMapping> _channels;
};

#endif // CHANNELMAPPING_H
