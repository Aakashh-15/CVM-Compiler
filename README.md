# CVM++ — Custom Stack-Based Virtual Machine & Compiler

> A lightweight scripting language built from scratch in C++ that compiles source code to proprietary bytecode and executes it on a custom stack-based Virtual Machine.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Project Structure](#project-structure)
- [Architecture](#architecture)
- [Instruction Set Architecture](#instruction-set-architecture-isa)
- [Language Syntax](#language-syntax)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Sample Scripts](#sample-scripts)
- [Debug Mode](#debug-mode)
- [How It Works](#how-it-works)
- [Roadmap](#roadmap)
- [References](#references)

---

## Overview

Most developers use high-level languages like Python, JavaScript, or Java without understanding how raw text is translated into instructions a computer can execute. CVM++ demystifies this by building every piece of that pipeline from scratch — the Lexer, Parser, Compiler, and Virtual Machine — in pure C++17.

The name stands for **Custom Virtual Machine** written in **C++**.

---

## Features

- Full **4-stage compilation pipeline** — Lexer → Parser → AST → Bytecode
- **30+ custom opcodes** in a proprietary Instruction Set Architecture
- **Stack-based Virtual Machine** with a scope chain and call frame system
- **Recursive functions** — factorial, fibonacci verified working
- **Interactive REPL** — Read-Eval-Print Loop for live scripting
- **Debug modes** — AST pretty-printer and bytecode disassembler built in
- **File runner** — run any `.cvm` script directly from the CLI
- Header-only architecture — single `g++` command builds everything

---

## Project Structure

```
cvm++/
│
├── cvm.h               # Shared types: Token, ASTNode, OpCode, Chunk, Value, FnObject
├── lexer.h             # Lexical analysis — raw text to token stream
├── parser.h            # Recursive descent parser — tokens to AST
├── compiler.h          # Code generation — AST to bytecode chunk
├── vm.h                # Stack-based execution engine
│
├── ast_printer.h       # Debug utility — pretty-print AST tree
├── disassembler.h      # Debug utility — human-readable opcode listing
│
├── main.cpp            # Entry point — CLI, file runner, REPL
├── Makefile            # Build configuration
│
└── tests/
    ├── hello.cvm       # Variables, arithmetic, strings, booleans
    ├── control.cvm     # if/else, while loops, FizzBuzz
    └── functions.cvm   # Functions, recursion, factorial, fibonacci
```

---

## Architecture

CVM++ processes source code through four sequential stages:

```
Source Code (.cvm)
        │
        ▼
┌───────────────┐
│     LEXER     │  Reads raw characters, produces 18 token types
│   lexer.h     │  e.g. LET · IDENT(x) · EQUAL · NUMBER(10) · SEMICOLON
└───────┬───────┘
        │
        ▼
┌───────────────┐
│    PARSER     │  Recursive descent, produces 17 AST node types
│   parser.h    │  e.g. VarDecl → BinaryOp → Identifier / NumberLiteral
└───────┬───────┘
        │
        ▼
┌───────────────┐
│   COMPILER    │  Walks AST, emits bytecode into std::vector<uint8_t>
│  compiler.h   │  e.g. PUSH_NUM 10 · DEFINE "x" · LOAD "x" · PRINT
└───────┬───────┘
        │
        ▼
┌───────────────┐
│      VM       │  Stack-based execution loop, scope chain, call frames
│     vm.h      │  e.g. Stack: [10] → [10,5] → [15] → stdout: 15
└───────────────┘
        │
        ▼
   Program Output
```

### Core Data Structures

| Structure | File | Purpose |
|---|---|---|
| `Token` | cvm.h | Type + value + line number |
| `ASTNode` | cvm.h | std::variant of 17 node types |
| `Chunk` | cvm.h | Bytecode array + string/number pools |
| `Value` | cvm.h | Runtime value — Null, Number, Bool, String |
| `FnObject` | cvm.h | Function name, params, own Chunk |
| `Frame` | vm.h | Call frame — chunk ptr, IP, scope base |
| `Scope` | vm.h | Hash map of variable names to Values |

---

## Instruction Set Architecture (ISA)

CVM++ defines 30+ opcodes across 8 categories:

| Category | Opcodes |
|---|---|
| Push literals | `PUSH_NUM` `PUSH_BOOL` `PUSH_STR` `PUSH_NULL` |
| Variables | `LOAD` `STORE` `DEFINE` |
| Arithmetic | `ADD` `SUB` `MUL` `DIV` |
| Comparison | `EQ` `NEQ` `LT` `LTE` `GT` `GTE` |
| Logic | `AND_OP` `OR_OP` `NOT` |
| Control flow | `JUMP` `JUMP_IF_FALSE` |
| Functions | `CALL` `RETURN` `RETURN_NULL` |
| Scope | `PUSH_SCOPE` `POP_SCOPE` |
| Stack | `POP` `DUP` |
| I/O | `PRINT` `INPUT` |
| Meta | `HALT` |

Bytecode is stored as a flat `std::vector<uint8_t>`. Multi-byte operands use big-endian encoding. Doubles are stored as raw 8-byte IEEE 754 values.

---

## Language Syntax

### Variables

```
let x = 10;
let name = "CVM++";
let flag = true;
x = x + 1;
```

### Data Types

```
let integer = 42;
let decimal = 3.14;
let text    = "hello";
let yes     = true;
let no      = false;
```

### Arithmetic & Operators

```
let a = 10 + 5;     // 15
let b = 10 - 3;     // 7
let c = 4 * 3;      // 12
let d = 10 / 4;     // 2.5

let eq  = (a == b); // false
let neq = (a != b); // true
let lt  = (a < b);  // false
let gt  = (a > b);  // true

let both = (true and false);  // false
let either = (true or false); // true
let inv = !true;              // false
```

### String Concatenation

```
let greeting = "Hello, " + "World!";
print greeting;
```

### if / else

```
let score = 85;

if (score >= 90) {
    print "Grade: A";
} else {
    if (score >= 80) {
        print "Grade: B";
    } else {
        print "Grade: C";
    }
}
```

### while Loop

```
let i = 1;
while (i <= 5) {
    print i;
    i = i + 1;
}
```

### Functions

```
fn add(a, b) {
    return a + b;
}

fn factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

print add(10, 20);        // 30
print factorial(10);      // 3628800
```

### I/O

```
// Print any value
print "Enter your name:";

// Read from stdin into a variable
input name;
print "Hello, " + name;
```

---

## Getting Started

### Prerequisites

You need a C++17 compatible compiler and `make`.

```bash
# Ubuntu / Debian
sudo apt install g++ make

# macOS
xcode-select --install

# Windows
# Use WSL, or install MinGW and run commands in Git Bash
```

### Build

```bash
# Clone or download the project files into a folder
cd cvm++

# Build with make
make

# Or build manually
g++ -std=c++17 -O2 -o cvm main.cpp
```

This produces a single `cvm` executable.

---

## Usage

### Run a script

```bash
./cvm script.cvm
```

### Run the interactive REPL

```bash
./cvm
```

```
cvm> let x = 10;
cvm> print x + 5;
15
cvm> exit
```

### Run all sample tests

```bash
make test
```

### Clean build artifacts

```bash
make clean
```

### Command-line options

| Flag | Description |
|---|---|
| `./cvm file.cvm` | Run a .cvm script file |
| `./cvm` | Start interactive REPL |
| `./cvm --ast file.cvm` | Show AST tree then execute |
| `./cvm --bytecode file.cvm` | Show bytecode then execute |
| `./cvm --debug file.cvm` | Show AST + bytecode + execute |
| `./cvm --help` | Show usage information |

---

## Sample Scripts

### hello.cvm — Basics

```
print "Hello, CVM++!";

let x = 10;
let y = 3;
print x + y;
print x * y;

let name = "World";
print "Hello, " + name + "!";

print 5 < 10;
print 5 == 10;
```

**Output:**
```
Hello, CVM++!
13
30
Hello, World!
true
false
```

### control.cvm — Control Flow

```
let n = 5;
while (n > 0) {
    print n;
    n = n - 1;
}
print "Liftoff!";
```

**Output:**
```
5
4
3
2
1
Liftoff!
```

### functions.cvm — Recursion

```
fn fibonacci(n) {
    if (n <= 1) {
        return n;
    } else {
        let a = fibonacci(n - 1);
        let b = fibonacci(n - 2);
        return a + b;
    }
}

let k = 0;
while (k < 10) {
    print fibonacci(k);
    k = k + 1;
}
```

**Output:**
```
0
1
1
2
3
5
8
13
21
34
```

---

## Debug Mode

Use `--debug` to inspect the internals of the compiler pipeline:

```bash
./cvm --debug tests/hello.cvm
```

### AST output example

```
Program
├─ VarDecl(x)
│  ├─ NumLit(10)
├─ PrintStmt
│  ├─ BinaryOp(+)
│  │  ├─ Ident(x)
│  │  ├─ NumLit(5)
```

### Bytecode output example

```
═══════════════════════════════════════
  BYTECODE: <main>
═══════════════════════════════════════
    0  [L 1]  PUSH_NUM    10
    9  [L 1]  DEFINE      "x"
   12  [L 2]  LOAD        "x"
   15  [L 2]  PUSH_NUM    5
   24  [L 2]  ADD
   25  [L 2]  PRINT
   26  [L 2]  HALT
```

---

## How It Works

### Scoping

Every `{ }` block pushes a new scope onto the scope chain. Variable lookup walks from the innermost scope outward. When a block closes, its scope is popped and all local variables are destroyed.

### Function Calls

When `CALL` executes, the VM records the current scope stack depth as `scopeBase` in a new `Frame`, pushes a fresh scope, and binds parameters. When `RETURN` executes, all scopes down to `scopeBase` are unwound — this correctly handles recursive calls where inner blocks may have pushed additional scopes.

### Jump Patching

`if/else` and `while` emit placeholder jump targets during compilation, then patch the real byte offset once the target instruction is known. This is the standard backpatching technique used in production compilers.

### Short-Circuit Evaluation

`and` and `or` use `DUP` + `JUMP_IF_FALSE` to avoid evaluating the right operand when the result is already determined by the left — matching the behavior of every major language.

*Built as part of a systems programming project to demystify compilers, bytecode, and runtime environments by constructing them entirely from scratch.*
