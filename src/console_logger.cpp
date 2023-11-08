#include "console_logger.hpp"

#include <iomanip>
#include <iostream>

using namespace events;

void console_logger::consume(event const& e) { std::visit(visitor, e); }

void console_logger::event_visitor::operator()(fork_event const& e) {
  fmt.format(std::cout, e);
}

void console_logger::event_visitor::operator()(exec_event const& e) {
  fmt.format(std::cout, e);
}

void console_logger::event_visitor::operator()(exit_event const& e) {
  fmt.format(std::cout, e);
}

void console_logger::event_visitor::operator()(write_event const& e) {
  fmt.format(std::cout, e);
}
