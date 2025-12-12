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

#ifndef CHECKSUMCALCULATOR_H
#define CHECKSUMCALCULATOR_H

#include <cstdint>
#include <QString>

/**
 * Supported checksum/CRC algorithms
 */
enum class ChecksumAlgorithm
{
    None = 0,
    CRC8,
    CRC16,
    CRC16_CCITT,
    CRC16_MODBUS,
    CRC32,
    SUM8,
    SUM16,
    SUM24,
    SUM32,
    XOR8
};

/**
 * Calculates various checksum and CRC algorithms
 */
class ChecksumCalculator
{
public:
    /// Calculate checksum for given data range
    static uint32_t calculate(ChecksumAlgorithm algo, const uint8_t* data, unsigned length);
    
    /// Convert algorithm enum to string
    static QString algorithmToString(ChecksumAlgorithm algo);
    
    /// Convert string to algorithm enum
    static ChecksumAlgorithm stringToAlgorithm(const QString& str);
    
    /// Get the output size in bytes for the algorithm
    static unsigned getOutputSize(ChecksumAlgorithm algo);

private:
    static uint8_t calculateCRC8(const uint8_t* data, unsigned length);
    static uint16_t calculateCRC16(const uint8_t* data, unsigned length);
    static uint16_t calculateCRC16_CCITT(const uint8_t* data, unsigned length);
    static uint16_t calculateCRC16_MODBUS(const uint8_t* data, unsigned length);
    static uint32_t calculateCRC32(const uint8_t* data, unsigned length);
    static uint8_t calculateSUM8(const uint8_t* data, unsigned length);
    static uint16_t calculateSUM16(const uint8_t* data, unsigned length);
    static uint32_t calculateSUM24(const uint8_t* data, unsigned length);
    static uint32_t calculateSUM32(const uint8_t* data, unsigned length);
    static uint8_t calculateXOR8(const uint8_t* data, unsigned length);
    
    static uint16_t crc16_standard(const uint8_t* data, unsigned length, uint16_t poly, uint16_t init, bool refIn, bool refOut, uint16_t xorOut);
};

#endif // CHECKSUMCALCULATOR_H
