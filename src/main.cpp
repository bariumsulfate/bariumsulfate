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
#include <iostream>

#include <boost/asio.hpp>

#include <connection/connection.hpp>
#include <misc/log.hpp>
#include <server/io_service_pool.hpp>
#include <server/server.hpp>

int main()
{
    try
    {
        log::stream(std::cout, log::level::debug);
        log::stream("bariumsulfate.log", log::level::debug);
        log::notice("Starting Bariumsulfate");

        io_service_pool io_pool{2};
        server<connection, void*> s{io_pool, "0.0.0.0", "25565", nullptr};
        io_pool.run();
    }
    catch (...)
    {
        log::error("Unhandled exception reached main");
    }
}
