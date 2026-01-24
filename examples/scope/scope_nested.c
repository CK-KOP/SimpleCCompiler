// 作用域综合测试
// 测试多层嵌套作用域、变量遮蔽

int main() {
    int x = 10;
    int y = 20;
    int z = 0;

    // 第一层嵌套
    {
        int x = 100;  // 遮蔽外层 x
        int w = 5;

        // 验证变量遮蔽
        if (x != 100) return 1;
        if (y != 20) return 2;  // y 仍然是外层的

        // 第二层嵌套
        {
            int x = 1000;  // 再次遮蔽
            int y = 200;   // 遮蔽外层 y
            int v = 10;

            if (x != 1000) return 3;
            if (y != 200) return 4;
            if (w != 5) return 5;    // w 来自上一层
            if (v != 10) return 6;

            z = x + y + w + v;  // z = 1000 + 200 + 5 + 10 = 1215

            // 第三��嵌套
            {
                int a = x / 10;
                int b = y / 10;
                z = z + a + b;  // z = 1215 + 100 + 20 = 1335
            }

            // 回到第二层
            if (x != 1000) return 7;
        }

        // 回到第一层
        if (x != 100) return 8;
        if (y != 20) return 9;  // y 又是外层的了
    }

    // 回到最外层
    if (x != 10) return 10;
    if (y != 20) return 11;
    if (z != 1335) return 12;

    // 更多嵌套测试
    int result = 0;
    {
        int a = 1;
        {
            int b = 2;
            {
                int c = 3;
                {
                    int d = 4;
                    result = a + b + c + d;  // result = 10
                }
            }
        }
    }

    if (result != 10) return 13;

    return x + y + z + result;  // 10 + 20 + 1335 + 10 = 1375
}
