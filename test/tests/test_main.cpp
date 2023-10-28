#include <gtest/gtest.h>

#include <iostream>

#include "bpf_provider.hpp"

std::vector<events::event> run_bpf(const std::vector<std::string> &args) {
  std::vector<char *> argv;
  for (auto &arg : args) argv.push_back(const_cast<char *>(arg.data()));
  argv.push_back(nullptr);

  bpf_provider provider;
  provider.run(argv.data());

  std::vector<events::event> events;
  while (provider.is_active()) {
    auto event = provider.provide();
    if (event.has_value()) events.emplace_back(*event);
  }

  for (const auto &event : events) std::cout << event << '\n';

  return events;
}

TEST(a, b) { auto x = run_bpf({"ls"}); }
