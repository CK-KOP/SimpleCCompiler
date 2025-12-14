// 测试前向调用：main 调用 foo，但 foo 定义在后面

int main() {
    return foo();
}

int foo() {
    return 42;
}
