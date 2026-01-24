// 指针综合测试
// 测试基本指针、多级指针、指针参数、指针运算
// TODO: 数组指针暂未支持

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    // 1. 基本指针测试
    int x = 10;
    int* p = &x;
    *p = 20;
    if (x != 20) return 1;

    // 2. 多级指针测试
    int** pp = &p;
    **pp = 30;
    if (x != 30) return 2;
    if (*p != 30) return 3;

    // 3. 指针参数测试
    int a = 100;
    int b = 200;
    swap(&a, &b);
    if (a != 200) return 4;
    if (b != 100) return 5;

    // 4. 指针赋值测试
    int y = 50;
    int* ptr = &y;
    *ptr = 60;
    if (y != 60) return 6;

    // 5. 多级指针修改
    **pp = 70;
    if (x != 70) return 7;

    return 0;  // 所有测试通过
}
