#include "testing_utility.hpp"

#include "bpf_provider.hpp"

const std::filesystem::path programs{PATH_TO_PROGRAMS},
    debugger{PATH_TO_DEBUGGER};

// https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::vector<events::event> run_bpf_provider(const std::vector<arg_t> &args) {
  std::vector<char *> argv;
  for (auto &arg : args)
    std::visit(
        [&argv](auto &&data) {
          argv.push_back(const_cast<char *>(data.c_str()));
        },
        arg);
  argv.push_back(nullptr);

  bpf_provider provider;
  provider.run(argv.data());

  std::vector<events::event> events;
  while (provider.is_active()) {
    auto event = provider.provide();
    if (event.has_value()) events.push_back(std::move(*event));
  }

  return events;
}

// this is not very efficient, but it is enough for testing
void run_bpf_provider(const std::vector<arg_t> &args,
                      std::function<void(events::fork_event)> fork_func,
                      std::function<void(events::exec_event)> exec_func,
                      std::function<void(events::exit_event)> exit_func,
                      std::function<void(events::write_event)> write_func) {
  for (const auto &event : run_bpf_provider(args))
    std::visit(overloaded{fork_func, exec_func, exit_func, write_func}, event);
}
