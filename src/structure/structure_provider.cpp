#include "structure/structure_provider.hpp"

#include <iostream>
#include <queue>

using namespace events;

structure_provider::structure_provider(std::unique_ptr<structure_consumer> root)
    : root(std::move(root)), visitor(*this) {}

structure_provider::event_visitor::event_visitor(structure_provider& provider)
    : provider(provider) {}

void structure_provider::consume(event const& e) { std::visit(visitor, e); }

void structure_provider::event_visitor::operator()(fork_event const& e) {
  provider.pid_to_children[e.source_pid].push_back(e.child_pid);
  provider.pid_to_group[e.child_pid] = provider.pid_to_group[e.source_pid];
  provider.pid_to_group[e.source_pid]->consume(e);
}

void structure_provider::event_visitor::operator()(exec_event const& e) {
  std::unique_ptr<structure_consumer> new_consumer;
  if (!provider.pid_to_group.contains(e.source_pid))
    new_consumer = provider.root->consume(e);
  else
    new_consumer = provider.pid_to_group[e.source_pid]->consume(e);
  provider.set_subtree_group(e.source_pid, new_consumer.get());
  provider.structure_consumers.push_back(std::move(new_consumer));
}

void structure_provider::set_subtree_group(pid_t root,
                                           structure_consumer* group) {
  std::queue<pid_t> pids;
  pids.push(root);

  while (!pids.empty()) {
    pid_t current = pids.front();
    pids.pop();
    for (pid_t child : pid_to_children[current])
      if (pid_to_group[child] == pid_to_group[current]) pids.push(child);
    pid_to_group[current] = group;
  }
}

void structure_provider::event_visitor::operator()(exit_event const& e) {
  provider.pid_to_group[e.source_pid]->consume(e);
}

void structure_provider::event_visitor::operator()(write_event const& e) {
  provider.pid_to_group[e.source_pid]->consume(e);
}