#pragma once

#include "event_provider.hpp"
#include "events.hpp"

namespace events {
class event_consumer {
 public:
  void consume(event const&);
  virtual ~event_consumer() {}
};
}