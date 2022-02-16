; Courtesy of wapcaplet (https://github.com/wapcaplet/CCataSound)
; Opening door - latch sound, followed by squeak/swish
instr DoorOpen
    iDur = p3
    iAmp = p4
    schedule "Swish", 0, iDur*.5, iAmp
    schedule "CreakRising", 0, iDur, iAmp
endin

; Closing door - squeak/swish followed by latch/thump sound
instr DoorClose
    iDur = p3
    iAmp = p4
    schedule "CreakFalling", 0, iDur, iAmp
    schedule "Swish", iDur*.3, iDur*.5, iAmp
endin

