#pragma once
#include "cvm.h"
#include <stdexcept>
#include <cctype>

class Lexer {
    std::string src;
    size_t pos = 0;
    int line = 1;

    char peek(int offset = 0) const {
        size_t idx = pos + offset;
        return (idx < src.size()) ? src[idx] : '\0';
    }
    char advance() {
        char c = src[pos++];
        if (c == '\n') line++;
        return c;
    }
    bool match(char expected) {
        if (peek() == expected) { advance(); return true; }
        return false;
    }
    void skipWhitespaceAndComments() {
        while (pos < src.size()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else if (c == '/' && peek(1) == '/') {
                while (pos < src.size() && peek() != '\n') advance();
            } else break;
        }
    }
    Token makeToken(TokenType t, const std::string& val = "") {
        return { t, val, line };
    }
    Token number() {
        size_t start = pos - 1;
        while (std::isdigit(peek())) advance();
        if (peek() == '.' && std::isdigit(peek(1))) {
            advance();
            while (std::isdigit(peek())) advance();
        }
        return makeToken(TokenType::NUMBER, src.substr(start, pos - start));
    }
    Token identifier() {
        size_t start = pos - 1;
        while (std::isalnum(peek()) || peek() == '_') advance();
        std::string word = src.substr(start, pos - start);
        static const std::unordered_map<std::string, TokenType> keywords = {
            {"let",    TokenType::LET},
            {"if",     TokenType::IF},
            {"else",   TokenType::ELSE},
            {"while",  TokenType::WHILE},
            {"print",  TokenType::PRINT},
            {"input",  TokenType::INPUT},
            {"fn",     TokenType::FN},
            {"return", TokenType::RETURN},
            {"true",   TokenType::BOOL_TRUE},
            {"false",  TokenType::BOOL_FALSE},
            {"and",    TokenType::AND},
            {"or",     TokenType::OR},
        };
        auto it = keywords.find(word);
        if (it != keywords.end()) return makeToken(it->second, word);
        return makeToken(TokenType::IDENTIFIER, word);
    }
    Token string_literal() {
        std::string result;
        while (pos < src.size() && peek() != '"') {
            char c = advance();
            if (c == '\\') {
                char esc = advance();
                switch (esc) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    default: result += esc;
                }
            } else result += c;
        }
        if (pos >= src.size()) throw std::runtime_error("Unterminated string at line " + std::to_string(line));
        advance(); // closing "
        return makeToken(TokenType::STRING, result);
    }

public:
    Lexer(const std::string& source) : src(source) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < src.size()) {
            skipWhitespaceAndComments();
            if (pos >= src.size()) break;

            char c = advance();
            switch (c) {
                case '+': tokens.push_back(makeToken(TokenType::PLUS, "+")); break;
                case '-': tokens.push_back(makeToken(TokenType::MINUS, "-")); break;
                case '*': tokens.push_back(makeToken(TokenType::STAR, "*")); break;
                case '/': tokens.push_back(makeToken(TokenType::SLASH, "/")); break;
                case ';': tokens.push_back(makeToken(TokenType::SEMICOLON, ";")); break;
                case '(': tokens.push_back(makeToken(TokenType::LPAREN, "(")); break;
                case ')': tokens.push_back(makeToken(TokenType::RPAREN, ")")); break;
                case '{': tokens.push_back(makeToken(TokenType::LBRACE, "{")); break;
                case '}': tokens.push_back(makeToken(TokenType::RBRACE, "}")); break;
                case ',': tokens.push_back(makeToken(TokenType::COMMA, ",")); break;
                case '!':
                    tokens.push_back(match('=') ? makeToken(TokenType::BANG_EQUAL, "!=")
                                                : makeToken(TokenType::BANG, "!")); break;
                case '=':
                    tokens.push_back(match('=') ? makeToken(TokenType::EQUAL_EQUAL, "==")
                                                : makeToken(TokenType::EQUAL, "=")); break;
                case '<':
                    tokens.push_back(match('=') ? makeToken(TokenType::LESS_EQUAL, "<=")
                                                : makeToken(TokenType::LESS, "<")); break;
                case '>':
                    tokens.push_back(match('=') ? makeToken(TokenType::GREATER_EQUAL, ">=")
                                                : makeToken(TokenType::GREATER, ">")); break;
                case '"': tokens.push_back(string_literal()); break;
                default:
                    if (std::isdigit(c)) tokens.push_back(number());
                    else if (std::isalpha(c) || c == '_') tokens.push_back(identifier());
                    else throw std::runtime_error(
                        std::string("Unknown character '") + c + "' at line " + std::to_string(line));
            }
        }
        tokens.push_back({ TokenType::EOF_TOKEN, "", line });
        return tokens;
    }
};
