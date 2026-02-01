// 全局变量初始化扩展性测试
// 展示重构后支持的所有初始化方式

// 1. 基本常量初始化
int basic = 42;

// 2. 算术表达式初始化
int expr_add = 10 + 20;
int expr_mul = 5 * 6;
int expr_complex = (10 + 5) * 2 - 3;

// 3. 比较和逻辑表达式初始化
int cmp_result = 10 < 20;
int logic_and = (5 > 3) && (10 == 10);
int logic_or = (1 == 2) || (3 < 5);
int logic_not = !(0);

// 4. 负数初始化
int negative = -100;
int neg_expr = -(10 + 5);

// 5. 指针初始化（取全局变量地址）
int target1 = 999;
int target2 = 888;
int *ptr1 = &target1;
int *ptr2 = &target2;

// 6. 未初始化（默认为 0）
int uninitialized;
int arr[3];

int main() {
    // 测试基本常量
    if (basic != 42) return 1;

    // 测试算术表达式
    if (expr_add != 30) return 2;
    if (expr_mul != 30) return 3;
    if (expr_complex != 27) return 4;  // (10+5)*2-3 = 27

    // 测试比较和逻辑表达式
    if (cmp_result != 1) return 5;
    if (logic_and != 1) return 6;
    if (logic_or != 1) return 7;
    if (logic_not != 1) return 8;

    // 测试负数
    if (negative != -100) return 9;
    if (neg_expr != -15) return 10;

    // 测试指针
    if (*ptr1 != 999) return 11;
    if (*ptr2 != 888) return 12;

    // 通过指针修改
    *ptr1 = 111;
    *ptr2 = 222;
    if (target1 != 111) return 13;
    if (target2 != 222) return 14;

    // 测试未初始化
    if (uninitialized != 0) return 15;
    if (arr[0] != 0) return 16;
    if (arr[1] != 0) return 17;
    if (arr[2] != 0) return 18;

    return 0;  // 所有测试通过
}
