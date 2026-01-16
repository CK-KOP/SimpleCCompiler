// 测试结构体整体赋值 - 综合测试
struct Point {
    int x;
    int y;
};

struct Line {
    struct Point start;
    struct Point end;
};

struct Data {
    int value;
    int arr[3];
};

int main() {
    // 测试1：基本结构体赋值
    struct Point p1;
    p1.x = 10;
    p1.y = 20;

    struct Point p2;
    p2 = p1;

    int test1 = p2.x + p2.y;  // 应该是 30

    // 测试2：嵌套结构体赋值
    struct Line line1;
    line1.start.x = 0;
    line1.start.y = 0;
    line1.end.x = 100;
    line1.end.y = 200;

    struct Line line2;
    line2 = line1;

    int test2 = line2.end.x + line2.end.y;  // 应该是 300

    // 测试3：包含数组的结构体赋值
    struct Data d1;
    d1.value = 50;
    d1.arr[0] = 1;
    d1.arr[1] = 2;
    d1.arr[2] = 3;

    struct Data d2;
    d2 = d1;

    int test3 = d2.value + d2.arr[0] + d2.arr[1] + d2.arr[2];  // 应该是 56

    // 测试4：连续赋值
    struct Point p3;
    p3 = p2;  // p3 = p2 = p1

    int test4 = p3.x + p3.y;  // 应该是 30

    return test1 + test2 + test3 + test4;  // 30 + 300 + 56 + 30 = 416
}
