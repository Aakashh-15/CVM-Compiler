#include "cvm.h"
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "compiler.h"
#include "disassembler.h"
#include "vm.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────
//  ANSI colour helpers
// ─────────────────────────────────────────────────────────────────
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define DIM     "\033[2m"

// ─────────────────────────────────────────────────────────────────
//  Pipeline runner
// ─────────────────────────────────────────────────────────────────
struct RunOptions {
    bool showAST    = false;
    bool showBytecode = false;
    bool repl       = false;
};

void runSource(const std::string& source, const RunOptions& opts, VM& vm) {
    // 1. Lex
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    // 2. Parse
    Parser parser(std::move(tokens));
    ASTPtr ast = parser.parse();

    // 3. Optional AST dump
    if (opts.showAST) {
        std::cout << CYAN << "\n── AST ──────────────────────────────────\n" << RESET;
        ASTPrinter printer;
        printer.dump(*ast);
    }

    // 4. Compile
    Compiler compiler;
    auto [chunk, functions] = compiler.compile(*ast);

    // 5. Optional bytecode dump
    if (opts.showBytecode) {
        std::cout << YELLOW << "\n── BYTECODE ─────────────────────────────\n" << RESET;
        Disassembler dis;
        dis.disassemble(chunk, "main");
        for (auto& [name, fn] : functions)
            dis.disassemble(fn.chunk, name);
    }

    // 6. Execute
    if (opts.showAST || opts.showBytecode)
        std::cout << GREEN << "\n── OUTPUT ───────────────────────────────\n" << RESET;
    vm.execute(chunk, functions);
}

// ─────────────────────────────────────────────────────────────────
//  REPL
// ─────────────────────────────────────────────────────────────────
void repl(const RunOptions& opts) {
    VM vm;
    std::cout << BOLD << CYAN
              << "  ╔═══════════════════════════════════╗\n"
              << "  ║        CVM++ Interactive REPL     ║\n"
              << "  ║   Type 'exit' or Ctrl-D to quit   ║\n"
              << "  ╚═══════════════════════════════════╝\n"
              << RESET;
    if (opts.showAST)      std::cout << DIM << "  [AST debug ON]\n" << RESET;
    if (opts.showBytecode) std::cout << DIM << "  [Bytecode debug ON]\n" << RESET;
    std::cout << "\n";

    std::string line, accumulated;
    int depth = 0;
    while (true) {
        std::cout << (accumulated.empty() ? BOLD GREEN "cvm> " RESET : BOLD YELLOW "...> " RESET);
        if (!std::getline(std::cin, line)) { std::cout << "\nBye!\n"; break; }
        if (line == "exit" || line == "quit") { std::cout << "Bye!\n"; break; }
        if (line.empty()) continue;

        for (char c : line) {
            if (c == '{') depth++;
            if (c == '}') depth--;
        }
        accumulated += line + "\n";

        if (depth <= 0) {
            try {
                runSource(accumulated, opts, vm);
            } catch (const std::exception& e) {
                std::cerr << RED << "Error: " << e.what() << RESET << "\n";
            }
            accumulated.clear();
            depth = 0;
        }
    }
}

// ─────────────────────────────────────────────────────────────────
//  File runner
// ─────────────────────────────────────────────────────────────────
void runFile(const std::string& path, const RunOptions& opts) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << RED << "Cannot open file: " << path << RESET << "\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string source = ss.str();

    if (opts.showAST || opts.showBytecode) {
        std::cout << BOLD MAGENTA
                  << "═══════════════════════════════════════\n"
                  << "  CVM++  ·  " << path << "\n"
                  << "═══════════════════════════════════════\n" << RESET;
    }
    VM vm;
    try {
        runSource(source, opts, vm);
    } catch (const std::exception& e) {
        std::cerr << RED << "\n[Runtime Error] " << e.what() << RESET << "\n";
        std::exit(1);
    }
}

// ─────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────
void printHelp(const char* argv0) {
    std::cout <<
        BOLD CYAN "CVM++" RESET " — Custom Stack-Based VM & Scripting Language\n\n"
        BOLD "Usage:\n" RESET
        "  " << argv0 << " [options] [file.cvm]\n\n"
        BOLD "Options:\n" RESET
        "  --ast        Show Abstract Syntax Tree\n"
        "  --bytecode   Show compiled bytecode\n"
        "  --debug      Show both AST and bytecode\n"
        "  --repl       Force interactive REPL mode\n"
        "  --help       Show this message\n\n"
        BOLD "Examples:\n" RESET
        "  " << argv0 << " script.cvm\n"
        "  " << argv0 << " --debug script.cvm\n"
        "  " << argv0 << " --repl\n";
}

int main(int argc, char** argv) {
    RunOptions opts;
    std::string filename;
    bool forceRepl = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--ast")      opts.showAST = true;
        else if (arg == "--bytecode") opts.showBytecode = true;
        else if (arg == "--debug")    { opts.showAST = true; opts.showBytecode = true; }
        else if (arg == "--repl")     forceRepl = true;
        else if (arg == "--help" || arg == "-h") { printHelp(argv[0]); return 0; }
        else if (arg[0] != '-')  filename = arg;
        else { std::cerr << "Unknown option: " << arg << "\n"; return 1; }
    }

    if (!filename.empty()) {
        runFile(filename, opts);
    } else {
        opts.repl = true;
        repl(opts);
    }
    return 0;
}
