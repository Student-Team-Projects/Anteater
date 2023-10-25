#pragma once

#include "event_consumer.hpp"

class console_logger : public events::event_consumer {
  struct event_visitor {
    void operator()(events::fork_event const& e);
    void operator()(events::exec_event const& e);
    void operator()(events::exit_event const& e);
    void operator()(events::write_event const& e);
  };

  event_visitor visitor;


 public:
  void consume(events::event const&);
};
