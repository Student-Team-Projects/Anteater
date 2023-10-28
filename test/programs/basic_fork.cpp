#include <unistd.h>

#include <iostream>

int main() {
  for (int i = 0; i < 4; ++i) {
    std::cout << i << std::endl;
    fork();
  }

  return 0;
}
