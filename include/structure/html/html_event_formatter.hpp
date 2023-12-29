#pragma once

#include "events.hpp"
#include <iostream>
#include <filesystem>

struct html_event_formatter
{
    void begin(std::ostream&, events::exec_event const& source_event, std::optional<std::filesystem::path>);
    void end(std::ostream&);
    void child_exit(std::ostream& os, events::exit_event const& pid);
    void format(std::ostream&, events::fork_event const&);
    void format(std::ostream&, events::exit_event const&);
    void format(std::ostream&, events::exec_event const&, std::filesystem::path);
    void format(std::ostream&, events::write_event const&);
};