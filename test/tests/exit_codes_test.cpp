#include <gtest/gtest.h>

#include <vector>

#include "testing_utility.hpp"

TEST(PROGRAMS, EXIT_CODES) {
  std::vector<int> codes, expected = {0, 1, 2, 3, 4, 42};

  run_bpf_provider(
      {programs / "exit_codes"}, [&](events::fork_event fork) {},
      [&](events::exec_event exec) {},
      [&](events::exit_event exit) { codes.push_back(exit.exit_code); },
      [&](events::write_event write) {});

  ASSERT_EQ(codes, expected);
}
