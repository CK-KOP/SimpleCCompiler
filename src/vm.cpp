#include "../include/vm.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

std::string opcodeName(OpCode op) {
    switch (op) {
        case OpCode::PUSH:   return "PUSH";
        case OpCode::POP:    return "POP";
        case OpCode::LOAD:   return "LOAD";
        case OpCode::STORE:  return "STORE";
        case OpCode::LOADM:  return "LOADM";
        case OpCode::STOREM: return "STOREM";
        case OpCode::LEA:    return "LEA";
        case OpCode::ADDPTR: return "ADDPTR";
        case OpCode::ADDPTRD:return "ADDPTRD";
        case OpCode::ADD:    return "ADD";
        case OpCode::SUB:    return "SUB";
        case OpCode::MUL:    return "MUL";
        case OpCode::DIV:    return "DIV";
        case OpCode::MOD:    return "MOD";
        case OpCode::NEG:    return "NEG";
        case OpCode::EQ:     return "EQ";
        case OpCode::NE:     return "NE";
        case OpCode::LT:     return "LT";
        case OpCode::LE:     return "LE";
        case OpCode::GT:     return "GT";
        case OpCode::GE:     return "GE";
        case OpCode::AND:    return "AND";
        case OpCode::OR:     return "OR";
        case OpCode::NOT:    return "NOT";
        case OpCode::JMP:    return "JMP";
        case OpCode::JZ:     return "JZ";
        case OpCode::JNZ:    return "JNZ";
        case OpCode::CALL:   return "CALL";
        case OpCode::RET:    return "RET";
        case OpCode::PRINT:  return "PRINT";
        case OpCode::HALT:   return "HALT";
        case OpCode::ADJSP:  return "ADJSP";
        case OpCode::POPN:   return "POPN";
        default:             return "???";
    }
}

std::string ByteCode::toString() const {
    std::ostringstream ss;
    for (size_t i = 0; i < code.size(); ++i) {
        ss << i << ":\t" << opcodeName(code[i].op);
        if (code[i].op == OpCode::PUSH || code[i].op == OpCode::LOAD ||
            code[i].op == OpCode::STORE || code[i].op == OpCode::JMP ||
            code[i].op == OpCode::JZ || code[i].op == OpCode::JNZ ||
            code[i].op == OpCode::CALL || code[i].op == OpCode::LEA ||
            code[i].op == OpCode::ADDPTR || code[i].op == OpCode::ADDPTRD ||
            code[i].op == OpCode::ADJSP || code[i].op == OpCode::POPN) {
            ss << " " << code[i].operand;
        }
        ss << "\n";
    }
    return ss.str();
}

void VM::push(int32_t val) {
    if (sp_ >= STACK_SIZE) {
        throw std::runtime_error("Stack overflow");
    }
    stack_[sp_++] = val;
}

int32_t VM::pop() {
    if (sp_ <= 0) {
        throw std::runtime_error("Stack underflow");
    }
    return stack_[--sp_];
}

int VM::execute(const ByteCode& bytecode) {
    if (bytecode.entry_point < 0) {
        throw std::runtime_error("No entry point (main function)");
    }

    // 设置虚拟调用帧：返回地址=-1表示程序结束
    sp_ = 0;
    push(-1);   // 返回地址（-1 表示结束）
    push(0);    // 旧的帧指针
    fp_ = sp_;
    pc_ = bytecode.entry_point;
    running_ = true;

    const auto& code = bytecode.code;

    while (running_ && pc_ >= 0 && pc_ < (int)code.size()) {
        const auto& instr = code[pc_];

        if (debug_) {
            std::cout << "[" << pc_ << "] " << opcodeName(instr.op);
            if (instr.operand != 0) std::cout << " " << instr.operand;
            std::cout << "  (sp=" << sp_ << ", fp=" << fp_ << ")\n";
        }

        pc_++;

        switch (instr.op) {
            case OpCode::PUSH:
                push(instr.operand);
                break;

            case OpCode::POP:
                pop();
                break;

            case OpCode::LOAD:
                push(stack_[fp_ + instr.operand]);
                break;

            case OpCode::STORE:
                stack_[fp_ + instr.operand] = pop();
                break;

            case OpCode::LOADM: {
                // 内存加载: addr = pop(); push(stack[addr])
                int32_t addr = pop();
                push(stack_[addr]);
                break;
            }

            case OpCode::STOREM: {
                // 内存存储: addr = pop(); value = pop(); stack[addr] = value
                int32_t addr = pop();
                int32_t value = pop();
                stack_[addr] = value;
                break;
            }

            case OpCode::LEA:
                // 加载有效地址: push(fp + operand)
                push(fp_ + instr.operand);
                break;

            case OpCode::ADDPTR: {
                // 地址加静态偏移: addr = pop(); push(addr + operand)
                int32_t addr = pop();
                push(addr + instr.operand);
                break;
            }

            case OpCode::ADDPTRD: {
                // 地址加动态偏移: base = pop(); index = pop(); push(base + index * operand)
                int32_t base = pop();
                int32_t index = pop();
                push(base + index * instr.operand);
                break;
            }

            case OpCode::ADD: {
                int32_t b = pop(), a = pop();
                push(a + b);
                break;
            }
            case OpCode::SUB: {
                int32_t b = pop(), a = pop();
                push(a - b);
                break;
            }
            case OpCode::MUL: {
                int32_t b = pop(), a = pop();
                push(a * b);
                break;
            }
            case OpCode::DIV: {
                int32_t b = pop(), a = pop();
                if (b == 0) throw std::runtime_error("Division by zero");
                push(a / b);
                break;
            }
            case OpCode::MOD: {
                int32_t b = pop(), a = pop();
                if (b == 0) throw std::runtime_error("Division by zero");
                push(a % b);
                break;
            }
            case OpCode::NEG:
                push(-pop());
                break;

            case OpCode::EQ: {
                int32_t b = pop(), a = pop();
                push(a == b ? 1 : 0);
                break;
            }
            case OpCode::NE: {
                int32_t b = pop(), a = pop();
                push(a != b ? 1 : 0);
                break;
            }
            case OpCode::LT: {
                int32_t b = pop(), a = pop();
                push(a < b ? 1 : 0);
                break;
            }
            case OpCode::LE: {
                int32_t b = pop(), a = pop();
                push(a <= b ? 1 : 0);
                break;
            }
            case OpCode::GT: {
                int32_t b = pop(), a = pop();
                push(a > b ? 1 : 0);
                break;
            }
            case OpCode::GE: {
                int32_t b = pop(), a = pop();
                push(a >= b ? 1 : 0);
                break;
            }

            case OpCode::AND: {
                int32_t b = pop(), a = pop();
                push((a && b) ? 1 : 0);
                break;
            }
            case OpCode::OR: {
                int32_t b = pop(), a = pop();
                push((a || b) ? 1 : 0);
                break;
            }
            case OpCode::NOT:
                push(pop() == 0 ? 1 : 0);
                break;

            case OpCode::JMP:
                pc_ = instr.operand;
                break;

            case OpCode::JZ:
                if (pop() == 0) pc_ = instr.operand;
                break;

            case OpCode::JNZ:
                if (pop() != 0) pc_ = instr.operand;
                break;

            case OpCode::CALL: {
                // 保存返回地址和帧指针
                push(pc_);
                push(fp_);
                fp_ = sp_;
                pc_ = instr.operand;
                break;
            }

            case OpCode::RET: {
                // 保存返回值（如果栈上有值的话，读取但不pop）
                int32_t retval = (sp_ > fp_) ? stack_[sp_ - 1] : 0;

                // 恢复栈帧
                sp_ = fp_;
                fp_ = pop();  // 恢复旧的帧指针
                int32_t ret_addr = pop();  // 获取返回地址

                // 压入返回值
                push(retval);

                // 跳转
                if (ret_addr == -1) {
                    running_ = false;
                } else {
                    pc_ = ret_addr;
                }
                break;
            }

            case OpCode::PRINT:
                std::cout << "OUTPUT: " << stack_[sp_ - 1] << "\n";
                break;

            case OpCode::HALT:
                running_ = false;
                break;

            case OpCode::ADJSP: {
                // 调整栈指针: sp -= operand
                sp_ -= instr.operand;
                break;
            }

            case OpCode::POPN: {
                // 弹出N个值但保留栈顶: 用于函数调用后清理参数
                int n = instr.operand;
                if (n > 0 && sp_ > 0) {
                    int32_t retval = stack_[sp_ - 1];
                    sp_ -= n;
                    stack_[sp_ - 1] = retval;
                }
                break;
            }
        }
    }

    return sp_ > 0 ? stack_[sp_ - 1] : 0;
}
