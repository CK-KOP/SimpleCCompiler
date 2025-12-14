# SimpleC编译器 Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -I include
SRCDIR = src
TESTDIR = tests
BUILDDIR = build

# 核心源文件
CORE_SRC = $(SRCDIR)/lexer.cpp $(SRCDIR)/parser.cpp $(SRCDIR)/token.cpp $(SRCDIR)/type.cpp $(SRCDIR)/sema.cpp $(SRCDIR)/vm.cpp $(SRCDIR)/codegen.cpp
CORE_OBJ = $(BUILDDIR)/lexer.o $(BUILDDIR)/parser.o $(BUILDDIR)/token.o $(BUILDDIR)/type.o $(BUILDDIR)/sema.o $(BUILDDIR)/vm.o $(BUILDDIR)/codegen.o

# 测试文件列表
TEST_FILES = $(wildcard $(TESTDIR)/test_*.cpp)
TEST_BINS = $(TEST_FILES:$(TESTDIR)/%.cpp=$(BUILDDIR)/%)

# 主程序
MAIN_BIN = $(BUILDDIR)/simplec

# 默认目标
.PHONY: all clean test help

all: $(MAIN_BIN)
	@echo ""
	@echo "SimpleC编译器构建完成！"
	@echo "用法: ./build/simplec <源文件> [选项]"

# 创建构建目录
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# 编译核心源文件
$(BUILDDIR)/lexer.o: $(SRCDIR)/lexer.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/parser.o: $(SRCDIR)/parser.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/token.o: $(SRCDIR)/token.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/type.o: $(SRCDIR)/type.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/sema.o: $(SRCDIR)/sema.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/vm.o: $(SRCDIR)/vm.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/codegen.o: $(SRCDIR)/codegen.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 链接主程序
$(MAIN_BIN): $(CORE_OBJ) main.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(CORE_OBJ) main.cpp -o $@

# 编译测试文件
$(BUILDDIR)/test_%: $(TESTDIR)/test_%.cpp $(CORE_OBJ) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(CORE_OBJ) $< -o $@

# 运行所有测试
test: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "运行 $$test..."; \
		$$test; \
	done

# 清理
clean:
	rm -rf $(BUILDDIR)

# 帮助信息
help:
	@echo "SimpleC编译器"
	@echo ""
	@echo "目标："
	@echo "  all    - 构建编译器 (默认)"
	@echo "  test   - 运行所有测试"
	@echo "  clean  - 清理构建文件"
	@echo ""
	@echo "使用："
	@echo "  ./build/simplec <file.c>      # 语法分析"
	@echo "  ./build/simplec <file.c> -l   # 仅词法分析"
