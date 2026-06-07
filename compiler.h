#pragma once
#include "cvm.h"
#include <cstring>
#include <stdexcept>
#include <unordered_map>

class Compiler {
    Chunk mainChunk;
    std::unordered_map<std::string, FnObject> functions;
    Chunk* current;

    void emit(OpCode op, int line = 0) { current->write((uint8_t)op, line); }

    void emitJump(OpCode op, int line = 0) {
        current->write((uint8_t)op, line);
        current->writeU32(0, line);
    }
    size_t currentOffset() { return current->code.size(); }
    void patchJump(size_t jumpInstrOffset) {
        uint32_t target = (uint32_t)current->code.size();
        current->patchU32(jumpInstrOffset + 1, target);
    }
    void emitPushNum(double val, int line) {
        current->write((uint8_t)OpCode::PUSH_NUM, line);
        uint64_t bits; memcpy(&bits, &val, 8);
        for (int i = 7; i >= 0; i--) current->write((bits >> (i*8)) & 0xFF, line);
    }
    void emitU16(OpCode op, uint16_t idx, int line) {
        current->write((uint8_t)op, line);
        current->writeU16(idx, line);
    }

    // ── Expression compiler ──────────────────────────────────────
    void compileExpr(const ASTNode& node) {
        int line = node.line;
        std::visit([&](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, NumberLiteral>) {
                emitPushNum(n.value, line);
            } else if constexpr (std::is_same_v<T, BoolLiteral>) {
                current->write((uint8_t)OpCode::PUSH_BOOL, line);
                current->write(n.value ? 1 : 0, line);
            } else if constexpr (std::is_same_v<T, StringLiteral>) {
                emitU16(OpCode::PUSH_STR, current->addString(n.value), line);
            } else if constexpr (std::is_same_v<T, Identifier>) {
                emitU16(OpCode::LOAD, current->addString(n.name), line);
            } else if constexpr (std::is_same_v<T, Assignment>) {
                compileExpr(*n.value);
                emitU16(OpCode::STORE, current->addString(n.name), line);
            } else if constexpr (std::is_same_v<T, UnaryOp>) {
                compileExpr(*n.operand);
                if (n.op == "-") {
                    emitPushNum(-1.0, line);
                    emit(OpCode::MUL, line);
                } else if (n.op == "!") {
                    emit(OpCode::NOT, line);
                }
            } else if constexpr (std::is_same_v<T, BinaryOp>) {
                compileBinaryOp(n, line);
            } else if constexpr (std::is_same_v<T, CallExpr>) {
                for (auto& arg : n.args) compileExpr(*arg);
                current->write((uint8_t)OpCode::CALL, line);
                current->writeU16(current->addString(n.name), line);
                current->write((uint8_t)n.args.size(), line);
            } else {
                // Statement types shouldn't appear in expression context
                throw std::runtime_error("Statement used as expression");
            }
        }, node.data);
    }

    void compileBinaryOp(const BinaryOp& n, int line) {
        if (n.op == "and") {
            compileExpr(*n.left);
            emit(OpCode::DUP, line);
            size_t jmpFalse = currentOffset(); emitJump(OpCode::JUMP_IF_FALSE, line);
            emit(OpCode::POP, line);
            compileExpr(*n.right);
            patchJump(jmpFalse);
            return;
        }
        if (n.op == "or") {
            compileExpr(*n.left);
            emit(OpCode::DUP, line);
            size_t jmpFalse = currentOffset(); emitJump(OpCode::JUMP_IF_FALSE, line);
            size_t jmpEnd   = currentOffset(); emitJump(OpCode::JUMP, line);
            patchJump(jmpFalse);
            emit(OpCode::POP, line);
            compileExpr(*n.right);
            patchJump(jmpEnd);
            return;
        }
        compileExpr(*n.left);
        compileExpr(*n.right);
        static const std::unordered_map<std::string, OpCode> ops = {
            {"+", OpCode::ADD}, {"-", OpCode::SUB},
            {"*", OpCode::MUL}, {"/", OpCode::DIV},
            {"==", OpCode::EQ},  {"!=", OpCode::NEQ},
            {"<",  OpCode::LT},  {"<=", OpCode::LTE},
            {">",  OpCode::GT},  {">=", OpCode::GTE},
        };
        auto it = ops.find(n.op);
        if (it == ops.end()) throw std::runtime_error("Unknown operator: " + n.op);
        emit(it->second, line);
    }

    // ── Statement compiler ───────────────────────────────────────
    void compileStmt(const ASTNode& node) {
        int line = node.line;
        std::visit([&](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, VarDecl>) {
                compileExpr(*n.value);
                emitU16(OpCode::DEFINE, current->addString(n.name), line);
            } else if constexpr (std::is_same_v<T, PrintStmt>) {
                compileExpr(*n.expr);
                emit(OpCode::PRINT, line);
            } else if constexpr (std::is_same_v<T, InputStmt>) {
                emitU16(OpCode::INPUT, current->addString(n.varName), line);
            } else if constexpr (std::is_same_v<T, Block>) {
                emit(OpCode::PUSH_SCOPE, line);
                for (auto& s : n.stmts) compileStmt(*s);
                emit(OpCode::POP_SCOPE, line);
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                compileExpr(*n.condition);
                size_t jmpFalse = currentOffset(); emitJump(OpCode::JUMP_IF_FALSE, line);
                emit(OpCode::POP, line);
                compileStmt(*n.thenBranch);
                size_t jmpEnd = currentOffset(); emitJump(OpCode::JUMP, line);
                patchJump(jmpFalse);
                emit(OpCode::POP, line);
                if (n.elseBranch) compileStmt(*n.elseBranch);
                patchJump(jmpEnd);
            } else if constexpr (std::is_same_v<T, WhileStmt>) {
                size_t loopStart = currentOffset();
                compileExpr(*n.condition);
                size_t jmpOut = currentOffset(); emitJump(OpCode::JUMP_IF_FALSE, line);
                emit(OpCode::POP, line);
                compileStmt(*n.body);
                current->write((uint8_t)OpCode::JUMP, line);
                current->writeU32((uint32_t)loopStart, line);
                patchJump(jmpOut);
                emit(OpCode::POP, line);
            } else if constexpr (std::is_same_v<T, FnDecl>) {
                FnObject fn;
                fn.name   = n.name;
                fn.params = n.params;
                Chunk* saved = current;
                current = &fn.chunk;
                // Compile body statements
                for (auto& s : std::get<Block>(n.body->data).stmts)
                    compileStmt(*s);
                emit(OpCode::RETURN_NULL, line);
                current = saved;
                functions[n.name] = std::move(fn);
            } else if constexpr (std::is_same_v<T, ReturnStmt>) {
                if (n.value) { compileExpr(*n.value); emit(OpCode::RETURN, line); }
                else emit(OpCode::RETURN_NULL, line);
            } else if constexpr (std::is_same_v<T, Program>) {
                for (auto& s : n.stmts) compileStmt(*s);
                emit(OpCode::HALT, line);
            } else if constexpr (std::is_same_v<T, Assignment>) {
                // Assignment used as statement
                compileExpr(node);
                emit(OpCode::POP, line);
            } else {
                // Expression statements: compile as expression, pop result
                compileExpr(node);
                emit(OpCode::POP, line);
            }
        }, node.data);
    }

public:
    struct CompileResult {
        Chunk main;
        std::unordered_map<std::string, FnObject> functions;
    };

    CompileResult compile(const ASTNode& ast) {
        current = &mainChunk;
        compileStmt(ast);
        return { std::move(mainChunk), std::move(functions) };
    }
};
