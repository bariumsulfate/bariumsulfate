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
#ifndef BARIUMSULFATE__SERVER__SERVER_HPP
#define BARIUMSULFATE__SERVER__SERVER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <server/io_service_pool.hpp>

/**
 * A tcp server that assigns each connection to a different io_service from
 * an io_service_pool. Based on boost::asio example code.
 *
 * \tparam D Connection data type. The connection data is passed to each
 *           connection in its constructor.
 * \tparam C The connection type. The connection type must be at least:
 * \code{.cpp}
 * class minimal_connection
 *   public boost::enable_shared_from_this<minimal_connection>,
 *   private boost::noncopyable
 * {
 * public:
 *   connection(boost::asio::io_service& io, D data) : socket_(io) {}
 *
 *   boost::asio::ip::tcp::socket& socket()
 *   {
 *     return socket_;
 *   }
 *
 *   void start()
 *   {
 *     // do async stuff here
 *   }
 *
 * private:
 *   boost::asio::ip::tcp::socket socket_;
 *   D data_;
 * };
 * \endcode
 */
template <class C, class D>
class server : private boost::noncopyable
{
public:
  /**
   * Construct a tcp server. Hostname lookup may block. The listening socket is created
   * and bound. Connections will be accepted as soon as io_service_pool::run is invoked.
   *
   * \param io      The io_service_pool that will be used to distribute the connecions.
   * \param host    The ip/host to listen on.
   * \param service The port to listen on.
   * \param data    Some data that will be passed on to each connection
   */
	server(io_service_pool& io, const std::string host, const std::string service, D data) :
		io_pool_(io), acceptor_(io.get_io_service()), connection_(), data_(data)
	{
		boost::asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
		boost::asio::ip::tcp::resolver::query query(host, service);
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();

		start_accept();
	}

private:
	typedef boost::shared_ptr<C> connection_ptr;

	void start_accept()
	{
		connection_.reset(new C(io_pool_.get_io_service(), data_));

		acceptor_.async_accept(connection_->socket(),
			boost::bind(&server::handle_accept, this,
				boost::asio::placeholders::error));
	}

	void handle_accept(const boost::system::error_code& e)
	{
		if (!e)
			connection_->start();
		start_accept();
	}

	io_service_pool&               io_pool_;
	boost::asio::ip::tcp::acceptor acceptor_;  
	connection_ptr                 connection_;
	D                              data_;
};

#endif // BARIUMSULFATE__SERVER__SERVER_HPP
