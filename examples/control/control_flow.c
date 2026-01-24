// 控制流综合测试
// 测试 if-else, for, while, do-while, break, continue 的���合使用

int main() {
    int sum = 0;
    int i = 0;

    // 测试 for 循环
    for (i = 0; i < 10; i = i + 1) {
        if (i == 2) {
            continue;  // 跳过 2
        }
        if (i == 5) {
            break;  // 在 5 时退出
        }
        sum = sum + i;
    }
    // sum = 0 + 1 + 3 + 4 = 8

    // 测试 while 循环
    int j = 0;
    while (j < 3) {
        sum = sum + j;  // sum = 8 + 0 + 1 + 2 = 11
        j = j + 1;
    }

    // 测试 do-while 循环
    int k = 0;
    do {
        sum = sum + k;  // sum = 11 + 0 + 1 + 2 = 14
        k = k + 1;
    } while (k < 3);

    // 测试嵌套 if-else
    int value = 85;
    if (value > 90) {
        sum = sum + 100;
    } else if (value > 80) {
        if (value > 85) {
            sum = sum + 10;  
        } else {
            sum = sum + 20; // sum = 14 + 20 = 34
        }
    } else {
        sum = sum + 30;
    }

    return sum;  // 应该返回 34 (8+3+3+20 = 34)
}
