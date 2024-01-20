#pragma once

#include "events.hpp"
#include <iostream>
#include <filesystem>

#include "structure/html/common.hpp"

struct html_event_formatter {
    void begin_index(std::ostream& os) const;
    void format_index_link(
        std::ostream& os,
        std::chrono::system_clock::time_point timestamp,
        std::string const& href, 
        std::string const& displayed_text
    ) const;

    void begin(
        std::ostream& os, 
        events::exec_event const& source_event, 
        root_path_info const& root_info,
        std::optional<parent_path_info> const& parent_info
    ) const;
    void end(std::ostream&) const;
    void child_exit(std::ostream& os, events::exit_event const& pid) const;
    void format(std::ostream&, events::fork_event const&) const;
    void format(std::ostream&, events::exit_event const&) const;
    void format(std::ostream&, events::exec_event const&, std::filesystem::path) const;
    void format(std::ostream&, events::write_event const&) const;
};