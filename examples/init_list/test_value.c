// 端到端测试：验证初始化列表的值是否正确

struct Point {
    int x;
    int y;
};

// 全局变量
int global_arr[3] = {10, 20, 30};
struct Point global_p = {100, 200};
int global_scalar = {42};

int main() {
    // 测试全局数组
    int sum_global_arr = global_arr[0] + global_arr[1] + global_arr[2];
    // 应该是 10 + 20 + 30 = 60

    // 测试全局结构体
    int sum_global_p = global_p.x + global_p.y;
    // 应该是 100 + 200 = 300

    // 测试全局标量
    int test_scalar = global_scalar;
    // 应该是 42

    // 测试局部数组
    int local_arr[3] = {1, 2, 3};
    int sum_local_arr = local_arr[0] + local_arr[1] + local_arr[2];
    // 应该是 1 + 2 + 3 = 6

    // 测试局部结构体
    struct Point local_p = {5, 10};
    int sum_local_p = local_p.x + local_p.y;
    // 应该是 5 + 10 = 15

    // 测试运行时表达式
    int x = 5;
    int runtime_arr[3] = {x, x + 1, x + 2};
    int sum_runtime = runtime_arr[0] + runtime_arr[1] + runtime_arr[2];
    // 应该是 5 + 6 + 7 = 18

    // 测试部分初始化
    int partial_arr[5] = {1, 2};
    int sum_partial = partial_arr[0] + partial_arr[1] + partial_arr[2] + partial_arr[3] + partial_arr[4];
    // 应该是 1 + 2 + 0 + 0 + 0 = 3

    // 计算总和验证
    int total = sum_global_arr + sum_global_p + test_scalar +
                sum_local_arr + sum_local_p + sum_runtime + sum_partial;
    // 应该是 60 + 300 + 42 + 6 + 15 + 18 + 3 = 444

    return total;
}
