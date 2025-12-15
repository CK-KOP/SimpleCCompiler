#include "../include/vm.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

std::string opcodeName(OpCode op) {
    switch (op) {
        case OpCode::PUSH:  return "PUSH";
        case OpCode::POP:   return "POP";
        case OpCode::LOAD:  return "LOAD";
        case OpCode::STORE: return "STORE";
        case OpCode::LOADI: return "LOADI";
        case OpCode::STOREI:return "STOREI";
        case OpCode::ADD:   return "ADD";
        case OpCode::SUB:   return "SUB";
        case OpCode::MUL:   return "MUL";
        case OpCode::DIV:   return "DIV";
        case OpCode::MOD:   return "MOD";
        case OpCode::NEG:   return "NEG";
        case OpCode::EQ:    return "EQ";
        case OpCode::NE:    return "NE";
        case OpCode::LT:    return "LT";
        case OpCode::LE:    return "LE";
        case OpCode::GT:    return "GT";
        case OpCode::GE:    return "GE";
        case OpCode::AND:   return "AND";
        case OpCode::OR:    return "OR";
        case OpCode::NOT:   return "NOT";
        case OpCode::JMP:   return "JMP";
        case OpCode::JZ:    return "JZ";
        case OpCode::JNZ:   return "JNZ";
        case OpCode::CALL:  return "CALL";
        case OpCode::RET:   return "RET";
        case OpCode::RETV:  return "RETV";
        case OpCode::PRINT: return "PRINT";
        case OpCode::HALT:  return "HALT";
        case OpCode::POPN:  return "POPN";
        case OpCode::ADJSP: return "ADJSP";
        default:            return "???";
    }
}

std::string ByteCode::toString() const {
    std::ostringstream ss;
    for (size_t i = 0; i < code.size(); ++i) {
        ss << i << ":\t" << opcodeName(code[i].op);
        if (code[i].op == OpCode::PUSH || code[i].op == OpCode::LOAD ||
            code[i].op == OpCode::STORE || code[i].op == OpCode::JMP ||
            code[i].op == OpCode::JZ || code[i].op == OpCode::JNZ ||
            code[i].op == OpCode::CALL || code[i].op == OpCode::POPN ||
            code[i].op == OpCode::LOADI || code[i].op == OpCode::STOREI ||
            code[i].op == OpCode::ADJSP) {
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

            case OpCode::LOADI: {
                // 间接加载：基地址在operand，偏移在栈顶
                // stack[fp + base + index]
                int32_t index = pop();
                push(stack_[fp_ + instr.operand + index]);
                break;
            }

            case OpCode::STOREI: {
                // 间接存储：基地址在operand，偏移和值在栈上
                // 栈顶是value，栈顶-1是index
                int32_t value = pop();
                int32_t index = pop();
                stack_[fp_ + instr.operand + index] = value;
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
                // 恢复帧指针和返回地址
                sp_ = fp_;
                fp_ = pop();
                pc_ = pop();
                break;
            }

            case OpCode::RETV: {
                int32_t retval = pop();
                sp_ = fp_;
                fp_ = pop();
                pc_ = pop();
                push(retval);
                break;
            }

            case OpCode::PRINT:
                std::cout << "OUTPUT: " << stack_[sp_ - 1] << "\n";
                break;

            case OpCode::HALT:
                running_ = false;
                break;

            case OpCode::POPN: {
                // 弹出 N 个值，但保留栈顶
                int n = instr.operand;
                if (n > 0) {
                    int32_t top = stack_[sp_ - 1];
                    sp_ -= n;
                    stack_[sp_ - 1] = top;
                }
                break;
            }

            case OpCode::ADJSP: {
                // 简单调整栈指针，不保留任何值
                sp_ -= instr.operand;
                break;
            }
        }
    }

    return sp_ > 0 ? stack_[sp_ - 1] : 0;
}
