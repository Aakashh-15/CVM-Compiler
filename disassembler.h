#pragma once
#include "cvm.h"
#include <iostream>
#include <iomanip>

class Disassembler {
    std::ostream& out;

    size_t readU16(const Chunk& c, size_t offset) {
        return ((size_t)c.code[offset] << 8) | c.code[offset+1];
    }
    uint32_t readU32(const Chunk& c, size_t offset) {
        return ((uint32_t)c.code[offset]   << 24) |
               ((uint32_t)c.code[offset+1] << 16) |
               ((uint32_t)c.code[offset+2] << 8)  |
                (uint32_t)c.code[offset+3];
    }
    double readDouble(const Chunk& c, size_t offset) {
        uint64_t bits = 0;
        for (int i = 0; i < 8; i++) bits = (bits << 8) | c.code[offset+i];
        double d; memcpy(&d, &bits, 8);
        return d;
    }

    size_t disassembleInstr(const Chunk& chunk, size_t offset) {
        out << std::setw(5) << offset << "  ";
        if (offset < chunk.lines.size())
            out << "[L" << std::setw(3) << chunk.lines[offset] << "]  ";
        else
            out << "        ";

        OpCode op = (OpCode)chunk.code[offset];
        switch (op) {
            case OpCode::PUSH_NUM: {
                double val = readDouble(chunk, offset+1);
                out << "PUSH_NUM    " << val << "\n";
                return offset + 9;
            }
            case OpCode::PUSH_BOOL: {
                out << "PUSH_BOOL   " << (chunk.code[offset+1] ? "true" : "false") << "\n";
                return offset + 2;
            }
            case OpCode::PUSH_STR: {
                uint16_t idx = readU16(chunk, offset+1);
                out << "PUSH_STR    [" << idx << "] \"" << (idx < chunk.strings.size() ? chunk.strings[idx] : "?") << "\"\n";
                return offset + 3;
            }
            case OpCode::PUSH_NULL:   out << "PUSH_NULL\n";  return offset+1;
            case OpCode::LOAD: {
                uint16_t idx = readU16(chunk, offset+1);
                out << "LOAD        \"" << (idx < chunk.strings.size() ? chunk.strings[idx] : "?") << "\"\n";
                return offset + 3;
            }
            case OpCode::STORE: {
                uint16_t idx = readU16(chunk, offset+1);
                out << "STORE       \"" << (idx < chunk.strings.size() ? chunk.strings[idx] : "?") << "\"\n";
                return offset + 3;
            }
            case OpCode::DEFINE: {
                uint16_t idx = readU16(chunk, offset+1);
                out << "DEFINE      \"" << (idx < chunk.strings.size() ? chunk.strings[idx] : "?") << "\"\n";
                return offset + 3;
            }
            case OpCode::ADD:    out << "ADD\n";   return offset+1;
            case OpCode::SUB:    out << "SUB\n";   return offset+1;
            case OpCode::MUL:    out << "MUL\n";   return offset+1;
            case OpCode::DIV:    out << "DIV\n";   return offset+1;
            case OpCode::EQ:     out << "EQ\n";    return offset+1;
            case OpCode::NEQ:    out << "NEQ\n";   return offset+1;
            case OpCode::LT:     out << "LT\n";    return offset+1;
            case OpCode::LTE:    out << "LTE\n";   return offset+1;
            case OpCode::GT:     out << "GT\n";    return offset+1;
            case OpCode::GTE:    out << "GTE\n";   return offset+1;
            case OpCode::AND_OP: out << "AND_OP\n";return offset+1;
            case OpCode::OR_OP:  out << "OR_OP\n"; return offset+1;
            case OpCode::NOT:    out << "NOT\n";   return offset+1;
            case OpCode::JUMP: {
                uint32_t target = readU32(chunk, offset+1);
                out << "JUMP        -> " << target << "\n";
                return offset + 5;
            }
            case OpCode::JUMP_IF_FALSE: {
                uint32_t target = readU32(chunk, offset+1);
                out << "JUMP_IF_FALSE -> " << target << "\n";
                return offset + 5;
            }
            case OpCode::PRINT:       out << "PRINT\n";       return offset+1;
            case OpCode::INPUT: {
                uint16_t idx = readU16(chunk, offset+1);
                out << "INPUT       \"" << (idx < chunk.strings.size() ? chunk.strings[idx] : "?") << "\"\n";
                return offset + 3;
            }
            case OpCode::CALL: {
                uint16_t nameIdx = readU16(chunk, offset+1);
                uint8_t argc = chunk.code[offset+3];
                out << "CALL        \"" << (nameIdx < chunk.strings.size() ? chunk.strings[nameIdx] : "?") 
                    << "\" (" << (int)argc << " args)\n";
                return offset + 4;
            }
            case OpCode::RETURN:      out << "RETURN\n";      return offset+1;
            case OpCode::RETURN_NULL: out << "RETURN_NULL\n"; return offset+1;
            case OpCode::PUSH_SCOPE:  out << "PUSH_SCOPE\n";  return offset+1;
            case OpCode::POP_SCOPE:   out << "POP_SCOPE\n";   return offset+1;
            case OpCode::POP:         out << "POP\n";         return offset+1;
            case OpCode::DUP:         out << "DUP\n";         return offset+1;
            case OpCode::HALT:        out << "HALT\n";        return offset+1;
            default:
                out << "UNKNOWN(" << (int)chunk.code[offset] << ")\n";
                return offset+1;
        }
    }

public:
    Disassembler(std::ostream& os = std::cout) : out(os) {}

    void disassemble(const Chunk& chunk, const std::string& name = "main") {
        out << "═══════════════════════════════════════\n";
        out << "  BYTECODE: <" << name << ">\n";
        out << "═══════════════════════════════════════\n";
        size_t offset = 0;
        while (offset < chunk.code.size())
            offset = disassembleInstr(chunk, offset);
        out << "\n";
    }
};
