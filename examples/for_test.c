// for 循环测试 (含 break/continue)
// 预期结果: 0 + 1 + 3 + 4 = 8 (跳过2, 在5时退出)

int main() {
    int sum = 0;
    int i = 0;

    for (i = 0; i < 10; i = i + 1) {
        if (i == 2) {
            continue;
        }

        if (i == 5) {
            break;
        }

        sum = sum + i;
    }

    return sum;
}
