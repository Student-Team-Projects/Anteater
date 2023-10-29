#pragma once

#include <map>
#include <memory>
#include <vector>

#include "event_consumer.hpp"
#include "structure_consumer.hpp"

/**
 * Consumes events and organizes them into a tree-like debug structure.
 * Each process is assigned to a group that gathers its events.
 * Every EXEC (not FORK) event creates a separate group for the process and all
 * its descendants in the same group.
 */
class structure_provider : public events::event_consumer {
  std::unique_ptr<structure_consumer> root;
  std::vector<std::unique_ptr<structure_consumer>> structure_consumers;
  std::map<pid_t, structure_consumer*> pid_to_group;
  std::map<pid_t, std::vector<pid_t>> pid_to_children;

  struct event_visitor {
    structure_provider& provider;
    event_visitor(structure_provider& provider);

    void operator()(events::fork_event const& e);
    void operator()(events::exec_event const& e);
    void operator()(events::exit_event const& e);
    void operator()(events::write_event const& e);
  };

  event_visitor visitor;

  void set_subtree_group(pid_t root, structure_consumer* group);

 public:
  structure_provider(std::unique_ptr<structure_consumer> root);
  void consume(events::event const& e);
};