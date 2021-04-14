// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include "http_server/routing.hpp"
#include "http_server/response.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace std;
using namespace mtconnect;
using namespace mtconnect::http_server;
using verb = boost::beast::http::verb;

class RoutingTest : public testing::Test
{
 protected:
  void SetUp() override
  {
    m_socket = make_unique<boost::asio::ip::tcp::socket>(m_context);
  }

  void TearDown() override
  {
    m_socket.reset();
  }

  std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;
  boost::asio::io_context m_context;
  
  const Routing::Function m_func { [](const RequestPtr ) { return true; } };

};

TEST_F(RoutingTest, TestSimplePattern)
{
  RequestPtr request = make_shared<Request>();
  request->m_verb = verb::get;
  
  Routing probe(verb::get, "/probe", m_func);
  EXPECT_EQ(0, probe.getPathParameters().size());
  EXPECT_EQ(0, probe.getQueryParameters().size());
  
  request->m_path = "/probe";
  ASSERT_TRUE(probe.matches(request));
  request->m_path = "/probe";
  ASSERT_TRUE(probe.matches(request));
  request->m_verb = verb::put;
  ASSERT_FALSE(probe.matches(request));

  Routing probeWithDevice(verb::get, "/{device}/probe", m_func);
  ASSERT_EQ(1, probeWithDevice.getPathParameters().size());
  EXPECT_EQ(0, probe.getQueryParameters().size());
  EXPECT_EQ("device", probeWithDevice.getPathParameters().front().m_name);
  
  request->m_verb = verb::get;
  request->m_path = "/ABC123/probe";
  ASSERT_TRUE(probeWithDevice.matches(request));
  ASSERT_EQ("ABC123", get<string>(request->m_parameters["device"]));
}

TEST_F(RoutingTest, TestComplexPatterns)
{
  RequestPtr request = make_shared<Request>();
  request->m_verb = verb::get;

  Routing r(verb::get, "/asset/{assets}", m_func);
  ASSERT_EQ(1, r.getPathParameters().size());
  EXPECT_EQ("assets", r.getPathParameters().front().m_name);

  request->m_path = "/asset/A1,A2,A3";
  ASSERT_TRUE(r.matches(request));
  ASSERT_EQ("A1,A2,A3", get<string>(request->m_parameters["assets"]));
  request->m_path = "/ABC123/probe";
  ASSERT_FALSE(r.matches(request));
}

TEST_F(RoutingTest, TestCurrentAtQueryParameter)
{
  Routing r(verb::get, "/{device}/current?at={unsigned_integer}", m_func);
  ASSERT_EQ(1, r.getPathParameters().size());
  ASSERT_EQ(1, r.getQueryParameters().size());
  
  auto pp = r.getPathParameters().front();
  EXPECT_EQ("device", pp.m_name);
  EXPECT_EQ(PATH, pp.m_part);

  auto &qp = *r.getQueryParameters().begin();
  EXPECT_EQ("at", qp.m_name);
  EXPECT_EQ(UNSIGNED_INTEGER, qp.m_type);
  EXPECT_EQ(QUERY, qp.m_part);
  EXPECT_TRUE(holds_alternative<monostate>(qp.m_default));
}

TEST_F(RoutingTest, TestSampleQueryParameters)
{
  Routing r(verb::get, "/{device}/sample?from={unsigned_integer}&"
            "interval={double}&count={integer:100}&"
            "heartbeat={double:10000}", m_func);
  ASSERT_EQ(1, r.getPathParameters().size());
  ASSERT_EQ(4, r.getQueryParameters().size());
  
  auto pp = r.getPathParameters().front();
  EXPECT_EQ("device", pp.m_name);
  EXPECT_EQ(PATH, pp.m_part);

  auto qp = r.getQueryParameters().begin();
  EXPECT_EQ("count", qp->m_name);
  EXPECT_EQ(INTEGER, qp->m_type);
  EXPECT_EQ(QUERY, qp->m_part);
  EXPECT_TRUE(holds_alternative<int32_t>(qp->m_default));
  EXPECT_EQ(100, get<int32_t>(qp->m_default));
  qp++;

  EXPECT_EQ("from", qp->m_name);
  EXPECT_EQ(UNSIGNED_INTEGER, qp->m_type);
  EXPECT_EQ(QUERY, qp->m_part);
  EXPECT_TRUE(holds_alternative<monostate>(qp->m_default));
  qp++;
  
  EXPECT_EQ("heartbeat", qp->m_name);
  EXPECT_EQ(DOUBLE, qp->m_type);
  EXPECT_EQ(QUERY, qp->m_part);
  EXPECT_TRUE(holds_alternative<double>(qp->m_default));
  EXPECT_EQ(10000.0, get<double>(qp->m_default));
  qp++;

  EXPECT_EQ("interval", qp->m_name);
  EXPECT_EQ(DOUBLE, qp->m_type);
  EXPECT_EQ(QUERY, qp->m_part);
  EXPECT_TRUE(holds_alternative<monostate>(qp->m_default));
}

TEST_F(RoutingTest, TestQueryParameterMatch)
{
  RequestPtr request = make_shared<Request>();
  request->m_verb = verb::get;

  Routing r(verb::get, "/{device}/sample?from={unsigned_integer}&"
            "interval={double}&count={integer:100}&"
            "heartbeat={double:10000}", m_func);
  ASSERT_EQ(1, r.getPathParameters().size());
  ASSERT_EQ(4, r.getQueryParameters().size());

  request->m_path = "/ABC123/sample";
  ASSERT_TRUE(r.matches(request));
  ASSERT_EQ("ABC123", get<string>(request->m_parameters["device"]));
  ASSERT_EQ(100, get<int32_t>(request->m_parameters["count"]));
  ASSERT_EQ(10000.0, get<double>(request->m_parameters["heartbeat"]));

  request->m_path = "/ABC123/sample";
  request->m_query = {{ "count", "1000"}, {"from", "12345"}};
  ASSERT_TRUE(r.matches(request));
  ASSERT_EQ("ABC123", get<string>(request->m_parameters["device"]));
  ASSERT_EQ(1000, get<int32_t>(request->m_parameters["count"]));
  ASSERT_EQ(12345, get<uint64_t>(request->m_parameters["from"]));
  ASSERT_EQ(10000.0, get<double>(request->m_parameters["heartbeat"]));

  request->m_query = {{ "count", "1000"}, {"from", "12345"}, { "dummy" , "1" }
  };
  ASSERT_TRUE(r.matches(request));
  ASSERT_EQ("ABC123", get<string>(request->m_parameters["device"]));
  ASSERT_EQ(1000, get<int32_t>(request->m_parameters["count"]));
  ASSERT_EQ(12345, get<uint64_t>(request->m_parameters["from"]));
  ASSERT_EQ(10000.0, get<double>(request->m_parameters["heartbeat"]));
  ASSERT_EQ(request->m_parameters.end(), request->m_parameters.find("dummy"));
}

TEST_F(RoutingTest, TestQueryParameterError)
{
  Routing r(verb::get, "/{device}/sample?from={unsigned_integer}&"
            "interval={double}&count={integer:100}&"
            "heartbeat={double:10000}", m_func);
  RequestPtr request = make_shared<Request>();
  request->m_verb = verb::get;
  request->m_path = "/ABC123/sample";
  request->m_query = {{ "count", "xxx"} };
  ASSERT_THROW(r.matches(request),
               ParameterError);
}

TEST_F(RoutingTest, RegexPatterns)
{
  Routing r(verb::get, regex("/.+"), m_func);
  RequestPtr request = make_shared<Request>();
  request->m_verb = verb::get;
  request->m_path = "/some random stuff";
  ASSERT_TRUE(r.matches(request));

  Routing no(verb::get, regex("/.+"), [](const RequestPtr) { return false; });
  ASSERT_FALSE(no.matches(request));
}

TEST_F(RoutingTest, simple_put_with_trailing_slash)
{
  Routing r(verb::put, "/{device}", m_func);
  RequestPtr request = make_shared<Request>();
  request->m_verb = verb::put;
  request->m_path = "/ADevice";
  ASSERT_TRUE(r.matches(request));
  ASSERT_EQ("ADevice", get<string>(request->m_parameters["device"]));

  request->m_path = "/ADevice/";
  ASSERT_TRUE(r.matches(request));
  ASSERT_EQ("ADevice", get<string>(request->m_parameters["device"]));
}
