#include "structure/html/html_structure_consumer.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>

// Lynx does not work well with filenames that have colons in them
static std::string normalize_string(std::string name) {
  if(name.size() > 64)
    name = name.substr(0, 64);
  std::replace_if(
      name.begin(),
      name.end(),
      [](char ch){
        return (!::isascii(ch) || ::ispunct(ch) || iscntrl(ch));
      }, 
      '-'      
  );
  return name;
}

static std::string event_to_filename(events::exec_event const& e) {
  std::string cmd = e.command;
  std::replace(cmd.begin(), cmd.end(), '/', '_');
  std::stringstream ss;
  ss << e.timestamp << " " << cmd;

  std::string name = ss.str(); 

  return normalize_string(name);
}

static void update_index(
  html_event_formatter const& fmt,
  std::chrono::system_clock::time_point timestamp,
  std::filesystem::path const& index_path, 
  std::filesystem::path const& root_path, 
  std::string const& command) {
  if (!std::filesystem::exists(index_path)) {
    std::filesystem::create_directories(index_path.parent_path());
  }
  if (!std::filesystem::exists(index_path)) {
    std::ofstream file;
    file.open(index_path);
    fmt.begin_index(file);
  }
  std::ofstream file;
  file.open(index_path, std::ios_base::app);

  auto relative_path = std::filesystem::relative(root_path, index_path.parent_path());
  fmt.format_index_link(file, timestamp, relative_path.string(), command);
  file.close();
}

html_structure_consumer_root::html_structure_consumer_root(std::filesystem::path logs_directory) : logs_directory(logs_directory) {}

std::unique_ptr<structure_consumer> html_structure_consumer_root::consume(events::exec_event const& e) {
  std::string filename = event_to_filename(e);
  std::filesystem::path path = logs_directory / filename / (filename + ".html");
  root_info = {
    e.command,
    "./" + path.filename().string(),
    "../index.html"
  };
  update_index(fmt, e.timestamp, logs_directory / "index.html", path, e.command);
  return std::make_unique<html_structure_consumer>(fmt, e, path, root_info);
}

html_structure_consumer::html_structure_consumer(
    html_event_formatter const& fmt,
    events::exec_event const& source_event, 
    std::filesystem::path filename, 
    root_path_info const& root_info
  ) : fmt(fmt), filename(filename), my_pid(source_event.source_pid), command(source_event.command), root_info(root_info) {
  std::filesystem::create_directories(filename.parent_path());
  file.open(filename);
  fmt.begin(file, source_event, root_info, {});
}

html_structure_consumer::html_structure_consumer(
    html_event_formatter const& fmt,
    events::exec_event const& source_event, 
    std::filesystem::path filename, 
    root_path_info const& root_info,
    parent_path_info const& parent_info
  ) : fmt(fmt), filename(filename), my_pid(source_event.source_pid), command(source_event.command), root_info(root_info) {
  std::filesystem::create_directories(filename.parent_path());
  file.open(filename);
  fmt.begin(file, source_event, root_info, {parent_info});
}

void html_structure_consumer::consume(events::fork_event const& e) {}

std::unique_ptr<structure_consumer> html_structure_consumer::consume(events::exec_event const& e) {
  std::filesystem::path childname = event_to_filename(e) + ".html";
  fmt.format(file, e, childname);

  std::filesystem::path subfilename = filename.parent_path() / childname;
  parent_path_info parent_info{this->filename.filename(), this->command};
  return std::make_unique<html_structure_consumer>(fmt, e, subfilename, root_info, parent_info);
}

void html_structure_consumer::consume(events::exit_event const& e) {
  if (e.source_pid == my_pid)
    fmt.format(file, e);
  fmt.child_exit(file, e);
}

void html_structure_consumer::consume(events::write_event const& e) {
  fmt.format(file, e);
}

html_structure_consumer::~html_structure_consumer() {
  fmt.end(file);
}