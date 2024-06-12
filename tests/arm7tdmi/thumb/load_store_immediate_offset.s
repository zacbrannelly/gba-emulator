.section .text
.global _start
.thumb

_start:
  str r0, [r1, #4]
  ldr r0, [r1, #4]
  strb r0, [r1, #4]
  ldrb r0, [r1, #4]
