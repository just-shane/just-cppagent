//
// Copyright Copyright 2012, System Insights, Inc.
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

#include <stdio.h>
#include "data_set_test.hpp"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(DataSetTest);

using namespace std;

void DataSetTest::setUp()
{
  // Create an agent with only 16 slots and 8 data items.
  m_agent = nullptr;
  m_checkpoint = nullptr;
  m_agent = new Agent("../samples/data_set.xml", 4, 4);
  m_agentId = int64ToString(getCurrentTimeInSec());
  m_adapter = nullptr;
  m_checkpoint = new Checkpoint();
  m_agentTestHelper.m_agent = m_agent;
  
  std::map<string, string> attributes;
  auto device = m_agent->getDeviceByName("LinuxCNC");
  m_dataItem1 = device->getDeviceDataItem("v1");
}


void DataSetTest::tearDown()
{
  if (m_agent != nullptr) {
    delete m_agent; m_agent = nullptr;
  }
  if (m_checkpoint != nullptr) {
    delete m_checkpoint; m_checkpoint = nullptr;
  }
  if (m_dataItem1 != nullptr) {
    delete m_dataItem1; m_dataItem1 = nullptr;
  }
}

void DataSetTest::testDataItem()
{
  CPPUNIT_ASSERT(m_dataItem1->isDataSet());
  auto &attrs = m_dataItem1->getAttributes();
  
  CPPUNIT_ASSERT_EQUAL((string) "DATA_SET", attrs.at("representation"));
  CPPUNIT_ASSERT_EQUAL((string) "VariableDataSet", m_dataItem1->getElementName());
}

void DataSetTest::testInitialSet()
{
  string value("a:1 b:2 c:3 d:4");
  auto ce = new ComponentEvent(*m_dataItem1, 2, "time", value);
  
  CPPUNIT_ASSERT_EQUAL((size_t) 4, ce->getDataSet().size());
  auto &al = ce->getAttributes();
  std::map<string, string> attrs;
  
  for (const auto &attr : al)
    attrs[attr.first] = attr.second;

  CPPUNIT_ASSERT_EQUAL((string) "4", attrs.at("sampleCount"));
  
  auto map1 = ce->getDataSet();
  CPPUNIT_ASSERT_EQUAL((string) "1", map1.at("a"));
  CPPUNIT_ASSERT_EQUAL((string) "2", map1.at("b"));
  CPPUNIT_ASSERT_EQUAL((string) "3", map1.at("c"));
  CPPUNIT_ASSERT_EQUAL((string) "4", map1.at("d"));

  m_checkpoint->addComponentEvent(ce);
  auto c2 = *m_checkpoint->getEventPtr("v1");
  auto al2 = c2->getAttributes();
  
  attrs.clear();
  for (const auto &attr : al2)
    attrs[attr.first] = attr.second;

  CPPUNIT_ASSERT_EQUAL((string) "4", attrs.at("sampleCount"));
  
  auto map2 = c2->getDataSet();
  CPPUNIT_ASSERT_EQUAL((string) "1", map2.at("a"));
  CPPUNIT_ASSERT_EQUAL((string) "2", map2.at("b"));
  CPPUNIT_ASSERT_EQUAL((string) "3", map2.at("c"));
  CPPUNIT_ASSERT_EQUAL((string) "4", map2.at("d"));
  
  ce->unrefer();
}

void DataSetTest::testUpdateOneElement()
{
  string value("a:1 b:2 c:3 d:4");
  ComponentEventPtr ce(new ComponentEvent(*m_dataItem1, 2, "time", value));
  m_checkpoint->addComponentEvent(ce);

  auto cecp = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 4, cecp->getDataSet().size());

  string value2("c:5");
  ComponentEventPtr ce2(new ComponentEvent(*m_dataItem1, 2, "time", value2));
  m_checkpoint->addComponentEvent(ce2);

  auto ce3 = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 4, ce3->getDataSet().size());

  auto map1 = ce3->getDataSet();
  CPPUNIT_ASSERT_EQUAL((string) "1", map1.at("a"));
  CPPUNIT_ASSERT_EQUAL((string) "2", map1.at("b"));
  CPPUNIT_ASSERT_EQUAL((string) "5", map1.at("c"));
  CPPUNIT_ASSERT_EQUAL((string) "4", map1.at("d"));
  
  string value3("e:6");
  ComponentEventPtr ce4(new ComponentEvent(*m_dataItem1, 2, "time", value3));
  m_checkpoint->addComponentEvent(ce4);
  
  auto ce5 = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 5, ce5->getDataSet().size());
  
  auto map2 = ce5->getDataSet();
  CPPUNIT_ASSERT_EQUAL((string) "1", map2.at("a"));
  CPPUNIT_ASSERT_EQUAL((string) "2", map2.at("b"));
  CPPUNIT_ASSERT_EQUAL((string) "5", map2.at("c"));
  CPPUNIT_ASSERT_EQUAL((string) "4", map2.at("d"));
  CPPUNIT_ASSERT_EQUAL((string) "6", map2.at("e"));
}

void DataSetTest::testUpdateMany()
{
  string value("a:1 b:2 c:3 d:4");
  ComponentEventPtr ce(new ComponentEvent(*m_dataItem1, 2, "time", value));
  m_checkpoint->addComponentEvent(ce);
  
  auto cecp = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 4, cecp->getDataSet().size());
  
  string value2("c:5 e:6");
  ComponentEventPtr ce2(new ComponentEvent(*m_dataItem1, 2, "time", value2));
  m_checkpoint->addComponentEvent(ce2);
  
  auto ce3 = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 5, ce3->getDataSet().size());
  
  auto map1 = ce3->getDataSet();
  CPPUNIT_ASSERT_EQUAL((string) "1", map1.at("a"));
  CPPUNIT_ASSERT_EQUAL((string) "2", map1.at("b"));
  CPPUNIT_ASSERT_EQUAL((string) "5", map1.at("c"));
  CPPUNIT_ASSERT_EQUAL((string) "4", map1.at("d"));
  CPPUNIT_ASSERT_EQUAL((string) "6", map1.at("e"));

  string value3("e:7 a:8 f:9");
  ComponentEventPtr ce4(new ComponentEvent(*m_dataItem1, 2, "time", value3));
  m_checkpoint->addComponentEvent(ce4);
  
  auto ce5 = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 6, ce5->getDataSet().size());
  
  auto map2 = ce5->getDataSet();
  CPPUNIT_ASSERT_EQUAL((string) "8", map2.at("a"));
  CPPUNIT_ASSERT_EQUAL((string) "2", map2.at("b"));
  CPPUNIT_ASSERT_EQUAL((string) "5", map2.at("c"));
  CPPUNIT_ASSERT_EQUAL((string) "4", map2.at("d"));
  CPPUNIT_ASSERT_EQUAL((string) "7", map2.at("e"));
  CPPUNIT_ASSERT_EQUAL((string) "9", map2.at("f"));
}

void DataSetTest::testReset()
{
  string value("a:1 b:2 c:3 d:4");
  ComponentEventPtr ce(new ComponentEvent(*m_dataItem1, 2, "time", value));
  m_checkpoint->addComponentEvent(ce);
  
  auto cecp = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 4, cecp->getDataSet().size());
  
  string value2("RESET|c:5 e:6");
  ComponentEventPtr ce2(new ComponentEvent(*m_dataItem1, 2, "time", value2));
  m_checkpoint->addComponentEvent(ce2);
  
  auto ce3 = *m_checkpoint->getEventPtr("v1");
  CPPUNIT_ASSERT_EQUAL((size_t) 2, ce3->getDataSet().size());
  
  auto map1 = ce3->getDataSet();
  CPPUNIT_ASSERT_EQUAL((string) "5", map1.at("c"));
  CPPUNIT_ASSERT_EQUAL((string) "6", map1.at("e"));
}

void DataSetTest::testBadData()
{
  string value("12356");
  auto ce = new ComponentEvent(*m_dataItem1, 2, "time", value);
  
  CPPUNIT_ASSERT_EQUAL((size_t) 0, ce->getDataSet().size());
}

void DataSetTest::testCurrent()
{
  m_adapter = m_agent->addAdapter("LinuxCNC", "server", 7878, false);
  CPPUNIT_ASSERT(m_adapter);
  
  m_agentTestHelper.m_path = "/current";
  
  {
    PARSE_XML_RESPONSE;
    CPPUNITTEST_ASSERT_XML_PATH_EQUAL(doc,
                                      "//m:DeviceStream//m:VariableDataSet[@dataItemId='v1']",
                                      "UNAVAILABLE");
  }
  
  m_adapter->processData("TIME|vars|a:1 b:2 c:3");

  {
    PARSE_XML_RESPONSE;
    CPPUNITTEST_ASSERT_XML_PATH_EQUAL(doc,
                                      "//m:DeviceStream//m:VariableDataSet[@dataItemId='v1']",
                                      "a:1 b:2 c:3");
  }

  m_adapter->processData("TIME|vars|c:6");
  
  {
    PARSE_XML_RESPONSE;
    CPPUNITTEST_ASSERT_XML_PATH_EQUAL(doc,
                                      "//m:DeviceStream//m:VariableDataSet[@dataItemId='v1']",
                                      "a:1 b:2 c:6");
  }

  m_adapter->processData("TIME|vars|RESET|d:10");
  
  {
    PARSE_XML_RESPONSE;
    CPPUNITTEST_ASSERT_XML_PATH_EQUAL(doc,
                                      "//m:DeviceStream//m:VariableDataSet[@dataItemId='v1']",
                                      "d:10");
    CPPUNITTEST_ASSERT_XML_PATH_EQUAL(doc,
                                      "//m:DeviceStream//m:VariableDataSet[@dataItemId='v1']@resetTriggered",
                                      "RESET");

  }
}

void DataSetTest::testSample()
{
  
}

void DataSetTest::testCurrentAt()
{
  
}
