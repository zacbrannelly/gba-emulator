.section .text
.global _start

_start:
  ldr r0, [r1] // Load the value at the address in r1 into r0.
  ldr r0, [r1, #4] // Load the value at the address in r1 + 4 into r0.
  ldr r0, [r1, r2] // Load the value at the address in r1 + r2 into r0.
  ldr r0, [r1, r2, lsl #2] // Load the value at the address in r1 + r2 * 4 into r0.
  ldr r0, [r1, r2, lsl #2]! // Load the value at the address in r1 + r2 * 4 into r0 and increment r1 by r2 * 4.
  ldr r0, [r1], r2 // Load the value at the address in r1 into r0 and increment r1 by r2.
  ldr r0, [r1], r2, lsl #2 // Load the value at the address in r1 into r0 and increment r1 by r2 * 4.
  ldrb r0, [r1] // Load the byte at the address in r1 into r0.
  ldr r0, [r1, -r2] // Load the value at the address in r1 - r2 into r0.

  str r0, [r1] // Store the value in r0 at the address in r1.
  str r0, [r1, #4] // Store the value in r0 at the address in r1 + 4.
  str r0, [r1, r2] // Store the value in r0 at the address in r1 + r2.
  str r0, [r1, r2, lsl #2] // Store the value in r0 at the address in r1 + r2 * 4.
  str r0, [r1, r2, lsl #2]! // Store the value in r0 at the address in r1 + r2 * 4 and increment r1 by r2 * 4.
  str r0, [r1], r2 // Store the value in r0 at the address in r1 and increment r1 by r2.
  str r0, [r1], r2, lsl #2 // Store the value in r0 at the address in r1 and increment r1 by r2 * 4.
  strb r0, [r1] // Store the byte in r0 at the address in r1.
  str r0, [r1, -r2] // Store the value in r0 at the address in r1 - r2.
