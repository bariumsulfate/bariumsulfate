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
#include <boost/bind.hpp>

#include <connection/connection.hpp>
#include <misc/log.hpp>

using namespace boost::asio;
using namespace boost::posix_time;

connection::connection(io_service& io, void*) :
    io_(io), socket_(io), client_(this), sending_(false), shutdown_{false},
    flush_timer_{io_}
{

}

void connection::start()
{
    socket_.set_option(boost::asio::ip::tcp::no_delay(true));
    remote_addr_ = socket_.remote_endpoint().address().to_string();
    
    read_header();
}

void connection::shutdown()
{
    DE(log::debug(log::dbg::connection, remote_addr_, "soft connection shutdown"));
    boost::lock_guard<boost::mutex> lock(mutex_);
    shutdown_ = true;
    if (sending_ == false && send_queue_.size() == 0)
        stop();
}
    
void connection::read_header()
{
    len_idx_ = 0;
    buffer_.resize(1);
    async_read(socket_, buffer(const_cast<std::vector<uint8_t>&>(buffer_.data())),
        boost::bind(&connection::handle_read_header, shared_from_this(),
        placeholders::error, placeholders::bytes_transferred));
}

void connection::send(std::shared_ptr<byte_stream>& data, bool force_flush /* = false */)
{
    DE(log::debug(log::dbg::connection, remote_addr_, "adding packet of", data->size(), "bytes to send queue"));
    DE(log::debug(log::dbg::packet, remote_addr_, "sending packet", data->hexdump()));
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (shutdown_)
        return;

    send_queue_.push_back(data);
    if (sending_)
        return;
    
    if (force_flush)
    {
        flush_queue();
    }
    else if (send_queue_.size() == 1)
    {
        // If we aren't already sending, and this is the first packet we queue we
        // start the flush_timer_
        flush_timer_.expires_from_now(milliseconds{10});
        flush_timer_.async_wait(boost::bind(&connection::handle_flush_timeout, shared_from_this()));
    }
}

// Read byte by byte until we have a complete header size (VarInt for sizes in
// your protocol seems kind of stupid to me but whatever)
void connection::handle_read_header(const boost::system::error_code& e, std::size_t)
{
    if (e)
    {
        DE(log::debug(log::dbg::connection, remote_addr_, "handle_read_header", e));
        stop();
        return;
    }
    
    if (len_.append_byte(buffer_.data()[0], len_idx_++))
    {
        async_read(socket_, buffer(const_cast<std::vector<uint8_t>&>(buffer_.data())),
            boost::bind(&connection::handle_read_header, shared_from_this(),
            placeholders::error, placeholders::bytes_transferred));
    }
    else
    {
        if (len_ > buffer_limit)
        {
            log::error(remote_addr_, "tried to send a packet of", len_, "bytes, max allowed is", buffer_limit);
            stop();
            return;
        }
        buffer_.resize(len_);
        
        async_read(socket_, buffer(const_cast<std::vector<uint8_t>&>(buffer_.data())),
            boost::bind(&connection::handle_read_body, shared_from_this(),
            placeholders::error, placeholders::bytes_transferred));
    }
}

void connection::handle_read_body(const boost::system::error_code& e, std::size_t)
{
    if (e)
    {
        DE(log::debug(log::dbg::connection, remote_addr_, "handle_read_body", e));
        stop();
        return;
    }

    buffer_.pos(0);
    DE(log::debug(log::dbg::packet, remote_addr_, "received packet", buffer_.hexdump()));
    client_.add_packet(buffer_);

    read_header();
}

void connection::handle_write(const boost::system::error_code& e, std::size_t)
{
    if (e)
    {
        DE(log::debug(log::dbg::connection, remote_addr_, "handle_write", e));
        stop();
        return;
    }
    
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (send_queue_.size())
    {
        flush_queue();
    }
    else
    {
        sending_ =  false;
        if (shutdown_)
            stop();
    }
}

void connection::handle_flush_timeout()
{
    boost::lock_guard<boost::mutex> lock(mutex_);
    flush_queue();
}

// This function dequeues a packet, sets the sending_ value and starts sending
// the header of the packet.
// To call this:
//   You MUST have ownership of mutex_
//   You MUST NOT call this if sending_ is true already
//   You MUST make sure send_queue_ is NOT empty
void connection::flush_queue()
{
    DE(log::debug(log::dbg::connection, remote_addr_, "flushing send queue with", send_queue_.size(), "packets"));
    sending_ = true;
    send_buffer_.clear();
    header_buffer_.clear();
    std::swap(send_queue_, send_buffer_);
    std::vector<const_buffer> scatter_buffer;
    for (auto& buf : send_buffer_)
    {
        size_t size_start = header_buffer_.size();
        varint<size_t>::bytes(buf->size(), header_buffer_);
        size_t size = header_buffer_.size() - size_start;

        scatter_buffer.emplace_back(&header_buffer_[size_start], size);
        scatter_buffer.emplace_back(&buf->data()[0], buf->size());
    }
    
    socket_.async_send(scatter_buffer, boost::bind(&connection::handle_write,
        shared_from_this(), placeholders::error, placeholders::bytes_transferred));
}

void connection::stop()
{
    DE(log::debug(log::dbg::connection, remote_addr_, "hard connection stop"));
    boost::system::error_code err;
    if (socket_.is_open())
        socket_.shutdown(ip::tcp::socket::shutdown_both, err);
}

