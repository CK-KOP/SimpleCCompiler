#ifndef VM_H
#define VM_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

// 虚拟机指令
enum class OpCode : uint8_t {
    // 栈操作
    PUSH,       // 压入常量
    POP,        // 弹出

    // 变量操作
    LOAD,       // 加载局部变量: push(stack[fp + operand])
    STORE,      // 存储局部变量: stack[fp + operand] = pop()
    LOADM,      // 内存加载: addr = pop(); push(stack[addr] 或 globals_[addr - GLOBAL_BASE])
    STOREM,     // 内存存储: addr = pop(); value = pop(); stack[addr] 或 globals_[...] = value

    // 全局变量操作 (Phase 6)
    LOADG,      // 加载全局变量: push(globals_[operand])
    STOREG,     // 存储全局变量: globals_[operand] = pop()
    LEAG,       // 加载全局变量地址: push(GLOBAL_BASE + operand)

    // 地址计算
    LEA,        // 加载有效地址: push(fp + operand)
    ADDPTR,     // 地址加静态偏移: addr = pop(); push(addr + operand)
    ADDPTRD,    // 地址加动态偏移: base = pop(); index = pop(); push(base + index * operand)

    // 算术运算
    ADD, SUB, MUL, DIV, MOD,
    NEG,        // 取负

    // 比较运算
    EQ, NE, LT, LE, GT, GE,

    // 逻辑运算
    AND, OR, NOT,

    // 控制流
    JMP,        // 无条件跳转
    JZ,         // 栈顶为0时跳转
    JNZ,        // 栈顶非0时跳转

    // 函数
    CALL,       // 调用函数
    RET,        // 返回: operand = ret_slot_offset (相对于 fp)
                // TODO: 支持 struct 返回值时，需考虑多 slot 写入

    // 其他
    PRINT,      // 打印栈顶（调试用）
    HALT,       // 停止
    ADJSP,      // 调整栈指针: sp -= operand
    MEMCPY      // 内存复制: size = operand; dst = pop(); src = pop();
                // 复制 size 个 slot: stack[dst..dst+size-1] = stack[src..src+size-1]
};

// 单条指令
struct Instruction {
    OpCode op;
    int32_t operand;

    Instruction(OpCode o, int32_t val = 0) : op(o), operand(val) {}
};

// 全局变量初始化信息 (Phase 6)
struct GlobalVarInit {
    int offset;           // 全局变量偏移
    int slot_count;       // 占用的 slot 数
    std::vector<int32_t> init_data;  // 初始化数据（可以是多个值）

    // init_data 为空 = 全部初始化为 0
    // init_data 有值 = 按顺序初始化每个 slot，剩余 slot 初始化为 0
    // 支持：
    //   - 单值：{42} → int x = 42;
    //   - 多值：{1, 2, 3} → int arr[3] = {1, 2, 3};
    //   - 指针：{GLOBAL_BASE + offset} → int *p = &global_x;
    //   - 结构体：{10, 20} → struct Point p = {10, 20};

    GlobalVarInit() : offset(0), slot_count(0) {}
};

// 字节码程序
class ByteCode {
public:
    std::vector<Instruction> code;
    std::unordered_map<std::string, int> functions;  // 函数名 -> 地址
    std::vector<GlobalVarInit> global_inits;         // 全局变量初始化信息 (Phase 6)
    int entry_point = -1;

    void emit(OpCode op, int32_t operand = 0) {
        code.emplace_back(op, operand);
    }

    int currentAddress() const { return code.size(); }

    void patch(int addr, int target) {
        code[addr].operand = target;
    }

    std::string toString() const;
};

// 栈式虚拟机
class VM {
public:
    static const int GLOBAL_BASE = 0x40000000;  // 全局变量地址基址（Phase 6）

private:
    static const int STACK_SIZE = 4096;

    std::vector<int32_t> stack_;
    std::vector<int32_t> globals_;  // 全局变量存储区（Phase 6）
    int sp_ = 0;    // 栈指针
    int fp_ = 0;    // 帧指针
    int pc_ = 0;    // 程序计数器
    bool running_ = false;
    bool debug_ = false;

public:
    VM() : stack_(STACK_SIZE, 0) {}

    int execute(const ByteCode& code);
    void setDebug(bool d) { debug_ = d; }

private:
    void push(int32_t val);
    int32_t pop();
};

std::string opcodeName(OpCode op);

#endif // VM_H
