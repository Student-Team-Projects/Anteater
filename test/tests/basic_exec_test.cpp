#include <gtest/gtest.h>

#include <vector>

#include "testing_utility.hpp"

TEST(PROGRAMS, BASIC_EXEC) {
  std::vector<events::exec_event> execs;

  run_bpf_provider(
      {programs / "basic_exec"}, [&](events::fork_event fork) {},
      [&](events::exec_event exec) { execs.push_back(exec); },
      [&](events::exit_event exit) {}, [&](events::write_event write) {});

  ASSERT_EQ(execs.size(), 2);
  ASSERT_EQ(execs[0].command, programs / "basic_exec");
  ASSERT_EQ(execs[1].command, "ls -al");
}
