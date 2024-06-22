.section .text
.global _start
.thumb

.align 2, 0

_start:
  ldr r0, _data // 0x0
  mov r0, #0 // 0x2
  ldr r0, _data // 0x4
  mov r0, #0 // 0x6
  b _start // 0x8

_data: .4byte 0x04000110 // 0xA
