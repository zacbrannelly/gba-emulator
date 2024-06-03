.section .text
.global _start

_start:
  b ahead
  mov r0, #0
  mov r7, #1
  swi 0

before:
  b before

ahead:
  b before
