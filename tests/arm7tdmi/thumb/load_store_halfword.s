.section .text
.global _start
.thumb

_start:
  strh r0, [r1, #4]
  ldrh r0, [r1, #4]
