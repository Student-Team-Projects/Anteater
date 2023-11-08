#include <bpf/bpf.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <string>

#include "bpf_provider.hpp"
#include "console_logger.hpp"
#include "structure/plain_structure_consumer.hpp"
#include "structure/structure_provider.hpp"

int main(int argc, char *argv[]) {
  bpf_provider provider;
  console_logger logger;
  structure_provider structure(std::make_unique<plain_structure_consumer>());
  provider.run(argv + 1);
  while (provider.is_active()) {
    auto v = provider.provide();
    if (v.has_value()) {
      logger.consume(v.value());
      structure.consume(v.value());
    }
  }
  return 0;
}
