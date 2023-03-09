#include "syscall.h"
#include "copyright.h"

int main() {
    // use fileName variable here
    int result = Create("abc.txt");
    Halt();
    return 0;
}