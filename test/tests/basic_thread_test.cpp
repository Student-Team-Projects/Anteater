#include <gtest/gtest.h>

#include <map>

#include "testing_utility.hpp"

TEST(PROGRAMS, BASIC_THREAD) {
  int forks = 0, exits = 0;

  run_bpf_provider(
      {programs / "basic_thread"}, [&](events::fork_event fork) { ++forks; },
      [&](events::exec_event exec) {},
      [&](events::exit_event exit) { ++exits; },
      [&](events::write_event write) {});

  ASSERT_EQ(forks, (2 << 10) - 2);
  ASSERT_GT(exits, 0);
}
