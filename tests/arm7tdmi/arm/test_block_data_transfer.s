.section .text
.global _start

_start:
  ldmib r0, {r1, r2, r3} // Pre-increment load, load memory at r0 + 4 into r1, r2, and r3.
  ldmib r0!, {r1, r2, r3} // Pre-increment load, load memory at r0 + 4 into r1, r2, and r3 and increment r0 by 12.
  ldmia r0, {r1, r2, r3} // Post-increment load, load memory at r0 into r1, r2, and r3.
  ldmia r0!, {r1, r2, r3} // Post-increment load, load memory at r0 into r1, r2, and r3 and increment r0 by 12.
  ldmdb r0, {r1, r2, r3} // Pre-decrement load, load memory at r0 - 4 into r1, r2, and r3.
  ldmdb r0!, {r1, r2, r3} // Pre-decrement load, load memory at r0 - 4 into r1, r2, and r3 and decrement r0 by 12.
  ldmda r0, {r1, r2, r3} // Post-decrement load, load memory at r0 into r1, r2, and r3.
  ldmda r0!, {r1, r2, r3} // Post-decrement load, load memory at r0 into r1, r2, and r3 and decrement r0 by 12.

  // PC: 0x20
  stmib r0, {r1, r2, r3} // Pre-increment store, store r1, r2, and r3 into memory at r0 + 4.
  stmib r0!, {r1, r2, r3} // Pre-increment store, store r1, r2, and r3 into memory at r0 + 4 and increment r0 by 12.
  stmia r0, {r1, r2, r3} // Post-increment store, store r1, r2, and r3 into memory at r0.
  stmia r0!, {r1, r2, r3} // Post-increment store, store r1, r2, and r3 into memory at r0 and increment r0 by 12.
  stmdb r0, {r1, r2, r3} // Pre-decrement store, store r1, r2, and r3 into memory at r0 - 4.
  stmdb r0!, {r1, r2, r3} // Pre-decrement store, store r1, r2, and r3 into memory at r0 - 4 and decrement r0 by 12.
  stmda r0, {r1, r2, r3} // Post-decrement store, store r1, r2, and r3 into memory at r0.
  stmda r0!, {r1, r2, r3} // Post-decrement store, store r1, r2, and r3 into memory at r0 and decrement r0 by 12.

  // PC: 0x40
  ldmfd sp!, {r15}^ // R15 <- (SP), CPSR <- SPSR_mode
  stmfd r13, {r0-r14}^ // Save user mode regs on stack (allowed only in privileged modes).
