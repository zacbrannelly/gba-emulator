.section .text
.global _start
.thumb

_start:
  b test_func // 0

test_func:
  mov r0, #0x0 // 2 (0x2)
