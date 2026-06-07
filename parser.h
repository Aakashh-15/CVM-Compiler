#pragma once
#include "cvm.h"
#include <stdexcept>

class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

    Token& peek(int offset = 0) { 
        size_t idx = pos + offset;
        if (idx >= tokens.size()) return tokens.back();
        return tokens[idx]; 
    }
    Token advance() { 
        Token t = tokens[pos]; 
        if (pos + 1 < tokens.size()) pos++; 
        return t; 
    }
    bool check(TokenType t) { return peek().type == t; }
    bool match(TokenType t) { 
        if (check(t)) { advance(); return true; } 
        return false; 
    }
    Token expect(TokenType t, const std::string& msg) {
        if (!check(t)) throw std::runtime_error(msg + " at line " + std::to_string(peek().line));
        return advance();
    }

    // ---- Grammar rules (lowest to highest precedence) ----

    ASTPtr statement() {
        if (check(TokenType::LET))    return varDecl();
        if (check(TokenType::IF))     return ifStmt();
        if (check(TokenType::WHILE))  return whileStmt();
        if (check(TokenType::PRINT))  return printStmt();
        if (check(TokenType::INPUT))  return inputStmt();
        if (check(TokenType::FN))     return fnDecl();
        if (check(TokenType::RETURN)) return returnStmt();
        if (check(TokenType::LBRACE)) return block();
        return exprStmt();
    }

    ASTPtr varDecl() {
        int ln = peek().line; advance(); // consume 'let'
        Token name = expect(TokenType::IDENTIFIER, "Expected variable name after 'let'");
        expect(TokenType::EQUAL, "Expected '=' after variable name");
        ASTPtr val = expression();
        expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
        return std::make_unique<ASTNode>(VarDecl{name.value, std::move(val)}, ln);
    }

    ASTPtr ifStmt() {
        int ln = peek().line; advance(); // consume 'if'
        expect(TokenType::LPAREN, "Expected '(' after 'if'");
        ASTPtr cond = expression();
        expect(TokenType::RPAREN, "Expected ')' after condition");
        ASTPtr then = block_or_stmt();
        ASTPtr els = nullptr;
        if (match(TokenType::ELSE)) els = block_or_stmt();
        return std::make_unique<ASTNode>(IfStmt{std::move(cond), std::move(then), std::move(els)}, ln);
    }

    ASTPtr whileStmt() {
        int ln = peek().line; advance(); // consume 'while'
        expect(TokenType::LPAREN, "Expected '(' after 'while'");
        ASTPtr cond = expression();
        expect(TokenType::RPAREN, "Expected ')' after condition");
        ASTPtr body = block_or_stmt();
        return std::make_unique<ASTNode>(WhileStmt{std::move(cond), std::move(body)}, ln);
    }

    ASTPtr printStmt() {
        int ln = peek().line; advance();
        ASTPtr val = expression();
        expect(TokenType::SEMICOLON, "Expected ';' after print");
        return std::make_unique<ASTNode>(PrintStmt{std::move(val)}, ln);
    }

    ASTPtr inputStmt() {
        int ln = peek().line; advance();
        Token name = expect(TokenType::IDENTIFIER, "Expected variable name after 'input'");
        expect(TokenType::SEMICOLON, "Expected ';' after input");
        return std::make_unique<ASTNode>(InputStmt{name.value}, ln);
    }

    ASTPtr fnDecl() {
        int ln = peek().line; advance(); // consume 'fn'
        Token name = expect(TokenType::IDENTIFIER, "Expected function name");
        expect(TokenType::LPAREN, "Expected '(' after function name");
        std::vector<std::string> params;
        if (!check(TokenType::RPAREN)) {
            do {
                Token p = expect(TokenType::IDENTIFIER, "Expected parameter name");
                params.push_back(p.value);
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "Expected ')' after parameters");
        ASTPtr body = block();
        return std::make_unique<ASTNode>(FnDecl{name.value, params, std::move(body)}, ln);
    }

    ASTPtr returnStmt() {
        int ln = peek().line; advance();
        ASTPtr val = nullptr;
        if (!check(TokenType::SEMICOLON)) val = expression();
        expect(TokenType::SEMICOLON, "Expected ';' after return");
        return std::make_unique<ASTNode>(ReturnStmt{std::move(val)}, ln);
    }

    ASTPtr block() {
        int ln = peek().line;
        expect(TokenType::LBRACE, "Expected '{'");
        std::vector<ASTPtr> stmts;
        while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN))
            stmts.push_back(statement());
        expect(TokenType::RBRACE, "Expected '}'");
        return std::make_unique<ASTNode>(Block{std::move(stmts)}, ln);
    }

    ASTPtr block_or_stmt() {
        if (check(TokenType::LBRACE)) return block();
        return statement();
    }

    ASTPtr exprStmt() {
        int ln = peek().line;
        ASTPtr expr = expression();
        expect(TokenType::SEMICOLON, "Expected ';' after expression");
        return expr;
    }

    // Expressions (precedence climbing)
    ASTPtr expression() { return assignment(); }

    ASTPtr assignment() {
        int ln = peek().line;
        if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::EQUAL) {
            Token name = advance(); advance(); // consume name and '='
            ASTPtr val = assignment();
            return std::make_unique<ASTNode>(Assignment{name.value, std::move(val)}, ln);
        }
        return logicalOr();
    }

    ASTPtr logicalOr() {
        int ln = peek().line;
        ASTPtr left = logicalAnd();
        while (check(TokenType::OR)) {
            advance();
            ASTPtr right = logicalAnd();
            left = std::make_unique<ASTNode>(BinaryOp{"or", std::move(left), std::move(right)}, ln);
        }
        return left;
    }

    ASTPtr logicalAnd() {
        int ln = peek().line;
        ASTPtr left = equality();
        while (check(TokenType::AND)) {
            advance();
            ASTPtr right = equality();
            left = std::make_unique<ASTNode>(BinaryOp{"and", std::move(left), std::move(right)}, ln);
        }
        return left;
    }

    ASTPtr equality() {
        int ln = peek().line;
        ASTPtr left = comparison();
        while (check(TokenType::EQUAL_EQUAL) || check(TokenType::BANG_EQUAL)) {
            std::string op = advance().value;
            ASTPtr right = comparison();
            left = std::make_unique<ASTNode>(BinaryOp{op, std::move(left), std::move(right)}, ln);
        }
        return left;
    }

    ASTPtr comparison() {
        int ln = peek().line;
        ASTPtr left = addSub();
        while (check(TokenType::LESS) || check(TokenType::LESS_EQUAL) ||
               check(TokenType::GREATER) || check(TokenType::GREATER_EQUAL)) {
            std::string op = advance().value;
            ASTPtr right = addSub();
            left = std::make_unique<ASTNode>(BinaryOp{op, std::move(left), std::move(right)}, ln);
        }
        return left;
    }

    ASTPtr addSub() {
        int ln = peek().line;
        ASTPtr left = mulDiv();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            std::string op = advance().value;
            ASTPtr right = mulDiv();
            left = std::make_unique<ASTNode>(BinaryOp{op, std::move(left), std::move(right)}, ln);
        }
        return left;
    }

    ASTPtr mulDiv() {
        int ln = peek().line;
        ASTPtr left = unary();
        while (check(TokenType::STAR) || check(TokenType::SLASH)) {
            std::string op = advance().value;
            ASTPtr right = unary();
            left = std::make_unique<ASTNode>(BinaryOp{op, std::move(left), std::move(right)}, ln);
        }
        return left;
    }

    ASTPtr unary() {
        int ln = peek().line;
        if (check(TokenType::MINUS) || check(TokenType::BANG)) {
            std::string op = advance().value;
            ASTPtr operand = unary();
            return std::make_unique<ASTNode>(UnaryOp{op, std::move(operand)}, ln);
        }
        return call();
    }

    ASTPtr call() {
        int ln = peek().line;
        if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::LPAREN) {
            Token name = advance(); advance(); // name + '('
            std::vector<ASTPtr> args;
            if (!check(TokenType::RPAREN)) {
                do { args.push_back(expression()); } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')' after arguments");
            return std::make_unique<ASTNode>(CallExpr{name.value, std::move(args)}, ln);
        }
        return primary();
    }

    ASTPtr primary() {
        int ln = peek().line;
        if (check(TokenType::NUMBER)) {
            double val = std::stod(advance().value);
            return std::make_unique<ASTNode>(NumberLiteral{val}, ln);
        }
        if (check(TokenType::BOOL_TRUE))  { advance(); return std::make_unique<ASTNode>(BoolLiteral{true}, ln); }
        if (check(TokenType::BOOL_FALSE)) { advance(); return std::make_unique<ASTNode>(BoolLiteral{false}, ln); }
        if (check(TokenType::STRING)) {
            std::string s = advance().value;
            return std::make_unique<ASTNode>(StringLiteral{s}, ln);
        }
        if (check(TokenType::IDENTIFIER)) {
            return std::make_unique<ASTNode>(Identifier{advance().value}, ln);
        }
        if (match(TokenType::LPAREN)) {
            ASTPtr expr = expression();
            expect(TokenType::RPAREN, "Expected ')' after expression");
            return expr;
        }
        throw std::runtime_error("Unexpected token '" + peek().value + "' at line " + std::to_string(ln));
    }

public:
    Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

    ASTPtr parse() {
        int ln = peek().line;
        std::vector<ASTPtr> stmts;
        while (!check(TokenType::EOF_TOKEN))
            stmts.push_back(statement());
        return std::make_unique<ASTNode>(Program{std::move(stmts)}, ln);
    }
};
