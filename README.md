# Brainfuck JIT

A from-scratch just-in-time compiler for Brainfuck in C, for Linux x86-64.

It reads a Brainfuck source file, turns it into a small optimized IR, emits x86-64 machine code straight into memory and runs it in-process. There is no interpreter loop, no assembler and no dependencies beyond libc.

## About

This is a small learning project. It was built to understand how a JIT works: how machine code is encoded by hand, how jumps are backpatched and how executable memory is set up safely. Brainfuck was chosen because the language is tiny, so the compiler stays small and the interesting part is the code generation.

It is not meant to be a fast or complete Brainfuck implementation.

## Usage

Write a program (this one prints `Hello World!`):

```
++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.
```

Build and run it:

```bash
make
./build/bfjit hello.bf
```

Or let `make` do both:

```bash
make run hello.bf
```

Any character that is not one of `+ - < > . , [ ]` is a comment. Input is read from stdin and output is written to stdout.

Errors are printed to stderr and the process exits with a failure code:

| Exit code | Meaning                                                                                  |
|-----------|------------------------------------------------------------------------------------------|
| 0         | The program ran to completion                                                            |
| 1         | Wrong number of arguments, unreadable file, unmatched bracket, or JIT / `mmap` failure   |

## How it works

The source goes through three stages, each in its own module, and then runs.

| Stage | File                         | What it does                                                                         |
|-------|------------------------------|--------------------------------------------------------------------------------------|
| Lexer | [src/lexer.c](src/lexer.c)   | Skips everything that is not a command and returns the next command character       |
| IR    | [src/ir.c](src/ir.c)         | Single pass that builds a list of ops, folding runs and applying the optimizations   |
| JIT   | [src/jit.c](src/jit.c)       | Emits one machine code sequence per op, patches the jumps and makes the code runnable |
| Main  | [src/main.c](src/main.c)     | Allocates the tape with guard pages and calls the generated code                     |

**IR.** Each op is a kind plus an operand. Runs of `+ - < > . ,` collapse into a single op with a repeat count, so `+++++` is one `OP_INC` with operand 5. A stack of open brackets matches `[` with `]`, and each side stores the index of the other. Three optimizations are applied while building the ops:

| Optimization   | Effect                                                                                                    |
|----------------|-----------------------------------------------------------------------------------------------------------|
| Run-length     | `+++`, `<<<` and so on become one op with a count                                                         |
| Clear loop     | `[-]` and `[+]` (any odd run of `+` or `-`, since cells wrap) become a single `OP_CLEAR`                  |
| Fake loops     | A loop at the start of the program, or right after a `]` or a clear, can never run because the cell is zero, so it is dropped entirely |

**Code generation.** The generated function follows the System V ABI and takes the tape pointer in `rdi`. It saves the callee-saved `rbx`, keeps the tape pointer in it for the whole run and restores it on return, so no register allocation is needed. Every op is a fixed encoding:

| Op                  | Machine code                                          |
|---------------------|-------------------------------------------------------|
| `+` / `-`           | `addb` / `subb` an 8-bit immediate on `(%rbx)`        |
| `>` / `<`           | `addq` / `subq` a 32-bit immediate on `%rbx`          |
| `.` / `,`           | `write` / `read` syscall for one byte, repeated by the count |
| `[` / `]`           | `cmpb $0, (%rbx)` then `jz` / `jnz` with a 32-bit relative offset |
| clear               | `movb $0, (%rbx)`                                     |

**Jumps.** Loop jumps are emitted with a placeholder offset. The start address of every op is recorded as the code is written, and once all the code exists each jump is patched with `target - end of jump`. `[` jumps to its `]` and `]` jumps back to its `[`, and both re-test the current cell.

**Executable memory.** The code is built in a normal heap buffer, copied into an anonymous `mmap` region that is writable, then switched to read and execute with `mprotect` before it is called. The pages are never writable and executable at the same time (W^X).

**Tape.** A 10 MiB zeroed `mmap` region of byte cells that wrap around on overflow. The pointer starts in the middle, so the program can go about 5 MiB in either direction. Four inaccessible guard pages sit before and after the tape.

## Known limitations

- Linux x86-64 only. The emitted instructions and the syscall numbers are hard coded.
- No bounds checking on the tape pointer. Running off the tape hits a guard page and the process dies with a segmentation fault, but a long enough jump can skip over the guard pages.
- The tape is fixed at 10 MiB and cells are always 8-bit.
- A single run of `+ - < >` of 5 MiB or more is rejected at compile time.
- `.` and `,` are one syscall per byte, with no buffering and no error checking. At end of input `,` leaves the cell unchanged.
- A run of `.` or `,` is unrolled in the machine code, so very long runs make the code larger.
- Only three optimizations: run-length folding, clear loops and fake loops. No scan loops, no multiply loops and no tiered compilation.
- Unmatched brackets are reported as an error, but without a position.
- No optimization of the tape pointer movement across loops, no debugger and no disassembler.
- The input is always a file, there is no REPL.

## Documentation

Design notes live in [docs/DESIGN.md](docs/DESIGN.md).
Alongside them there is the documentation used to build this project [docs/DOCUMENTATION.md](docs/DOCUMENTATION.md).

---
# Tree
``` bash
Brainfuck JIT
├── docs
│   ├── DESIGN.md
│   └── DOCUMENTATION.md
├── .github
│   └── workflows
│       └── ci.yml
├── .gitignore
├── LICENSE
├── Makefile
├── README.md
├── src
│   ├── ir.c
│   ├── ir.h
│   ├── jit.c
│   ├── jit.h
│   ├── lexer.c
│   ├── lexer.h
│   ├── main.c
│   ├── opcode.h
│   ├── utils.c
│   └── utils.h
└── tests
    ├── test_ir.c
    ├── test_jit.c
    ├── test_lexer.c
    ├── test_main.c
    ├── test_main.h
    └── test_utils.c
```
# Build

```bash
# Compile the compiler (build/bfjit)
make all

# Compile and run a Brainfuck file
make run hello.bf

# Clean all build artifacts
make clean
```

# Tests

```bash
# Run the whole test suite
make test

# Run tests with Undefined Behavior Sanitizer
make test-ubsan
```

The suite has 228 checks covering the utilities, the lexer, the IR (run-length folding, clear loops, fake loops, bracket matching and errors) and the JIT (the emitted code and the programs it runs). CI runs `test` and `test-ubsan` with both gcc and clang on every push to `main` and on pull requests, on an x86-64 runner.

---
# License
See [LICENSE](LICENSE).
