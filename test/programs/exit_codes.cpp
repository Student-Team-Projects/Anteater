#include <unistd.h>

int main() {
  for (int i = 0; i < 5; ++i)
    if (!fork())
      return i;
    else
      usleep(1000 * 50);

  return 42;
}
