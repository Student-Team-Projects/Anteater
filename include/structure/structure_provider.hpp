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
 * 
 * When a process exits, the EXIT event is logged in every group that this process has created.
 */
class structure_provider : public events::event_consumer {
  std::unique_ptr<structure_consumer> root;
  std::vector<std::unique_ptr<structure_consumer>> structure_consumers;
  
  // The current group that process is logging to
  std::map<pid_t, structure_consumer*> pid_to_group;

  // Children created by forks
  std::map<pid_t, std::vector<pid_t>> pid_to_children;

  // For each process, the groups that contain the exec of it
  std::map<pid_t, std::vector<structure_consumer*>> pid_to_exec_groups;  

  struct event_visitor {
    structure_provider& provider;
    event_visitor(structure_provider& provider);

    void operator()(const events::fork_event& e);
    void operator()(const events::exec_event& e);
    void operator()(const events::exit_event& e);
    void operator()(const events::write_event& e);
  };

  event_visitor visitor;

  void set_subtree_group(pid_t root, structure_consumer* group);

 public:
  structure_provider(std::unique_ptr<structure_consumer> root);
  void consume(events::event const& e);
};