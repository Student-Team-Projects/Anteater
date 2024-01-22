#pragma once

#include "events.hpp"
#include <iostream>

struct plain_event_formatter
{
    void format(std::ostream&, events::fork_event const&);
    void format(std::ostream&, events::exit_event const&);
    void format(std::ostream&, events::exec_event const&);
    void format(std::ostream&, events::write_event const&);
};