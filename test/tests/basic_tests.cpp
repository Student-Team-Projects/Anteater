#include <gtest/gtest.h>

#include <set>

#include "testing_utility.hpp"

TEST(bpf_provider, ls) {
  int exits = 0, writes = 0;

  run_bpf_provider(
      {"ls"}, [&](events::fork_event fork) {}, [&](events::exec_event exec) {},
      [&](events::exit_event exit) { ++exits; },
      [&](events::write_event write) { ++writes; });

  ASSERT_GT(exits, 0);
  ASSERT_GT(writes, 0);
}

TEST(bpf_provider, basic_recursive_test) {
  std::set<std::string> printed;

  run_bpf_provider(
      {"bin/main", "echo", "ABC"}, [&](events::fork_event fork) {},
      [&](events::exec_event exec) {},
      [&](events::exit_event exit) { ASSERT_EQ(exit.exit_code, 0); },
      [&](events::write_event write) { printed.insert(write.data); });

  ASSERT_TRUE(printed.contains("ABC\n"));
}
