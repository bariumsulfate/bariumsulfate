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
#ifndef BARIUMSULFATE__PROTOCOL__HANDLERS_HPP
#define BARIUMSULFATE__PROTOCOL__HANDLERS_HPP

#include <connection/client.hpp>
#include <vector>

constexpr int supported_protocol_version = 47;

typedef void (client::*handler_fn)(byte_stream&);

enum class handler_type {instant, delayed};

struct handler_info {
    handler_type type;
    handler_fn fn;
};

std::vector<handler_info> fresh_handlers{
    {handler_type::instant, &client::handle_handshake},       /* 0x00 - handshake */
};

std::vector<handler_info> login_handlers{
    {handler_type::instant, &client::handle_login_start},     /* 0x00 - login start*/
};

std::vector<handler_info> status_handlers{
    {handler_type::instant, &client::handle_status_request},  /* 0x00 - status request */
    {handler_type::instant, &client::handle_status_ping},     /* 0x01 - status ping */
};

std::vector<handler_info> game_handlers{
    {handler_type::instant, &client::unhandled_packet},       /* 0x00 - keep alive */
    {handler_type::instant, &client::unhandled_packet},       /* 0x01 - chat message */
    {handler_type::instant, &client::unhandled_packet},       /* 0x02 - use entity */
    {handler_type::instant, &client::unhandled_packet},       /* 0x03 - player ground state */
    {handler_type::instant, &client::unhandled_packet},       /* 0x04 - player position */
    {handler_type::instant, &client::unhandled_packet},       /* 0x05 - player look */
    {handler_type::instant, &client::unhandled_packet},       /* 0x06 - player position & look */
    {handler_type::instant, &client::unhandled_packet},       /* 0x07 - player dig */
    {handler_type::instant, &client::unhandled_packet},       /* 0x08 - player place block */
    {handler_type::instant, &client::unhandled_packet},       /* 0x09 - player switch current item */
    {handler_type::instant, &client::unhandled_packet},       /* 0x0A - animation */
    {handler_type::instant, &client::unhandled_packet},       /* 0x0B - entity action */
    {handler_type::instant, &client::unhandled_packet},       /* 0x0C - steer vehicle */
    {handler_type::instant, &client::unhandled_packet},       /* 0x0D - close window */
    {handler_type::instant, &client::unhandled_packet},       /* 0x0E - click window */
    {handler_type::instant, &client::unhandled_packet},       /* 0x0F - confirm transaction */
    {handler_type::instant, &client::unhandled_packet},       /* 0x10 - creative inventory action */
    {handler_type::instant, &client::unhandled_packet},       /* 0x11 - enchant item */
    {handler_type::instant, &client::unhandled_packet},       /* 0x12 - update sign */
    {handler_type::instant, &client::unhandled_packet},       /* 0x13 - player flags */
    {handler_type::instant, &client::unhandled_packet},       /* 0x14 - tab complete */
    {handler_type::instant, &client::unhandled_packet},       /* 0x15 - client settings */
    {handler_type::instant, &client::unhandled_packet},       /* 0x16 - client status */
    {handler_type::instant, &client::unhandled_packet},       /* 0x17 - plugin message */
    {handler_type::instant, &client::unhandled_packet},       /* 0x18 - spectate */
    {handler_type::instant, &client::unhandled_packet},       /* 0x19 - resource pack status */
};

#endif
