.section .text
.global _start
.thumb

_start:
  bl test_func // 0x0 - 0x2 (expands to two thumb instructions)
  mov r0, #0x0 // 0x4 (expected next address)

test_func:
  mov r0, #0x0 // 0x6
