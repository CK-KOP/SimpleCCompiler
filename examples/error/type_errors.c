// 类型检查错误测试：测试编译器能否正确检测各种类型不匹配错误
// 预期：所有测试都应该报告语义错误

struct Point {
    int x;
    int y;
};

struct Rectangle {
    int width;
    int height;
};

int test_struct_pointer_mismatch() {
    struct Point p;
    struct Rectangle r;
    // 错误1: 结构体指针类型不匹配
    struct Point *ptr = &r;  // 应该报错：不能将 Rectangle* 赋值给 Point*
    return ptr->x;
}

int test_int_to_pointer() {
    int num = 10;
    // 错误2: int 不能赋值给指针
    struct Point *ptr = num;  // 应该报错：不能将 int 赋值给 Point*
    return ptr->x;
}

int test_pointer_to_int() {
    struct Point p;
    // 错误3: 指针不能赋值给 int
    int num = &p;  // 应该报错：不能将 Point* 赋值给 int
    return num;
}

int test_pointer_level_mismatch() {
    int x = 10;
    int *ptr1 = &x;
    // 错误4: 指针层级不匹配
    int **ptr2 = ptr1;  // 应该报错：不能将 int* 赋值给 int**
    return **ptr2;
}

int test_struct_type_mismatch() {
    struct Point p;
    struct Rectangle r;
    // 错误5: 结构体类型不匹配
    p = r;  // 应该报错：不能将 Rectangle 赋值给 Point
    return p.x;
}

int test_arrow_on_non_pointer() {
    struct Point p;
    // 错误6: 对非指针使用箭头操作符
    return p->x;  // 应该报错：箭头操作符只能用于指针
}

int test_arrow_on_int_pointer() {
    int x = 10;
    int *ptr = &x;
    // 错误7: 对非结构体指针使用箭头操作符
    return ptr->value;  // 应该报错：箭头操作符只能用于结构体指针
}

int test_nonexistent_member() {
    struct Point p;
    struct Point *ptr = &p;
    // 错误8: 访问不存在的成员
    return ptr->z;  // 应该报错：Point 没有成员 z
}

int test_address_of_literal() {
    // 错误9: 对字面量取地址
    int *ptr = &10;  // 应该报错：不能对字面量取地址
    return *ptr;
}

int main() {
    // 注意：这个文件用于测试错误检测，不应该编译通过
    // 每个函数都包含一个或多个类型错误
    return 0;
}
