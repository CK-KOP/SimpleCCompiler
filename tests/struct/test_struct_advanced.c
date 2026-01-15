// 结构体指针和数组的高级测试
// 测试复杂的指针操作和数组访问

struct Point {
    int x;
    int y;
};

struct Matrix {
    int rows;
    int cols;
    int data[2][3];  // 2x3 矩阵
};

struct PointerArray {
    struct Point *points[5];
    int count;
};

int main() {
    // 测试1: 多维数组成员
    struct Matrix m;
    m.rows = 2;
    m.cols = 3;
    m.data[0][0] = 1;
    m.data[0][1] = 2;
    m.data[0][2] = 3;
    m.data[1][0] = 4;
    m.data[1][1] = 5;
    m.data[1][2] = 6;

    int test1 = m.data[0][0] + m.data[1][2];  // 1 + 6 = 7

    // 测试2: 通过指针访问多维数组
    struct Matrix *mPtr = &m;
    int test2 = mPtr->data[0][1] + mPtr->data[1][1];  // 2 + 5 = 7

    // 测试3: 多维数组的完整遍历
    int sum = 0;
    sum = m.data[0][0] + m.data[0][1] + m.data[0][2];
    sum = sum + m.data[1][0] + m.data[1][1] + m.data[1][2];
    int test3 = sum;  // 1+2+3+4+5+6 = 21

    // 测试4: 指针数组的复杂操作
    struct Point p1;
    struct Point p2;
    struct Point p3;

    p1.x = 10;
    p1.y = 20;
    p2.x = 30;
    p2.y = 40;
    p3.x = 50;
    p3.y = 60;

    struct PointerArray pa;
    pa.count = 3;
    pa.points[0] = &p1;
    pa.points[1] = &p2;
    pa.points[2] = &p3;

    int test4 = pa.points[0]->x + pa.points[1]->x + pa.points[2]->x;  // 10 + 30 + 50 = 90

    // 测试5: 通过指针数组访问成员
    int test5 = pa.points[0]->y + pa.points[1]->y;  // 20 + 40 = 60

    // 测试6: 指针数组的指针
    struct PointerArray *paPtr = &pa;
    int test6 = paPtr->points[2]->x + paPtr->points[2]->y;  // 50 + 60 = 110

    // 测试7: 复杂的指针和数组组合
    struct Point arr[3];
    arr[0].x = 100;
    arr[0].y = 200;
    arr[1].x = 300;
    arr[1].y = 400;
    arr[2].x = 500;
    arr[2].y = 600;

    pa.points[3] = &arr[0];
    pa.points[4] = &arr[2];
    int test7 = pa.points[3]->x + pa.points[4]->y;  // 100 + 600 = 700

    return test1 + test2 + test3 + test4 + test5 + test6 + test7;
    // 7 + 7 + 21 + 90 + 60 + 110 + 700 = 995
}
