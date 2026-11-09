# Controls (design)

The control mapping as implemented in `ui/ui.c`. For using the instrument, read the [manual](MANUAL.md); the history
of the design (the proposal of step 1 and ElectroNick's answers) is in Italian in [CONTROLS.it.md](CONTROLS.it.md).

Principle: **everything is played from one screen**. Daily functions have a fixed control and no menu; rarely used
settings live on one separate page (EDIT).

| Control | Main page | EDIT page |
|---|---|---|
| PLAY | tabla (taal) on / off, from the sam; green light | — |
| SEQ | tanpura on / off | — |
| ARP | reference tone on / off | — |
| key | the tone on that swara while held (the last key pressed wins) | — |
| SEL + key | Sa = that key's note, cents kept | — |
| SEL + OCT− / OCT+ | Sa an octave down / up | — |
| OCT− / OCT+ | the tone's octave | — |
| SELECT | the taal (round the list); while playing, the change waits for the end of the cycle | the row |
| PRESETS | Sa by one cent | the row's value |
| ALGORITHM | jawari | jawari |
| KNOB1 | tempo (SEL: ×10) | tempo |
| KNOB2, KNOB3, KNOB4 | tanpura, tabla, tone volume | the same |
| EDIT / HOME | to the EDIT page / back | |
| SAVE | save the settings (fade, write, fade back) | the same |
| FX, ENV, LFO, GLO, REC | not used | |

Lights: SEQ, ARP, PLAY (green), EDIT when their function is on; SAVE while there are unsaved changes; keys from Sa up,
Sa and Pa bright, Re Ga Ma Dha Ni dim, a held key bright.
