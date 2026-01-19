// 测试：混合结构体定义和函数定义（验证语法歧义解决方案）
// 这个测试验证 Parser 能够在同一个文件中正确处理：
// 1. 多个结构体定义
// 2. 返回结构体的函数
// 3. 返回基本类型的函数

struct Point {
    int x;
    int y;
};

struct Rectangle {
    struct Point topLeft;
    struct Point bottomRight;
};

// 返回 int 的函数
int add(int a, int b) {
    return a + b;
}

// 返回 struct Point 的函数
struct Point makePoint(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

// 返回 struct Rectangle 的函数
struct Rectangle makeRectangle(int x1, int y1, int x2, int y2) {
    struct Rectangle rect;
    rect.topLeft = makePoint(x1, y1);
    rect.bottomRight = makePoint(x2, y2);
    return rect;
}

// 返回 void 的函数
void doNothing() {
    int x = 0;
}

int main() {
    struct Point p = makePoint(5, 10);
    struct Rectangle r = makeRectangle(0, 0, 100, 200);
    int sum = add(p.x, p.y);

    doNothing();

    return sum + r.topLeft.x + r.bottomRight.y;  // 期望: 15 + 0 + 200 = 215
}
