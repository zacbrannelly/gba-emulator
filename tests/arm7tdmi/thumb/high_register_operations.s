.section .text
.global _start
.thumb

_start:
  add r0, r9
  add r9, r0
  add r8, r9
  cmp r0, r9
  cmp r9, r0
  cmp r8, r9
  mov r0, r9
  mov r9, r0
  mov r8, r9
  bx r0
  bx r9
