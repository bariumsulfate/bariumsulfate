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
#ifndef BARIUMSULFATE__CONNECTION__CLIENT_HPP
#define BARIUMSULFATE__CONNECTION__CLIENT_HPP

#include <vector>
#include <cstdint>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <protocol/byte_stream.hpp>

class connection;

class client
{
public:
    explicit client(connection* con);

    void add_packet(byte_stream& data);
    
    void unhandled_packet(byte_stream& data);
    void handle_handshake(byte_stream& data);

    void handle_status_request(byte_stream& data);
    void handle_status_ping(byte_stream& data);

    void handle_login_start(byte_stream& data);

private:
    enum class state { fresh, status, login, world };

    connection* connection_;
    // When the client_ goes into threaded mode (it is added to world and updates now
    // happen in world ticks) it gets a connection_->shared_from_this() pointer to make
    // sure the connection doesn't go away. When the client is removed from the world this
    // shared pointer is reset, allowing the connection to die. See connection.hpp for more
    // info on the connection object lifetime.
    boost::shared_ptr<connection> shared_connection_;
    
    state state_;
    
    // mutex_ protects the packets_ queue
    boost::mutex mutex_;
    std::vector<byte_stream> packets_;
};

#endif
