# Brainfuck JIT

Compiles Brainfuck to x86-64 machine code at load time and runs it in-process.

---
## Scope
- Target: Linux x86-64 (System V ABI), AT&T syntax.
- Input: a Brainfuck source file. Any non-command character is a comment.
- Out of scope: other architectures, debugger, tiered compilation.

## Optimizations
- Run-length folding.
- Clear loop: `[-]` becomes a single set instruction.
- Remove "fake" loops.

## Executable memory
- `mmap` an anonymous RW region, write the code, then `mprotect` to RX before calling.
- Never RWX: pages are writable or executable, not both (W^X).
