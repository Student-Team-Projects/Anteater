#pragma once

#include "events.hpp"

class structure_consumer {
 public:
  virtual void consume(events::fork_event const&) = 0;
  // Consume exec event and create new consumer for the executed program
  virtual std::unique_ptr<structure_consumer> consume(events::exec_event const&, structure_consumer* parent=nullptr) = 0;
  virtual void consume(events::exit_event const&) = 0;
  virtual void consume(events::write_event const&) = 0;
  virtual ~structure_consumer() = default;
};