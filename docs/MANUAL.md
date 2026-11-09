# TeenTaal — user manual

TeenTaal turns the FM-1 into a practice instrument for Indian classical music: a **tanpura**, **tabla** playing ten
taals, and a **reference tone** for intonation. Everything is played from one screen. At power-on everything is
silent; the saved settings come back as you left them.

> Version 1.0, AI-generated, tested only in emulators: see the [Disclaimer](../README.md#disclaimer). Updates may
> follow soon.

## The screen

```
 TEENTAAL                    UNSAVED     <- name; UNSAVED when there are changes not saved yet
 SA A3         TANPURA A2                <- your Sa; the tanpura's Sa
 Teentaal > Ektaal          120 /MIN     <- the taal (and the next one, when you changed it); the tempo
 9 /16         KHALI                     <- the matra playing now: red on sam, blue in khali
 X       2       0       3               <- the cycle: a cell for each matra, grouped by vibhag
 # . . . . . . . # . . . . . . .
 TANPURA ON    PA J50 V100               <- first string, jawari, volume
 TONE    OFF   SA V70                    <- the tone's swara, octave, volume
 CPU ...  TABLA V100
```

## Controls

| Control | What it does |
|---|---|
| **SEQ** | tanpura on / off |
| **PLAY** | tabla: start from the sam / stop (green light while playing) |
| **ARP** | reference tone on / off |
| **a key** | the tone on that key's swara **while you hold it** (like an harmonium) |
| **SEL + a key** | Sa becomes that key's note (the cents are kept) |
| **SEL + OCT− / OCT+** | Sa an octave down / up |
| **PRESETS** | Sa up or down by one cent per click |
| **SELECT** | the taal; while the tabla play, the new taal starts **at the end of the cycle** |
| **KNOB1** | tempo, 1 matra a minute per click (**SEL + KNOB1**: 10) |
| **KNOB2 / KNOB3 / KNOB4** | volume of tanpura / tabla / tone |
| **ALGORITHM** | jawari: the tanpura's buzz, from a short thread position (bright) to a long one (darker, slower) |
| **OCT− / OCT+** | the tone's octave (mandra, madhya, tar) |
| **MASTER** | overall volume |
| **EDIT** | the settings page (below); **HOME** goes back |
| **SAVE** | saves the settings (UNSAVED disappears) |

The keys light from your Sa upwards: **Sa and Pa bright**, the other shuddha swaras (Re, Ga, Ma, Dha, Ni) dim, so the
keyboard shows where the swaras are for the Sa you chose.

## Sa

The default Sa is **A3 (220 Hz)**, for a bansuri in A (the flute's Sa is an octave higher). For a bansuri in G: **SEL
+ the leftmost G key** (G3, 196 Hz). Sa can go from F#2 to G4. The tanpura plays an octave under your Sa (EDIT →
TANPURA REGISTER changes it). The dayan of the tabla tunes itself to Sa (to the octave between 190 and 380 Hz).

## Settings (EDIT)

SELECT picks the row, PRESETS changes the value, HOME goes back. The dots at the bottom show where you are in the list.

| Row | What it sets |
|---|---|
| FIRST STRING | the tanpura's first string: Pa (default), Ma, Ni or any swara |
| FINE: 1ST STRING, SA (2ND), SA (3RD), KHARAJ | fine tuning of each string, ±50 cents |
| TANPURA REGISTER | the tanpura an octave under Sa, or at Sa |
| PLUCK CYCLE | the length of the plucking cycle (2.5–8 s) |
| TANPURA IRREG. | how irregular the plucking is, like a real hand |
| TABLA IRREG. | the same for the tabla (timing and strength of the strokes) |
| TONE SWARA | the swara of the tone switched on with ARP |
| A REFERENCE | the reference A, 415–466 Hz (default 440) |

## The taals

| Taal | Matras | Vibhags | Theka |
|---|---|---|---|
| Teentaal | 16 | 4+4+4+4 | Dha Dhin Dhin Dha, Dha Dhin Dhin Dha, Dha Tin Tin Ta, Ta Dhin Dhin Dha |
| Ektaal | 12 | 2×6 | Dhin Dhin, DhaGe TiRaKiTa, Tu Na, Kat Ta, DhaGe TiRaKiTa, Dhin Na |
| Jhaptaal | 10 | 2+3+2+3 | Dhi Na, Dhi Dhi Na, Ti Na, Dhi Dhi Na |
| Rupak | 7 | 3+2+2 | Tin Tin Na, Dhi Na, Dhi Na (sam on khali) |
| Keherwa | 8 | 4+4 | Dha Ge Na Ti, Na Ka Dhi Na |
| Dadra | 6 | 3+3 | Dha Dhi Na, Dha Ti Na |
| Jhoomra | 14 | 3+4+3+4 | Dhin –Dha TiRaKiTa, Dhin Dhin DhaGe TiRaKiTa, Tin –Ta TiRaKiTa, Dhin Dhin DhaGe TiRaKiTa |
| Deepchandi | 14 | 3+4+3+4 | Dha Dhin –, Dha Dha Tin –, Ta Tin –, Dha Dha Dhin – |
| Dhamar | 14 | 5+2+3+4 | Ka Dhi Te Dhi Te, Dha –, Ga Ti Te, Ti Te Ta – |
| Chautaal | 12 | 2×6 | Dha Dha, Din Ta, KiTa Dha, Din Ta, TiTa KaTa, GaDi GaNa |

Tempo: 20 to 600 matras a minute. At slow (vilambit) tempos the theka is played as it is, without the fillers a tabla
player would add. Thekas vary between schools; the sources are in each file of `data/taal/`. To add a taal, write a
new file there (format: [data/taal/FORMAT.md](../data/taal/FORMAT.md)) and rebuild the firmware.

## Saving

**SAVE** keeps: Sa, the reference A, the taal and tempo, the three volumes, jawari, first string, the strings' fine
tuning, the tanpura's register and cycle, both irregularities, the tone's swara and octave. Which sounds are on is not
saved: the FM-1 always starts silent. While it saves, the sound dips for a moment (under a tenth of a second): writing
the flash stops the audio for about 12 ms, and the firmware fades out and in around it to avoid a click.

## Credits

The tabla strokes are recordings from the Freesound pack "tabla bols" by **mmiron** (CC0). The tanpura follows the
model by van Walstijn, Bridges and Mehes (DAFx-16). Platform: **fm1-base** by Charles Vestal and **felucca-bsp**
by Leo Kuroshita (MPL-2.0). TeenTaal-FM1 is by **ElectroNick** (<https://github.com/ELENICK97>). See
[CREDITS.md](../CREDITS.md).
