#include "syscall.h"

int main() {
    Open("hello.txt", 0);

    Halt();
    return 0;
}