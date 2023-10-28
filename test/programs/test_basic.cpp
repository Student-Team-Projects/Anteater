#include <iostream>

int main() {
  for (int i = 0; i < 4; ++i) {
    fork();
    std::cout << i << std::endl;
  }

  return 0;
}
