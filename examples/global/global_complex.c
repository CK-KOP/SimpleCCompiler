// 综合测试：全局数组、结构体、指针操作
struct Point {
    int x;
    int y;
};

struct Point global_points[3];  // 全局结构体数组
int global_arr[5];
int global_sum;

void initPoints() {
    global_points[0].x = 10;
    global_points[0].y = 20;
    global_points[1].x = 30;
    global_points[1].y = 40;
    global_points[2].x = 50;
    global_points[2].y = 60;
}

int sumPoints() {
    int s = 0;
    for (int i = 0; i < 3; i = i + 1) {
        s = s + global_points[i].x + global_points[i].y;
    }
    return s;
}

int main() {
    // 初始化
    initPoints();

    // 测试1：访问全局结构体数组
    if (global_points[0].x != 10) return 1;
    if (global_points[2].y != 60) return 2;

    // 测试2：计算总和
    global_sum = sumPoints();
    if (global_sum != 210) return 3;  // (10+20) + (30+40) + (50+60) = 210

    // 测试3：初始化全局数组
    for (int i = 0; i < 5; i = i + 1) {
        global_arr[i] = i * 10;
    }

    // 测试4：访问全局数组
    if (global_arr[0] != 0) return 4;
    if (global_arr[4] != 40) return 5;

    // 测试5：再次使用结构体
    global_points[1].x = 100;
    if (global_points[1].x != 100) return 6;

    return global_sum + global_arr[2];  // 210 + 20 = 230
}
