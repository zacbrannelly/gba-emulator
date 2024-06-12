.section .text
.global _start
.thumb

_start:
  str r0, [r1, r2]
  strb r0, [r1, r2]
  ldr r0, [r1, r2]
  ldrb r0, [r1, r2]
