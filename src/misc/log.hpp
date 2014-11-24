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
#ifndef BARIUMSULFATE__MISC__LOG_HPP
#define BARIUMSULFATE__MISC__LOG_HPP

#include <fstream>
#include <boost/date_time.hpp>

// DE allows you to write expressions that do not get evaluated in release mode
// useful for excessive debug logging calls
#ifdef NDEBUG
#  define DE(x)
#else
#  define DE(x) (x)
#endif


class log
{
public:
    enum class level {none = -1, error, warning, notice, info, debug};
    enum class dbg {general, connection, packet};


    template <typename... T>
    static void error(T... t)
    {
        instance().write(level::error, dbg::general, t...);
    }
    
    template <typename... T>
    static void warning(T... t)
    {
        instance().write(level::warning, dbg::general, t...);
    }
    
    template <typename... T>
    static void notice(T... t)
    {
        instance().write(level::notice, dbg::general, t...);
    }
    
    template <typename... T>
    static void info(T... t)
    {
        instance().write(level::info, dbg::general, t...);
    }
    
    template <typename... T>
    static void debug(dbg d, T... t)
    {
        instance().write(level::debug, d, t...);
    }

    static void stream(std::ostream& stream, level l, uint64_t d = 0xFFFFFFFFFFFFFFFFLL)
    {
        boost::lock_guard<boost::mutex> lock(instance().mutex_);
        instance().logs_.push_back(stream_data{l, stream, d});
    }

    static void stream(const std::string& name, level l, uint64_t d = 0xFFFFFFFFFFFFFFFFLL)
    {
        std::shared_ptr<std::ofstream> file{new std::ofstream(name, std::fstream::app)};
        bool success = *file;

        if (success)
        {
            boost::lock_guard<boost::mutex> lock(instance().mutex_);
            instance().files_.push_back(file);
            instance().logs_.push_back(stream_data{l, *file, d});
        }
        else
            throw std::runtime_error("failed to open log file for appending. Wrong permissions?");
    }

private:
    struct stream_data
    {
        log::level level;
        std::ostream& stream;
        uint64_t debug_flags; 
    };

    log()
    {
    }

    log(const log&) = delete;
    log& operator=(const log&) = delete;
    
    void write(std::ostream& l)
    {
        l << std::endl;
    }

    template <typename H>
    void write(std::ostream& l, H head)
    {
        l << head << std::endl;
    }

    template <typename H, typename... T>
    void write(std::ostream& l, H head, T... tail)
    {
        l << head << " ";
        write(l, tail...);
    }
    
    template <typename... T>
    void write(level ll, dbg d, T... t)
    {
        boost::lock_guard<boost::mutex> lock(mutex_);
        std::string ts = timestamp(ll);
        for (auto l : logs_)
            if (l.level >= ll && should_log(ll, d, l.debug_flags)) 
                write(l.stream, ts, t...);
    }

    static std::string timestamp(level ll)
    {
        std::ostringstream s;
        s << "[" << boost::posix_time::microsec_clock::universal_time() << "]";
        switch (ll)
        {
        case log::level::error:   s << "  <Error>   "; break;
        case log::level::warning: s << "  <Warning> "; break;
        case log::level::notice:  s << "  <Notice>  "; break;
        case log::level::info:    s << "  <Info>    "; break;
        case log::level::debug:   s << "  <Debug>   "; break;
        case log::level::none: break;
        }

        return s.str();
    }

    static bool should_log(level ll, dbg d, uint64_t flags)
    {
        if (ll != level::debug)
            return true;

        return (flags & (1 << static_cast<int>(d))) != 0;
    }
    
    static log& instance()
    {
        static log l;
        return l;
    }
    
    std::vector<stream_data> logs_;
    std::vector<std::shared_ptr<std::ofstream>> files_;
    boost::mutex mutex_;
};

#endif
