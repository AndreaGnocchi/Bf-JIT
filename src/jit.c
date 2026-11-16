#include "jit.h"
#include "ir.h"
#include "opcode.h"
#include "utils.h"
#include <sys/mman.h>
#include <stdint.h>

bool jit_compile(ops_t ops, code_t* code) {
  string_builder_t sb          = {0};
  backpatches_t    backpatches = {0};
  addr_t           addrs       = {0};

  SB_APPEND_CSTR(&sb, "\x53\x48\x89\xfb");
  
  for (size_t i = 0; i < ops.count; ++i) {
    op_t op = ops.items[i];
    DA_APPEND(&addrs, sb.count);
    switch (op.kind) {
    case OP_INC: {
      SB_APPEND_CSTR(&sb, "\x80\x03"); // addb $imm8, (%rbx)
      DA_APPEND(&sb, op.operand & 0xFF);
    } break;
      
    case OP_DEC: {
      SB_APPEND_CSTR(&sb, "\x80\x2b"); // subb $imm8, (%rbx)
      DA_APPEND(&sb, op.operand & 0xFF);
    } break;
      
    case OP_LEFT: {
      SB_APPEND_CSTR(&sb, "\x48\x81\xeb"); // subq $imm32, %rbx
      uint32_t operand = (uint32_t)op.operand;
      DA_APPEND_MANY(&sb, &operand, sizeof(operand));
    } break;
      
    case OP_RIGHT: {
      SB_APPEND_CSTR(&sb, "\x48\x81\xc3"); // addq $imm32, %rbx
      uint32_t operand = (uint32_t)op.operand;
      DA_APPEND_MANY(&sb, &operand, sizeof(operand));
    } break;
      
    case OP_OUTPUT: {
      static const uint8_t emit[] = {
        0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // movq $1, %rax (SYS_write)
        0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00, // movq $1, %rdi (stdout)
        0x48, 0x89, 0xde,                         // movq %rbx, %rsi (tape ptr)
        0x48, 0xc7, 0xc2, 0x01, 0x00, 0x00, 0x00, // movq $1, %rdx (len = 1)
        0x0f, 0x05                                // syscall
      };
      
      for (size_t i = 0; i < op.operand; ++i) {
        DA_APPEND_MANY(&sb, emit, sizeof(emit));
      }
    } break;
      
    case OP_INPUT: {
      static const uint8_t emit[] = {
        0x48, 0x31, 0xc0,                         // xorq %rax, %rax       (SYS_read = 0)
        0x48, 0x31, 0xff,                         // xorq %rdi, %rdi       ( stdin)
        0x48, 0x89, 0xde,                         // movq %rbx, %rsi       (buf = tape ptr)
        0x48, 0xc7, 0xc2, 0x01, 0x00, 0x00, 0x00, // movq $1, %rdx         (count = 1 byte)
        0x0f, 0x05                                // syscall
      };
      
      for (size_t i = 0; i < op.operand; ++i) {
        DA_APPEND_MANY(&sb, emit, sizeof(emit));
      }
    } break;
      
    case OP_JUMP_IF_ZERO: {
      static const uint8_t emit[] = {
        0x80, 0x3b, 0x00,             // cmpb $0, (%rbx)
        0x0f, 0x84,                   // jz rel32 (near jump)
        0x00, 0x00, 0x00, 0x00        // 4-byte placeholder for rel32
      };
      DA_APPEND_MANY(&sb, emit, sizeof(emit));
      
      size_t operand_byte_addr = sb.count - 4;
      size_t src_byte_addr = sb.count;
      backpatch_t bp = { operand_byte_addr, src_byte_addr, op.operand };

      DA_APPEND(&backpatches, bp);
    } break;
      
    case OP_JUMP_IF_NONZERO: {
      static const uint8_t emit[] = {
        0x80, 0x3b, 0x00,             // cmpb $0, (%rbx)
        0x0f, 0x85,                   // jnz rel32 (near jump)
        0x00, 0x00, 0x00, 0x00        // 4-byte placeholder for rel32
      };
      DA_APPEND_MANY(&sb, emit, sizeof(emit));
      
      size_t operand_byte_addr = sb.count - 4;
      size_t src_byte_addr = sb.count;
      backpatch_t bp = { operand_byte_addr, src_byte_addr, op.operand };

      DA_APPEND(&backpatches, bp);
    } break;
      
    case OP_CLEAR: {
      static const uint8_t emit[] = { 0xc6, 0x03, 0x00 }; // movb $0, (%rbx)
      DA_APPEND_MANY(&sb, emit, sizeof(emit));
    } break;
    }
  }
  DA_APPEND(&addrs, sb.count);

  static const uint8_t epilogue[] = { 0x5b, 0xc3 };
  DA_APPEND_MANY(&sb, epilogue, sizeof(epilogue));
  
  for (size_t i = 0; i < backpatches.count; ++i) {
    backpatch_t bp       = backpatches.items[i];
    int32_t     src_addr = bp.src_byte_addr;
    int32_t     dst_addr = addrs.items[bp.dst_op_index];
    int32_t     operand  = dst_addr - src_addr;
    memcpy(&sb.items[bp.operand_byte_addr], &operand, sizeof(operand));
  }

  bool result = true;
  code->len = sb.count;
  code->run = mmap(NULL, code->len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code->run == MAP_FAILED) {
    code->run = NULL;
    code->len = 0;
    result = false;
  } else {
    memcpy(code->run, sb.items, code->len);

    if (mprotect(code->run, code->len, PROT_READ | PROT_EXEC) == -1) {
        munmap(code->run, code->len);
        code->run = NULL;
        code->len = 0;
        result = false;
    }
  }

  free(sb.items);
  free(backpatches.items);
  free(addrs.items);
  return result;
}
