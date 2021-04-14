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

// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include "agent.hpp"
#include <nlohmann/json.hpp>
#include "agent_test_helper.hpp"
#include "http_server/server.hpp"

#include <cstdio>

using namespace std;
using namespace std::chrono;
using namespace mtconnect;
using namespace mtconnect::http_server;
namespace beast = boost::beast;
namespace http = beast::http;

void AgentTestHelper::makeRequest(const char *file, int line,
                                  boost::beast::http::verb verb, const std::string &body,
                                  const mtconnect::http_server::QueryMap &aQueries,
                                  const char *path)
{
  m_dispatched = false;
  m_out.str("");
  m_request->m_verb = verb;
  m_request->m_query = aQueries;
  m_request->m_body = body;
  m_request->m_parameters.clear();
  
  if (path != nullptr)
    m_request->m_path = path;
  
  ASSERT_FALSE(m_request->m_path.empty());
  m_dispatched = m_agent->getServer()->dispatch(m_request);
}


void AgentTestHelper::responseHelper(const char *file, int line,
                                     const QueryMap &aQueries,
                                     xmlDocPtr *doc, const char *path)
{
  makeRequest(file, line, http::verb::get, "", aQueries, path);
  if (ends_with(m_session->m_mimeType, "xml"))
    *doc = xmlParseMemory(m_session->m_body.c_str(), m_session->m_body.size());
}

void AgentTestHelper::responseStreamHelper(const char *file, int line,
                                     const QueryMap &aQueries,
                                     xmlDocPtr *doc, const char *path)
{
  makeRequest(file, line, http::verb::get, "", aQueries, path);
  if (ends_with(m_session->m_chunkMimeType, "xml"))
    *doc = xmlParseMemory(m_session->m_chunkBody.c_str(), m_session->m_chunkBody.size());
}

void AgentTestHelper::putResponseHelper(const char *file, int line, const string &body,
                                        const QueryMap &aQueries, xmlDocPtr *doc,
                                        const char *path)
{
  makeRequest(file, line, http::verb::put, body, aQueries, path);
  if (ends_with(m_session->m_mimeType, "xml"))
    *doc = xmlParseMemory(m_session->m_body.c_str(), m_session->m_body.size());
}

void AgentTestHelper::deleteResponseHelper(const char *file, int line,
                                           const QueryMap &aQueries, xmlDocPtr *doc,
                                           const char *path)
{
  makeRequest(file, line, http::verb::delete_, "", aQueries, path);
  if (ends_with(m_session->m_mimeType, "xml"))
      *doc = xmlParseMemory(m_session->m_body.c_str(), m_session->m_body.size());
}

void AgentTestHelper::responseHelper(const char *file, int line,
                                     const QueryMap &aQueries,
                                     nlohmann::json &doc,
                                     const char *path)
{
  makeRequest(file, line, http::verb::get, "", aQueries, path);
  doc = nlohmann::json::parse(m_session->m_body);
}
