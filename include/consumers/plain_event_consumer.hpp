#pragma once

#include "event_consumer.hpp"
#include "event_provider.hpp"

namespace events {
class plain_event_consumer : public event_consumer {
  struct event_visitor {
    void operator()(fork_event const& e);
    void operator()(exec_event const& e);
    void operator()(exit_event const& e);
    void operator()(write_event const& e);
  };

  event_visitor visitor;


 public:
  void consume(event const&);
};
}  // namespace events
