.section .text
.global _start

_start:
  moveq r0, #1 // Move 1 into r0 if the zero flag is set.
  movne r0, #0 // Move 0 into r0 if the zero flag is not set.
  movcs r0, #1 // Move 1 into r0 if the carry flag is set.
  movcc r0, #0 // Move 0 into r0 if the carry flag is not set.
  movmi r0, #1 // Move 1 into r0 if the negative flag is set.
  movpl r0, #0 // Move 0 into r0 if the negative flag is not set.
  movvs r0, #1 // Move 1 into r0 if the overflow flag is set.
  movvc r0, #0 // Move 0 into r0 if the overflow flag is not set.
  movhi r0, #1 // Move 1 into r0 if the carry flag is set and the zero flag is not set.
  movls r0, #0 // Move 0 into r0 if the carry flag is not set or the zero flag is set.
  movge r0, #1 // Move 1 into r0 if the negative flag is equal to the overflow flag.
  movlt r0, #0 // Move 0 into r0 if the negative flag is not equal to the overflow flag.
  movgt r0, #1 // Move 1 into r0 if the zero flag is not set and the negative flag is equal to the overflow flag.
  movle r0, #0 // Move 0 into r0 if the zero flag is set or the negative flag is not equal to the overflow flag.
  moval r0, #1 // Move 1 into r0 unconditionally.
