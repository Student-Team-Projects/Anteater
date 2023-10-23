#include <bpf/bpf.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <string>

#include "bpf_provider.hpp"
#include "consumers/plain_event_consumer.hpp"

static void increase_memlock_limit() {
  rlimit lim{
      .rlim_cur = RLIM_INFINITY,
      .rlim_max = RLIM_INFINITY,
  };
  if (setrlimit(RLIMIT_MEMLOCK, &lim))
    throw "Failed to increase RLIMIT_MEMLOCK\n";
}

int main(int argc, char *argv[]) {
  increase_memlock_limit();

  bpf_provider provider;
  events::plain_event_consumer consumer;
  provider.run(argv + 1);
  while (provider.is_active()) {
    auto v = provider.provide();
    if (v.has_value())
      consumer.consume(v.value());
  }
  return 0;
}