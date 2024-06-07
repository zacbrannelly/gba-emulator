.section .text
.global _start

_start:
  mul r0, r1, r2 // Multiply r1 and r2 and store the result in r0
  mla r0, r1, r2, r3 // Multiply r1 and r2, add r3 and store the result in r0
  umull r0, r1, r2, r3 // Unsigned multiply r2 and r3 and store the result in r0 and r1
  umlal r0, r1, r2, r3 // Unsigned multiply r2 and r3, add r0 and r1 and store the result in r0 and r1
  smull r0, r1, r2, r3 // Signed multiply r2 and r3 and store the result in r0 and r1
  smlal r0, r1, r2, r3 // Signed multiply r2 and r3, add r0 and r1 and store the result in r0 and r1
