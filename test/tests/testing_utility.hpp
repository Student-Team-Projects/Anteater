#pragma once

#include <functional>

#include "events.hpp"

std::vector<events::event> run_bpf_provider(
    const std::vector<std::string> &args);

void run_bpf_provider(const std::vector<std::string> &args,
                      std::function<void(events::fork_event)> fork_func,
                      std::function<void(events::exec_event)> exec_func,
                      std::function<void(events::exit_event)> exit_func,
                      std::function<void(events::write_event)> write_func);
