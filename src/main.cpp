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

int main(int argc, char *argv[]) {
  bpf_provider provider;
  events::plain_event_consumer consumer;
  provider.run(argv + 1);
  while (provider.is_active()) {
    auto v = provider.provide();
    if (v.has_value()) consumer.consume(v.value());
  }
  return 0;
}
