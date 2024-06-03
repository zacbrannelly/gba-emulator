.section .text
.global _start

_start:
  b ahead # jump to ahead
  mov r0, #0 # never reached
  mov r7, #1
  swi 0

ahead:
  b ahead # loop forever
