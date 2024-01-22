#include "structure/structure_provider.hpp"

#include <iostream>
#include <stack>

using namespace events;

structure_provider::structure_provider(std::unique_ptr<structure_consumer> root)
    : root(std::move(root)), visitor(*this) {}

structure_provider::event_visitor::event_visitor(structure_provider& provider)
    : provider(provider) {}

void structure_provider::consume(const event& e) { std::visit(visitor, e); }

void structure_provider::event_visitor::operator()(const fork_event& e) {
  // Fork creates a new process which belongs to the same group as the parent
  provider.pid_to_children[e.source_pid].push_back(e.child_pid);
  provider.pid_to_group[e.child_pid] = provider.pid_to_group[e.source_pid];
}

void structure_provider::event_visitor::operator()(const exec_event& e) {
  // Exec creates a new "program" which belongs to a separate group

  structure_consumer* parent;
  // The only process that we didn't create a group for is the root
  if (!provider.pid_to_group.contains(e.source_pid))
    parent = provider.root.get();
  else
    parent = provider.pid_to_group[e.source_pid];
  
  std::unique_ptr<structure_consumer> new_consumer = parent->consume(e);
  provider.pid_to_exec_groups[e.source_pid].push_back(parent);
  provider.set_subtree_group(e.source_pid, new_consumer.get());
  provider.structure_consumers.push_back(std::move(new_consumer));
}

void structure_provider::set_subtree_group(pid_t root, structure_consumer* group) {
  // When process execs (i.e. creates a new program) all its children change the group

  // Traverse the process tree using DFS
  std::stack<pid_t> pids;
  pids.push(root);

  while (!pids.empty()) {
    pid_t current = pids.top();
    pids.pop();
    for (pid_t child : pid_to_children[current])
      if (pid_to_group[child] == pid_to_group[current]) pids.push(child);
    pid_to_group[current] = group;
  }
}

void structure_provider::event_visitor::operator()(const exit_event& e) {
  provider.pid_to_group[e.source_pid]->consume(e);
  for (auto group : provider.pid_to_exec_groups[e.source_pid])
    group->consume(e);
}

void structure_provider::event_visitor::operator()(const write_event& e) {
  provider.pid_to_group[e.source_pid]->consume(e);
}