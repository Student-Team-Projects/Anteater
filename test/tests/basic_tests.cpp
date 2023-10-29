#include <gtest/gtest.h>

#include <set>

#include "testing_utility.hpp"

TEST(BPF_PROVIDER, LS) {
  int exits = 0, writes = 0;

  run_bpf_provider(
      {"ls"s}, [&](events::fork_event fork) {}, [&](events::exec_event exec) {},
      [&](events::exit_event exit) { ++exits; },
      [&](events::write_event write) { ++writes; });

  ASSERT_GT(exits, 0);
  ASSERT_GT(writes, 0);
}

TEST(BPF_PROVIDER, RECURSIVE_DEBUGGER) {
  std::set<std::string> printed;

  run_bpf_provider(
      {debugger, "echo"s, "ABC"s}, [&](events::fork_event fork) {},
      [&](events::exec_event exec) {},
      [&](events::exit_event exit) { ASSERT_EQ(exit.exit_code, 0); },
      [&](events::write_event write) { printed.insert(write.data); });

  ASSERT_TRUE(printed.contains("ABC\n"));
}
