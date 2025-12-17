// 指针作为函数参数测试
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    int x = 10;
    int y = 20;

    swap(&x, &y);

    if (x != 20) return 1;
    if (y != 10) return 2;

    return 0;
}
