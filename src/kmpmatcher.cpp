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

#include "kmpmatcher.h"

KMPMatcher::KMPMatcher(const QByteArray& pattern)
    : m_pattern(pattern)
{
    computeLPS();
}

void KMPMatcher::setPattern(const QByteArray& pattern)
{
    m_pattern = pattern;
    computeLPS();
}

void KMPMatcher::computeLPS()
{
    int m = m_pattern.size();
    m_lpsTable.resize(m);
    
    if (m == 0) return;
    
    int len = 0;  // Length of the previous longest prefix suffix
    m_lpsTable[0] = 0;  // lps[0] is always 0
    int i = 1;
    
    // Build the LPS table
    while (i < m)
    {
        if (m_pattern[i] == m_pattern[len])
        {
            len++;
            m_lpsTable[i] = len;
            i++;
        }
        else
        {
            if (len != 0)
            {
                // Don't increment i here
                len = m_lpsTable[len - 1];
            }
            else
            {
                m_lpsTable[i] = 0;
                i++;
            }
        }
    }
}

int KMPMatcher::search(const uint8_t* data, int dataSize, int startPos) const
{
    int m = m_pattern.size();
    if (m == 0 || dataSize == 0 || startPos >= dataSize)
        return -1;
    
    int i = startPos;  // Index for data[]
    int j = 0;         // Index for pattern[]
    
    while (i < dataSize)
    {
        if (data[i] == (uint8_t)m_pattern[j])
        {
            i++;
            j++;
        }
        
        if (j == m)
        {
            // Found a match at position (i - j)
            return i - j;
        }
        else if (i < dataSize && data[i] != (uint8_t)m_pattern[j])
        {
            // Mismatch after j matches
            if (j != 0)
            {
                // Use the LPS table to skip comparisons
                j = m_lpsTable[j - 1];
            }
            else
            {
                i++;
            }
        }
    }
    
    return -1;  // Not found
}

int KMPMatcher::search(const QByteArray& data, int startPos) const
{
    return search(reinterpret_cast<const uint8_t*>(data.constData()), 
                  data.size(), 
                  startPos);
}
