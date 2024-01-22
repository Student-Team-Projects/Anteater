#pragma once

#include <filesystem>
#include <fstream>

#include "structure/structure_consumer.hpp"
#include "plain_event_formatter.hpp"

class plain_structure_consumer : public structure_consumer {
  class subconsumer : public structure_consumer {
    std::filesystem::path filename;
    std::ofstream file;
    plain_event_formatter fmt;

   public:
    void consume(events::fork_event const&);
    std::unique_ptr<structure_consumer> consume(events::exec_event const&);
    void consume(events::exit_event const&);
    void consume(events::write_event const&);
    subconsumer(std::filesystem::path filename);
  };

 std::filesystem::path logs_directory;
 public:
  plain_structure_consumer(std::filesystem::path logs_directory);
  void consume(events::fork_event const&) {}
  std::unique_ptr<structure_consumer> consume(events::exec_event const&);
  void consume(events::exit_event const&) {}
  void consume(events::write_event const&) {}
};