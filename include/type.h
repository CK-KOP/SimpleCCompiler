#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <memory>
#include <vector>

// 类型种类
enum class TypeKind {
    Int,        // int
    Void,       // void
    Function,   // 函数类型
    Pointer,    // 指针类型 (预留)
    Array,      // 数组类型
};

// 类型基类
class Type {
protected:
    TypeKind kind_;

public:
    explicit Type(TypeKind kind) : kind_(kind) {}
    virtual ~Type() = default;

    TypeKind getKind() const { return kind_; }

    bool isInt() const { return kind_ == TypeKind::Int; }
    bool isVoid() const { return kind_ == TypeKind::Void; }
    bool isFunction() const { return kind_ == TypeKind::Function; }
    bool isPointer() const { return kind_ == TypeKind::Pointer; }
    bool isArray() const { return kind_ == TypeKind::Array; }

    virtual std::string toString() const = 0;

    // 预定义的基本类型（单例）
    static std::shared_ptr<Type> getIntType();
    static std::shared_ptr<Type> getVoidType();
};

// 基本类型 (int, void)
class PrimaryType : public Type {
public:
    explicit PrimaryType(TypeKind kind) : Type(kind) {}

    std::string toString() const override {
        switch (kind_) {
            case TypeKind::Int: return "int";
            case TypeKind::Void: return "void";
            default: return "unknown";
        }
    }
};

// 函数类型
class FunctionType : public Type {
public:
    struct Param {
        std::shared_ptr<Type> type;
        std::string name;

        Param(std::shared_ptr<Type> t, const std::string& n)
            : type(t), name(n) {}
    };

private:
    std::shared_ptr<Type> return_type_;
    std::vector<Param> params_;

public:
    FunctionType(std::shared_ptr<Type> ret_type, std::vector<Param> params)
        : Type(TypeKind::Function), return_type_(ret_type), params_(std::move(params)) {}

    std::shared_ptr<Type> getReturnType() const { return return_type_; }
    const std::vector<Param>& getParams() const { return params_; }

    std::string toString() const override {
        std::string result = return_type_->toString() + "(";
        for (size_t i = 0; i < params_.size(); ++i) {
            if (i > 0) result += ", ";
            result += params_[i].type->toString();
        }
        result += ")";
        return result;
    }
};

// 指针类型 (预留)
class PointerType : public Type {
private:
    std::shared_ptr<Type> base_type_;

public:
    explicit PointerType(std::shared_ptr<Type> base)
        : Type(TypeKind::Pointer), base_type_(base) {}

    std::shared_ptr<Type> getBaseType() const { return base_type_; }

    std::string toString() const override {
        return base_type_->toString() + "*";
    }
};

// 数组类型
class ArrayType : public Type {
private:
    std::shared_ptr<Type> element_type_;  // 元素类型
    int size_;                             // 数组大小

public:
    ArrayType(std::shared_ptr<Type> elem, int size)
        : Type(TypeKind::Array), element_type_(elem), size_(size) {}

    std::shared_ptr<Type> getElementType() const { return element_type_; }
    int getSize() const { return size_; }

    std::string toString() const override {
        return element_type_->toString() + "[" + std::to_string(size_) + "]";
    }
};

#endif // TYPE_H
