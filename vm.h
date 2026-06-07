#pragma once
#include "cvm.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cstring>

class VM {
public:
    // ── Call frame ──────────────────────────────────────────────
    struct Frame {
        const Chunk* chunk;
        size_t ip;
        size_t stackBase;  // where this frame's locals start
        std::string fnName;
    };

    // ── Scope (variable environment) ────────────────────────────
    struct Scope {
        std::unordered_map<std::string, Value> vars;
    };

private:
    std::vector<Value>  stack;
    std::vector<Frame>  frames;
    std::vector<Scope>  scopes;          // scope chain
    std::unordered_map<std::string, FnObject>* functions = nullptr;
    std::ostream& out;
    std::istream& in;

    // ── Stack helpers ────────────────────────────────────────────
    void push(const Value& v)  { stack.push_back(v); }
    Value pop() {
        if (stack.empty()) throw std::runtime_error("Stack underflow!");
        Value v = stack.back(); stack.pop_back(); return v;
    }
    Value& top() {
        if (stack.empty()) throw std::runtime_error("Stack underflow!");
        return stack.back();
    }

    // ── Variable helpers ─────────────────────────────────────────
    void define(const std::string& name, const Value& val) {
        scopes.back().vars[name] = val;
    }
    void set(const std::string& name, const Value& val) {
        for (int i = (int)scopes.size()-1; i >= 0; i--) {
            if (scopes[i].vars.count(name)) { scopes[i].vars[name] = val; return; }
        }
        throw std::runtime_error("Undefined variable '" + name + "'");
    }
    Value get(const std::string& name) {
        for (int i = (int)scopes.size()-1; i >= 0; i--) {
            auto it = scopes[i].vars.find(name);
            if (it != scopes[i].vars.end()) return it->second;
        }
        throw std::runtime_error("Undefined variable '" + name + "'");
    }

    // ── Read helpers (from bytecode) ─────────────────────────────
    uint8_t readByte(Frame& f) { return f.chunk->code[f.ip++]; }
    uint16_t readU16(Frame& f) {
        uint16_t v = ((uint16_t)f.chunk->code[f.ip] << 8) | f.chunk->code[f.ip+1];
        f.ip += 2; return v;
    }
    uint32_t readU32(Frame& f) {
        uint32_t v = ((uint32_t)f.chunk->code[f.ip]   << 24) |
                     ((uint32_t)f.chunk->code[f.ip+1] << 16) |
                     ((uint32_t)f.chunk->code[f.ip+2] << 8)  |
                      (uint32_t)f.chunk->code[f.ip+3];
        f.ip += 4; return v;
    }
    double readDouble(Frame& f) {
        uint64_t bits = 0;
        for (int i = 0; i < 8; i++) bits = (bits << 8) | f.chunk->code[f.ip++];
        double d; memcpy(&d, &bits, 8); return d;
    }
    std::string readStr(Frame& f) {
        uint16_t idx = readU16(f);
        return f.chunk->strings[idx];
    }

    // ── Arithmetic helpers ────────────────────────────────────────
    void numOp(const std::string& op) {
        Value b = pop(), a = pop();
        if (a.type != Value::Type::Number || b.type != Value::Type::Number) {
            // String concatenation for +
            if (op == "+" ) { push(Value::makeStr(a.toString() + b.toString())); return; }
            throw std::runtime_error("Operands must be numbers for '" + op + "'");
        }
        if      (op == "+")  push(Value::makeNum(a.num + b.num));
        else if (op == "-")  push(Value::makeNum(a.num - b.num));
        else if (op == "*")  push(Value::makeNum(a.num * b.num));
        else if (op == "/") {
            if (b.num == 0) throw std::runtime_error("Division by zero");
            push(Value::makeNum(a.num / b.num));
        }
        else if (op == "<")  push(Value::makeBool(a.num <  b.num));
        else if (op == "<=") push(Value::makeBool(a.num <= b.num));
        else if (op == ">")  push(Value::makeBool(a.num >  b.num));
        else if (op == ">=") push(Value::makeBool(a.num >= b.num));
    }

    // ── Main execution loop ────────────────────────────────────────
    void run() {
        while (!frames.empty()) {
            Frame& f = frames.back();
            if (f.ip >= f.chunk->code.size()) { frames.pop_back(); continue; }

            OpCode op = (OpCode)readByte(f);

            switch (op) {
                case OpCode::PUSH_NUM:  push(Value::makeNum(readDouble(f)));  break;
                case OpCode::PUSH_BOOL: push(Value::makeBool(readByte(f)));   break;
                case OpCode::PUSH_STR:  push(Value::makeStr(f.chunk->strings[readU16(f)])); break;
                case OpCode::PUSH_NULL: push(Value::makeNull());               break;

                case OpCode::LOAD:   push(get(readStr(f)));           break;
                case OpCode::STORE:{ std::string n=readStr(f); set(n, top()); break; }
                case OpCode::DEFINE:{ std::string n=readStr(f); define(n, pop()); break; }

                case OpCode::ADD: numOp("+"); break;
                case OpCode::SUB: numOp("-"); break;
                case OpCode::MUL: numOp("*"); break;
                case OpCode::DIV: numOp("/"); break;
                case OpCode::LT:  numOp("<"); break;
                case OpCode::LTE: numOp("<=");break;
                case OpCode::GT:  numOp(">");break;
                case OpCode::GTE: numOp(">=");break;
                case OpCode::EQ:  { Value b=pop(), a=pop(); push(Value::makeBool(a==b)); break; }
                case OpCode::NEQ: { Value b=pop(), a=pop(); push(Value::makeBool(!(a==b))); break; }
                case OpCode::NOT: { Value v=pop(); push(Value::makeBool(!v.isTruthy())); break; }

                case OpCode::JUMP:
                    f.ip = readU32(f);
                    break;
                case OpCode::JUMP_IF_FALSE: {
                    uint32_t target = readU32(f);
                    if (!top().isTruthy()) f.ip = target;
                    break;
                }

                case OpCode::PRINT: {
                    Value v = pop();
                    out << v.toString() << "\n";
                    break;
                }
                case OpCode::INPUT: {
                    std::string varName = f.chunk->strings[readU16(f)];
                    std::string line; std::getline(in, line);
                    // Try to parse as number
                    try {
                        size_t consumed;
                        double d = std::stod(line, &consumed);
                        if (consumed == line.size()) { set(varName, Value::makeNum(d)); break; }
                    } catch (...) {}
                    if (line == "true")       { set(varName, Value::makeBool(true));  break; }
                    if (line == "false")      { set(varName, Value::makeBool(false)); break; }
                    set(varName, Value::makeStr(line));
                    break;
                }

                case OpCode::CALL: {
                    std::string name = f.chunk->strings[readU16(f)];
                    uint8_t argc = readByte(f);
                    auto it = functions->find(name);
                    if (it == functions->end())
                        throw std::runtime_error("Undefined function '" + name + "'");
                    FnObject& fn = it->second;
                    if (argc != fn.params.size())
                        throw std::runtime_error("Function '" + name + "' expects " +
                            std::to_string(fn.params.size()) + " args, got " + std::to_string(argc));
                    // Collect args
                    std::vector<Value> args(argc);
                    for (int i = argc-1; i >= 0; i--) args[i] = pop();
                    // Push new scope and bind params
                    scopes.push_back(Scope{});
                    size_t scopeBase = scopes.size() - 1; // track where this frame starts
                    for (size_t i = 0; i < fn.params.size(); i++)
                        scopes.back().vars[fn.params[i]] = args[i];
                    // Push new frame (stackBase = scope base index)
                    frames.push_back({ &fn.chunk, 0, scopeBase, name });
                    break;
                }

                case OpCode::RETURN: {
                    Value retVal = pop();
                    // Pop all scopes down to (and including) this frame's base scope
                    size_t scopeBase = frames.back().stackBase;
                    while (scopes.size() > scopeBase) scopes.pop_back();
                    frames.pop_back();
                    push(retVal);
                    break;
                }
                case OpCode::RETURN_NULL: {
                    size_t scopeBase = frames.back().stackBase;
                    while (scopes.size() > scopeBase) scopes.pop_back();
                    frames.pop_back();
                    push(Value::makeNull());
                    break;
                }

                case OpCode::PUSH_SCOPE: scopes.push_back(Scope{}); break;
                case OpCode::POP_SCOPE:  scopes.pop_back();          break;

                case OpCode::POP: pop();                   break;
                case OpCode::DUP: push(top());             break;
                case OpCode::HALT: return;

                default:
                    throw std::runtime_error("Unknown opcode: " + std::to_string((int)op));
            }
        }
    }

public:
    VM(std::ostream& os = std::cout, std::istream& is = std::cin)
        : out(os), in(is) {}

    void execute(const Chunk& main, std::unordered_map<std::string, FnObject>& fns) {
        functions = &fns;
        stack.clear();
        frames.clear();
        scopes.clear();
        scopes.push_back(Scope{}); // global scope
        frames.push_back({ &main, 0, 0, "main" });
        run();
    }
};
