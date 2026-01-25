// 测试全局结构体
struct Point {
    int x;
    int y;
};

struct Point global_p1;
struct Point global_p2;

int main() {
    // 手动初始化
    global_p2.x = 10;
    global_p2.y = 20;

    // 测试1：读取初始化的全局结构体
    if (global_p2.x != 10) return 1;
    if (global_p2.y != 20) return 2;

    // 测试2：读取未初始化的全局结构体（应该为 0）
    if (global_p1.x != 0) return 3;
    if (global_p1.y != 0) return 4;

    // 测试3：修改全局结构体
    global_p1.x = 100;
    global_p1.y = 200;
    if (global_p1.x != 100) return 5;
    if (global_p1.y != 200) return 6;

    // 测试4：结构体整体赋值
    global_p1 = global_p2;
    if (global_p1.x != 10) return 7;
    if (global_p1.y != 20) return 8;

    return 0;
}
