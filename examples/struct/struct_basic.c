// 结构体基础功能测试
// 测试 Phase 1 的核心功能

struct Point {
    int x;
    int y;
};

struct Line {
    struct Point start;
    struct Point end;
};

int main() {
    // 测试1: 基本结构体定义和成员访问
    struct Point p;
    p.x = 10;
    p.y = 20;
    int test1 = p.x + p.y;  // 30

    // 测试2: 嵌套结构体
    struct Line line;
    line.start.x = 0;
    line.start.y = 0;
    line.end.x = 100;
    line.end.y = 50;
    int test2 = line.end.x + line.end.y;  // 150

    // 测试3: 链式成员访问
    int test3 = line.start.x + line.start.y + line.end.x + line.end.y;  // 150

    // 测试4: 多个结构体变量
    struct Point p2;
    p2.x = 5;
    p2.y = 15;
    int test4 = p2.x + p2.y;  // 20

    return test1 + test2 + test3 + test4;
    // 30 + 150 + 150 + 20 = 350
}
