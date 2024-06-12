.section .text
.global _start
.thumb

_start:
  str r0, [sp, #4]
  ldr r0, [sp, #4]
