#pragma once

#include <playground.h>
#include <experimental/any>
#include <parameters/ValueRange.h>
#include <map>
#include "proxies/hwui/descriptive-layouts/TemplateEnums.h"

namespace DescriptiveLayouts
{
  class EventSourceBase;

  class GlobalEventSourceBroker
  {
   public:
    explicit GlobalEventSourceBroker();
    ~GlobalEventSourceBroker();

    using Callback = std::function<void(std::experimental::any)>;

    sigc::connection connect(EventSources source, const Callback& cb);

   private:
    std::map<EventSources, std::unique_ptr<EventSourceBase>> m_map;
  };
}