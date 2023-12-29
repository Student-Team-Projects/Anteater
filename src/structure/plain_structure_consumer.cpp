#include "structure/plain_structure_consumer.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>

const std::filesystem::path PLAIN_LOGS_ROOT = "/var/lib/debugger/logs/plain";

std::string event_to_filename(events::exec_event const& e) {
  std::string cmd = e.command;
  std::replace(cmd.begin(), cmd.end(), '/', '_');
  std::stringstream ss;
  ss << e.timestamp << " " << cmd;
  return ss.str();
}


std::unique_ptr<structure_consumer> plain_structure_consumer::consume(events::exec_event const& e, structure_consumer*) {
  std::string filename = event_to_filename(e);
  std::filesystem::path path = PLAIN_LOGS_ROOT / filename / (filename + ".txt");
  return std::make_unique<subconsumer>(path);
}

plain_structure_consumer::subconsumer::subconsumer(std::filesystem::path filename)
    : filename(filename) {
  std::filesystem::create_directories(filename.parent_path());
  file.open(filename);
}

void plain_structure_consumer::subconsumer::consume(events::fork_event const& e) {
  fmt.format(file, e);
}
std::unique_ptr<structure_consumer> plain_structure_consumer::subconsumer::consume(events::exec_event const& e, structure_consumer*) {
  fmt.format(file, e);

  std::filesystem::path subfilename = filename.parent_path() / (event_to_filename(e) + ".txt");
  return std::make_unique<plain_structure_consumer::subconsumer>(subfilename);
}
void plain_structure_consumer::subconsumer::consume(events::exit_event const& e) {
  fmt.format(file, e);
}
void plain_structure_consumer::subconsumer::consume(events::write_event const& e) {
  fmt.format(file, e);
}