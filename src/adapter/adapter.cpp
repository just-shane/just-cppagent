//
// Copyright Copyright 2009-2021, AMT – The Association For Manufacturing Technology (“AMT”)
// All rights reserved.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#define __STDC_LIMIT_MACROS 1
#include "adapter/adapter.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>

#include <boost/log/attributes.hpp>
#include <boost/log/trivial.hpp>

#include "configuration/config_options.hpp"
#include "device_model/device.hpp"

using namespace std;
using namespace std::literals;
using namespace date::literals;

namespace mtconnect
{
  namespace adapter
  {
    // Adapter public methods
    Adapter::Adapter(const string &server, const unsigned int port, const ConfigOptions &options,
                     std::unique_ptr<AdapterPipeline> &pipeline)
      : Connector(server, port, 60s),
        m_pipeline(std::move(pipeline)),
        m_running(true),
        m_reconnectInterval {10000ms},
        m_options(options)
    {
      auto timeout = options.find(configuration::LegacyTimeout);
      if (timeout != options.end())
        m_legacyTimeout = get<Seconds>(timeout->second);

      stringstream url;
      url << "shdr://" << server << ':' << port;
      m_url = url.str();

      stringstream identity;
      identity << '_' << server << '_' << port;
      m_identity = identity.str();
      m_options[configuration::AdapterIdentity] = m_identity;
      m_handler = m_pipeline->makeHandler();
      if (m_pipeline->hasContract())
        m_pipeline->build(m_options);
    }

    void Adapter::processData(const string &data)
    {
      if (m_terminator)
      {
        if (data == *m_terminator)
        {
          if (m_handler && m_handler->m_processData)
            m_handler->m_processData(m_body.str(), getIdentity());
          m_terminator.reset();
          m_body.str("");
        }
        else
        {
          m_body << endl << data;
        }

        return;
      }

      if (size_t multi = data.find("--multiline--"); multi != string::npos)
      {
        m_body.str("");
        m_body << data.substr(0, multi);
        m_terminator = data.substr(multi);
        return;
      }

      if (m_handler && m_handler->m_processData)
        m_handler->m_processData(data, getIdentity());
    }

    void Adapter::stop()
    {
      BOOST_LOG_NAMED_SCOPE("input.adapter.stop");
      // Will stop threaded object gracefully Adapter::thread()
      BOOST_LOG_TRIVIAL(debug) << "Waiting for adapter to stop: " << m_url;
      m_running = false;
      close();
      if (m_thread.joinable())
        m_thread.join();
      BOOST_LOG_TRIVIAL(debug) << "Adapter exited: " << m_url;
    }

    inline bool is_true(const std::string &value) { return value == "yes" || value == "true"; }

    void Adapter::protocolCommand(const std::string &data)
    {
      static auto pattern = regex("\\*[ ]*([^:]+):[ ]*(.+)");
      smatch match;

      if (std::regex_match(data, match, pattern))
      {
        auto command = match[1].str();
        auto value = match[2].str();

        ConfigOptions options;

        if (command == "conversionRequired")
          options[configuration::ConversionRequired] = is_true(value);
        else if (command == "relativeTime")
          options[configuration::RelativeTime] = is_true(value);
        else if (command == "realTime")
          options[configuration::RealTime] = is_true(value);
        else if (command == "device")
          options[configuration::Device] = value;
        else if (command == "shdrVersion")
          options[configuration::ShdrVersion] = value;

        if (options.size() > 0)
          setOptions(options);
        else if (m_handler && m_handler->m_command)
          m_handler->m_command(data, getIdentity());
      }
    }

    // Adapter private methods
    void Adapter::thread()
    {
      BOOST_LOG_NAMED_SCOPE("input.adapter");
      while (m_running)
      {
        try
        {
          // Start the connection to the socket
          connect();

          // make sure we're closed...
          close();
        }
        catch (std::invalid_argument &err)
        {
          BOOST_LOG_TRIVIAL(error) << "Adapter for " << m_url
                   << "'s thread threw an argument error, stopping adapter: " << err.what();
          stop();
        }
        catch (std::exception &err)
        {
          BOOST_LOG_TRIVIAL(error) << "Adapter for " << m_url
                   << "'s thread threw an exceotion, stopping adapter: " << err.what();
          stop();
        }
        catch (...)
        {
          BOOST_LOG_TRIVIAL(error) << "Thread for adapter " << m_url
                   << "'s thread threw an unhandled exception, stopping adapter";
          stop();
        }

        if (!m_running)
          break;

        // Try to reconnect every 10 seconds
        BOOST_LOG_TRIVIAL(info) << "Will try to reconnect in " << m_reconnectInterval.count()
                 << " milliseconds";
        this_thread::sleep_for(m_reconnectInterval);
      }
      BOOST_LOG_TRIVIAL(info) << "Adapter thread stopped";
    }
  }  // namespace adapter
}  // namespace mtconnect
