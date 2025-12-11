# 更通用的Makefile示例
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -I include
SRCDIR = src
TESTDIR = tests
EXAMPLEDIR = examples
BUILDDIR = build

# 源文件列表（自动发现）
SRC_FILES = $(wildcard $(SRCDIR)/*.cpp)
OBJ_FILES = $(SRC_FILES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# 测试文件列表
TEST_FILES = $(wildcard $(TESTDIR)/test_*.cpp)
TEST_BINS = $(TEST_FILES:$(TESTDIR)/%.cpp=$(BUILDDIR)/%)

# 示例文件列表
EXAMPLE_FILES = $(wildcard $(EXAMPLEDIR)/*.cpp)
EXAMPLE_BINS = $(EXAMPLE_FILES:$(EXAMPLEDIR)/%.cpp=$(BUILDDIR)/%)

# 默认目标
.PHONY: all clean test examples run-demo help

all: $(BUILDDIR)
	@echo "SimpleC编译器 - 第一阶段：基础词法分析器"
	@echo "可用的make目标："
	@echo "  test     - 编译并运行测试"
	@echo "  examples - 编译并运行示例"
	@echo "  run-demo - 编译并运行演示程序"
	@echo "  clean    - 清理构建文件"
	@echo "  info     - 显示项目信息"
	@echo "  help     - 显示帮助信息"

# 创建构建目录
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# 编译源文件为目标文件的规则
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译测试文件的规则
$(BUILDDIR)/%: $(TESTDIR)/%.cpp $(OBJ_FILES) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) $< -o $@

# 编译示例文件的规则
$(BUILDDIR)/%: $(EXAMPLEDIR)/%.cpp $(OBJ_FILES) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) $< -o $@

# 运行所有测试
test: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "运行 $$test..."; \
		$$test; \
	done

# 运行示例
examples: $(EXAMPLE_BINS)
	@for example in $(EXAMPLE_BINS); do \
		echo "运行 $$example..."; \
		$$example; \
	done

# 运行演示程序
run-demo: $(BUILDDIR)/lexer_demo
	./$(BUILDDIR)/lexer_demo

# 清理
clean:
	rm -rf $(BUILDDIR)

# 显示信息
info:
	@echo "源文件: $(SRC_FILES)"
	@echo "目标文件: $(OBJ_FILES)"
	@echo "测试文件: $(TEST_FILES)"
	@echo "测试程序: $(TEST_BINS)"
	@echo "示例程序: $(EXAMPLE_BINS)"

# 帮助信息
help:
	@echo "SimpleC编译器构建系统"
	@echo ""
	@echo "主要目标："
	@echo "  test     - 编译并运行所有测试"
	@echo "  run-demo - 编译并运行演示程序"
	@echo "  examples - 编译并运行所有示例"
	@echo "  clean    - 清理构建文件"
	@echo "  info     - 显示项目文件信息"
	@echo "  help     - 显示此帮助信息"
	@echo ""
	@echo "示例："
	@echo "  make test     # 编译并运行测试"
	@echo "  make run-demo # 编译并运行演示程序"
	@echo "  make clean    # 清理构建文件"