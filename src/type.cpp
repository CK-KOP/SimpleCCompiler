#include "../include/type.h"

// 单例：int 类型
std::shared_ptr<Type> Type::getIntType() {
    static auto int_type = std::make_shared<PrimaryType>(TypeKind::Int);
    return int_type;
}

// 单例：void 类型
std::shared_ptr<Type> Type::getVoidType() {
    static auto void_type = std::make_shared<PrimaryType>(TypeKind::Void);
    return void_type;
}
