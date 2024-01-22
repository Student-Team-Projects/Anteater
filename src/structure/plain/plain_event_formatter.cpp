#include "structure/plain/plain_event_formatter.hpp"

using namespace events;

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

void plain_event_formatter::format(std::ostream& os, fork_event const& e) {
  os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
     << std::setw(6) << "FORK" << std::setw(8) << e.child_pid << "\n";
}
void plain_event_formatter::format(std::ostream& os, exit_event const& e) {
  os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
     << std::setw(6) << "EXIT"
     << " " << e.exit_code << "\n";
}
void plain_event_formatter::format(std::ostream& os, exec_event const& e) {
  os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
     << std::setw(6) << "EXEC" << std::setw(8) << e.user_id << " "
     << unescape(e.command) << "\n";
}

std::string descriptor_name(write_event::descriptor fd) {
  switch (fd) {
    case write_event::descriptor::STDOUT: return "STDOUT";
    case write_event::descriptor::STDERR: return "STDERR";
  }
}

void plain_event_formatter::format(std::ostream& os, write_event const& e) {
  os << std::setw(30) << e.timestamp << std::setw(8) << e.source_pid
     << std::setw(6) << "WRITE" << " " << descriptor_name(e.file_descriptor) << " "
     << unescape(e.data) << "\n";
}