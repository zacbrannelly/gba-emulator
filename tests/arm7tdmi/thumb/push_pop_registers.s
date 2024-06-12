.section .text
.global _start
.thumb

_start:
  push {r0, r1, r2}
  push {r0, r1, r2, lr}
  pop {r0, r1, r2}
  pop {r0, r1, r2, pc}
