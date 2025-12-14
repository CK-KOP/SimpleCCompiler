#ifndef SCOPE_H
#define SCOPE_H

#include "type.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

// 符号种类
enum class SymbolKind {
    Variable,   // 变量
    Function,   // 函数
    Parameter,  // 函数参数
};

// 符号：代表一个声明的变量或函数
class Symbol {
private:
    std::string name_;
    std::shared_ptr<Type> type_;
    SymbolKind kind_;

public:
    Symbol(const std::string& name, std::shared_ptr<Type> type, SymbolKind kind)
        : name_(name), type_(type), kind_(kind) {}

    const std::string& getName() const { return name_; }
    std::shared_ptr<Type> getType() const { return type_; }
    SymbolKind getKind() const { return kind_; }

    bool isVariable() const { return kind_ == SymbolKind::Variable; }
    bool isFunction() const { return kind_ == SymbolKind::Function; }
    bool isParameter() const { return kind_ == SymbolKind::Parameter; }
};

// 环境：一个作用域内的符号表
class Env {
private:
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols_;

public:
    // 添加符号，返回是否成功（重复声明返回 false）
    bool addSymbol(std::shared_ptr<Symbol> symbol) {
        auto [it, inserted] = symbols_.emplace(symbol->getName(), symbol);
        return inserted;
    }

    // 在当前环境中查找符号
    std::shared_ptr<Symbol> findSymbol(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) {
            return it->second;
        }
        return nullptr;
    }
};

// 作用域管理器：管理嵌套的作用域
class Scope {
private:
    std::vector<std::shared_ptr<Env>> envs_;  // 作用域栈

public:
    Scope() {
        // 创建全局作用域
        enterScope();
    }

    // 进入新作用域
    void enterScope() {
        envs_.push_back(std::make_shared<Env>());
    }

    // 退出当前作用域
    void exitScope() {
        if (envs_.size() > 1) {  // 保留全局作用域
            envs_.pop_back();
        }
    }

    // 在当前作用域添加符号
    bool addSymbol(const std::string& name, std::shared_ptr<Type> type, SymbolKind kind) {
        auto symbol = std::make_shared<Symbol>(name, type, kind);
        return envs_.back()->addSymbol(symbol);
    }

    // 在所有作用域中查找符号（从内到外）
    std::shared_ptr<Symbol> findSymbol(const std::string& name) const {
        for (auto it = envs_.rbegin(); it != envs_.rend(); ++it) {
            auto symbol = (*it)->findSymbol(name);
            if (symbol) {
                return symbol;
            }
        }
        return nullptr;
    }

    // 仅在当前作用域查找符号
    std::shared_ptr<Symbol> findSymbolInCurrentScope(const std::string& name) const {
        return envs_.back()->findSymbol(name);
    }

    // 获取当前作用域深度
    size_t depth() const { return envs_.size(); }

    // 是否在全局作用域
    bool isGlobalScope() const { return envs_.size() == 1; }
};

#endif // SCOPE_H
