#include <gtest/gtest.h>

#include <vector>

#include "testing_utility.hpp"

TEST(PROGRAMS, STRESS) {
  int count = 0;

  run_bpf_provider(
      {programs / "stress"}, [&](events::fork_event fork) {},
      [&](events::exec_event exec) {},
      [&](events::exit_event exit) {}, 
      [&](events::write_event write) { 
        count++; 
        }
    );

  ASSERT_EQ(count, 1000);
}

TEST(PROGRAMS, QUEUE_OVERFLOW) {
  int count = 0;

  run_bpf_provider(
      {programs / "stress"}, [&](events::fork_event fork) {},
      [&](events::exec_event exec) {
        sleep(3);
      },
      [&](events::exit_event exit) {}, 
      [&](events::write_event write) { 
        count++; 
        }
    );

  ASSERT_EQ(count, 1000);
}

TEST(PROGRAMS, BIG_WRITE) {
  int exit_count = 0;
  std::string write_value;

  run_bpf_provider(
      {programs / "big_write"}, [&](events::fork_event fork) {},
      [&](events::exec_event exec) {},
      [&](events::exit_event exit) {
        exit_count++;
      }, 
      [&](events::write_event write) { 
        write_value = write.data;
        }
    );

  //big write is truncated
  ASSERT_EQ(write_value, std::string(1024, 'A'));
  ASSERT_EQ(exit_count, 1);
}
