.section .text
.global _start

_start:
  b infinite_loop

infinite_loop:
  b infinite_loop
