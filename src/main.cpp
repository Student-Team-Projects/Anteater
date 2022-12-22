#include "runner.h"

int main(int argc, char **argv) {
    Runner runner;
    return runner.run(argv + 1);
}
