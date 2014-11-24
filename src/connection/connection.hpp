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
#ifndef BARIUMSULFATE__CONNECTION__CONNECTION_HPP
#define BARIUMSULFATE__CONNECTION__CONNECTION_HPP

#include <array>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include <connection/client.hpp>
#include <protocol/byte_stream.hpp>
#include <protocol/varint.hpp>

/* 
 Lifetime of this object:
 ------------------------
    - The server creates a connection object for each connection it accepts
    - As long as there are pending io operations on the network this object is kept alive by
       the shared_from_this() pointer
    - While the client_ object handles packets instantly, it will NOT keep a shared pointer to the 
       connection. If the remote endpoint ends the connection at this point the connection object
       and its client_ member object will go away. At this point ALL client_ operations are handled
       instantly in the network thread and no other thread is aware of the connection or client_ object.
    - After the client_ object is added to the world and packets are handled in world ticks
       the client will assume ownership of the connection_ and keep it alive. The world ticks will
       then destroy the connection when the client is removed from the world. This also happens on
       the next tick if the client shuts down the connection.
*/


class connection :
    public boost::enable_shared_from_this<connection>,
    private boost::noncopyable
{
public:
    explicit connection(boost::asio::io_service& io, void*);
    void start();
    void shutdown();

    void send(std::shared_ptr<byte_stream>& data, bool force_flush = false);
    
    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }
    
    const std::string& address()
    {
        return remote_addr_;
    }

private:
    // Maximum packet size we are willing to accept
    static constexpr size_t buffer_limit = 8192;
    static constexpr size_t buffer_initial_size = 512;
    static constexpr size_t header_buffer_size = 4;

    void read_header();

    void handle_read_header(const boost::system::error_code& e, std::size_t bytes_transferred);
    void handle_read_body(const boost::system::error_code& e, std::size_t bytes_transferred);
    void handle_write(const boost::system::error_code& e, std::size_t bytes_transferred);

    void handle_flush_timeout();
    void flush_queue(); // You MAY NOT call this function unless you have ownership of mutex_

    void stop();

    boost::asio::io_service& io_;
    boost::asio::ip::tcp::socket socket_;
    
    varint<size_t> len_;
    int len_idx_;
    byte_stream buffer_;

    client client_;
    
    // The mutex protects sending_, shutdown_ and send_queue_
    // While sending_ is true send_buffer_ and header_buffer_ should not be
    // altered in any way.
    boost::mutex mutex_;
    std::vector<std::shared_ptr<byte_stream>> send_queue_;
    std::vector<std::shared_ptr<byte_stream>> send_buffer_;
    std::vector<uint8_t> header_buffer_;
    bool sending_;
    bool shutdown_;

    std::string remote_addr_;

    boost::asio::deadline_timer flush_timer_;
};

#endif
