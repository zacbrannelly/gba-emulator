.section .text
.global _start
.thumb

_start:
  stmia r0!, {r1, r2}
  ldmia r0!, {r1, r2}
