#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <variant>

// events used in the client written in CPP
namespace events {
using time_point = std::chrono::system_clock::time_point;

struct event_base {
  pid_t source_pid;
  std::chrono::system_clock::time_point timestamp;
};

struct fork_event : event_base {
  pid_t child_pid;
};

struct exec_event : event_base {
  uid_t user_id;
  std::string command;
};

struct exit_event : event_base {
  int exit_code;
};

struct write_event : event_base {
  int file_descriptor;
  std::string data;
};

using event = std::variant<fork_event, exec_event, exit_event, write_event>;

std::ostream& operator<<(std::ostream&, const fork_event&);
std::ostream& operator<<(std::ostream&, const exec_event&);
std::ostream& operator<<(std::ostream&, const exit_event&);
std::ostream& operator<<(std::ostream&, const write_event&);
std::ostream& operator<<(std::ostream&, const event&);
}  // namespace events
