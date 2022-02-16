; Courtesy of wapcaplet (https://github.com/wapcaplet/CCataSound)
; Partial or component sounds that may be reused for several effects

; Subtle swish of air, as when opening or closing a door
instr Swish
    iDur = p3
    iAmp = p4
    ; pink noise with a lowpass filter and triangle envelope
    aSwish pinkish iAmp/2
    aSwish lowpass2 aSwish, 200, 1
    aSwishEnv linseg 0, iDur/2, 1, iDur/2, 0
    aSig = aSwish * aSwishEnv
    outs aSig, aSig
endin

; Creak sound, rising in pitch, as when opening a door
instr CreakRising
    iDur = p3
    iAmp = p4
    kPitchEnv expon 0.05, iDur, 0.001
    aCreak mpulse iAmp, kPitchEnv
    aSig = aCreak
    outs aSig, aSig
endin

; Creak sound, falling in pitch, as when closing a door
instr CreakFalling
    iDur = p3
    iAmp = p4
    kPitchEnv expon 0.001, iDur, 0.05
    aCreak mpulse iAmp, kPitchEnv
    aSig = aCreak
    outs aSig, aSig
endin

