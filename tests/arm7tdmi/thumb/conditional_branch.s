.section .text
.global _start
.thumb
.align 2

_start:
  beq test_func // 0
  bne test_func // 2
  bcs test_func // 4
  bcc test_func // 6
  bmi test_func // 8
  bpl test_func // 10
  bvs test_func // 12
  bvc test_func // 14
  bhi test_func // 16
  bls test_func // 18
  bge test_func // 20
  blt test_func // 22
  bgt test_func // 24
  ble test_func // 26

test_func:
  mov r0, #0x0 // 28 (0x1c)
