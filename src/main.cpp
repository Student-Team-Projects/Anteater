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
#include "structure/html/html_structure_consumer.hpp"
#include "structure/structure_provider.hpp"

void html_version(int argc, char *argv[]) {
  bpf_provider provider;
  structure_provider structure(std::make_unique<html_structure_consumer>());
  provider.run(argv + 1);
  while (provider.is_active()) {
    auto v = provider.provide();
    seteuid(getuid());
    if (v.has_value())
      structure.consume(v.value());
    seteuid(0);
  }
}

void text_version(int argc, char *argv[]) {
  if(argc < 3)
    throw std::runtime_error{"Command expected"};
  bpf_provider provider;
  console_logger logger;
  provider.run(argv + 2);
  while (provider.is_active()) {
    auto v = provider.provide();
    seteuid(getuid());
    if (v.has_value())
      logger.consume(v.value());
    seteuid(0);
  }
}

int main(int argc, char *argv[]) {
  if(argc < 2)
    throw std::runtime_error{"Command expected"};
  if(std::string{argv[1]} == "-L")
    text_version(argc, argv);
  else
    html_version(argc, argv);
  return 0;
}
