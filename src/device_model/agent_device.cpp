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
#include "utilities.hpp"
#include "config_options.hpp"

#include <dlib/logger.h>
#include <dlib/md5.h>

using namespace std;

namespace mtconnect
{
  static dlib::logger g_logger("agent_device");

  AgentDevice::AgentDevice(const Attributes &attributes) : Device(attributes, "Agent")
  {
    addRequiredDataItems();

    m_adapters = new Component("Adapters", {{"id", "__adapters__"}});
    addChild(m_adapters);
  }

  DataItem *AgentDevice::getConnectionStatus(const std::string &adapter)
  {
    return getDeviceDataItem(adapter + "_connection_status");
  }

  void AgentDevice::addAdapter(const adapter::Adapter *adapter)
  {
    bool suppress = GetOption<bool>(adapter->getOptions(), configuration::SuppressIPAddress).value_or(false);
    auto id = adapter->getIdentity();

    std::map<string, string> attrs{{"id", id}};
    if (!suppress)
    {
      stringstream name;
      name << adapter->getServer() << ':' << adapter->getPort();
      attrs["name"] = name.str();
    }
    else
    {
      auto device = GetOption<string>(adapter->getOptions(), configuration::Device);
      if (device)
        attrs["name"] = *device;
    }
    auto comp = new Component("Adapter", attrs);
    m_adapters->addChild(comp);

    {
      auto di = new DataItem({{"type", "CONNECTION_STATUS"},
                              {"id", id + "_connection_status"},
                              {"category", "EVENT"}});
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    if (!suppress)
    {
      auto di = new DataItem(
          {{"type", "ADAPTER_URI"}, {"id", id + "_adapter_uri"}, {"category", "EVENT"}});
      di->setComponent(*comp);
      di->addConstrainedValue(adapter->getUrl());
      comp->addDataItem(di);
    }

    {
      auto di = new DataItem({{"type", "OBSERVATION_UPDATE_RATE"},
                              {"id", id + "_observation_update_rate"},
                              {"units", "COUNT/SECOND"},
                              {"statistic", "AVERAGE"},
                              {"category", "SAMPLE"}});
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      auto di = new DataItem({{"type", "ASSET_UPDATE_RATE"},
                              {"id", id + "_asset_update_rate"},
                              {"units", "COUNT/SECOND"},
                              {"statistic", "AVERAGE"},
                              {"category", "SAMPLE"}});
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      auto di = new DataItem({{"type", "ADAPTER_SOFTWARE_VERSION"},
                              {"id", id + "_adapter_software_version"},
                              {"category", "EVENT"}});
      di->setComponent(*comp);
      comp->addDataItem(di);
    }

    {
      auto di = new DataItem({{"type", "MTCONNECT_VERSION"},
                              {"id", id + "_mtconnect_version"},
                              {"category", "EVENT"}});
      di->setComponent(*comp);
      comp->addDataItem(di);
    }
  }

  void AgentDevice::addRequiredDataItems()
  {
    // Add the required data items
    auto di =
        new DataItem({{"type", "AVAILABILITY"}, {"id", "agent_avail"}, {"category", "EVENT"}});
    di->setComponent(*this);
    addDataItem(di);

    // Add the required data items
    auto add =
        new DataItem({{"type", "DEVICE_ADDED"}, {"id", "device_added"}, {"category", "EVENT"}});
    add->setComponent(*this);
    addDataItem(add);

    auto removed =
        new DataItem({{"type", "DEVICE_REMOVED"}, {"id", "device_removed"}, {"category", "EVENT"}});
    removed->setComponent(*this);
    addDataItem(removed);

    auto changed =
        new DataItem({{"type", "DEVICE_CHANGED"}, {"id", "device_changed"}, {"category", "EVENT"}});
    changed->setComponent(*this);
    addDataItem(changed);
  }
}  // namespace mtconnect