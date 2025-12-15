#include "../include/lexer.h"
#include <fstream>
#include <sstream>

Lexer::Lexer(const std::string& source)
    : source_(source) {
    initialize();
}

Lexer::Lexer(const char* filename) {
    source_ = loadFromFile(filename);
    initialize();
}

Lexer::~Lexer() {
}

Token Lexer::getNextToken() {
    // 如果有缓存的Token，直接返回
    if (has_cached_token_) {
        has_cached_token_ = false;
        return current_token_;
    }

    // 跳过空白字符
    skipWhitespace();

    // 检查是否到达结尾
    if (isAtEnd()) {
        return Token(TokenType::End, "", line_, column_);
    }

    char c = getCurrentChar();

    // 处理数字
    if (isDigit(c)) {
        return readNumber();
    }

    // 处理标识符（以字母开头）
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        return readIdentifier();
    }

    // 处理运算符和分隔符
    switch (c) {
        // 单字符运算符
        case '+': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::Plus, "+", token_line, token_column);
        }
        case '-': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::Minus, "-", token_line, token_column);
        }
        case '*': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::Multiply, "*", token_line, token_column);
        }
        case '/': {
            int token_line = line_;
            int token_column = column_;
            advance();
            // 检查是否是单行注释
            if (getCurrentChar() == '/') {
                skipSingleLineComment();
                return getNextToken(); // 递归获取下一个Token
            }
            return Token(TokenType::Divide, "/", token_line, token_column);
        }
        case '(': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::LParen, "(", token_line, token_column);
        }
        case ')': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::RParen, ")", token_line, token_column);
        }
        case '{': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::LBrace, "{", token_line, token_column);
        }
        case '}': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::RBrace, "}", token_line, token_column);
        }
        case '[': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::LBracket, "[", token_line, token_column);
        }
        case ']': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::RBracket, "]", token_line, token_column);
        }
        case ';': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::Semicolon, ";", token_line, token_column);
        }
        case ',': {
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::Comma, ",", token_line, token_column);
        }

        // 双字符运算符
        case '=': {
            return readMultiCharOperator(TokenType::Assign);
        }
        case '!': {
            return readMultiCharOperator(TokenType::LogicalNot);
        }
        case '<': {
            return readMultiCharOperator(TokenType::Less);
        }
        case '>': {
            return readMultiCharOperator(TokenType::Greater);
        }
        case '&': {
            return readMultiCharOperator(TokenType::LogicalAnd);
        }
        case '|': {
            return readMultiCharOperator(TokenType::LogicalOr);
        }

        default:
            // 未知字符
            std::string error_msg = "未知字符: '";
            error_msg += c;
            error_msg += "'";
            int token_line = line_;
            int token_column = column_;
            advance();
            return Token(TokenType::Invalid, error_msg, token_line, token_column);
    }
}

Token Lexer::peekNextToken() {
    // 如果已经有缓存的Token，直接返回
    if (has_cached_token_) {
        return current_token_;
    }

    // 获取下一个Token并缓存
    current_token_ = getNextToken();
    has_cached_token_ = true;
    return current_token_;
}

char Lexer::getCurrentChar() const {
    if (isAtEnd()) {
        return '\0';
    }
    return source_[current_pos_];
}

void Lexer::advance() {
    if (!isAtEnd()) {
        char c = source_[current_pos_];
        current_pos_++;

        // 更新行列信息
        if (c == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
    }
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && isWhitespace(getCurrentChar())) {
        advance();
    }
}

Token Lexer::readNumber() {
    int start_pos = current_pos_;
    int start_line = line_;
    int start_column = column_;

    // 读取所有数字
    while (!isAtEnd() && isDigit(getCurrentChar())) {
        advance();
    }

    // 提取数字字符串
    std::string number_str = source_.substr(start_pos, current_pos_ - start_pos);

    return Token(TokenType::Number, number_str, start_line, start_column);
}

void Lexer::reset() {
    initialize();
}

void Lexer::initialize() {
    current_pos_ = 0;
    line_ = 1;
    column_ = 1;
    has_cached_token_ = false;
    current_token_ = Token(TokenType::Invalid, "", 1, 1);  // 初始化缓存Token
}

Token Lexer::readIdentifier() {
    int start_pos = current_pos_;
    int start_line = line_;
    int start_column = column_;

    // 读取标识符（字母、数字、下划线）
    while (!isAtEnd() && (getCurrentChar() == '_' ||
                         (getCurrentChar() >= 'a' && getCurrentChar() <= 'z') ||
                         (getCurrentChar() >= 'A' && getCurrentChar() <= 'Z') ||
                         (getCurrentChar() >= '0' && getCurrentChar() <= '9'))) {
        advance();
    }

    std::string identifier = source_.substr(start_pos, current_pos_ - start_pos);

    // 检查是否是关键字
    if (isKeyword(identifier)) {
        TokenType type = getKeywordType(identifier);
        return Token(type, identifier, start_line, start_column);
    }

    return Token(TokenType::Identifier, identifier, start_line, start_column);
}

void Lexer::skipSingleLineComment() {
    // 跳过第二个 '/'
    advance();

    // 跳过直到行尾的所有字符
    while (!isAtEnd() && getCurrentChar() != '\n') {
        advance();
    }
}

// 统一读取多字符运算符（处理 =, ==, !, !=, <, <=, >, >=, &, &&, |, ||）
Token Lexer::readMultiCharOperator(TokenType type) {
    int token_line = line_;
    int token_column = column_;
    advance(); // 消费第一个字符

    char second_char = getCurrentChar();

    switch (type) {
        case TokenType::Assign:           // = 或 ==
            if (second_char == '=') {
                advance(); // 消费=
                return Token(TokenType::Equal, "==", token_line, token_column);
            } else {
                return Token(TokenType::Assign, "=", token_line, token_column);
            }

        case TokenType::LogicalNot:       // ! 或 !=
            if (second_char == '=') {
                advance(); // 消费=
                return Token(TokenType::NotEqual, "!=", token_line, token_column);
            } else {
                return Token(TokenType::LogicalNot, "!", token_line, token_column);
            }

        case TokenType::Less:             // < 或 <=
            if (second_char == '=') {
                advance(); // 消费=
                return Token(TokenType::LessEqual, "<=", token_line, token_column);
            } else {
                return Token(TokenType::Less, "<", token_line, token_column);
            }

        case TokenType::Greater:          // > 或 >=
            if (second_char == '=') {
                advance(); // 消费=
                return Token(TokenType::GreaterEqual, ">=", token_line, token_column);
            } else {
                return Token(TokenType::Greater, ">", token_line, token_column);
            }

        case TokenType::LogicalAnd:       // &&
            if (second_char == '&') {
                advance(); // 消费&
                return Token(TokenType::LogicalAnd, "&&", token_line, token_column);
            } else {
                return Token(TokenType::Invalid, "无效运算符", token_line, token_column);
            }

        case TokenType::LogicalOr:        // ||
            if (second_char == '|') {
                advance(); // 消费|
                return Token(TokenType::LogicalOr, "||", token_line, token_column);
            } else {
                return Token(TokenType::Invalid, "无效运算符", token_line, token_column);
            }

        default:
            return Token(TokenType::Invalid, "无效运算符", token_line, token_column);
    }
}

bool Lexer::isKeyword(const std::string& str) {
    return str == "int" || str == "void" || str == "return" ||
           str == "if" || str == "else" ||
           str == "while" || str == "for" || str == "do" ||
           str == "break" || str == "continue";
}

TokenType Lexer::getKeywordType(const std::string& str) {
    if (str == "int") {
        return TokenType::Int;
    } else if (str == "void") {
        return TokenType::Void;
    } else if (str == "return") {
        return TokenType::Return;
    } else if (str == "if") {
        return TokenType::If;
    } else if (str == "else") {
        return TokenType::Else;
    } else if (str == "while") {
        return TokenType::While;
    } else if (str == "for") {
        return TokenType::For;
    } else if (str == "do") {
        return TokenType::Do;
    } else if (str == "break") {
        return TokenType::Break;
    } else if (str == "continue") {
        return TokenType::Continue;
    }
    return TokenType::Invalid;
}

std::string Lexer::loadFromFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + std::string(filename));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

