// 测试：结构体作为函数参数（完整测试）
// 这个测试验证编译器能够正确处理：
// 1. 结构体作为函数参数（值传递）
// 2. 结构体指针作为函数参数（引用传递）
// 3. 多个结构体参数
// 4. 混合参数类型（结构体 + int + 结构体）
// 5. 嵌套结构体作为参数

struct Point {
    int x;
    int y;
};

struct Line {
    struct Point start;
    struct Point end;
};

// 测试1: 单个结构体参数（值传递）
int getSum(struct Point p) {
    return p.x + p.y;
}

// 测试2: 结构体指针作为参数（引用传递）
void setPoint(struct Point *p, int x, int y) {
    p->x = x;
    p->y = y;
}

// 测试3: 混合参数类型
int addPointAndValue(struct Point p, int value) {
    return p.x + p.y + value;
}

// 测试4: 多个结构体参数
int addPoints(struct Point p1, struct Point p2) {
    return p1.x + p1.y + p2.x + p2.y;
}

// 测试5: 混合多个参数（结构体 + int + 结构体）
int calculate(struct Point p1, int multiplier, struct Point p2) {
    return (p1.x + p1.y) * multiplier + (p2.x + p2.y);
}

// 测试6: 嵌套结构体作为参数
int getLineLength(struct Line line) {
    int dx = line.end.x - line.start.x;
    int dy = line.end.y - line.start.y;
    return dx + dy;
}

int main() {
    // 测试1: 单个结构体参数
    struct Point p1;
    p1.x = 10;
    p1.y = 20;
    int sum1 = getSum(p1);  // 期望: 30

    // 测试2: 结构体指针参数
    struct Point p2;
    setPoint(&p2, 5, 15);
    int sum2 = p2.x + p2.y;  // 期望: 20

    // 测试3: 混合参数
    int sum3 = addPointAndValue(p2, 100);  // 期望: 120

    // 测试4: 多个结构体参数
    struct Point a;
    a.x = 1;
    a.y = 2;
    struct Point b;
    b.x = 3;
    b.y = 4;
    int sum4 = addPoints(a, b);  // 期望: 10

    // 测试5: 混合多个参数
    struct Point c;
    c.x = 2;
    c.y = 3;
    struct Point d;
    d.x = 5;
    d.y = 7;
    int sum5 = calculate(c, 10, d);  // 期望: 62

    // 测试6: 嵌套结构体参数
    struct Line myLine;
    myLine.start.x = 0;
    myLine.start.y = 0;
    myLine.end.x = 10;
    myLine.end.y = 20;
    int sum6 = getLineLength(myLine);  // 期望: 30

    // 总和: 30 + 20 + 120 + 10 + 62 + 30 = 272
    return sum1 + sum2 + sum3 + sum4 + sum5 + sum6;
}
