#include <exception>
#include <iostream>
#include <string>

#include "bpf_provider.hpp"
#include "console_logger.hpp"
#include "structure/html/html_structure_consumer.hpp"
#include "structure/structure_provider.hpp"

std::string APP_NAME = "anteater";

void html_version(int argc, char *argv[]) {
  const std::filesystem::path home{getenv("HOME")};
  const std::filesystem::path html_logs_directory = home / ".local/share" / APP_NAME / "logs/html";
  structure_provider structure(std::make_unique<html_structure_consumer_root>(html_logs_directory));

  bpf_provider provider;
  provider.run(argv + 1);
  //set uid only for current thread (breaking posix)
  syscall(SYS_setuid, getuid());

  //busy waiting
  while (provider.is_active()) {
    auto v = provider.provide();
    if (v.has_value())
      structure.consume(v.value());
  }
}

void text_version(int argc, char *argv[]) {
  if(argc < 3)
    throw std::runtime_error{"Command expected"};
  bpf_provider provider;
  console_logger logger;
  provider.run(argv + 2);

  syscall(SYS_setuid, getuid());

  //busy waiting
  while (provider.is_active()) {
    auto v = provider.provide();
    if (v.has_value())
      logger.consume(v.value());
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
