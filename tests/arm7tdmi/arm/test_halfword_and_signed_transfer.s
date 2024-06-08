.section .text
.global _start

_start:
  ldrh r0, [r1] // Load the halfword at the address in r1 into r0.
  ldrh r0, [r1, #4] // Load the halfword at the address in r1 + 4 into r0.
  ldrh r0, [r1, r2] // Load the halfword at the address in r1 + r2 into r0.
  ldrh r0, [r1, r2]! // Load the halfword at the address in r1 + r2 into r0 and increment r1 by r2.
  ldrh r0, [r1], r2 // Load the halfword at the address in r1 into r0 and increment r1 by r2.
  ldrsh r0, [r1] // Load the signed halfword at the address in r1 into r0.
  ldrsb r0, [r1] // Load the signed byte at the address in r1 into r0.
  
  strh r0, [r1] // Store the halfword in r0 at the address in r1.
  strh r0, [r1, #4] // Store the halfword in r0 at the address in r1 + 4.
  strh r0, [r1, r2] // Store the halfword in r0 at the address in r1 + r2.
  strh r0, [r1, r2]! // Store the halfword in r0 at the address in r1 + r2 and increment r1 by r2.
  strh r0, [r1], r2 // Store the halfword in r0 at the address in r1 and increment r1 by r2.
