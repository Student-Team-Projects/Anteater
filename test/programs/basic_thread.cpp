#include <iostream>
#include <thread>

void rec(int dep) {
  if (!dep) return;
  std::jthread jthread1(rec, dep - 1), jthread2(rec, dep - 1);
}

int main() {
  rec(10);
  return 0;
}
