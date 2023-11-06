#pragma once

#include <filesystem>
#include <functional>
#include <variant>

#include "events.hpp"

using namespace std::string_literals;
using arg_t = std::variant<std::string, std::filesystem::path>;

extern const std::filesystem::path programs, debugger;

std::vector<events::event> run_bpf_provider(const std::vector<arg_t> &args);

void run_bpf_provider(const std::vector<arg_t> &args,
                      std::function<void(events::fork_event)> fork_func,
                      std::function<void(events::exec_event)> exec_func,
                      std::function<void(events::exit_event)> exit_func,
                      std::function<void(events::write_event)> write_func);
