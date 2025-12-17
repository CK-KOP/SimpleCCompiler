// 指针功能测试：基本指针 + 多级指针
int main() {
    // 基本指针测试
    int x = 10;
    int *p = &x;
    *p = 20;
    if (x != 20) return 1;

    // 多级指针测试
    int **pp = &p;
    **pp = 30;
    if (x != 30) return 2;
    if (*p != 30) return 3;

    // 多次解引用读取
    int result = **pp;
    if (result != 30) return 4;

    return 0;
}
