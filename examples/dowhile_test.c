// do-while 循环测试 (含 break/continue)
// 预期结果: 1 + 2 + 4 + 5 + 6 = 18 (跳过3, 在7时退出)

int main() {
    int sum = 0;
    int i = 0;

    do {
        i = i + 1;

        if (i == 3) {
            continue;
        }

        if (i == 7) {
            break;
        }

        sum = sum + i;
    } while (i < 10);

    return sum;
}
