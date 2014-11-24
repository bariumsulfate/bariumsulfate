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
#ifndef BARIUMSULFATE__PROTOCOL__BUFFER_HPP
#define BARIUMSULFATE__PROTOCOL__BUFFER_HPP

#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <type_traits>

#include <protocol/varint.hpp>

class byte_stream
{
public:
    byte_stream() : pos_{0}
    {
        data_.reserve(512);
    }
    
    byte_stream(byte_stream&& rhs) noexcept : 
        pos_{rhs.pos_}, data_{std::move(rhs.data_)}
    {
    }

    byte_stream(const byte_stream& rhs) noexcept :
        pos_{rhs.pos_}, data_{rhs.data_}
    {
    }
    
    const std::vector<uint8_t>& data()
    {
        return data_;
    }

    void resize(size_t s) 
    {
        data_.resize(s);
    }

    size_t size() const
    {
        return data_.size();
    }

    size_t write(const uint8_t* src, size_t len, bool reverse)
    {
        if (pos_ + len > data_.size())
            data_.resize(pos_ + len);
        
        std::vector<uint8_t>::iterator end;
        if (reverse)
            end = std::reverse_copy(src, src + len, data_.begin() + pos_);
        else
            end = std::copy(src, src + len, data_.begin() + pos_);
        size_t rlen = static_cast<size_t>(end - (data_.begin() + pos_));
        pos_ += rlen;
        return rlen;
    }

    byte_stream& operator<<(bool src)
    {
        uint8_t byte = static_cast<uint8_t>(src);
        write(&byte, 1, false);
        return *this;
    }
    
    template <typename T>
    byte_stream& operator<<(const varint<T>& src)
    {
        T value = src;
        
        uint8_t byte;
        while (value >= 0x80)
        {
            byte = static_cast<uint8_t>(value & 0x7F) | 0x80;
            value >>= 7;
            write(&byte, 1, false);
        }
        byte = static_cast<uint8_t>(value);
        write(&byte, 1, false);

        return *this;
    }
    
    byte_stream& operator<<(const std::string& src)
    {
        varint<size_t> len{src.length()};
        *this << len;
        write(reinterpret_cast<const uint8_t*>(src.data()), src.length(), false);
        return *this;
    }
    
    template <typename T>
    byte_stream& operator<<(const T& src)
    {
        write(reinterpret_cast<const uint8_t*>(&src), sizeof(T), std::is_integral<T>());
        return *this;
    }

    size_t read(uint8_t* dst, size_t len, bool reverse)
    {
        if (pos_ + len > data_.size())
            throw std::runtime_error("Trying to read past buffer of byte_stream.");

        uint8_t* end;
        if (reverse)
            end = std::reverse_copy(data_.begin() + pos_, data_.begin() + pos_ + len, dst);
        else
            end = std::copy(data_.begin() + pos_, data_.begin() + pos_ + len, dst);
        size_t rlen = static_cast<size_t>(end - dst);
        pos_ += rlen;
        return rlen;
    }
    
    template <typename T>
    byte_stream& operator>>(T& dst)
    {
        uint8_t* buffer = reinterpret_cast<uint8_t*>(&dst);
        size_t len = sizeof(T);
        
        read(buffer, len, std::is_integral<T>());
        
        return *this;
    }
    
    byte_stream& operator>>(bool& dst)
    {
        uint8_t byte;
        read(&byte, 1, false);
        dst = byte != 0;
        return *this;
    }
     
    template <typename T>
    byte_stream& operator>>(varint<T>& dst)
    {
        for (int i = 0; ; i++)
            if (!dst.append_byte(data_[pos_++], i))
                break;
        return *this;
    }

    byte_stream& operator>>(std::string& dst)
    {
        varint<size_t> len;
        *this >> len;
        if (len > 4096)
            throw std::runtime_error("trying to read an unusually large string from byte_stream.");
        
        dst.resize(len);
        read(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(dst.c_str())), len, false);
        return *this;
    }

    size_t pos() const
    {
        return pos_;
    }

    void pos(size_t pos)
    {
        pos_ = pos;
    }

    std::string hexdump() const
    {
        std::stringstream ss;
        ss << std::endl << std::hex;
        for (size_t i = 0; i < data_.size(); i++)
        {
            if (i > 0 && i % 16 == 0)
                ss << std::endl;
            unsigned int val = data_.at(i);
            ss << std::setw(2) << std::setfill('0') << val << " ";
        }
        return ss.str();
    }

private:
    size_t pos_;
    std::vector<uint8_t> data_;
};

#endif
