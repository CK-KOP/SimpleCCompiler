// 测试：结构体作为函数返回值（验证语法歧义解决方案）
// 这个测试验证 Parser 能够正确区分：
// 1. struct Point { ... }      -> 结构体定义
// 2. struct Point foo(...) {}  -> 函数定义（返回结构体）

struct Point {
    int x;
    int y;
};

// 返回结构体类型的函数
struct Point createPoint(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

// 另一个返回结构体的函数
struct Point getOrigin() {
    struct Point origin;
    origin.x = 0;
    origin.y = 0;
    return origin;
}

int main() {
    struct Point p1 = createPoint(10, 20);
    struct Point p2 = getOrigin();

    return p1.x + p1.y + p2.x + p2.y;  // 期望: 10 + 20 + 0 + 0 = 30
}
