# CLAUDE.md — Firmware "pratica musica indiana" per M-VAVE FM-1

Istruzioni per l'agente. Leggile per intero all'inizio di ogni sessione, poi leggi `PROGRESS.md` e `DECISIONS.md` prima di fare qualsiasi altra cosa.

---

## 1. Obiettivo

Firmware custom per il synth tascabile M-VAVE FM-1 dedicato alla **pratica della musica indiana classica**: un tanpura e una coppia di tabla con motore di taal, più toni di riferimento per l'intonazione.

Il riferimento d'uso è un'app come iTablaPro, ma **solo la parte essenziale**. L'utente (ElectroNick) vuole uno strumento di pratica sobrio, non un prodotto pieno di funzioni.

### Dentro l'ambito
- Tanpura sintetizzato, con corde configurabili e accordatura in cents
- Tabla sintetizzati, con motore di taal guidato da dati
- 10 taal di base, più la possibilità di aggiungerne altri scrivendo un file di testo
- Toni di riferimento generati dal firmware (es. Sa, Pa) per l'intonazione

### Fuori dall'ambito (non proporli, non implementarli)
- Registrazione o analisi dell'audio in ingresso, rilevamento dell'intonazione
- Altri strumenti (swarmandal, sitar, harmonium, ecc.)
- Funzioni accessorie: librerie di brani, social, esportazioni, statistiche, temi grafici
- Se pensi che una funzione fuori ambito sia davvero utile, scrivila in `DECISIONS.md` sotto "Proposte rimandate" e vai avanti senza implementarla.

---

## 2. Regola principale: si procede per step

Il lavoro è diviso negli step della sezione 8. **Lavori su un solo step alla volta.**

1. All'inizio di uno step, scrivi in `PROGRESS.md` cosa intendi fare e come verificherai che è fatto.
2. Implementi solo quello che lo step richiede. **Non scrivi codice per gli step successivi**, nemmeno "per comodità" o "già che ci sei". Le interfacce previste dalla sezione 6 sono l'unica eccezione: vanno rispettate fin da subito.
3. A fine step produci i deliverable richiesti (in genere file WAV in `renders/step-N/` e un breve report).
4. **Ti fermi.** Scrivi in `PROGRESS.md` cosa hai fatto, cosa hai misurato, cosa non va ancora, e quali domande hai per ElectroNick. Poi attendi la sua approvazione esplicita prima di passare allo step successivo.
5. Le iterazioni sul suono avvengono dentro lo stesso step: ElectroNick ascolta i WAV, dà indicazioni, tu correggi e rigeneri. Lo step si chiude solo quando lui lo dichiara chiuso.

Se uno step si rivela troppo grande, proponi di dividerlo invece di procedere.

Ogni decisione tecnica non banale (algoritmo scelto, compromesso di qualità, formato dati) va annotata in `DECISIONS.md` con data, motivazione e alternative scartate.

---

## 3. Sicurezza dell'hardware

L'FM-1 ha **un solo banco di flash e nessun header di debug**. Un aggiornamento fallito può rendere il dispositivo non avviabile; il recupero richiede un dongle hardware che ElectroNick al momento non ha.

Regole vincolanti:
- **Non flashare mai il dispositivo.** Il tuo compito termina con la produzione del pacchetto `.fwsc`. L'installazione la fa ElectroNick, manualmente.
- Ogni firmware va prima provato nell'emulatore (sezione 5). Non consegnare un `.fwsc` che non hai fatto girare nell'emulatore.
- Usa solo aggiornamenti dell'applicazione (app-only) che preservano il bootloader originale. Non modificare mai bootloader, SPL o loader OTA.
- Non scrivere nelle regioni di flash che non appartengono all'app o allo storage dati dell'app.
- Prima del primo `.fwsc` destinato all'hardware, ricorda a ElectroNick di avere a portata il firmware ufficiale V15 per tornare indietro.

---

## 4. Hardware (verificato allo step 0, 2026-11-06 — dettagli e metodo in `PROGRESS.md`)

| Voce | Valore | Note |
|---|---|---|
| SoC | JieLi AC791N / WL82 | |
| CPU | pi32v2, **360 MHz** (320 sotto m-bootloader; fm1-base riporta a 360), **2 core** | Secondo core via fm1-base `cpu2`. 8160 cicli per campione per core |
| FPU | **Presente, singola precisione** (`-mfprev1`). Niente double | Il firmware stock usa soft-float per scelta del compilatore. Verificata nell'emulatore: risultati identici bit per bit all'host, anche nell'interrupt audio |
| Audio | **44117,647 Hz** (24 MHz/544), uscita **stereo Q23 in int32** via I2S | Blocco configurabile (`APP_BLOCK`), default fm1-base 256 |
| Flash | 1 MB totali; slot app **581.564 B**, piattaforma ~79 KB → **~490 KB per l'app** | Area dati **292 KB** + 16 KB + 92 KB (`plat_data*`) |
| RAM | `.data+.bss` 96 KB (~70 KB liberi); pool 336 KB (~267 KB liberi); codice in RAM 32 KB (~27 liberi) | Misurato sulla build del probe senza BLE |
| Display | LCD SPI 240×240, RGB565 | |
| Ingressi | 27 tasti (F3–G5), 14 pulsanti, **7 encoder** (SELECT, ALGORITHM, PRESETS, KNOB1–4), potenziometro MASTER | Verificato allo step 1 su `plat.h`; LED su pulsanti e tasti (`plat_leds`) |
| Uscite | Speaker interno, uscita stereo TRS (motore originale mono) | |

Fonti:
- https://github.com/cicloid/awesome-fm-1 (indice di tutto l'ecosistema)
- https://github.com/AL-255/FM-1-RE (architettura, mappa memoria, protocollo OTA)

---

## 5. Base, toolchain e ambiente

### Base
Parti da **fm1-base** di Charles Vestal (template costruito su felucca-bsp: platform layer, simulatore host, emulatore browser, strumenti di installazione). Trovi il link in awesome-fm-1.

Allo step 0 verifica la licenza e lo stato del progetto. Se fm1-base risulta inadatto (abbandonato, incompleto, licenza problematica), **fermati e proponi un'alternativa** a ElectroNick (felucca-bsp da solo, oppure Felucca), con pro e contro. Non cambiare base di tua iniziativa.

Guarda anche il repository **mvave-fm1** (skill per Claude Code e container di build per l'FM-1) e la suite di test di **Felucca** (render di regressione con hash golden e controllo del costo CPU): sono modelli da seguire.

### Toolchain
- ElectroNick lavora su **Ubuntu in WSL**. Il toolchain JieLi (clang 4.0.1 per pi32v2) gira nativamente su Linux x86-64: niente Docker necessario.
- Serve anche l'SDK JieLi AC79 (Apache-2.0); segui le istruzioni della base scelta per la versione esatta.
- L'installazione su hardware avviene da **Chrome o Edge su Windows** tramite installer web (Web MIDI), perché da WSL l'accesso USB-MIDI non è diretto. Documenta la procedura in `INSTALL.md`.

### Due bersagli di build
1. **Host** (Linux, gcc/clang): lo stesso codice DSP compilato per PC, con un tool a riga di comando `render` che scrive file WAV. È il banco di prova principale per il suono.
2. **Target** (pi32v2): firmware `.fwsc`, provato prima nell'emulatore.

Il codice DSP deve essere **identico** sui due bersagli. Niente `#ifdef` nel DSP per distinguere host e target, tranne nel platform layer.

---

## 6. Architettura

### Moduli
```
platform/   hardware: audio out, tasti, encoder, display, storage (dalla base)
dsp/        primitive DSP condivise in virgola fissa (filtri, delay, oscillatori, rumore)
tanpura/    motore del tanpura
tabla/      sintesi degli stroke di dayan e bayan
taal/       motore dei taal: sequenziamento, laya, posizione nel ciclo
reftone/    toni di riferimento
mixer/      somma dei moduli, volumi, limiter di uscita
ui/         schermate e mappatura dei controlli
data/       taal in JSON e script di generazione delle tabelle C
tools/      render (host), test, script di supporto
renders/    WAV prodotti a ogni step
```

### Interfaccia comune dei moduli audio
Ogni modulo sonoro espone la stessa interfaccia (adatta i nomi alle convenzioni della base):

```c
void mod_init(mod_t *m, const mod_config_t *cfg);
void mod_set_param(mod_t *m, int param_id, int32_t value);
void mod_render(mod_t *m, int32_t *out, int n_samples);  /* somma in out: mono, scala Q23 in int32; n = 64 */
```

- Il motore dei taal non produce audio: emette eventi (stroke, dinamica, istante) che il modulo `tabla` esegue.
- I moduli non conoscono la UI. La UI chiama solo `set_param`.

### Vincoli del percorso audio
- Frequenza di campionamento 44117,647 Hz (verificato allo step 0). Blocco di piattaforma **256** (`APP_BLOCK`); i moduli lavorano a sotto-blocchi di **64**, chiamati 4 volte per blocco.
- **float32** nel DSP di tanpura e tabla (FPU a singola precisione verificata allo step 0), compilato con `-ffp-contract=off` su host e target. **Mai double.** Virgola fissa dove conviene (oscillatori a tabella, mixer). Il mixer satura e copia sui due canali stereo Q23.
- **Due core** (deciso da ElectroNick allo step 2b): il tanpura gira sul secondo core (stima 54–79 % di quel core, da misurare allo step 3); tabla, taal, tono, mixer e UI sul primo.
- **Nessuna allocazione dinamica** nel percorso audio. Tutta la memoria è allocata all'init.
- Nessuna divisione o funzione trascendente per campione: usa tabelle o approssimazioni.

### Budget (aggiornato allo step 0; percentuali di un core a 360 MHz = 8160 cicli/campione)
| Modulo | CPU max | Cicli/campione | Flash (codice+tabelle) | RAM |
|---|---|---|---|---|
| Tanpura | **secondo core**: max 85% del core 1 (deciso allo step 2b) | ~6940 | 64 KB | 64 KB |
| Tabla | 30% | ~2450 | 64 KB + **campioni registrati** nell'area dati, ~160 KB (deciso da ElectroNick allo step 4: vedi `DECISIONS.md`) | 48 KB |
| Taal, reftone, mixer | 5% | ~410 | 32 KB | 8 KB |
| UI e sistema | resto, con almeno il 20% di margine | | UI 100 KB; piattaforma ~79 KB | |
| Margine | | | ≥ 150 KB dello slot app | ≥ 25% di `.data+.bss` e pool |

Le percentuali CPU sono stime finché non c'è una misura `plat_cpu_pct()` sul dispositivo (l'emulatore conta istruzioni, non cicli). Il secondo core è dedicato al tanpura (ElectroNick, step 2b): le altre percentuali restano riferite al primo core. Budget di flash e RAM proposti allo step 0, da confermare.

**Ogni step che tocca il DSP deve misurare il costo CPU** e confrontarlo con il budget. I test di regressione controllano sia l'hash dei render sia il costo, sul modello di Felucca. Se un modulo supera il budget, è un problema da segnalare, non da ignorare.

---

## 7. Specifiche musicali

### Tanpura
- **4 corde**, ognuna configurabile:
  - nota della prima corda: Pa, Ma, Ni (e varianti, es. Ni komal), con la possibilità di scegliere qualsiasi swara
  - due corde centrali a Sa medio
  - quarta corda a Sa grave (kharaj)
- **Sa intonabile in cents** su tutta l'estensione utile, rispetto a un riferimento esplicito (es. La = 440 Hz). Ogni corda ha in più un'accordatura fine in cents.
- **Jawari**: è la caratteristica che rende il suono convincente (il ronzio ricco e l'evoluzione spettrale lenta dovuta al contatto della corda con il ponte). Va modellato esplicitamente, non approssimato con un riverbero.
- **Riferimento scientifico**: M. van Walstijn, J. Bridges, S. Mehes, *A Real-Time Synthesis Oriented Tanpura Model*, DAFx-16 (PDF in `docs/references/vanwalstijn-dafx16-tanpura.pdf`). Modello modale della corda con contatto col ponte risolto in forma chiusa e filo di cotone come molla-smorzatore. Elementi da preservare in qualsiasi semplificazione:
  - rigidità della corda (EI): senza, il precursore ad alta frequenza che costituisce il jawari scompare
  - posizione del filo di cotone: controlla la discesa del centroide spettrale; è il parametro naturale per il "jawari" in UI
  - risposta passa-alto del corpo: dà la sensazione che il suono cresca nei primi ~500 ms; va realizzata con filtri IIR, non con i FIR del paper
  - smorzamento dipendente dalla frequenza (σ0, σ1, σ3)
- Limiti noti del modello per il nostro target: costo proporzionale al numero di modi (fino a Nyquist nel paper), stabilità dimostrata solo in aritmetica esatta, eccesso di alte frequenze a 44.1 kHz senza sovracampionamento. Ogni riduzione (meno modi, virgola fissa, scalature) va documentata in `DECISIONS.md` con il suo effetto misurato rispetto al riferimento.
- Parametri: quantità di jawari (posizione del filo), velocità del ciclo di pizzico, leggera irregolarità umana dei tempi e delle dinamiche, volume per corda.
- Ciclo di pizzico classico: prima corda, Sa, Sa, Sa grave, con pausa prima di ricominciare.

### Tabla
- **Sintesi modale** come approccio principale: dayan con parziali quasi armonici (effetto dello syahi), bayan con il caratteristico glissando di pitch sul Ge.
- Il dayan si accorda sul Sa del tanpura (in cents, come sopra).
- **Libreria di stroke** separati per dayan e bayan (es. Na/Ta, Tin, Tun, Te, Re, Ke/Ka, Ge/Ghe…). I **bol** composti sono combinazioni di stroke (Dha = Ge + Na, Dhin = Ge + Tin…), definite nei dati, non nel codice.
- Dinamica per stroke e leggera umanizzazione.
- I campioni sono un'opzione solo se la sintesi non convince all'ascolto, e solo come transienti brevissimi compressi (es. ADPCM), entro il budget di flash. La decisione è di ElectroNick, allo step 4.

### Motore dei taal
- Taal definiti in **file JSON** in `data/taal/`. Uno script Python li valida e li converte in tabelle C alla build. **Nessun parser JSON sul dispositivo.**
- Formato di partenza (estendibile; documentalo in `data/taal/FORMAT.md`):

```json
{
  "name": "Teentaal",
  "matras": 16,
  "vibhag": [4, 4, 4, 4],
  "marks": ["X", "2", "0", "3"],
  "theka": ["Dha","Dhin","Dhin","Dha", "Dha","Dhin","Dhin","Dha",
            "Dha","Tin","Tin","Ta", "Ta","Dhin","Dhin","Dha"]
}
```
  - più bol in una matra: `"Ti-Ra-Ki-Ta"`
  - i bol usati devono esistere nella tabella bol→stroke, altrimenti lo script fallisce con un errore chiaro
- **10 taal iniziali**: Teentaal, Ektaal, Jhaptaal, Rupak, Keherwa, Dadra, Jhoomra, Deepchandi, Dhamar, Chautaal. Le theka hanno varianti tra scuole: usa fonti affidabili, citale in un commento nel file, e segnala a ElectroNick le varianti dubbie invece di sceglierne una in silenzio.
- Tempo regolabile in matra al minuto, da vilambit lento a drut veloce. Il cambio di tempo non deve produrre salti o click.
- Sul display: nome del taal, tempo, numero della matra corrente, indicazione chiara di sam (X) e khali (0).

### Toni di riferimento
- Tono stabile e pulito su uno swara scelto (Sa di default), accordato come il tanpura.
- Pochi parametri: swara, ottava, volume. Niente di più.

### UI
- Semplice e leggibile, pensata per suonare, non per configurare.
- Idea di partenza da validare con ElectroNick allo step 1: tastiera per scegliere il Sa (o lo swara del tono di riferimento), un encoder per il tempo, uno per il volume, pulsanti per avviare/fermare tanpura e tabla e per cambiare taal.

---

## 8. Roadmap

Ogni step termina con uno **STOP** e l'approvazione di ElectroNick.

### Step 0 — Ricognizione
- Clona la base, compila host e target, fai girare l'emulatore.
- Verifica e misura: core utilizzabili, presenza di FPU, RAM e flash liberi, frequenza di campionamento e dimensione del blocco reali, costo CPU di un oscillatore di prova.
- Verifica la licenza della base.
- Aggiorna la sezione 4 e i budget della sezione 6 con i valori misurati.
- **Deliverable**: report in `PROGRESS.md`. Nessuna funzione musicale.

### Step 1 — Scheletro
- Struttura dei moduli della sezione 6 con moduli vuoti, mixer, tool `render` su host, test di regressione (hash dei render e costo CPU).
- Unico suono: il tono di riferimento (seno sul Sa, accordabile in cents).
- Proposta di mappatura dei controlli (solo documento, da discutere).
- **Deliverable**: WAV del tono di riferimento a diverse accordature; `.fwsc` provato nell'emulatore.

### Step 2a — Tanpura, modello di riferimento (solo host)
- Implementa il modello del paper DAFx-16 (sezione 7) **completo, in floating point**, su host: tutti i modi fino a Nyquist, filtri del corpo, pattern a 4 corde. Non è codice per il firmware: è il riferimento d'oro con cui confrontare le versioni ridotte. Mettilo in `tools/reference/`, separato da `dsp/` e `tanpura/`.
- Verifica il modello riproducendo i risultati del paper (corda in Do3 con i parametri della Tabella 1; pattern a 4 corde) e confrontandoli con gli esempi sonori sul sito degli autori.
- Rendi anche una versione sovracampionata 2× come riferimento di qualità massima.
- **Deliverable**: WAV di riferimento (Do3 singola, pattern a 4 corde, diverse posizioni del filo), spettrogrammi, report su eventuali scostamenti dal paper. STOP.

### Step 2b — Tanpura, versione per il target (solo host)
- Riduci il modello fino a farlo rientrare nel budget: numero di modi per corda, virgola fissa, corpo IIR, eventuali altre semplificazioni. Il codice va in `tanpura/` e deve compilare identico per host e target.
- Per ogni riduzione misura costo CPU stimato e differenza rispetto al riferimento (spettrogrammi affiancati, WAV A/B alla stessa accordatura).
- Prova la robustezza in virgola fissa su tutta l'estensione di Sa e su tutte le posizioni del filo: nessuna instabilità, nessun overflow, nessun click.
- Iterazione all'ascolto con ElectroNick finché non è soddisfatto.
- **Deliverable**: WAV di almeno 30 s per diverse configurazioni (Pa, Ma, Ni; Sa diversi; jawari leggero e marcato), confronti A/B con il riferimento, tabella costo/qualità delle varianti provate.

### Step 3 — Tanpura su target
- Integrazione sul firmware, UI minima del tanpura, misura del costo reale.
- **Deliverable**: firmware "solo tanpura" provato nell'emulatore e pronto per essere installato da ElectroNick.

### Step 4 — Tabla, valutazione dell'approccio
- Prototipi su host di 3–4 stroke rappresentativi (Na, Tin, Ge, Ke) in sintesi modale; se utile, confronto con una variante ibrida.
- **Deliverable**: WAV di confronto e stima dei costi (CPU, flash). La scelta la fa ElectroNick.

### Step 5 — Tabla, libreria completa di stroke (solo host)
- Tutti gli stroke e la tabella bol→stroke.
- **Deliverable**: WAV di ogni stroke e di ogni bol, a diverse dinamiche e accordature.

### Step 6 — Motore dei taal (solo host)
- Formato JSON, script di validazione e generazione, 10 taal, gestione del tempo.
- **Deliverable**: WAV di ogni taal per alcuni cicli a tempo medio, più uno a tempo lento e uno veloce.

### Step 7 — Tabla e taal su target
- Integrazione sul firmware, UI dei tabla.
- **Deliverable**: firmware "solo tabla" provato nell'emulatore.

### Step 8 — Integrazione
- Tanpura, tabla e tono di riferimento insieme, mixer, salvataggio delle impostazioni, UI finale.
- Verifica che il budget complessivo sia rispettato con tutto attivo.
- **Deliverable**: firmware completo provato nell'emulatore, `INSTALL.md` e un breve manuale d'uso.

---

## 9. Convenzioni

- C99, senza dipendenze esterne nel codice del firmware oltre a quelle della base.
- Commenti e documentazione in italiano o inglese, ma coerenti dentro lo stesso file.
- Commit piccoli, con messaggi che indicano lo step (es. `step2: jawari con non-linearità del ponte`).
- File di stato da mantenere aggiornati:
  - `PROGRESS.md`: step corrente, cosa è fatto, misure, domande aperte
  - `DECISIONS.md`: decisioni tecniche con motivazione e proposte rimandate
- Quando non sei sicuro di qualcosa che riguarda il suono o la musica, chiedi. ElectroNick preferisce una domanda in più a una scelta presa in silenzio.
