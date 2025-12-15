// 调试：检查排序后的每个元素

int main() {
    int arr[5];
    arr[0] = 5;
    arr[1] = 2;
    arr[2] = 8;
    arr[3] = 1;
    arr[4] = 9;

    int temp = 0;
    int i = 0;
    while (i < 5) {
        int j = 0;
        while (j < 4) {
            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    // 返回 arr[0]*10000 + arr[1]*1000 + arr[2]*100 + arr[3]*10 + arr[4]
    // 正确结果应该是 12589
    return arr[0] * 10000 + arr[1] * 1000 + arr[2] * 100 + arr[3] * 10 + arr[4];
}
