#pragma once

#include <filesystem>
#include <fstream>

#include "structure/structure_consumer.hpp"
#include "structure/html/html_event_formatter.hpp"

/**
 * Consumes events and outputs them as HTML files.
*/
class html_structure_consumer : public structure_consumer {
  // Consumer for non-root files (programs)
  class subconsumer : public structure_consumer {
    std::filesystem::path filename;
    std::ofstream file;
    html_event_formatter fmt;
    pid_t my_pid;
    std::string command;

   public:
    void consume(events::fork_event const&);
    std::unique_ptr<structure_consumer> consume(events::exec_event const&, structure_consumer* parent=nullptr);
    void consume(events::exit_event const&);
    void consume(events::write_event const&);
    subconsumer(events::exec_event const& source_event, std::filesystem::path filename);
    subconsumer(events::exec_event const& source_event, std::filesystem::path filename, std::filesystem::path parent_path, std::string const& parent_command);
    ~subconsumer();
  };

 public:
  void consume(events::fork_event const&) {}
  std::unique_ptr<structure_consumer> consume(events::exec_event const&, structure_consumer*);
  void consume(events::exit_event const&) {}
  void consume(events::write_event const&) {}
};