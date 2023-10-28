#include "events.hpp"

#include <iomanip>
#include <iostream>

namespace events {

std::string unescape(std::string const& s) {
  std::string result;
  for (char c : s) {
    if (c == '\n')
      result += "\\n";
    else if (c == 0)
      result += " ";
    else
      result.push_back(c);
  }
  return result;
}

std::ostream& operator<<(std::ostream& os, const fork_event& e) {
  return os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
            << std::setw(6) << "FORK" << std::setw(8) << e.child_pid;
}

std::ostream& operator<<(std::ostream& os, const exec_event& e) {
  return os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
            << std::setw(6) << "EXEC" << std::setw(8) << e.user_id << " "
            << unescape(e.command);
}

std::ostream& operator<<(std::ostream& os, const exit_event& e) {
  return os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
            << std::setw(6) << "EXIT"
            << " " << e.exit_code;
}

std::ostream& operator<<(std::ostream& os, const write_event& e) {
  return os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
            << std::setw(6) << "WRITE" << std::setw(3) << e.file_descriptor
            << " " << unescape(e.data);
}

std::ostream& operator<<(std::ostream& os, const event& e) {
  std::visit([&os](auto&& arg) { os << arg; }, e);
  return os;
}

}  // namespace events
