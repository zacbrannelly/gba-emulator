.section .text
.global _start
.thumb

_start:
  lsl r0, r1, #5
  lsr r0, r1, #5
  asr r0, r1, #5
