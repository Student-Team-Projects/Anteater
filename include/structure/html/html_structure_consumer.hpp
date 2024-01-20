#pragma once

#include <filesystem>
#include <fstream>

#include "structure/structure_consumer.hpp"
#include "structure/html/html_event_formatter.hpp"
#include "structure/html/common.hpp"

/**
  * Root consumer which does not represent any program
  * The only purpose is to catch the first exec event and create actual consumers
*/
class html_structure_consumer_root : public structure_consumer {
  root_path_info root_info;
  html_event_formatter fmt;

public:
  void consume(events::fork_event const&) {}
  std::unique_ptr<structure_consumer> consume(events::exec_event const&);
  void consume(events::exit_event const&) {}
  void consume(events::write_event const&) {}
};

/**
 * Consumes and represents events emitted by a single program
*/
class html_structure_consumer : public structure_consumer {
  html_event_formatter const& fmt;
  std::filesystem::path filename;
  std::ofstream file;
  pid_t my_pid;
  std::string command;
  root_path_info const& root_info;

  public:
  void consume(events::fork_event const&);
  std::unique_ptr<structure_consumer> consume(events::exec_event const&);
  void consume(events::exit_event const&);
  void consume(events::write_event const&);
  html_structure_consumer(
    html_event_formatter const& fmt,
    events::exec_event const& source_event, 
    std::filesystem::path filename, 
    root_path_info const& root_info
  );
  html_structure_consumer(
    html_event_formatter const& fmt,
    events::exec_event const& source_event, 
    std::filesystem::path filename, 
    root_path_info const& root_info,
    parent_path_info const& parent_info
  );
  ~html_structure_consumer();
};