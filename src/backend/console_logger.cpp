#include "console_logger.hpp"

#include <iomanip>
#include <iostream>

using namespace events;

void console_logger::consume(event const& e) {
  std::visit(visitor, e);
}

std::string unescape(std::string const& s)
{
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

void console_logger::event_visitor::operator()(fork_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "FORK" 
    << std::setw(8) << e.child_pid
    << "\n";
}

void console_logger::event_visitor::operator()(exec_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "EXEC" 
    << std::setw(8) << e.user_id
    << " " << unescape(e.command)
    << "\n";
}

void console_logger::event_visitor::operator()(exit_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "EXIT"
    << " " << e.exit_code
    << "\n";
}


void console_logger::event_visitor::operator()(write_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "WRITE"
    << std::setw(3) << e.file_descriptor
    << " " << unescape(e.data)
    << "\n";
}
