#include "syscall.h"
#include "copyright.h"

int main(int argc, char *argv[]) {
    //char* filename = argv[3];
    // use fileName variable here
    if (Create("hello.txt") == -1)
    {
    // xuất thông báo lỗi tạo tập tin
    }
    else
    {
    // xuất thông báo tạo tập tin thành công
    }
    Halt();
    return 0;
}