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

    // 处理运算符
    switch (c) {
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
            return Token(TokenType::Divide, "/", token_line, token_column);
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

std::string Lexer::loadFromFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + std::string(filename));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}