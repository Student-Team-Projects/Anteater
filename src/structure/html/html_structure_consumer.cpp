#include "structure/html/html_structure_consumer.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>

// const std::filesystem::path HTML_LOGS_ROOT = "/var/lib/debugger/logs/html";
const std::filesystem::path HTML_LOGS_ROOT = "logs/html";

static std::string event_to_filename(events::exec_event const& e) {
  std::string cmd = e.command;
  std::replace(cmd.begin(), cmd.end(), '/', '_');
  std::stringstream ss;
  ss << e.timestamp << " " << cmd;
  return ss.str();
}


std::unique_ptr<structure_consumer> html_structure_consumer::consume(events::exec_event const& e) {
  std::string filename = event_to_filename(e);
  std::filesystem::path path = HTML_LOGS_ROOT / filename / (filename + ".html");
  return std::make_unique<subconsumer>(e, path);
}

html_structure_consumer::subconsumer::subconsumer(events::exec_event const& source_event, std::filesystem::path filename)
    : my_pid(source_event.source_pid), filename(filename) {
  std::filesystem::create_directories(filename.parent_path());
  file.open(filename);
  fmt.begin(file, source_event);
}

void html_structure_consumer::subconsumer::consume(events::fork_event const& e) {
}

std::unique_ptr<structure_consumer> html_structure_consumer::subconsumer::consume(events::exec_event const& e) {
  std::filesystem::path subfilename = filename.parent_path() / (event_to_filename(e) + ".html");
  fmt.format(file, e, subfilename);
  return std::make_unique<html_structure_consumer::subconsumer>(e, subfilename);
}

void html_structure_consumer::subconsumer::consume(events::exit_event const& e) {
  if (e.source_pid == my_pid)
    fmt.format(file, e);
  else
    fmt.child_exit(file, e);
}

void html_structure_consumer::subconsumer::consume(events::write_event const& e) {
  fmt.format(file, e);
}

html_structure_consumer::subconsumer::~subconsumer() {
  fmt.end(file);
}