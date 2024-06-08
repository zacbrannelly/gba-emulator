.section .text
.global _start

_start:
  swp r0, r1, [r2] // Swap the value in r1 with the value at the address in r2 and store the original value in r0 (word).
  swpb r0, r1, [r2] // Swap the value in r1 with the value at the address in r2 and store the original value in r0 (byte).
