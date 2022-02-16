<CsoundSynthesizer>
; Courtesy of wapcaplet (https://github.com/wapcaplet/CCataSound)
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr     = 44100
kr     = 4410
ksmps  = 10
nchnls = 2

#include "soundbites.orc"
#include "effects.orc"

</CsInstruments>
<CsScore>
i "DoorOpen" 0 0.5 8000
e
</CsScore>
</CsoundSynthesizer>
