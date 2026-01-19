// 测试：结构体函数的类型错误检测
// 这个测试验证 Sema 能够正确检测类型错误

struct Point {
    int x;
    int y;
};

struct Rectangle {
    int width;
    int height;
};

// 返回 Point 的函数
struct Point createPoint(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

// 返回 Rectangle 的函数
struct Rectangle createRect(int w, int h) {
    struct Rectangle r;
    r.width = w;
    r.height = h;
    // 错误：返回类型不匹配
    struct Point p;
    p.x = 10;
    p.y = 20;
    return p;  // 应该报错：期望 Rectangle，实际 Point
}

int main() {
    struct Point p = createPoint(10, 20);
    return p.x;
}
