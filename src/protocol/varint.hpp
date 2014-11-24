/* 
 * Bariumsulfate, a clean room minecraft server implementatien
 * Copyright (C) 2014 Bert <bariumsulfate@openmailbox.org>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef BARIUMSULFATE__PROTOCOL__VARINT_HPP
#define BARIUMSULFATE__PROTOCOL__VARINT_HPP

#include <vector>

// TODO: consider whether or not we need to support signed numbers.
template<typename T>
class varint
{
public:
    typedef T value_type;

    varint() : value_{0} {}
    varint(T v) : value_{v} {}

    T operator =(T v)
    {
        return value_ = v;
    }

    operator T() const
    {
        return value_;
    }

    // Allows you to create a varint byte by byte. Useful in the unlikely situation
    // that someone decides to use varint for the size of packets in a network protocol
    // or something equally stupid. Oh wait, that's exactly what minecraft does.
    // 
    // b -> the byte to add
    // n -> the index of the byte in the sequence
    // return -> true if we expect more bytes, false if this number is done
    bool append_byte(uint8_t b, int n)
    {
        if (n == 0)
            value_ = 0;

        value_ |= (b & 0x7F) << (n * 7);
        return (b & 0x80) != 0;
    }

    static void bytes(T n, std::vector<uint8_t>& dst)
    {
        while (n >= 0x80)
        {
            dst.push_back(static_cast<uint8_t>(n & 0x7F) | 0x80);
            n >>= 7;
        }
        dst.push_back(static_cast<uint8_t>(n));
    }

private:
    T value_;
};

#endif
