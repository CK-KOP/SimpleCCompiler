// 测试多维数组和指针数组

// 测试二维数组
int test_2d_array() {
    int arr[2][3];
    arr[0][0] = 1;
    arr[0][1] = 2;
    arr[0][2] = 3;
    arr[1][0] = 4;
    arr[1][1] = 5;
    arr[1][2] = 6;
    return arr[1][2];  // 6
}

// 测试三维数组
int test_3d_array() {
    int arr[2][2][2];
    arr[0][0][0] = 1;
    arr[0][0][1] = 2;
    arr[0][1][0] = 3;
    arr[0][1][1] = 4;
    arr[1][0][0] = 5;
    arr[1][0][1] = 6;
    arr[1][1][0] = 7;
    arr[1][1][1] = 8;
    return arr[1][1][1];  // 8
}

// 测试指针数组
int test_ptr_array() {
    int a;
    int b;
    int c;
    a = 10;
    b = 20;
    c = 30;
    int *arr[3];
    arr[0] = &a;
    arr[1] = &b;
    arr[2] = &c;
    return *arr[1];  // 20
}

// 测试指针数组改变指向
int test_ptr_array_reassign() {
    int a;
    int b;
    int c;
    a = 100;
    b = 200;
    c = 300;
    int *p[1];

    p[0] = &a;
    int r1;
    r1 = *p[0];  // 100

    p[0] = &b;
    int r2;
    r2 = *p[0];  // 200

    p[0] = &c;
    int r3;
    r3 = *p[0];  // 300

    return r1 + r2 + r3;  // 600
}

int main() {
    int r1;
    int r2;
    int r3;
    int r4;
    r1 = test_2d_array();      // 6
    r2 = test_3d_array();      // 8
    r3 = test_ptr_array();     // 20
    r4 = test_ptr_array_reassign();  // 600
    return r1 + r2 + r3 + r4;  // 634
}
