// if-else 分支测试
// 测试多个 else if 分支的跳转

int main() {
    int x = 85;
    int result = 0;

    if (x > 90) {
        result = 1;
    } else if (x > 80) {
        result = 2;
    } else if (x > 60) {
        result = 3;
    } else {
        result = 4;
    }

    // x=85 应该返回 2
    return result;
}
