.section .text
.global _start

_start:
  bl function
  mov r0, #0

function:
  mov pc, lr
