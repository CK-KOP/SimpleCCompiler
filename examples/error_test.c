// 语义错误测试

int main() {
    int x = 10;
    int x = 20;         // 错误：重复声明

    y = 30;             // 错误：未声明的变量

    int z = foo(1, 2);  // 错误：未声明的函数

    return;             // 错误：非void函数应返回值
}

void test() {
    return 42;          // 错误：void函数不应返回值
}
