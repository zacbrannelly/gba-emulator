.section .text
.global _start
.thumb

_start:
  strh r0, [r1, r2]
  ldrh r0, [r1, r2]
  ldsb r0, [r1, r2]
  ldsh r0, [r1, r2]
