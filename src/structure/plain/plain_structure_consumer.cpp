#include "structure/plain/plain_structure_consumer.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>

std::string event_to_filename(events::exec_event const& e) {
  std::string cmd = e.command;
  std::replace(cmd.begin(), cmd.end(), '/', '_');
  std::stringstream ss;
  ss << e.timestamp << " " << cmd;
  return ss.str();
}

plain_structure_consumer::plain_structure_consumer(std::filesystem::path logs_directory) : logs_directory(logs_directory) {}

std::unique_ptr<structure_consumer> plain_structure_consumer::consume(events::exec_event const& e) {
  std::string filename = event_to_filename(e);
  std::filesystem::path path = logs_directory / filename / (filename + ".txt");
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
std::unique_ptr<structure_consumer> plain_structure_consumer::subconsumer::consume(events::exec_event const& e) {
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