#pragma once

#include "event_consumer.hpp"
#include "structure/plain/plain_event_formatter.hpp"

class console_logger : public events::event_consumer {
  struct event_visitor {
    plain_event_formatter fmt;

    void operator()(events::fork_event const& e);
    void operator()(events::exec_event const& e);
    void operator()(events::exit_event const& e);
    void operator()(events::write_event const& e);
  };

  event_visitor visitor;

 public:
  void consume(events::event const&);
};
