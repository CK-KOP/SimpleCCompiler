// 测试基本全局变量
int global_x = 100;
int global_y = 200;
int global_z;  // 未初始化的全局变量

int getGlobalX() {
    return global_x;
}

int setGlobalY(int val) {
    global_y = val;
    return global_y;
}

int main() {
    // 测试1：读取全局变量
    if (getGlobalX() != 100) return 1;

    // 测试2：修改全局变量
    if (setGlobalY(300) != 300) return 2;

    // 测试3：全局变量运算
    int sum = global_x + global_y;  // 100 + 300 = 400
    if (sum != 400) return 3;

    // 测试4：未初始化的全局变量（应该是 0）
    if (global_z != 0) return 4;

    // 测试5：修改未初始化的全局变量
    global_z = 500;
    if (global_z != 500) return 5;

    // 测试6：全局变量作为函数参数
    int result = getGlobalX() + setGlobalY(400);  // 100 + 400 = 500
    if (result != 500) return 6;

    return sum + global_z;  // 400 + 500 = 900
}
