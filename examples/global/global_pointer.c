// 测试全局指针
int global_x = 100;
int global_y = 200;
int* global_ptr = &global_y;

int main() {
    // 测试1：指针指向全局变量
    global_ptr = &global_x;
    if (*global_ptr != 100) return 1;

    // 测试2：通过指针修改全局变量
    *global_ptr = 200;
    if (global_x != 200) return 2;

    // 测试3：指针指向另一个全局变量
    global_ptr = &global_y;
    if (*global_ptr != 200) return 3;

    *global_ptr = 300;
    if (global_y != 300) return 4;

    // 测试4：指针指向局部变量
    int local_z = 400;
    global_ptr = &local_z;
    if (*global_ptr != 400) return 5;

    // 测试5：通过指针修改局部变量
    *global_ptr = 500;
    if (local_z != 500) return 6;

    return 0;
}
