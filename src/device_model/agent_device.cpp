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

#include "agent_device.hpp"

#include "adapter/adapter.hpp"
#include "data_item/constraints.hpp"
#include "data_item/data_item.hpp"

#include <dlib/logger.h>

using namespace std;
using namespace std::literals;

namespace mtconnect
{
  static dlib::logger g_logger("agent_device");

  AgentDevice::AgentDevice(const Attributes &attributes) : Device(attributes, "Agent")
  {
    addRequiredDataItems();

    m_adapters = new Component("Adapters", {{"id", "__adapters__"}});
    addChild(m_adapters);
  }

  DataItemPtr AgentDevice::getConnectionStatus(const std::string &adapter)
  {
    return getDeviceDataItem(adapter + "_connection_status");
  }

  void AgentDevice::addAdapter(const adapter::Adapter *adapter)
  {
    using namespace entity;
    using namespace device_model::data_item;
    auto id = adapter->getIdentity();

    stringstream name;
    name << adapter->getServer() << ':' << adapter->getPort();

    auto comp = new Component("Adapter", {{"id", id}, {"name", name.str()}});
    m_adapters->addChild(comp);

    {
      ErrorList errors;
      auto di = DataItem::make({{"type", "CONNECTION_STATUS"s},
                                {"id", id + "_connection_status"s},
                                {"category", "EVENT"s}},
                               errors);
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      using namespace device_model::data_item;
      ErrorList errors;
      auto di = DataItem::make(
          Properties {
              {"type", "ADAPTER_URI"s}, {"id", id + "_adapter_uri"}, {"category", "EVENT"s}},
          errors);
      di->setConstantValue(adapter->getUrl());
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      ErrorList errors;
      auto di = DataItem::make({{"type", "OBSERVATION_UPDATE_RATE"s},
                                {"id", id + "_observation_update_rate"s},
                                {"units", "COUNT/SECOND"s},
                                {"statistic", "AVERAGE"s},
                                {"category", "SAMPLE"s}},
                               errors);
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      ErrorList errors;
      auto di = DataItem::make({{"type", "ASSET_UPDATE_RATE"s},
                                {"id", id + "_asset_update_rate"s},
                                {"units", "COUNT/SECOND"s},
                                {"statistic", "AVERAGE"s},
                                {"category", "SAMPLE"s}},
                               errors);
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      ErrorList errors;
      auto di = DataItem::make({{"type", "ADAPTER_SOFTWARE_VERSION"s},
                                {"id", id + "_adapter_software_version"s},
                                {"category", "EVENT"s}},
                               errors);
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      ErrorList errors;
      auto di = DataItem::make(
          {{"type", "MTCONNECT_VERSION"s}, {"id", id + "_mtconnect_version"s}, {"category", "EVENT"s}},
          errors);
      di->setComponent(*comp);
      comp->addDataItem(di);
    }
  }

  void AgentDevice::addRequiredDataItems()
  {
    using namespace entity;
    using namespace device_model::data_item;
    ErrorList errors;

    // Add the required data items
    auto di = DataItem::make(
        {{"type", "AVAILABILITY"s}, {"id", "agent_avail"s}, {"category", "EVENT"s}}, errors);
    di->setComponent(*this);
    addDataItem(di);

    // Add the required data items
    auto add = DataItem::make(
        {{"type", "DEVICE_ADDED"s}, {"id", "device_added"s}, {"category", "EVENT"s}}, errors);
    add->setComponent(*this);
    addDataItem(add);

    auto removed = DataItem::make(
        {{"type", "DEVICE_REMOVED"s}, {"id", "device_removed"s}, {"category", "EVENT"s}}, errors);
    removed->setComponent(*this);
    addDataItem(removed);

    auto changed = DataItem::make(
        {{"type", "DEVICE_CHANGED"s}, {"id", "device_changed"s}, {"category", "EVENT"s}}, errors);
    changed->setComponent(*this);
    addDataItem(changed);
  }
}  // namespace mtconnect
