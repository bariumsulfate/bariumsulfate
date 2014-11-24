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
#ifndef BARIUMSULFATE__SERVER__IO_SERVICE_POOL_HPP
#define BARIUMSULFATE__SERVER__IO_SERVICE_POOL_HPP

#include <vector>

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
 
// This class is heavily based on example code provided by the boost::asio docs

/**
 * a pool of io_service objects that each run in their own thread. The pool
 * hands out io_service objects in a round-robin way to spread out your work
 * over the threads.
 */
class io_service_pool :
	private boost::noncopyable
{
public:
	/**
	 * Construct the io_service_pool
	 * \param[in] count Number of io_services and accompanying threads that get created.
	 *                  Only the io_services are created in the constructor, threads are
	 *                  created when io_service_pool::run() is invoked.
	 */
	explicit io_service_pool(std::size_t count) : next_(0)
	{
		if (count == 0)
			count = 1;

		for (std::size_t i = 0; i < count; ++i)
		{
			io_service_ptr io(new boost::asio::io_service);
			work_ptr work(new boost::asio::io_service::work(*io));
			work_pool_.push_back(work);
			io_service_pool_.push_back(io);
		}
	}

	~io_service_pool()
	{
		// Ensure that the work is destroyed before the io_service objects
		// become invalid
		for (std::size_t i = 0; i < work_pool_.size(); ++i)
			work_pool_[i].reset();
	}
	
	/**
	 * Run the pool, this creates a new thread for each io_service in the pool
	 * and blocks until all threads complete running. To interrupt run  you can
	 * invoke io_service_pool::stop().
	 */
	void run()
	{
		std::vector<boost::shared_ptr<boost::thread>> threads;

		for (std::size_t i = 0; i < io_service_pool_.size(); ++i)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(
					boost::bind(&boost::asio::io_service::run, io_service_pool_[i])));
			threads.push_back(thread);
		}
		for (std::size_t i = 0; i < threads.size(); ++i)
			threads[i]->join();
	}
	
	/**
	 * Get an io_service object from the pool. The io_service objects are handed
	 * out in a round robin manner.
	 *
	 * \note This function is not thread safe and you should only invoke it from
	 *       one thread. It does not have to be the same thread that invokes
	 *       io_service_pool::run
	 */
	boost::asio::io_service& get_io_service()
	{
		boost::asio::io_service& io = *io_service_pool_[next_];

		next_++;
		if (next_ == io_service_pool_.size())
			next_ = 0;

		return io;
	}

	/**
	 * stops all io_service objects from running, this will also shut down the
	 * threads created for them in io_service
	 *
	 * \note This function is not thread safe and you should only invoke it from
	 *       one thread. It does not have to be the same thread that invokes
	 *       io_service_pool::run
	 */
	void stop()
	{
		for (std::size_t i = 0; i < io_service_pool_.size(); ++i)
			io_service_pool_[i]->stop();
	}

private:
	typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;
	typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;

	std::vector<work_ptr> work_pool_;
	std::vector<io_service_ptr> io_service_pool_;

	std::size_t next_;
};

#endif // BARIUMSULFATE__SERVER__IO_SERVICE_POOL_HPP
