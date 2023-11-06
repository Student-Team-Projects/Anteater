#include <unistd.h>

#include <iostream>
#include <vector>

int main() {
  char *argv[] = {const_cast<char *>("ls"), const_cast<char *>("-al"), nullptr};

  execvp("ls", argv);

  return 0;
}
