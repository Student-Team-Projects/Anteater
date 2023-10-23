#include "consumers/plain_event_consumer.hpp"

#include <iomanip>
#include <iostream>

using namespace events;

void plain_event_consumer::consume(event const& e) {
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

void plain_event_consumer::event_visitor::operator()(fork_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "FORK" 
    << std::setw(8) << e.child_pid
    << "\n";
}

void plain_event_consumer::event_visitor::operator()(exec_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "EXEC" 
    << std::setw(8) << e.user_id
    << " " << unescape(e.command)
    << "\n";
}

void plain_event_consumer::event_visitor::operator()(exit_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "EXIT"
    << " " << e.exit_code
    << "\n";
}


void plain_event_consumer::event_visitor::operator()(write_event const& e) {
    std::cout 
    << std::setw(30) << e.timestamp 
    << std::setw(8) << e.source_pid 
    << std::setw(6) << "WRITE"
    << std::setw(3) << e.file_descriptor
    << " " << unescape(e.data)
    << "\n";
}
