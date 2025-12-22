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

#include "checksumcalculator.h"

uint32_t ChecksumCalculator::calculate(ChecksumAlgorithm algo, const uint8_t* data, unsigned length)
{
    switch (algo)
    {
        case ChecksumAlgorithm::CRC8:
            return calculateCRC8(data, length);
        case ChecksumAlgorithm::CRC16:
            return calculateCRC16(data, length);
        case ChecksumAlgorithm::CRC16_CCITT:
            return calculateCRC16_CCITT(data, length);
        case ChecksumAlgorithm::CRC16_MODBUS:
            return calculateCRC16_MODBUS(data, length);
        case ChecksumAlgorithm::CRC32:
            return calculateCRC32(data, length);
        case ChecksumAlgorithm::SUM8:
            return calculateSUM8(data, length);
        case ChecksumAlgorithm::SUM16:
            return calculateSUM16(data, length);
        case ChecksumAlgorithm::SUM24:
            return calculateSUM24(data, length);
        case ChecksumAlgorithm::SUM32:
            return calculateSUM32(data, length);
        case ChecksumAlgorithm::XOR8:
            return calculateXOR8(data, length);
        default:
            return 0;
    }
}

QString ChecksumCalculator::algorithmToString(ChecksumAlgorithm algo)
{
    switch (algo)
    {
        case ChecksumAlgorithm::CRC8:
            return "CRC8";
        case ChecksumAlgorithm::CRC16:
            return "CRC16";
        case ChecksumAlgorithm::CRC16_CCITT:
            return "CRC16-CCITT";
        case ChecksumAlgorithm::CRC16_MODBUS:
            return "CRC16-MODBUS";
        case ChecksumAlgorithm::CRC32:
            return "CRC32";
        case ChecksumAlgorithm::SUM8:
            return "SUM8";
        case ChecksumAlgorithm::SUM16:
            return "SUM16";
        case ChecksumAlgorithm::SUM24:
            return "SUM24";
        case ChecksumAlgorithm::SUM32:
            return "SUM32";
        case ChecksumAlgorithm::XOR8:
            return "XOR8";
        default:
            return "None";
    }
}

ChecksumAlgorithm ChecksumCalculator::stringToAlgorithm(const QString& str)
{
    if (str == "CRC8") return ChecksumAlgorithm::CRC8;
    if (str == "CRC16") return ChecksumAlgorithm::CRC16;
    if (str == "CRC16-CCITT") return ChecksumAlgorithm::CRC16_CCITT;
    if (str == "CRC16-MODBUS") return ChecksumAlgorithm::CRC16_MODBUS;
    if (str == "CRC32") return ChecksumAlgorithm::CRC32;
    if (str == "SUM8") return ChecksumAlgorithm::SUM8;
    if (str == "SUM16") return ChecksumAlgorithm::SUM16;
    if (str == "SUM24") return ChecksumAlgorithm::SUM24;
    if (str == "SUM32") return ChecksumAlgorithm::SUM32;
    if (str == "XOR8") return ChecksumAlgorithm::XOR8;

    return ChecksumAlgorithm::None;
}

unsigned ChecksumCalculator::getOutputSize(ChecksumAlgorithm algo)
{
    switch (algo)
    {
        case ChecksumAlgorithm::CRC8:
        case ChecksumAlgorithm::SUM8:
        case ChecksumAlgorithm::XOR8:
            return 1;
        case ChecksumAlgorithm::CRC16:
        case ChecksumAlgorithm::CRC16_CCITT:
        case ChecksumAlgorithm::CRC16_MODBUS:
        case ChecksumAlgorithm::SUM16:
            return 2;
        case ChecksumAlgorithm::SUM24:
            return 3;
        case ChecksumAlgorithm::CRC32:
        case ChecksumAlgorithm::SUM32:
            return 4;
        default:
            return 0;
    }
}

uint8_t ChecksumCalculator::calculateCRC8(const uint8_t* data, unsigned length)
{
    uint8_t crc = 0xFF;
    for (unsigned i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

uint16_t ChecksumCalculator::crc16_standard(const uint8_t* data, unsigned length, 
                                              uint16_t poly, uint16_t init, 
                                              bool refIn, bool refOut, uint16_t xorOut)
{
    uint16_t crc = init;
    
    for (unsigned i = 0; i < length; i++)
    {
        uint8_t byte = data[i];
        
        if (refIn)
        {
            // Reflect input byte
            byte = ((byte & 0xF0) >> 4) | ((byte & 0x0F) << 4);
            byte = ((byte & 0xCC) >> 2) | ((byte & 0x33) << 2);
            byte = ((byte & 0xAA) >> 1) | ((byte & 0x55) << 1);
        }
        
        crc ^= ((uint16_t)byte) << 8;
        
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ poly;
            else
                crc = crc << 1;
        }
    }
    
    if (refOut)
    {
        uint16_t reflected = 0;
        for (int i = 0; i < 16; i++)
        {
            if (crc & (1 << i))
                reflected |= (1 << (15 - i));
        }
        crc = reflected;
    }
    
    return crc ^ xorOut;
}

uint16_t ChecksumCalculator::calculateCRC16(const uint8_t* data, unsigned length)
{
    // CRC-16 (ARC/LHA)
    return crc16_standard(data, length, 0x8005, 0x0000, true, true, 0x0000);
}

uint16_t ChecksumCalculator::calculateCRC16_CCITT(const uint8_t* data, unsigned length)
{
    // CRC-16-CCITT (XMODEM)
    return crc16_standard(data, length, 0x1021, 0x0000, false, false, 0x0000);
}

uint16_t ChecksumCalculator::calculateCRC16_MODBUS(const uint8_t* data, unsigned length)
{
    // CRC-16-MODBUS
    return crc16_standard(data, length, 0xA001, 0xFFFF, true, true, 0x0000);
}

uint32_t ChecksumCalculator::calculateCRC32(const uint8_t* data, unsigned length)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t poly = 0x04C11DB7;
    
    for (unsigned i = 0; i < length; i++)
    {
        crc ^= ((uint32_t)data[i]) << 24;
        
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ poly;
            else
                crc = crc << 1;
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

uint8_t ChecksumCalculator::calculateSUM8(const uint8_t* data, unsigned length)
{
    uint8_t sum = 0;
    for (unsigned i = 0; i < length; i++)
        sum += data[i];
    return sum;
}

uint16_t ChecksumCalculator::calculateSUM16(const uint8_t* data, unsigned length)
{
    uint16_t sum = 0;
    for (unsigned i = 0; i < length; i++)
        sum += data[i];
    return sum;
}

uint32_t ChecksumCalculator::calculateSUM24(const uint8_t* data, unsigned length)
{
    uint32_t sum = 0;
    for (unsigned i = 0; i < length; i++)
        sum += data[i];
    return sum & 0xFFFFFF;
}

uint32_t ChecksumCalculator::calculateSUM32(const uint8_t* data, unsigned length)
{
    uint32_t sum = 0;
    for (unsigned i = 0; i < length; i++)
        sum += data[i];
    return sum;
}

uint8_t ChecksumCalculator::calculateXOR8(const uint8_t* data, unsigned length)
{
    uint8_t xor_val = 0;
    for (unsigned i = 0; i < length; i++)
        xor_val ^= data[i];
    return xor_val;
}
