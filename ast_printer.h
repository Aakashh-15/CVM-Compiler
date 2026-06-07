#pragma once
#include "cvm.h"
#include <iostream>
#include <string>

class ASTPrinter {
    std::ostream& out;

    void indent(int depth) {
        for (int i = 0; i < depth; i++) out << (i == depth-1 ? "├─ " : "│  ");
    }

    void print(const ASTNode& node, int depth = 0) {
        indent(depth);
        std::visit([&](auto&& n) { printNode(n, depth); }, node.data);
    }

    void printNode(const NumberLiteral& n, int)    { out << "NumLit(" << n.value << ")\n"; }
    void printNode(const BoolLiteral& n, int)      { out << "BoolLit(" << (n.value?"true":"false") << ")\n"; }
    void printNode(const StringLiteral& n, int)    { out << "StrLit(\"" << n.value << "\")\n"; }
    void printNode(const Identifier& n, int)       { out << "Ident(" << n.name << ")\n"; }
    void printNode(const BinaryOp& n, int d) {
        out << "BinaryOp(" << n.op << ")\n";
        print(*n.left, d+1);
        print(*n.right, d+1);
    }
    void printNode(const UnaryOp& n, int d) {
        out << "UnaryOp(" << n.op << ")\n";
        print(*n.operand, d+1);
    }
    void printNode(const Assignment& n, int d) {
        out << "Assign(" << n.name << ")\n";
        print(*n.value, d+1);
    }
    void printNode(const VarDecl& n, int d) {
        out << "VarDecl(" << n.name << ")\n";
        print(*n.value, d+1);
    }
    void printNode(const PrintStmt& n, int d) {
        out << "Print\n";
        print(*n.expr, d+1);
    }
    void printNode(const InputStmt& n, int) {
        out << "Input(" << n.varName << ")\n";
    }
    void printNode(const Block& n, int d) {
        out << "Block\n";
        for (auto& s : n.stmts) print(*s, d+1);
    }
    void printNode(const IfStmt& n, int d) {
        out << "If\n";
        indent(d+1); out << "Condition:\n"; print(*n.condition, d+2);
        indent(d+1); out << "Then:\n"; print(*n.thenBranch, d+2);
        if (n.elseBranch) { indent(d+1); out << "Else:\n"; print(*n.elseBranch, d+2); }
    }
    void printNode(const WhileStmt& n, int d) {
        out << "While\n";
        indent(d+1); out << "Condition:\n"; print(*n.condition, d+2);
        indent(d+1); out << "Body:\n"; print(*n.body, d+2);
    }
    void printNode(const FnDecl& n, int d) {
        out << "FnDecl(" << n.name << ")(";
        for (size_t i = 0; i < n.params.size(); i++) {
            out << n.params[i];
            if (i+1 < n.params.size()) out << ", ";
        }
        out << ")\n";
        print(*n.body, d+1);
    }
    void printNode(const CallExpr& n, int d) {
        out << "Call(" << n.name << ")\n";
        for (auto& a : n.args) print(*a, d+1);
    }
    void printNode(const ReturnStmt& n, int d) {
        out << "Return\n";
        if (n.value) print(*n.value, d+1);
    }
    void printNode(const Program& n, int d) {
        out << "Program\n";
        for (auto& s : n.stmts) print(*s, d+1);
    }

public:
    ASTPrinter(std::ostream& os = std::cout) : out(os) {}
    void dump(const ASTNode& root) { print(root); }
};
