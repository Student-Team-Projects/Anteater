#pragma once

#include <optional>

#include "events.hpp"

namespace events {
class event_provider {
 public:
  virtual bool is_active() = 0;
  virtual std::optional<event> provide() = 0;
  virtual ~event_provider() {}
};
}