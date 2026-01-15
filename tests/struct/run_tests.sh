#!/bin/bash

# 结构体功能测试脚本

echo "======================================"
echo "  SimpleC 编译器 - 结构体功能测试"
echo "======================================"
echo ""

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试计数
TOTAL=0
PASSED=0
FAILED=0

# 测试函数
run_test() {
    local test_name=$1
    local test_file=$2
    local expected=$3
    local is_error_test=$4

    TOTAL=$((TOTAL + 1))
    echo -n "测试 $TOTAL: $test_name ... "

    if [ "$is_error_test" = "true" ]; then
        # 错误检测测试
        result=$(./build/simplec "$test_file" -s 2>&1 | grep "发现.*个语义错误")
        if echo "$result" | grep -q "$expected"; then
            echo -e "${GREEN}✓ 通过${NC}"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}✗ 失败${NC}"
            echo "  预期: $expected"
            echo "  实际: $result"
            FAILED=$((FAILED + 1))
        fi
    else
        # 正确用例测试
        result=$(./build/simplec "$test_file" 2>&1 | grep "程序返回值:" | awk '{print $2}')
        if [ "$result" = "$expected" ]; then
            echo -e "${GREEN}✓ 通过${NC} (返回值: $result)"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}✗ 失败${NC}"
            echo "  预期返回值: $expected"
            echo "  实际返回值: $result"
            FAILED=$((FAILED + 1))
        fi
    fi
}

# 运行测试
echo "正确用例测试:"
echo "----------------------------------------"
run_test "基础功能" "tests/struct/test_struct_basic.c" "350" "false"
run_test "综合功能" "tests/struct/test_struct_comprehensive.c" "610" "false"
run_test "高级功能" "tests/struct/test_struct_advanced.c" "995" "false"

echo ""
echo "错误检测测试:"
echo "----------------------------------------"
run_test "类型检查" "tests/struct/test_type_errors.c" "10" "true"

# 输出总结
echo ""
echo "======================================"
echo "  测试总结"
echo "======================================"
echo "总计: $TOTAL"
echo -e "${GREEN}通过: $PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}失败: $FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}所有测试通过！${NC}"
    exit 0
fi
