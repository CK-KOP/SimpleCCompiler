// 综合测试：购物车系统
// 测试多种功能：数组、控制流、作用域的综合运用
// TODO: 暂不支持的功能：数组作为函数参数、取模运算符

int main() {
    // 购物车：5个商品的价格
    int prices[5];
    prices[0] = 99;
    prices[1] = 199;
    prices[2] = 49;
    prices[3] = 299;
    prices[4] = 149;

    // 1. 计算总价
    int total = 0;
    int i = 0;
    while (i < 5) {
        total = total + prices[i];
        i = i + 1;
    }
    if (total != 795) return 1;  // 99+199+49+299+149 = 795

    // 2. 找出最贵的商品（手动比较）
    int max_price = prices[0];
    int j = 1;
    while (j < 5) {
        if (prices[j] > max_price) {
            max_price = prices[j];
        }
        j = j + 1;
    }
    if (max_price != 299) return 2;

    // 3. 排序价格（冒泡排序）
    int k = 0;
    while (k < 5) {
        int l = 0;
        while (l < 4) {
            if (prices[l] > prices[l + 1]) {
                int temp = prices[l];
                prices[l] = prices[l + 1];
                prices[l + 1] = temp;
            }
            l = l + 1;
        }
        k = k + 1;
    }
    if (prices[0] != 49) return 3;
    if (prices[4] != 299) return 4;

    // 4. 计算折扣
    int discount = 0;
    if (total > 500) {
        if (total > 700) {
            discount = total / 10;  // 10% 折扣
        } else {
            discount = total / 20;  // 5% 折扣
        }
    }

    // 5. 嵌套作用域测试
    {
        int final_price = total - discount; // 795 - 79 = 716
        int tax = 0;

        {
            if (final_price > 600) {
                tax = final_price / 20;  // 5% 税  716 / 20 = 35
            } else {
                tax = 0;
            }
        }

        // 6. 循环计算积分
        int points = 0;
        int m = 0;
        for (m = 0; m < 5; m = m + 1) {
            if (prices[m] > 100) {
                points = points + (prices[m] / 10);
            } else {
                points = points + 5;
            }
        }

        // points = 5 + 5 + 14 + 19 + 29 = 72
        // 716 + 35 + 72 = 823
        return final_price + tax + points;
    }
}
