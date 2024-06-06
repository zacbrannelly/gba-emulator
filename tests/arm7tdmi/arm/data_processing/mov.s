.section .text
.global _start

_start:
  mov r0, #1
  mov r1, #2
  mov r2, #3
  mov r3, #4
  mov r4, r3
  mov r5, r2, lsl r1 // r2 = 3, r1 = 2, r5 = r2 << r1 = 3 << 2 = 12
  mov r5, r2, lsl #1 // r2 = 3, r5 = r2 << 1 = 3 << 1 = 6
  mov r5, r2, lsr #1 // r2 = 3, r5 = r2 >> 1 = 3 >> 1 = 1
  mov r5, r2, asr #1 // r2 = 3, r5 = r2 >> 1 = 3 >> 1 = 1
  mov r5, r2, ror #1 // r2 = 3, r5 = r2 ror 1 = 3 ror 1 = 0x80000001
  movs r5, r2, rrx // r2 = 3, r5 = r2 rrx = 0x80000001
