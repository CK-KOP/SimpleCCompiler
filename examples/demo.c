// SimpleC 示例程序

int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int x = 10;
    int y = 20;
    int sum = add(x, y);

    int fact = factorial(5);

    if (sum > 25) {
        return 1;
    } else {
        return 0;
    }
}
