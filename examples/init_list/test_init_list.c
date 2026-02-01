// ========================================
// 初始化列表综合测试
// 包含：数组、结构体、标量的初始化列表
// ========================================

// 结构体定义
struct Point {
    int x;
    int y;
};

struct Rectangle {
    int width;
    int height;
};

// ========================================
// 全局变量初始化（必须使用常量表达式）
// ========================================

// 数组初始化
int global_arr1[5] = {1, 2, 3, 4, 5};     // 完整初始化
int global_arr2[5] = {10, 20, 30};        // 部分初始化，剩余补0
int global_arr3[3] = {100, 200, 300};     // 正好填满

// 结构体初始化
struct Point global_p1 = {10, 20};        // 完整初始化
struct Point global_p2 = {5};             // 部分初始化，y补0
struct Rectangle global_rect = {100, 50};

// 标量初始化
int global_scalar1 = {42};                // 标量可以使用单元素初始化列表
int global_scalar2 = {100};

int main() {
    // ========================================
    // 局部变量初始化（可以使用运行时表达式）
    // ========================================

    // 数组初始化
    int local_arr1[3] = {1, 2, 3};
    int local_arr2[5] = {10, 20};         // 部分初始化

    // 使用运行时表达式初始化数组
    int x = 5;
    int local_arr3[3] = {x, x + 1, x + 2};
    int local_arr4[4] = {x * 2, x * 3, x * 4, x * 5};

    // 结构体初始化
    struct Point local_p1 = {1, 2};
    struct Point local_p2 = {3};          // 部分初始化

    // 使用运行时表达式初始化结构体
    int a = 100;
    struct Point local_p3 = {a, a + 50};
    struct Rectangle local_rect = {a * 2, a / 2};

    // 标量初始化
    int local_scalar1 = {99};
    int local_scalar2 = {x + 10};         // 运行时表达式

    return 0;
}
