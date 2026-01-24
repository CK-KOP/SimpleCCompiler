// 递归综合测试
// 测试多种递归算法
// TODO: 暂不支持的功能：全局变量、数组作为函数参数、取模运算符

// 1. 阶乘递归
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 2. 斐波那契数列递归
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 3. 递归求和（不用全局变量，只用局部变量）
int sum_recursive(int n) {
    if (n <= 0) {
        return 0;
    }
    return n + sum_recursive(n - 1);
}

int main() {
    // 测试阶乘: 5! = 120
    if (factorial(5) != 120) return 1;

    // 测试斐波那契: fib(10) = 55
    if (fibonacci(10) != 55) return 2;

    // 测试递归求和: sum(1..10) = 55
    if (sum_recursive(10) != 55) return 3;

    // 综合结果: 120 + 55 + 55 = 230
    return factorial(5) + fibonacci(10) + sum_recursive(10);
}
