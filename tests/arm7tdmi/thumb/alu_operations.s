.section .text
.global _start
.thumb

_start:
  and r0, r1
  eor r0, r1
  lsl r0, r1
  lsr r0, r1
  asr r0, r1
  adc r0, r1
  sbc r0, r1
  ror r0, r1
  tst r0, r1
  neg r0, r1
  cmp r0, r1
  cmn r0, r1
  orr r0, r1
  mul r0, r1
  bic r0, r1
  mvn r0, r1
