// 测试全局数组
int global_arr[5];

int main() {
    // 先初始化数组
    global_arr[0] = 1;
    global_arr[1] = 2;
    global_arr[2] = 3;
    global_arr[3] = 4;
    global_arr[4] = 5;

    // 测试1：读取全局数组元素
    if (global_arr[0] != 1) return 1;
    if (global_arr[4] != 5) return 2;

    // 测试2：修改全局数组元素
    global_arr[2] = 100;
    if (global_arr[2] != 100) return 3;

    // 测试3：遍历数组求和
    int sum = 0;
    for (int i = 0; i < 5; i = i + 1) {
        sum = sum + global_arr[i];
    }
    if (sum != 112) return 4;  // 1+2+100+4+5 = 112

    return 0;
}
