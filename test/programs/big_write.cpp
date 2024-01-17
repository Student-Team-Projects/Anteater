#include <unistd.h>
#include <string>

using namespace std;

int main() {
    string buf(8192, 'A');
    write(1, buf.c_str(), 8192);
}
