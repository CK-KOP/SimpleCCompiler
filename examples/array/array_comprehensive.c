// 数组综合测试
// 测试数组访问、遍历、排序算法

int main() {
    // 1. 基本数组测试
    int arr[5];
    arr[0] = 5;
    arr[1] = 2;
    arr[2] = 8;
    arr[3] = 1;
    arr[4] = 9;

    // 2. 冒泡排序（嵌套循环 + 数组访问）
    int i = 0;
    while (i < 5) {
        int j = 0;
        while (j < 4) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    // 3. 验证排序结果
    if (arr[0] != 1) return 1;
    if (arr[1] != 2) return 2;
    if (arr[2] != 5) return 3;
    if (arr[3] != 8) return 4;
    if (arr[4] != 9) return 5;

    // 4. 数组求和
    int sum = 0;
    int k = 0;
    for (k = 0; k < 5; k = k + 1) {
        sum = sum + arr[k];
    }
    // sum = 1 + 2 + 5 + 8 + 9 = 25

    // 5. 数组最大值
    int max = arr[0];
    int m = 1;
    while (m < 5) {
        if (arr[m] > max) {
            max = arr[m];
        }
        m = m + 1;
    }
    // max = 9

    return sum + max;  // 25 + 9 = 34
}
