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
#include <connection/client.hpp>
#include <connection/connection.hpp>
#include <misc/log.hpp>
#include <protocol/handlers.hpp>
#include <protocol/varint.hpp>

client::client(connection* con) :
    connection_(con), state_(state::fresh)
{

}

void client::add_packet(byte_stream& data)
try
{
    varint<unsigned int> opcode;
    data >> opcode;
    
    DE(log::debug(log::dbg::connection, connection_->address(),
        "received packet with opcode", opcode, "in state",
        static_cast<int>(state_)));
    
    std::vector<handler_info>* handlers = nullptr;
    switch (state_)
    {
    case state::fresh:  handlers = &fresh_handlers; break;
    case state::status: handlers = &status_handlers; break;
    case state::login:  handlers = &login_handlers; break;
    case state::world:  handlers = &game_handlers; break;
    }

    if (opcode >= handlers->size())
    {
        log::error(connection_->address(), "opcode", opcode, "is not valid in state", static_cast<int>(state_));
        connection_->shutdown();
        return;
    }
    
    auto info = (*handlers)[opcode];
    if (info.type == handler_type::instant)
    {
        auto handler = std::bind(handlers->at(opcode).fn, this, data);
        handler();
    }
    else
    {
        boost::lock_guard<boost::mutex> lock(mutex_);
        data.pos(0);
        packets_.emplace_back(std::move(data));
    }
}
catch (std::exception& e)
{
    log::error(connection_->address(), "client::add_packet exception:", e.what(), data.hexdump());
    connection_->shutdown();
}

void client::unhandled_packet(byte_stream& data)
{
    varint<unsigned int> opcode;
    data.pos(0);
    data >> opcode;

    log::error(connection_->address(), "unhandled packet with opcode", opcode, "in state", static_cast<int>(state_));
}

void client::handle_handshake(byte_stream& data)
{
    varint<int> protocol;
    std::string host;
    uint16_t port;
    varint<int> next_state;
    
    data >> protocol >> host >> port >> next_state;

    if (protocol != supported_protocol_version)
    {
        connection_->shutdown();
    }

    switch (next_state)
    {
    case 1:
        state_ = state::status;
        break;
    case 2:
        state_ = state::login;
        break;
    default:
        connection_->shutdown();
        return;
    }
}

const std::string status_value = R"({"description":"Bariumsulfate","players":{"max":20,"online":0},"version":{"name":"1.8","protocol":47}})";

void client::handle_status_request(byte_stream&)
{
    std::shared_ptr<byte_stream> response{new byte_stream};
    *response << varint<unsigned int>(0) << status_value;
    connection_->send(response);
}

void client::handle_status_ping(byte_stream& data)
{
    uint64_t ping_value;
    data >> ping_value;
    
    std::shared_ptr<byte_stream> response{new byte_stream};
    *response << varint<unsigned int>(1) << ping_value;
    connection_->send(response, true);
}

void client::handle_login_start(byte_stream& data)
{
    std::string username;
    data >> username;
    
    DE(log::info(connection_->address(), "login request for user", username));

    std::shared_ptr<byte_stream> response{new byte_stream};
    *response << varint<unsigned int>(2) << std::string("d99974de-50e1-4861-bb7a-60e0e59cf611") << username;
    connection_->send(response);
    
    std::shared_ptr<byte_stream> join{new byte_stream};
    *join << varint<unsigned int>(1)    // join game
        << static_cast<int32_t>(0)      // entity id
        << static_cast<uint8_t>(0)      // game mode (survival)
        << static_cast<int8_t>(0)       // dimension (overworld)
        << static_cast<uint8_t>(0)      // difficulty (peaceful)
        << static_cast<uint8_t>(10)     // max players
        << std::string("flat")
        << true;
    connection_->send(join);
}
