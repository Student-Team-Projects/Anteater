#include <gtest/gtest.h>

#include <map>

#include "testing_utility.hpp"

TEST(PROGRAMS, BASIC_FORK) {
  int forks = 0, exits = 0;
  std::map<char, int> writes;

  run_bpf_provider(
      {programs / "basic_fork"}, [&](events::fork_event fork) { ++forks; },
      [&](events::exec_event exec) {},
      [&](events::exit_event exit) { ++exits; },
      [&](events::write_event write) { ++writes[write.data.front()]; });

  ASSERT_EQ(forks, 15);
  ASSERT_GT(exits, 0);
  ASSERT_EQ(writes['0'], 1);
  ASSERT_EQ(writes['1'], 2);
  ASSERT_EQ(writes['2'], 4);
  ASSERT_EQ(writes['3'], 8);
}
