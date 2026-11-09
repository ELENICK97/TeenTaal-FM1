# TeenTaal — manuale d'uso

> Versione italiana. English: [MANUAL.md](MANUAL.md). Firmware generato con l'IA (Claude) e provato solo negli emulatori: si usa a proprio rischio (vedi il Disclaimer in [README.md](../README.md#disclaimer)).

TeenTaal trasforma l'FM-1 in uno strumento per esercitarsi nella musica classica indiana: un **tanpura**, i
**tabla** che suonano dieci taal, e un **tono di riferimento** per l'intonazione. Tutto si suona da una sola
schermata. All'accensione è tutto spento; le impostazioni salvate tornano come le avevi lasciate.

## La schermata

```
 TEENTAAL                     UNSAVED      ← nome; UNSAVED se ci sono modifiche non salvate
 SA A3          TANPURA A2                 ← il tuo Sa; il Sa del tanpura
 Teentaal > Ektaal            120 /MIN     ← il taal (e il prossimo, se l'hai cambiato); il tempo
 9 /16          KHALI                      ← la matra che suona ora: rossa sul sam, blu nel khali
 X       2       0       3                 ← il ciclo: una casella per matra, divise in vibhag
 ■ □ □ □ □ □ □ □ ■ □ □ □ □ □ □ □
 TANPURA ON     PA J50 V100                ← prima corda, jawari, volume
 TONE    OFF    SA V70                     ← swara del tono, ottava, volume
 CPU …  TABLA V100
```

## I comandi

| Comando | Cosa fa |
|---|---|
| **SEQ** | tanpura acceso / spento |
| **PLAY** | tabla: partono dal sam / si fermano (luce verde mentre suonano) |
| **ARP** | tono di riferimento acceso / spento |
| **un tasto** | il tono su quello swara **finché lo tieni premuto** (come un harmonium) |
| **SEL + un tasto** | il Sa diventa la nota di quel tasto (i cents restano) |
| **SEL + OCT− / OCT+** | il Sa un'ottava sotto / sopra |
| **PRESETS** | il Sa di un cent per scatto |
| **SELECT** | il taal; se i tabla suonano, il nuovo taal parte **alla fine del ciclo** |
| **KNOB1** | il tempo, 1 matra al minuto per scatto (**SEL + KNOB1**: 10) |
| **KNOB2 / KNOB3 / KNOB4** | volume di tanpura / tabla / tono |
| **ALGORITHM** | jawari (il ronzio del tanpura: dal filo vicino al ponte, più brillante, a lontano) |
| **OCT− / OCT+** | l'ottava del tono (mandra, madhya, tar) |
| **MASTER** | volume generale |
| **EDIT** | le impostazioni (sotto); **HOME** torna indietro |
| **SAVE** | salva le impostazioni (scompare UNSAVED) |

I tasti si illuminano dal Sa in su: **Sa e Pa accesi**, gli altri swara shuddha (Re, Ga, Ma, Dha, Ni) tenui. Così
la tastiera mostra dove sono gli swara rispetto al Sa scelto.

## Il Sa

Il Sa di default è **La3 (220 Hz)**, adatto a un bansuri in La (il Sa del flauto è un'ottava sopra). Per un bansuri
in Sol: **SEL + il primo tasto Sol a sinistra** (Sol3, 196 Hz). Il tanpura suona un'ottava sotto il tuo Sa (si può cambiare in EDIT → TANPURA
REGISTER). Il dayan dei tabla si accorda da solo sul Sa.

## Le impostazioni (EDIT)

SELECT sceglie la riga, PRESETS cambia il valore, HOME torna alla schermata principale. I puntini in basso mostrano
dove sei nell'elenco.

| Riga | Cosa regola |
|---|---|
| FIRST STRING | la prima corda del tanpura: Pa (default), Ma, Ni o qualsiasi swara |
| FINE: 1ST STRING, SA (2ND), SA (3RD), KHARAJ | accordatura fine di ogni corda, ±50 cents |
| TANPURA REGISTER | il tanpura un'ottava sotto il Sa, o sul Sa stesso |
| PLUCK CYCLE | la durata del ciclo di pizzico (2,5–8 s) |
| TANPURA IRREG. | quanto il pizzico è irregolare, come una mano vera |
| TABLA IRREG. | lo stesso per i tabla (tempi e forza dei colpi) |
| TONE SWARA | lo swara del tono acceso con ARP |
| A REFERENCE | il La di riferimento, 415–466 Hz (default 440) |

## I taal

Teentaal (16), Ektaal (12), Jhaptaal (10), Rupak (7), Keherwa (8), Dadra (6), Jhoomra (14), Deepchandi (14),
Dhamar (14), Chautaal (12). Il tempo va da 20 a 600 matra al minuto. Ogni taal suona la sua theka; le fonti sono
nei file `data/taal/`. Per aggiungere un taal basta scrivere un file nuovo (formato in `data/taal/FORMAT.md`) e
ricompilare.

## Salvare

**SAVE** salva: Sa, La di riferimento, il taal e il tempo, i tre volumi, jawari, prima corda, accordature fini,
registro e ciclo del tanpura, le irregolarità, swara e ottava del tono. Non salva se le sorgenti sono accese:
all'accensione l'FM-1 parte in silenzio. Durante il salvataggio il suono si abbassa per un istante (meno di un decimo di secondo): la scrittura nella
memoria flash ferma l'audio per circa 12 ms, e il firmware sfuma prima e dopo per non fare un click.

## Crediti

I colpi dei tabla sono registrazioni del pacchetto Freesound "tabla bols" di **mmiron** (CC0). Il tanpura segue il
modello di van Walstijn, Bridges e Mehes (DAFx-16). Piattaforma: **fm1-base** di Charles Vestal e felucca-bsp
(MPL-2.0).
