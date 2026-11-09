# DECISIONS

Formato: data — decisione — motivazione — alternative scartate.

## Decisioni

### 2026-11-06 — Base: fm1-base (proposta, da confermare con ElectroNick)
- **Decisione**: usare fm1-base (Charles Vestal, MPL-2.0) su felucca-bsp, fissata a un commit come submodule.
- **Motivazione**: licenza permissiva file per file, progetto attivo, supporto delle due flash (Puya e UC25HQ80),
  aggiornamento app-only con loader OTA, simulatore host con scenari di test, controlli automatici della build
  (soft-double, registri, limiti di memoria). Host e target compilati e funzionanti allo step 0.
- **Rischio**: è in beta, `plat.h` può cambiare → fissare il commit e aggiornare solo di proposito.
- **Scartate**: felucca-bsp da solo (dovremmo riscrivere display, pannello, storage, update); Felucca (GPL-3.0,
  firmware completo da svuotare); Dinghy/foma (più piccoli, pensati per Docker o un solo file).

### 2026-11-06 — Toolchain e SDK
- JieLi Linux toolchain `20260730.1` (clang 4.0.1), SDK AC79 `AC79NN_SDK_V1.2.1_2023-12-13` (la versione provata
  da fm1-base; hash dei tre file verificati). Nessun Docker: su WSL x86-64 il toolchain gira nativamente.

### 2026-11-06 — Metodo di misura della CPU
- **Host**: istruzioni per campione con `perf_event_open` (funziona sotto WSL2), tolleranza +25 %, come Felucca.
- **Target**: `plat_cpu_pct()` sul dispositivo. L'emulatore conta istruzioni (8 ns ciascuna per convenzione), non
  cicli: utile per il conteggio delle istruzioni, non per il costo reale.

### 2026-11-06 — float32 nel DSP (approvato da ElectroNick)
- **Decisione**: float32 per tanpura e tabla; virgola fissa solo dove conviene (oscillatori a tabella, mixer).
  `-ffp-contract=off` su host e target, niente double.
- **Motivazione**: FPU a singola precisione presente. Il modello DAFx-16 (contatto in forma chiusa) è molto più
  robusto in float; la riduzione dello step 2b si concentra sul numero di modi, non sulla stabilità numerica.
- **Scartata**: virgola fissa Q15/Q31 ovunque (più lavoro e più rischio di overflow e instabilità, senza vantaggi
  di velocità con la FPU).

### 2026-11-06 — Interfaccia dei moduli in int32 Q23 (approvato da ElectroNick)
- `mod_render(m, int32_t *out, n)`: somma in un buffer mono a 32 bit con scala Q23 (quella dell'uscita della
  piattaforma). Il mixer satura e copia su L/R.
- **Scartata**: `int16_t` (la somma di più moduli satura).

### 2026-11-06 — Blocco 256 di piattaforma, sotto-blocchi di 64 (approvato da ElectroNick)
- `APP_BLOCK 256` (default di fm1-base, meno overhead per blocco, latenza 5,8 ms ininfluente per la pratica);
  i moduli vengono chiamati 4 volte per blocco con n = 64.

### 2026-11-06 — Step 2b: il tanpura sul secondo core, variante Q (deciso da ElectroNick)
- **Decisione**: variante Q (troncamento 6/8/8/3 kHz, corde a metà frequenza) sul **secondo core**; il primo core resta
  per tabla, taal, tono, mixer e UI con i budget di §6. Budget del tanpura: 85 % del core 1 (stima 54–70 % a Sa A2,
  61–79 % a Sa F#2, da misurare allo step 3).
- **Motivazione**: Q è vicino al riferimento d'oro; E (che entrava nel 35 % di un core) ha un fruscio tra 4 e 10 kHz.
- **Da fare allo step 3**: far girare il render del tanpura come job del secondo core (`plat_cpu2_run` / `wait` di
  fm1-base, un job per blocco), misurare il costo reale, verificare che il primo core non aspetti.
- Sostituisce la decisione precedente "secondo core in riserva".

### 2026-11-06 — Secondo core in riserva (lasciato a me da ElectroNick) — **superata** dalla decisione dello step 2b
- **Decisione**: si parte con un solo core. Il secondo si attiva solo se allo step 2b/3 il tanpura non entra nel
  budget con un numero di modi accettabile all'ascolto.
- **Motivazione**: il secondo core si avvia solo in `app_init`, ha un fallback a un core e sincronizzazione per
  blocco. Introduce complessità e casi di errore (avvio fallito, job in ritardo) che conviene affrontare solo se
  servono. L'interfaccia dei moduli (render per blocco su buffer propri) permette comunque di spostare in seguito
  il tanpura sul secondo core senza riscriverlo.
- **Scartata**: dual core da subito.

### 2026-11-06 — fm1-base recente e audio muto in fm1-emu (da decidere con ElectroNick)
- felucca-bsp `e7a22e3`/`4d39207` (2026-11-05) rifiuta in silenzio l'avvio dell'audio se il PLL audio non ha i valori
  stock; fm1-emu (joakimeriksson) non li imposta → i firmware su fm1-base recente sono muti in quell'emulatore.
- **Proposta**: restare su fm1-base recente (dopo `3c4a57e` ci sono 91 commit, tra cui il "flash gate" che limita le
  scritture in flash alle regioni ammesse, utile per la sicurezza §3) e risolvere dal lato emulatore (segnalazione a
  fm1-emu; nel frattempo fm1-emulator di simonjohansson, che precarica i valori).
- **Scartata**: patch locale a `main_fm1.c` per ignorare il controllo (toglierebbe una protezione sul dispositivo
  reale e il firmware provato non sarebbe quello consegnato).
- **Alternativa di riserva**: fissare fm1-base a `3c4a57e` (suona in fm1-emu, supporta già entrambe le flash, ma
  perde il flash gate e le correzioni dell'installer).

### 2026-11-06 — Repository
- Niente operazioni git per ora (ElectroNick). `refs/` contiene cloni di sola consultazione.

### 2026-11-06 — Modifica locale B a fm1-base (approvata da ElectroNick)
- **Decisione**: in `platform/fm1-base/bsp/boards/fm1/fm1_audio.h`, `fm1_audio_init_checked` accetta anche un PLL
  audio mai impostato (0x119A0 = 0 e 0x119A4 & 0x07FFFFFF = 0), oltre ai valori stock. Ogni altro valore non stock è
  rifiutato come a monte. Elencata in `platform/FM1-BASE-VERSION.txt`.
- **Motivazione**: ElectroNick prova i firmware solo in fm1-emu (joakimeriksson), che non esegue il bootloader e lascia il
  PLL a 0; con il controllo originale l'audio non partiva mai. Sull'FM-1 il bootloader imposta sempre il PLL, quindi
  il comportamento non cambia; un link audio senza clock sarebbe comunque solo muto.
- **Verifica**: la build di prova ha suonato in fm1-emu con controlli funzionanti; la build del progetto è identica
  byte per byte (sha256 `a6a91137…839f5`).
- **Scartate**: fissare fm1-base a `3c4a57e` (perde 91 commit, tra cui la protezione delle scritture in flash);
  segnalare il problema a fm1-emu o a fm1-base (ElectroNick non vuole issue pubbliche); un altro emulatore (ElectroNick
  preferisce fm1-emu).
- Licenza: la modifica a un file MPL-2.0 va pubblicata sotto MPL-2.0 se il firmware viene distribuito.

### 2026-11-06 — Step 1: struttura del progetto
- **La radice del progetto è la APP_DIR di fm1-base** (`fm1.toml`, `app.h`, `app.c`, `words.tsv`). Motivo: fm1-base
  compila a parte (-O2) solo file dentro la APP_DIR, e il DSP deve stare in `dsp/`, `reftone/`, ecc. come da §6.
- fm1-base è una **copia fissata** in `platform/fm1-base/` (`317cb86`, bsp `5b0879b`, vedi
  `platform/FM1-BASE-VERSION.txt`), non un submodule: niente git per ora. Non va modificata.
- `vendored = ["platform/", "refs/", "tools/step0/"]` in `fm1.toml`: il controllo di fm1-base sugli accessi ai
  registri salta le cartelle non nostre (altrimenti scambia la copia della base per codice dell'app); il nostro
  codice resta controllato.
- `mixer/engine.c` è **l'unico percorso audio**: lo usano il firmware (`app.c`) e i tool host (`render`,
  `regress`), quindi un render su host è ciò che suona l'FM-1.
- Parametri di accordatura condivisi (`DSP_P_A4_DHZ`, `DSP_P_SA_CENTS`) inviati a tutti i moduli; parametri dei
  moduli da `DSP_P_MODULE` (16) in su.

### 2026-11-06 — Step 1: tono di riferimento
- **Seno da tabella float di 1024 punti con interpolazione lineare**, fase a 32 bit. Armoniche misurate sotto
  −135 dB (test), costo minimo. Scartati: risonatore a 2 poli (deriva d'ampiezza in float su tempi lunghi),
  `fm_sinf` per campione (più caro, nessun guadagno udibile).
- **Accordatura**: f = La · 2^((Sa − 900 + cents)/1200), Sa in cents sopra C4. Calcolata solo al cambio di
  parametro con `fm_exp2f` di fm1-base; errore misurato < 0,001 cents.
- **Intonazione degli swara: temperamento equabile** (100 cents a semitono), scelta di ElectroNick alla chiusura dello
  step 1. Scartata: intonazione giusta a 5 limiti (usata nei primi WAV dello step 1).
- **Timbro: seno puro** per ora (ElectroNick). Un'eventuale variante con armoniche leggere resta possibile più avanti.
- **Sa di default A3 (220 Hz)**: ElectroNick suona bansuri in La e in Sol. Convenzione confermata da ElectroNick (il suo flauto: Sa = A4, 440 Hz): il bansuri
  in La ha Sa = A4 (440 Hz); il Sa medio di tanpura e tono sta un'ottava sotto (A3), e l'ottava tar del tono dà il
  Sa del flauto. Estensione del Sa medio F#2–G4: copre i bansuri dal basso (Sa del flauto ~E4, Sa medio E3) ai
  medi e piccoli; il tono arriva un'ottava sopra (tar).
- La di riferimento 415–466 Hz a passi di 0,1 Hz: confermato.
- Livello: volume 100 % = −6 dBFS prima del mixer, curva quadratica del volume.

### 2026-11-06 — Step 1: niente click
- Ogni guadagno (volume e on/off del tono, livelli del mixer, MASTER) si muove al massimo di un'escursione piena
  in **20 ms**, a rampe lineari per blocco (`dsp/ramp.h`). Trovato dal test: con la rampa su un solo sotto-blocco
  (1,45 ms) un salto di MASTER da 500 a 4096 produceva un click (passo 96961 contro un limite di 83617).
- La fase non salta mai al cambio di nota.
- Il mixer satura a ±8388607 e conta i campioni saturati (i test richiedono zero).

### 2026-11-06 — Step 1: test e misura del costo
- `tools/test/run_tests.sh`: 9 scenari; hash FNV-1a 64 (`golden.txt`), istruzioni host per campione
  (`cpu_baseline.txt`, +25 %), costo stimato sull'FM-1 = istruzioni host × 1,2 / 8160 cicli, confrontato col
  budget; frequenza confrontata con un riferimento in double calcolato dai rapporti (non dalla tabella del
  firmware), armoniche, click (passo massimo tra campioni ≤ pendenza massima del seno), silenzio, saturazione.
- `tools/test/wavcheck.py`: verifica indipendente delle frequenze nei WAV.
- WAV a 24 bit con header a 44118 Hz (un intero è obbligatorio): 0,14 cents di differenza all'ascolto su PC.

### 2026-11-06 — Step 2a: modello di riferimento del tanpura
- **C99 in double** (`tools/reference/tanpura_ref.c`), non Python: 10 s di una corda in 0,4 s, quindi anche le
  versioni sovracampionate 8× restano rapide. Analisi in Python (numpy/scipy/matplotlib) nel venv
  `~/.venvs/teentaal` (solo host: nessuna dipendenza nel firmware).
- **Forza al ponte per casi in forma chiusa** invece del rapporto di differenze di energia dell'eq. 30: stesso
  valore, ma senza cancellazione numerica quando il passo s_b è minuscolo. Le quattro formule di s_b (Fig. 4) sono
  state ricavate di nuovo e coincidono con il diagramma.
- **Corpo**: le risposte all'impulso **misurate** degli autori (FIR 2048, come nel paper), recuperate dalla loro
  pagina su web.archive.org. L'IIR richiesto da CLAUDE.md §7 è compito dello step 2b, che avrà questo come bersaglio.
- **Parametri non dati dal paper** (assunzioni, da rivedere all'ascolto): stessi σ, ponte e filo per tutte le corde;
  corde G2 in acciaio r 0,14 mm (T 18,6 N); C2 in bronzo r 0,2 mm (ρ 8800, E 1,1·10¹¹); per il Sa A3 di ElectroNick un
  tanpura femminile L = 0,85 m, Pa E3 e Sa A3 in acciaio r 0,13 mm (T 33 e 58 N), kharaj A2 in bronzo r 0,18 mm
  (T 31 N). Ciclo di 4 s.
- **r_c = 1,2 kg/s** dalla Tabella 1. Il testo dice k_c·10⁻⁶ (= 0,12): il confronto con ex1 degli autori (1,2 dB)
  conferma il valore della tabella.
- "Senza smorzamento" = σ₀ = σ₁ = σ₃ = 0 **e** r_c = 0 (così combacia con ex3: 0,8 dB; con r_c mantenuto: 13 dB).
- La forza al capotasto è valutata a n+½ come nel paper (media di due campioni): a 44,1 kHz attenua di ~10 dB la
  banda 15–20 kHz del segnale del capotasto (60–80 dB sotto il picco, inudibile); sparisce sovracampionando.
- Il "senza filo" degli autori (ex4) non si riproduce con nessuna lettura plausibile (scarto minimo ~5 dB): non è
  documentato nel paper e non riguarda il nostro strumento, che ha sempre il filo.

### 2026-11-06 — Step 2a: scelte dopo il primo ascolto di ElectroNick
- **Filo a 6 mm** come default (ElectroNick).
- **Riferimento di qualità per lo step 2b: la versione sovracampionata 2×** (ElectroNick la sente più ricca).
- **Dopo il secondo ascolto (deciso)**: pizzico a dito (2 cm al 35 % della corda) invece di quello del paper su tutta
  la corda (che eccita solo il primo modo: effetto "Minimoog"); smorzamento σ₀ 0,4 e r_c 0,8 (Tabella 1: 0,6 e 1,2;
  0,3/0,6 era "troppo forte troppo a lungo"); irregolarità umana ±25 ms e ±15 %, seconda corda Sa +1 cent;
  **tanpura grande con Sa A2** (corde 1,1 m: Pa E2 e Sa in acciaio r 0,16 mm, kharaj A1 in bronzo r 0,25 mm).
  Il carattere del jvari del modello del paper è accettato da ElectroNick: nessuna estensione (contatto distribuito,
  due polarizzazioni).
- Scartato: aumentare la forza del pizzico per allungare il suono (il modello è invariante di scala: non cambia nulla).

### 2026-11-06 — Step 2b: il tanpura per il firmware (`tanpura/`)
- **Stesso schema del riferimento in float32**, con formule stabili per i coefficienti (1−cos, 1−e⁻ˣ senza
  cancellazione: in float l'errore sarebbe arrivato a mezzo cent sui modi bassi) e tensione risolta per avere il
  primo modo esattamente intonato. Forza al ponte F_b = (s_b + u)/θ (dalla eq. 48, valida in tutti i casi): nessuna
  divisione per campione; radice con `fm_sqrtf`; pizzico da tabella (512 punti).
- **Corde a metà frequenza (22058,8 Hz)** e corpo alla stessa frequenza, poi interpolatore half-band a 39 tap
  (immagini −45 dB). Motivo: sopra 11 kHz il riferimento ha −44 dB della sua energia; a parità di modi la qualità
  sotto 10 kHz è identica (2,26 contro 2,24 dB) a metà costo. Scartato: 44,1 kHz con meno modi (stesso costo,
  qualità peggiore).
- **Troncamento dei modi per corda** (prima corda 6 kHz, Sa 8 kHz, kharaj 3 kHz): la kharaj non ha bisogno di modi
  alti (2,0–2,4 dB contro 2,26 con tutte a 8 kHz). Troppi pochi modi non rende il suono solo più scuro: lo rende
  aspro (il contatto diventa impulsivo).
- **Passa-basso a 8,5 kHz** (Butterworth 4° ordine, alla metà frequenza) prima dell'interpolatore: il contatto
  impulsivo lasciava energia vicino a 11 kHz (+19 dB nella banda dei 10 kHz rispetto al riferimento); ora −6 dB.
  Scartato: 2° ordine a 9 kHz (+5 dB residui).
- **Ciclo interno unico per modo**, con y_b e y_c aggiornati dalla soluzione del contatto (ricalcolati esattamente a
  ogni chiamata), e **stato e coefficienti di un modo in un record** (con array separati il compilatore pi32v2
  ricostruiva le basi a ogni modo): da 39 a **31 istruzioni** per modo e passo (il minimo per questo algoritmo:
  10 load, 3 store, 16 operazioni float). Uscita identica bit per bit.
- **Corpo IIR**: 24 sezioni del secondo ordine in parallelo con poli condivisi tra ponte e capotasto (un'unica
  ricorsione per l'uscita mono), adattate (metodo di Bank, rif. [24] del paper) a un bersaglio a fase minima
  ricavato dallo spettro di potenza misurato, smussato a 1/6 d'ottava: errore 1,2 dB (ponte) e 1,5 dB (capotasto)
  per terzo d'ottava. Scartati: fit nel tempo (4–10 dB), fit complesso diretto (perdeva 5–10 dB sopra 3 kHz).
- **Lo strumento segue il Sa**: corde da 1,1 m (Sa A2 e sotto) a 0,85 m (A3) fino a 0,7 m (G4), diametri
  interpolati. Con corde da 1,1 m a ogni altezza, a G4 la tensione sarebbe stata ~470 N e i picchi 22–25 dB sopra
  l'RMS (saturazione).
- **Volume indipendente da Sa e jawari**: tabella di compensazione 6×5 misurata (RMS), interpolata; la cella G4 /
  jawari 0 limitata a +3 dB (picchi).
- **Riaccordatura**: entro un semitono la corda continua a suonare; oltre, dissolvenza di 20 ms, corde azzerate,
  ciclo riavviato (altrimenti l'energia della corda cambiava fino a 64 volte e l'uscita saturava).
- **Memoria**: 96 modi per corda al massimo; RAM statica del firmware 64,3 KB di 96.
- **Correzione della brillantezza** (dopo l'ascolto A/B di ElectroNick): filtro a campana 5,5 kHz, +6 dB, Q 1,1, alla metà
  frequenza, attivo per default (`tanpura_config_t.eq`). La metà frequenza e il troncamento lasciavano 4–6,4 kHz
  3,6–5,9 dB sotto il riferimento; ora entro ~1,5 dB. Scartati: Sa a frequenza piena e le altre corde a metà (non
  risolve), tutto a frequenza piena (~160 % di un core).
  **Spenta per default** (ElectroNick, 2026-11-06: non sente differenza tra con e senza); il codice resta, disattivato.
- **Pulizia delle alte rimandata** (ElectroNick: "lasciamolo così e miglioriamolo in futuro"): vedi "Proposte rimandate".

### 2026-11-07 — Step 3: emulatori in WSL (opzioni A + B, approvate da ElectroNick)
- **Contesto**: in fm1-emu nel browser il firmware a due core era muto (256 blocchi audio al secondo invece di 172),
  quello a un core suonava; ElectroNick ha poi sentito qualcosa anche dal due core: il problema è (almeno in parte)
  dell'emulatore nel browser.
- **A** (`tools/web.sh`): il codice del nostro firmware compilato in WebAssembly (l'emulatore browser di fm1-base, lo
  stesso tipo di fmel-1), servito da WSL su 127.0.0.1:8790 per Chrome. Per ascoltare e provare i controlli. Non è il
  `.fwsc`. Emscripten 6.0.10 (la versione indicata da fm1-base) in `~/emsdk`.
- **B** (`tools/emu.sh`): il `.fwsc` vero nel runner senza finestra di fm1-emu (joakimeriksson, commit `1c6e80a`,
  compilato in `refs/fm1-emu`), entrambi i core, audio in WAV, schermate, e la modalità `FM1_STRICT` (cicli misurati).
  È l'emulatore che conta per la regola §3. Rust in `~/.cargo` (rustup ufficiale, profilo minimo, nessuna modifica
  ai file di avvio della shell). Il runner non usa nessun crate esterno; `fm1-live` (finestra e audio) richiederebbe
  librerie di sistema (sudo): non compilato.
- **Regola di ElectroNick**: tutto resta in WSL, nessun accesso ai file di Windows.
- **Rimandato**: GitHub (Actions + Pages) quando il progetto sarà pubblicato: richiede la licenza, la modifica a
  fm1-base pubblicata sotto MPL-2.0, i crediti per le risposte all'impulso degli autori del paper (e i loro esempi
  sonori fuori dal repository pubblico).

### 2026-11-08 — Step 4: tabla da registrazioni, non da sintesi (ElectroNick)
- **Contesto**: i prototipi modali (Na, Tin, Ge, Ke) "sembrano usciti da un videogioco" (ElectroNick). ElectroNick non ha un
  tabla né un tablista per registrare; chiede campioni liberi e lascia a me l'approccio, **esplicitamente senza il
  vincolo di CLAUDE.md §7** ("campioni solo come transienti brevissimi").
- **Decisione**: colpi **registrati** (interi, compressi ADPCM, intonati sul Sa per ricampionamento), da fonti con
  licenza che consente di includerli in un firmware libero: solo CC0 o CC BY.
- **Fonti scelte**: pacchetto Freesound "tabla bols" di mmiron (26 colpi, **CC0**, un tablista e un tabla:
  na, na_sharp, na-open, tun ×3, te ×4, tas ×3, re, ke ×3, ghe ×8, dhec); per ciò che manca (Tin) e per varietà, il
  dataset "Four-Way Tabla" (Rohit, Bhattacharjee, Rao, IIT Bombay, ISMIR 2021, Zenodo 10.5281/zenodo.7110248,
  **CC BY 4.0**: va citato).
- **Scartati**: pacchetto Freesound di msarkar (set completo ma "Sampling+": non commerciale, non adatto); pacchetti
  commerciali e "free" senza diritto di redistribuzione; la sintesi modale (bocciata all'ascolto); un modello fisico
  della membrana (esito incerto, costo alto).

### 2026-11-08 — Step 4: scelte di ElectroNick sul tabla registrato
- Il tabla da registrazioni (pacchetto CC0 di mmiron) **suona come un tabla vero** (ElectroNick).
- **Dayan in La3 (220 Hz)**: ricampionato −6 semitoni dalle registrazioni (315,2 Hz); in generale il dayan sta sul Sa
  nell'ottava più vicina a quella scelta, il bayan alla sua altezza naturale.
- **Tin = "na-open"** (il pacchetto non ha un Tin): Dhin = Ge + na-open.
- **Code lunghe tagliate a ~1 s** con dissolvenza (Ge lunghi fino a 3,6 s): flash stimata ~160 KB in ADPCM 4 bit a
  22 kHz per tutte le prese.
- Da fare allo step 5: libreria completa (tutti i colpi del pacchetto e i bol composti, tabella bol→colpi nei dati),
  formato di memorizzazione (ADPCM, frequenza, dove in flash), misura della qualità dopo la compressione.

### 2026-11-08 — Step 5: memorizzazione dei colpi del tabla
- **Formato**: IMA ADPCM 4 bit (decodifica esatta, solo interi: tabella di 89 passi; indice di partenza scelto per
  presa, predittore da 0). Il decoder C (`tabla/tabla.c`) coincide bit per bit con quello Python
  (`tools/tabla_data/adpcm.py`): `tabla_demo -selftest` confronta l'hash di ogni presa decodificata.
- **Frequenza per tamburo**: **dayan a 44,1 kHz** (i suoi colpi sono brillanti e vengono ricampionati verso il basso
  per il Sa: a 22 kHz si perde tutto sopra 11 kHz, e sopra 7,7 kHz una volta portati in La3), **bayan a 22 kHz** (sta
  in basso: sopra 8 kHz c'è −57 dB della sua energia).
- **Misura** (`tools/tabla_data/eval_codecs.py`, tutte le prese tagliate a 1 s): mediana dell'errore sotto il segnale
  PCM16 22 kHz 41,4 dB / 693 KB; ADPCM 22 kHz 29,8 dB / 173 KB; ADPCM 44 kHz 35,4 dB / 346 KB; µ-law 22 kHz 36,0 dB /
  346 KB. Ma per i colpi brillanti del dayan (dhec, na_sharp, re, tas, na-open) il 22 kHz, anche in PCM16, si ferma a
  19–26 dB: è la banda che manca, non il codec. Scelta mista: **220 KB** (dayan 150, bayan 70) per 23 prese.
- **Più dei ~160 KB stimati allo step 4** (quella stima era tutto a 22 kHz). L'immagine dell'app passa da 109 a
  **339 KB** su 581: restano 243 KB, sopra i 150 di margine richiesti ma da sorvegliare con la UI (step 7–8). Se
  servirà posto: (a) le prese nell'area dati `plat_data` (292 KB, fuori dall'immagine), (b) meno prese (il Ge ne ha 5),
  (c) dayan a 44 kHz solo per i primi ~250 ms e il resto a 22 kHz (−48 KB circa).
- **Confermato** (ElectroNick, 2026-11-08: nessuna differenza udibile all'A/B; la scelta dello spazio lasciata a me):
  restano 220 KB; si rivede solo se allo step 7–8 lo slot scende sotto il margine.
- **Scartati**: µ-law (doppio dello spazio dell'ADPCM a pari frequenza, poco meglio), PCM16 (troppo grande),
  tutto a 22 kHz (perde la brillantezza del dayan, che è ciò che ElectroNick ha apprezzato).
- **Interpolazione**: Hermite a 4 punti (la stessa per il ricampionamento del dayan e per il raddoppio del bayan):
  sul bayan dista 58 dB da un sovracampionamento polifase; costo trascurabile.
- **Fonte**: solo il pacchetto CC0 di mmiron. Il dataset Four-Way Tabla (CC BY 4.0) non è servito: non va citato
  finché non se ne usa nulla.

### 2026-11-08 — Step 5: il modulo `tabla/`
- **6 voci** (3 per tamburo); il colpo nuovo spegne quello che suona sullo stesso tamburo in 8 ms; si ruba la voce più
  vecchia se servisse. Il bayan suona sotto il dayan.
- **Eventi**: `tabla_trigger(bol, dinamica, offset)` con offset al campione (coda di 16, ordinata nel tempo; gli
  offset oltre il blocco passano ai successivi). Per il motore dei taal (step 6).
- **Dinamica 0..127**: volume `d·(0,35 + 0,65·d)` e passa-basso a un polo da 2 kHz (0) a 22,6 kHz (127, spento); ±5 %
  di volume casuale a ogni colpo; prese a rotazione da un punto casuale.
- **Dayan**: Sa ripiegato nell'ottava [190, 380) Hz (Sa La3 → 220 Hz, G3 → 196, Do4 → 262, Fa#3 → 370). Il passo di
  ricampionamento si aggiorna a ogni sotto-blocco: un cambio di Sa vale anche per il colpo che suona.
- **Livelli fra i colpi** (`level_db` in `data/tabla/strokes.json`): messi a orecchio da me, da confermare.
- **Dati**: `data/tabla/strokes.json` (colpi → prese) e `data/tabla/bols.json` (bol → colpi, al massimo uno per
  tamburo), validati e convertiti da `tools/tabla_data/build_samples.py` (con `--check` solo validazione, in
  `make test`). Nessun parser JSON sul dispositivo.

### 2026-11-08 — Step 6: motore dei taal
- **Formato** (`data/taal/FORMAT.md`): quello di CLAUDE.md §7, più `S` (avagraha) come pausa (`"S"`, `"S-Dha"`),
  `tempo` consigliato, `source`. Segni di un carattere (X, 0, 1..9); il sam è sempre la prima matra, anche quando il
  suo vibhag è segnato 0 (Rupak). Al massimo 8 parti in una matra.
- **Generazione**: `tools/taal_data/build_taal.py` → `taal/taal_tables.h` (~0,5 KB di tabelle). Gli id dei bol sono le
  posizioni in `data/tabla/bols.json`; la tabella porta anche i nomi e `taal_selftest` li confronta con quelli del
  tabla (in `make test`), così un riordino di `bols.json` senza rigenerare viene scoperto.
- **Posizione esatta in interi**: una matra = 60·24 000 000 unità, un campione avanza 544·mpm unità (la frequenza è
  24 MHz/544): **nessun arrotondamento, nessuna deriva**. Scartata la prima versione con frazione a 32 bit: l'incremento
  arrotondato derivava di 16 campioni in 2 cicli a 20 mpm (misurato). Ora ogni evento cade entro 1 campione dall'istante
  esatto, per tutti i taal a 20, 137 e 600 mpm (regressione). Una divisione intera per evento, nessuna per campione.
- **Cambio di tempo**: cambia solo l'incremento; la posizione nella matra resta: nessun salto (verificato in
  regressione). Nessuna rampa: un passo di encoder è già piccolo, e l'accelerando continuo suona liscio.
- **Cambio di taal mentre suona**: **alla fine del ciclo** in corso, dal sam del nuovo taal (ElectroNick, 2026-11-08; la
  prima versione cambiava alla matra successiva).
- **Dinamica**: sam 112, prima matra degli altri vibhag 100, le altre 92; le parti successive di una matra divisa
  all'85 %. Irregolarità (0..100, default 30): a 100 ±10 di dinamica e ±4 ms (ritardo casuale 0..8 ms).
- **Tempo**: 20..600 matra al minuto.
- **Mappature dei bol dei taal da pakhawaj** (Dhamar, Chautaal) sul tabla: Ka = Ke, Ga = Ge, Di = Dhin, Ki-Ta =
  Ke + Na, Ti-Ta = Ti + Na, Ka-Ta = Ke + Na, Ga-Di = Ge + Dhin, Ga-Na = Ge + Na (da confermare con ElectroNick).

### 2026-11-08 — Step 7: tabla e taal nel firmware
- **Fili**: la UI scrive i parametri del taal dal ciclo principale; l'audio li legge. L'avvio è una richiesta
  (`start`) presa dal render, che azzera lì la posizione: il ciclo principale non tocca mai la posizione. Gli altri
  parametri sono parole singole. Il tabla riceve colpi solo dal percorso audio (dal taal); `TABLA_P_HIT` e
  `TABLA_P_STOP` restano per gli scenari dell'host.
- **Schermo**: la striscia del taal (100 righe) si ridisegna da sola a ogni cambio di matra, il resto della schermata
  come prima (a ogni comando e ogni 30 frame).
- **Colori fissi** per sam (rosso) e khali (blu), indipendenti dal tema: devono leggersi a colpo d'occhio.

### 2026-11-08 — Step 8: salvataggio, UI finale, installazione
- **Impostazioni**: un oggetto dello store di fm1-base (`OBJ_SETTINGS`: due settori A/B, CRC, sicuro contro le
  interruzioni) a **0xDA000/0xDB000**, nell'area dati dell'app (0x97000–0xDFFFF, `plat.h`), accanto alla calibrazione
  del pannello (0xDC000). Scartato 0xFC000 (quello del template di fm1-base): è la finestra XDATA2, dove Felucca tiene i
  suoi globali; l'area dati è senz'altro dell'app. Record di 36 byte con magic, versione e dimensione; al caricamento
  ogni campo è controllato: un record fuori gamma non si usa (default). Le sorgenti accese/spente non si salvano:
  l'FM-1 parte in silenzio (da confermare con ElectroNick).
- **UNSAVED** sullo schermo (e la luce di SAVE) finché le impostazioni differiscono da quelle salvate: confronto del
  record a ogni frame.
- **Tasto da solo = tono finché è premuto** (ElectroNick, step 1): swara e ottava dal Sa medio; più tasti: vince l'ultimo
  premuto, al rilascio il tono passa a uno ancora premuto, poi torna com'era. Il tono di un tasto non è un'impostazione
  (non segna UNSAVED).
- **Luci dei tasti**: dal Sa in su, Sa e Pa accesi, Re Ga Ma Dha Ni tenui (le luci dell'FM-1 hanno pochi livelli: i
  komal e il tivra restano spenti).
- **EDIT a scorrimento** (11 righe, 7 visibili), con l'accordatura fine delle 4 corde.
- **Installazione**: la pagina d'installazione di fm1-base (Web MIDI, Chrome/Edge), che scrive solo l'area dell'app,
  servita da WSL (`tools/site.sh`), **senza m-bootloader** (lo script toglie il pacchetto: la pagina allora non lo
  offre), perché sostituirebbe il bootloader (CLAUDE.md §3). Una build di release richiede in `fm1.toml` licenza,
  autore, repository, descrizione, tag, pagina d'installazione e uno screenshot: dati da decidere con ElectroNick.

### 2026-11-08 — Pubblicazione su GitHub (ElectroNick: open source, citazione obbligatoria)
- **Licenza: Apache-2.0** per i file del progetto, con un file **NOTICE** che dà credito a ElectroNick
  (https://github.com/ELENICK97): la sezione 4(d) obbliga chi ridistribuisce a conservarlo. È la licenza open source
  diffusa più vicina a "chi lo usa deve citarmi"; MIT/BSD chiedono solo di tenere la nota di copyright, MPL-2.0 e GPL
  non chiedono attribuzione in più e impongono copyleft. CC BY non è adatta al codice. Compatibile con fm1-base e
  felucca-bsp (MPL-2.0 file per file: "your own files may use any licence", LICENSING.md) e con le parti Apache dell'SDK.
  In più `CITATION.cff` e la richiesta esplicita nel README.
- **Campioni**: pacchetto "tabla bols" di mmiron, tutte le 26 prese **CC0** (verificato in `_readme_and_license.txt`):
  ridistribuibili senza obblighi; citati per cortesia. Il dataset Four-Way Tabla (CC BY 4.0) non è usato né pubblicato.
- **Non pubblicati** (`.gitignore`): il PDF del paper e gli esempi sonori degli autori (diritti non chiari: si citano e
  si linkano), i cloni in `refs/`, i render di sviluppo (centinaia di MB: cinque demo brevi in `docs/audio/`), le build.
- **File modificato di felucca-bsp** marcato nell'intestazione (MPL-2.0).
- **Release 1.0**: build di release (identità FM-1_77010000) provata in fm1-emu; il pacchetto va nelle Release di
  GitHub, non nel repository; la pagina d'installazione (senza m-bootloader) può andare su GitHub Pages.
- Documentazione in inglese (README, INSTALL, MANUAL, CONTROLS, DEVELOPMENT, CREDITS); quella italiana resta
  (`*.it.md`) insieme ai diari di sviluppo (PROGRESS, DECISIONS, CLAUDE.md).

## Proposte rimandate
- ~~Tap tempo~~: ElectroNick non lo vuole (2026-11-06).
- **Alte più pulite nel tanpura** (dopo lo step 2b): il riferimento ha alte più pulite e un po' più brillanti; la causa
  è la metà frequenza delle corde. Da riprendere quando ci sarà la misura reale della CPU (step 3): se il secondo
  core ha margine, alzare la frequenza delle corde (es. 2/3 o piena per alcune corde, con un trattamento dello
  sporco vicino a Nyquist), oppure altre idee (contatto meno impulsivo). La correzione a 5,5 kHz è già nel codice,
  spenta.
