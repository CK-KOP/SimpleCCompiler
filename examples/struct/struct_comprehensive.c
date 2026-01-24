// 结构体综合测试：覆盖 Phase 1 和 Phase 2 的所有核心功能
// 测试内容：
// 1. 基本结构体定义和成员访问
// 2. 嵌套结构体
// 3. 结构体指针和箭头操作符
// 4. 结构体数组
// 5. 结构体成员中的指针和数组
// 6. 指针数组
// 7. 复杂的组合使用

struct Point {
    int x;
    int y;
};

struct Rectangle {
    struct Point topLeft;
    struct Point bottomRight;
};

struct Container {
    int count;
    struct Point *current;
    struct Point points[3];
    struct Rectangle *rects[2];
};

int main() {
    // 测试1: 基本结构体和成员访问
    struct Point p1;
    p1.x = 10;
    p1.y = 20;
    int test1 = p1.x + p1.y;  // 30

    // 测试2: 结构体指针和箭头操作符
    struct Point *ptr = &p1;
    ptr->x = 15;
    ptr->y = 25;
    int test2 = ptr->x + ptr->y;  // 40

    // 测试3: 嵌套结构体
    struct Rectangle rect;
    rect.topLeft.x = 0;
    rect.topLeft.y = 0;
    rect.bottomRight.x = 100;
    rect.bottomRight.y = 50;
    int test3 = rect.bottomRight.x + rect.bottomRight.y;  // 150

    // 测试4: 指向嵌套结构体的指针
    struct Rectangle *rectPtr = &rect;
    int test4 = rectPtr->topLeft.x + rectPtr->bottomRight.x;  // 100

    // 测试5: 结构体数组
    struct Point arr[3];
    arr[0].x = 1;
    arr[0].y = 2;
    arr[1].x = 3;
    arr[1].y = 4;
    arr[2].x = 5;
    arr[2].y = 6;
    int test5 = arr[0].x + arr[1].x + arr[2].x;  // 9

    // 测试6: 指向数组元素的指针
    struct Point *arrPtr = &arr[1];
    int test6 = arrPtr->x + arrPtr->y;  // 7

    // 测试7: 复杂容器结构
    struct Container container;
    container.count = 3;
    container.current = &p1;
    container.points[0].x = 11;
    container.points[0].y = 22;
    container.points[1].x = 33;
    container.points[1].y = 44;
    int test7 = container.current->x + container.points[0].x + container.points[1].x;  // 59

    // 测试8: 指针数组
    struct Point p2;
    p2.x = 200;
    p2.y = 300;

    struct Rectangle r1;
    r1.topLeft = p1;
    r1.bottomRight = p2;

    struct Rectangle r2;
    r2.topLeft.x = 5;
    r2.topLeft.y = 10;
    r2.bottomRight.x = 15;
    r2.bottomRight.y = 20;

    container.rects[0] = &r1;
    container.rects[1] = &r2;
    int test8 = container.rects[0]->bottomRight.x + container.rects[1]->bottomRight.x;  // 215

    // 总和测试
    return test1 + test2 + test3 + test4 + test5 + test6 + test7 + test8;
    // 30 + 40 + 150 + 100 + 9 + 7 + 59 + 215 = 610
}
