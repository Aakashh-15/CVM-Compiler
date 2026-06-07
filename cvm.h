#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <functional>
#include <cstring>

// ============================================================
//  TOKENS
// ============================================================
enum class TokenType {
    // Literals
    NUMBER, BOOL_TRUE, BOOL_FALSE, IDENTIFIER, STRING,
    // Operators
    PLUS, MINUS, STAR, SLASH,
    EQUAL_EQUAL, BANG_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    AND, OR, BANG,
    // Assignment / delimiters
    EQUAL, SEMICOLON, LPAREN, RPAREN, LBRACE, RBRACE, COMMA,
    // Keywords
    LET, IF, ELSE, WHILE, PRINT, INPUT, FN, RETURN,
    // Meta
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

// ============================================================
//  AST NODES
// ============================================================
struct ASTNode;
using ASTPtr = std::unique_ptr<ASTNode>;

struct NumberLiteral   { double value; };
struct BoolLiteral     { bool value; };
struct StringLiteral   { std::string value; };
struct Identifier      { std::string name; };
struct BinaryOp        { std::string op; ASTPtr left, right; };
struct UnaryOp         { std::string op; ASTPtr operand; };
struct Assignment      { std::string name; ASTPtr value; };
struct VarDecl         { std::string name; ASTPtr value; };
struct PrintStmt       { ASTPtr expr; };
struct InputStmt       { std::string varName; };
struct Block           { std::vector<ASTPtr> stmts; };
struct IfStmt          { ASTPtr condition; ASTPtr thenBranch; ASTPtr elseBranch; };
struct WhileStmt       { ASTPtr condition; ASTPtr body; };
struct FnDecl          { std::string name; std::vector<std::string> params; ASTPtr body; };
struct CallExpr        { std::string name; std::vector<ASTPtr> args; };
struct ReturnStmt      { ASTPtr value; };
struct Program         { std::vector<ASTPtr> stmts; };

struct ASTNode {
    using NodeVariant = std::variant<
        NumberLiteral, BoolLiteral, StringLiteral, Identifier,
        BinaryOp, UnaryOp, Assignment, VarDecl,
        PrintStmt, InputStmt, Block,
        IfStmt, WhileStmt,
        FnDecl, CallExpr, ReturnStmt,
        Program
    >;
    NodeVariant data;
    int line = 0;

    template<typename T>
    ASTNode(T&& d, int ln = 0) : data(std::forward<T>(d)), line(ln) {}
};

// ============================================================
//  OPCODES  (Instruction Set Architecture)
// ============================================================
enum class OpCode : uint8_t {
    // Stack ops
    PUSH_NUM,    // operand: 8 bytes (double)
    PUSH_BOOL,   // operand: 1 byte
    PUSH_STR,    // operand: 2 bytes (string pool index)
    PUSH_NULL,

    // Variables
    LOAD,        // operand: 2 bytes (name pool index)
    STORE,       // operand: 2 bytes (name pool index)
    DEFINE,      // operand: 2 bytes (name pool index) - creates in current scope

    // Arithmetic
    ADD, SUB, MUL, DIV,

    // Comparison
    EQ, NEQ, LT, LTE, GT, GTE,

    // Logic
    AND_OP, OR_OP, NOT,

    // Control flow
    JUMP,        // operand: 4 bytes (absolute offset)
    JUMP_IF_FALSE, // operand: 4 bytes

    // I/O
    PRINT,
    INPUT,       // operand: 2 bytes (var name index)

    // Functions
    CALL,        // operand: 2 bytes (name index), 1 byte (argc)
    RETURN,
    RETURN_NULL,

    // Scope
    PUSH_SCOPE,
    POP_SCOPE,

    // Stack management
    POP,
    DUP,

    HALT
};

// ============================================================
//  BYTECODE CHUNK
// ============================================================
struct Chunk {
    std::vector<uint8_t> code;
    std::vector<double> numbers;       // number constant pool
    std::vector<std::string> strings;  // string constant pool
    std::vector<int> lines;            // line info per instruction

    // Helpers
    void write(uint8_t byte, int line = 0) {
        code.push_back(byte);
        lines.push_back(line);
    }
    void writeU16(uint16_t val, int line = 0) {
        write((val >> 8) & 0xFF, line);
        write(val & 0xFF, line);
    }
    void writeU32(uint32_t val, int line = 0) {
        write((val >> 24) & 0xFF, line);
        write((val >> 16) & 0xFF, line);
        write((val >> 8)  & 0xFF, line);
        write( val        & 0xFF, line);
    }
    void writeDouble(double val, int line = 0) {
        uint64_t bits; memcpy(&bits, &val, 8);
        for (int i = 7; i >= 0; i--) write((bits >> (i*8)) & 0xFF, line);
    }
    void patchU32(size_t offset, uint32_t val) {
        code[offset]   = (val >> 24) & 0xFF;
        code[offset+1] = (val >> 16) & 0xFF;
        code[offset+2] = (val >> 8)  & 0xFF;
        code[offset+3] =  val        & 0xFF;
    }
    uint16_t addString(const std::string& s) {
        for (size_t i = 0; i < strings.size(); i++)
            if (strings[i] == s) return (uint16_t)i;
        strings.push_back(s);
        return (uint16_t)(strings.size() - 1);
    }
    uint16_t addNumber(double n) {
        numbers.push_back(n);
        return (uint16_t)(numbers.size() - 1);
    }
};

// ============================================================
//  VALUE TYPE  (runtime)
// ============================================================
struct Value {
    enum class Type { Null, Number, Bool, String } type = Type::Null;
    double num = 0;
    bool boolean = false;
    std::string str;

    static Value makeNull()                { Value v; v.type = Type::Null; return v; }
    static Value makeNum(double n)         { Value v; v.type = Type::Number; v.num = n; return v; }
    static Value makeBool(bool b)          { Value v; v.type = Type::Bool; v.boolean = b; return v; }
    static Value makeStr(const std::string& s) { Value v; v.type = Type::String; v.str = s; return v; }

    bool isTruthy() const {
        if (type == Type::Null) return false;
        if (type == Type::Bool) return boolean;
        if (type == Type::Number) return num != 0;
        return !str.empty();
    }
    std::string toString() const {
        switch (type) {
            case Type::Null:   return "null";
            case Type::Number: {
                if (num == (long long)num) return std::to_string((long long)num);
                return std::to_string(num);
            }
            case Type::Bool:   return boolean ? "true" : "false";
            case Type::String: return str;
        }
        return "null";
    }
    bool operator==(const Value& o) const {
        if (type != o.type) return false;
        switch (type) {
            case Type::Null:   return true;
            case Type::Number: return num == o.num;
            case Type::Bool:   return boolean == o.boolean;
            case Type::String: return str == o.str;
        }
        return false;
    }
};

// ============================================================
//  FUNCTION OBJECT
// ============================================================
struct FnObject {
    std::string name;
    std::vector<std::string> params;
    Chunk chunk;
};
