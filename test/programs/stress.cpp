#include <iostream>
using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    for(int i = 0; i < 1000; i++)
        cout << string(1024, 'A') << endl;
}
