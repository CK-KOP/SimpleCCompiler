#include "../include/token.h"

Token::Token(TokenType type, const std::string& value, int line, int column)
    : type_(type), value_(value), line_(line), column_(column) {
}

std::string Token::toString() const {
    return "Token(" + typeToString(type_) +
           (value_.empty() ? "" : ", \"" + value_ + "\"") +
           ", line=" + std::to_string(line_) +
           ", col=" + std::to_string(column_) + ")";
}

std::string Token::typeToString(TokenType type) {
    switch (type) {
        case TokenType::Number:    return "Number";
        case TokenType::Plus:      return "Plus";
        case TokenType::Minus:     return "Minus";
        case TokenType::Multiply:  return "Multiply";
        case TokenType::Divide:    return "Divide";
        case TokenType::LParen:    return "LParen";
        case TokenType::RParen:    return "RParen";
        case TokenType::LBrace:    return "LBrace";
        case TokenType::RBrace:    return "RBrace";
        case TokenType::LBracket:  return "LBracket";
        case TokenType::RBracket:  return "RBracket";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Comma:     return "Comma";
        case TokenType::Assign:    return "Assign";
        case TokenType::Equal:     return "Equal";
        case TokenType::NotEqual:    return "NotEqual";
        case TokenType::Less:        return "Less";
        case TokenType::LessEqual:   return "LessEqual";
        case TokenType::Greater:     return "Greater";
        case TokenType::GreaterEqual:return "GreaterEqual";
        case TokenType::LogicalAnd: return "LogicalAnd";
        case TokenType::LogicalOr:  return "LogicalOr";
        case TokenType::LogicalNot: return "LogicalNot";
        case TokenType::Int:       return "Int";
        case TokenType::Void:      return "Void";
        case TokenType::Return:    return "Return";
        case TokenType::If:        return "If";
        case TokenType::Else:      return "Else";
        case TokenType::While:     return "While";
        case TokenType::For:       return "For";
        case TokenType::Do:        return "Do";
        case TokenType::Break:     return "Break";
        case TokenType::Continue:  return "Continue";
        case TokenType::Identifier:return "Identifier";
        case TokenType::End:       return "End";
        case TokenType::Invalid:   return "Invalid";
        default:                   return "Unknown";
    }
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.toString();
    return os;
}

std::ostream& operator<<(std::ostream& os, const TokenType& type) {
    os << Token::typeToString(type);
    return os;
}