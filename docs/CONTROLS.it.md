# Mappatura dei controlli — proposta (step 1, da discutere)

> Versione italiana. English: [CONTROLS.md](CONTROLS.md). Firmware generato con l'IA (Claude) e provato solo negli emulatori: si usa a proprio rischio (vedi il Disclaimer in [README.md](../README.md#disclaimer)).

Documento di proposta: niente di questo è implementato. Il firmware dello step 1 ha una schermata di prova
provvisoria, diversa da questa, che serve solo ad ascoltare il tono di riferimento.

Principio (CLAUDE.md §7): **si suona da una schermata sola**. Le funzioni d'uso quotidiano hanno un controllo
fisso e nessun menu; le impostazioni che si toccano di rado stanno in una pagina a parte.

## Il pannello dell'FM-1

```
 MASTER  SELECT          ┌──────────┐   KNOB1 KNOB2 KNOB3 KNOB4
                         │ schermo  │   [FX ][SCL][ENV][LFO][EDIT][GLO]
 PRESETS ALGORITHM       │ 240×240  │   [HOME][SAVE][ARP][SEQ][PLAY][REC]
 [OCT-][OCT+]            └──────────┘
 27 tasti: F3 … G5 (16 bianchi, 11 neri)
```

## Schermata principale (proposta)

| Controllo | Funzione |
|---|---|
| **PLAY** | tabla: avvio / arresto (si parte dal sam; LED verde mentre suona) |
| **SEQ** | tanpura: avvio / arresto |
| **ARP** | tono di riferimento: acceso / spento |
| **KNOB1** | tempo (matra al minuto) |
| **KNOB2** | volume tanpura |
| **KNOB3** | volume tabla |
| **KNOB4** | volume tono di riferimento |
| **MASTER** | volume generale (come oggi) |
| **SELECT** | taal: ruoti per scegliere (nome sullo schermo) |
| **PRESETS** | Sa fine: ±1 cent per scatto |
| **ALGORITHM** | jawari (posizione del filo) |
| **OCT− / OCT+** | ottava del tono di riferimento (mandra / madhya / tar) |
| **SCL + tasto** | Sa = la nota di quel tasto (i cents fini restano) |
| **tasto da solo** | tono di riferimento sullo swara di quel tasto, rispetto al Sa (vedi domanda 2) |
| **EDIT** | pagina delle impostazioni (sotto) |
| **HOME** | torna alla schermata principale |
| **SAVE** | salva le impostazioni (vedi domanda 4) |
| FX, ENV, LFO, GLO, REC | non usati |

I tasti dalla nota del Sa in su si illuminano sui swara della scala (Sa e Pa più accesi), così la tastiera
mostra dove sono gli swara rispetto al Sa scelto.

### Schermo (bozza)

```
 TEENTAAL                     SA C#3 +12
 ───────────────────────────────────────
         9 / 16                  TEENTAAL
     X . . . | 2 . . . | 0 ● . . | 3 . . .
 ───────────────────────────────────────
 120 matra/min              tanpura PA
 TANPURA ●   TABLA ●   TONO ○ PA tar
```
- in alto: Sa (nota + cents), sempre visibile
- al centro: numero della matra grande, il ciclo diviso in vibhag con **X** (sam) e **0** (khali) ben
  distinti (colori diversi), il punto acceso sulla matra corrente
- in basso: tempo, prima corda del tanpura, stato delle tre sorgenti

## Pagina EDIT (impostazioni rare)

SELECT sceglie la riga, PRESETS cambia il valore, HOME torna indietro.

- La di riferimento (415–466 Hz, a passi di 0,1 Hz)
- prima corda del tanpura (qualsiasi swara; default Pa) e accordatura fine in cents di ogni corda
- velocità del ciclo di pizzico, irregolarità umana (tanpura e tabla)
- accordatura del dayan rispetto al Sa (se serve, step 4–5)

## Risposte di ElectroNick (2026-11-06, chiusura dello step 1)

1. PLAY = tabla, SEQ = tanpura, ARP = tono: **sì**.
2. Tastiera: il tono suona **solo mentre il tasto è premuto** (come un harmonium).
3. Cambio di taal mentre i tabla suonano: **subito**.
4. Salvataggio: **con il pulsante SAVE**.
5. Tap tempo: **non serve**.

Si implementa agli step 3, 7 e 8 (UI del tanpura, dei tabla, UI finale).

## Implementato allo step 7 (2026-11-08)

| Controllo | Funzione |
|---|---|
| **PLAY** | tabla: avvio dal sam / arresto (i colpi che suonano si spengono da soli); LED acceso mentre suona |
| **SELECT** | taal (gira in tondo sui 10). Mentre suona, il nuovo taal parte **alla fine del ciclo** (ElectroNick, step 6, al posto di "subito" dello step 1): il nome del prossimo resta sullo schermo con ">" finché non parte |
| **KNOB1** | tempo, 1 matra/min per scatto; **SEL + KNOB1** 10 per scatto (20–600) |
| **KNOB3** | volume dei tabla |
| **EDIT → TABLA IRREG.** | irregolarità dei tabla (0–100 %, default 30) |

Schermo: il nome del taal e il tempo; la matra corrente grande (rossa sul sam, blu nei vibhag khali, con la scritta
SAM o KHALI); sotto, il ciclo a celle, una per matra, raggruppate per vibhag con il segno sopra (X rosso, 0 blu); la
cella della matra corrente accesa.

## Implementato allo step 8 (2026-11-08)

| Controllo | Funzione |
|---|---|
| **tasto da solo** | tono di riferimento su quello swara finché è premuto (più tasti: l'ultimo premuto; poi il tono torna com'era) |
| **SAVE** | salva le impostazioni (UNSAVED sullo schermo e luce di SAVE accesa finché ci sono modifiche) |
| **EDIT** | 11 righe, scorrono: prima corda, accordatura fine di ogni corda, registro, ciclo, irregolarità di tanpura e tabla, swara del tono, La di riferimento |

Luci dei tasti: dal Sa in su, Sa e Pa accesi, Re Ga Ma Dha Ni tenui. Manuale d'uso: `docs/MANUAL.md`.
