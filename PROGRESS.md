# PROGRESS

## Step corrente: 8 — Integrazione — **completato** (ElectroNick, 2026-11-08: "everything is okay"); pubblicazione preparata

### Risposte di ElectroNick (2026-11-08) e pubblicazione
1. Open source, autore "ElectroNick", GitHub https://github.com/ELENICK97 → licenza **Apache-2.0** con NOTICE (credito
   obbligatorio), `CITATION.cff`; `fm1.toml` completato; **release 1.0** (`release/teentaal-1.0.fwsc`, identità
   FM-1_77010000, sha256 `16270b82…f36068`) provata in fm1-emu realistico con tanpura, tabla, tono e SAVE: ok.
2. All'accensione in silenzio: confermato (già così). 3. Tutto chiaro. 4. Documenti: da rileggere da ElectroNick.
- Verifiche per la pubblicazione: campioni CC0 (ok), fm1-base/felucca-bsp MPL-2.0 (ok, file modificato marcato),
  paper ed esempi degli autori non pubblicati, `refs/` e render esclusi.
- Documentazione in inglese: `README.md` (con disclaimer IA ed esteso), `INSTALL.md`, `docs/MANUAL.md`,
  `docs/CONTROLS.md`, `docs/DEVELOPMENT.md`, `CREDITS.md`, `NOTICE`; versioni italiane in `*.it.md`.
- "Nicola" sostituito con "ElectroNick" in tutti i file del progetto.

## Step 8 — Integrazione (piano e report)

### Piano
1. **Salvataggio** con SAVE (ElectroNick, step 1): un oggetto dello store di fm1-base (A/B, CRC, sicuro contro le
   interruzioni) nell'area dati dell'app (0xDA000/0xDB000, accanto alla calibrazione del pannello a 0xDC000),
   caricato all'accensione; valori controllati al caricamento (fuori gamma → default). Salvati: La, Sa, tanpura
   (prima corda, jawari, ciclo, irregolarità, registro, accordatura fine delle 4 corde), taal, tempo, irregolarità
   dei tabla, tono (swara, ottava), i tre volumi. Un segno sullo schermo quando ci sono modifiche non salvate.
2. **UI finale** (le parti approvate allo step 1 non ancora fatte): **tasto da solo = tono di riferimento su quel
   tasto finché è premuto** (come un harmonium); **tasti illuminati** dal Sa in su (Sa e Pa accesi, gli altri swara
   shuddha tenui); pagina EDIT con **accordatura fine di ogni corda** (scorre: più righe dello schermo).
3. **Budget con tutto acceso** (tanpura, tabla, tono): CPU dei due core in fm1-emu realistico, flash, RAM.
4. **Documenti**: `INSTALL.md` (installazione da Chrome/Edge su Windows con l'installer web di fm1-base servito da
   WSL; ritorno al firmware ufficiale; **senza m-bootloader**, che sostituirebbe il bootloader: CLAUDE.md §3) e
   `docs/MANUAL.md` (manuale d'uso breve).
5. **Firmware finale** (build di release) provato in fm1-emu; nessuna installazione da parte mia.

### Verifica di completamento
- `make test`, scenario del simulatore (salva, spegni e riaccendi, ritrova le impostazioni), fm1-emu con tutto
  acceso; documenti scritti.

### Report step 8 (2026-11-08) — **STOP: in attesa di ElectroNick** (servono alcuni dati per la build di release)

**Fatto**
1. **Salvataggio con SAVE** — oggetto dello store di fm1-base a 0xDA000/0xDB000 (area dati dell'app), caricato
   all'accensione e controllato campo per campo. "UNSAVED" sullo schermo e luce di SAVE finché ci sono modifiche.
   Prima della scrittura il suono sfuma e poi torna: la flash ferma l'audio per ~12 ms, senza sfumatura era un click
   (misurato in fm1-emu, corretto e rimisurato).
2. **UI finale** — tasto da solo = tono su quello swara finché è premuto; tasti illuminati dal Sa in su (Sa e Pa
   accesi, Re Ga Ma Dha Ni tenui); pagina EDIT a scorrimento con l'accordatura fine delle 4 corde.
3. **Prove** — `make test` passa; simulatore: tono da tasto (anche con due tasti), luci, accordatura fine, SAVE,
   spegnimento e riaccensione con le impostazioni ritrovate e tutto spento: tutto ok. fm1-emu realistico, il pacchetto
   consegnato, tanpura + tabla + tono accesi e SAVE: audio continuo salvo la sfumatura voluta, schermo corretto.
4. **Documenti** — `INSTALL.md` (installazione da Chrome/Edge su Windows, ritorno al V15, cosa fare se va male),
   `docs/MANUAL.md` (manuale d'uso), `docs/CONTROLS.md` aggiornato, `tools/site.sh` (prepara e serve la pagina
   d'installazione, **senza m-bootloader**).

**Budget con tutto acceso** (fm1-emu realistico a 240 MHz; l'FM-1 va a 360)
| Voce | Misura (240 MHz) | Stima a 360 MHz | Budget |
|---|---|---|---|
| Primo core: render di tabla, taal, tono e mixer | **5 %** | ~3,3 % | 35 % (+ almeno 20 % di margine) |
| Secondo core: tanpura | **87 %** (max 89 %) | ~58–59 % | 85 % |
| Immagine app | 331.560 B su 581.564 | restano 250 KB | margine ≥ 150 KB |
| RAM `.data+.bss` | 71.412 B su 98.304 (73 %) | | ≥ 25 % libero: 27 % ✓ |
| Pool | 69.124 B su 344.064 | | ✓ |

**Cosa manca / limiti**
- **La build di release non c'è ancora**: la pagina d'installazione di fm1-base accetta solo un pacchetto di
  release, e la release vuole in `fm1.toml` licenza, autore, repository, descrizione, tag, pagina d'installazione e uno
  screenshot. Sono scelte tue (domanda 1). Il firmware consegnato ora è la build di sviluppo provata in fm1-emu.
- Nessuna misura sul dispositivo vero (niente FM-1 a disposizione): CPU, audio dello speaker, tempi reali della flash.
- La UI (disegno dello schermo) non è nella misura della CPU del primo core: gira nel ciclo principale, fuori dal
  render; in fm1-emu il sistema ha retto senza interruzioni dell'audio.

**Domande per ElectroNick**
1. **Dati per la release** (servono per installare): licenza del progetto (es. MPL-2.0 come fm1-base, GPL-3.0, o
   altro), nome dell'autore da mostrare, e il repository: lo pubblicherai su GitHub (quale indirizzo)? Se non vuoi
   pubblicarlo ora, posso fare una release solo locale se mi dici cosa scrivere al posto del repository.
2. **All'accensione** l'FM-1 parte in silenzio (le sorgenti accese non si salvano). Va bene, o vuoi che ricordi anche
   cosa era acceso?
3. Prova nel browser (`tools/web.sh`, http://localhost:8790/): un tasto da solo suona il tono finché lo tieni; le
   luci dei tasti ti sono utili così? SAVE e UNSAVED sono chiari?
4. Il manuale (`docs/MANUAL.md`) e `INSTALL.md`: manca qualcosa?

---

## Step 7 — Tabla e taal su target — **chiuso** (ElectroNick, 2026-11-08)

### Piano
1. **Fili**: i comandi della UI arrivano al taal dal ciclo principale mentre l'audio gira: l'avvio diventa una
   richiesta presa dal render (come per il tanpura), niente scritture concorrenti sulla posizione.
2. **UI dei tabla** (dalla proposta approvata in `docs/CONTROLS.md`, con il cambio di taal a fine ciclo deciso allo
   step 6): PLAY avvia/ferma (LED), KNOB1 tempo (SEL+KNOB1 a passi di 10), KNOB3 volume tabla, SELECT sceglie il taal
   (il prossimo è mostrato finché non parte), pagina EDIT: irregolarità dei tabla. Schermata principale ridisegnata:
   Sa; blocco del taal con nome, tempo, numero della matra grande, il ciclo diviso in vibhag con X (sam) e 0 (khali)
   in colori distinti e la matra corrente accesa; tanpura e tono su una riga ciascuno.
3. **Prove**: simulatore host di fm1-base (scenario `tools/test/fw_basic.txt` esteso ai tabla, schermate), poi il
   `.fwsc` vero in fm1-emu (B): audio dei tabla nel WAV, schermate, costo CPU in modalità realistica con tabla e
   tanpura insieme.
4. **Deliverable**: firmware con i tabla (il tanpura e il tono restano) provato nell'emulatore, istruzioni per provarlo.
   Nessuna installazione sul dispositivo.

### Verifica di completamento
- `make test` e lo scenario del simulatore passano; in fm1-emu i tabla si sentono nel WAV, lo schermo mostra matra e
  vibhag, il costo del primo core resta nel budget.

### Report step 7 (2026-11-08) — **STOP: in attesa della prova di ElectroNick**

**Fatto**
1. **Fili**: l'avvio del taal è una richiesta presa dal render; la UI non tocca la posizione del ciclo.
2. **UI** (`ui/ui.c`, `docs/CONTROLS.md`): PLAY tabla (LED), SELECT taal (il prossimo resta mostrato con ">" fino
   alla fine del ciclo), KNOB1 tempo (SEL+KNOB1 ×10), KNOB3 volume tabla, EDIT → TABLA IRREG. Schermata principale
   nuova: Sa e Sa del tanpura in una riga; blocco del taal con nome, tempo, matra grande (rossa sul sam, blu nel
   khali, con la scritta), il ciclo a celle per vibhag con X rosso e 0 blu; tanpura e tono una riga ciascuno; CPU e
   volume tabla in fondo. La striscia del taal si ridisegna a ogni matra.
3. **Prove**:
   - `make test` passa (host: hash, costo, tempi del taal).
   - Simulatore di fm1-base (`tools/fw.sh --host`, `tools/test/fw_basic.txt` esteso): PLAY, tempo 155, volume, cambio
     a Ektaal che parte alla fine del ciclo, irregolarità, arresto con silenzio finale: tutti ok.
   - **fm1-emu (B), il `.fwsc` vero**: i tabla si sentono; colpi misurati nel WAV ogni 0,500 s (Teentaal a 120:
     atteso 0,5 s); con tanpura e tabla insieme in modalità realistica nessuna interruzione dell'audio.
   - Il pacchetto consegnato (`renders/step-7/teentaal-step7.fwsc`) è stato rifatto girare in fm1-emu così com'è.

**Misure** (fm1-emu realistico, che gira a 240 MHz; l'FM-1 a 360)
| Voce | A 240 MHz (misura) | A 360 MHz (stima) | Budget |
|---|---|---|---|
| Primo core: tabla + taal + tono + mixer (render) | **4 %** | ~2,7 % | 30 % + 5 % |
| Secondo core: tanpura | 87 % (max 89 %) | ~58 % | 85 % |
| Immagine app | 329.592 B su 581.564 (restano 252 KB) | | margine ≥150 KB |
| RAM `.data+.bss` | 71.332 B su 98.304 | | |

**Limiti**
- La misura vera della CPU sul dispositivo manca ancora (ElectroNick non ha l'FM-1): il primo core ha comunque un margine
  enorme. La lettura dei campioni dalla flash (cache) è compresa nella misura dell'emulatore solo se ne modella i tempi.
- Il firmware contiene anche tanpura e tono (già consegnati allo step 3): ho preferito non toglierli per fare un
  firmware "solo tabla"; dimmi se ti serve davvero separato.
- Nessun salvataggio delle impostazioni: arriva allo step 8.

**Da provare (ElectroNick)**, nell'emulatore del browser (`tools/web.sh`, poi http://localhost:8790/):
1. PLAY: partono i tabla dal sam? La matra sullo schermo segue il suono? Si leggono bene sam e khali?
2. KNOB1 / SEL+KNOB1: il tempo cambia senza salti?
3. SELECT mentre suonano: il nuovo taal parte alla fine del ciclo, e lo schermo lo annuncia in modo chiaro?
4. KNOB3: il volume dei tabla, rispetto al tanpura, va bene di default (100)?
5. La schermata è leggibile così, o vuoi spostare/ingrandire qualcosa?

### Risposte di ElectroNick (2026-11-08)
1. I tabla partono dal sam, la matra segue il suono, sam e khali si leggono bene. 2. Il tempo cambia senza salti.
3. Il cambio di taal a fine ciclo è chiaro sullo schermo. 4. Il volume dei tabla di default va bene.
5. La schermata va bene così per ora (da rivedere eventualmente allo step 8).

**Step 7 completo dal mio lato: attendo che ElectroNick lo dichiari chiuso per passare allo step 8.**

---

## Step 6 — Motore dei taal (solo host) — **chiuso** (ElectroNick, 2026-11-08)

### Piano
1. **Formato** `data/taal/*.json` (quello di CLAUDE.md §7, esteso con: `S` = pausa, `"tempo"` consigliato, `"source"`),
   documentato in `data/taal/FORMAT.md`.
2. **Script** `tools/taal_data/build_taal.py`: valida (somma dei vibhag = matra, un segno per vibhag, theka lunga
   quanto le matra, ogni bol presente in `data/tabla/bols.json` → altrimenti errore chiaro) e genera
   `taal/taal_tables.h`. Nessun parser JSON sul dispositivo.
3. **I 10 taal** con le fonti citate nel file; le varianti dubbie segnalate a ElectroNick.
4. **Modulo `taal/`**: posizione nel ciclo in virgola fissa (matra + frazione a 32 bit), eventi al campione esatto
   verso `tabla_trigger`, tempo in matra al minuto (20–600), cambio di tempo senza salti (la posizione resta continua),
   cambio di taal alla matra successiva ripartendo dal sam, accento sul sam, leggera umanizzazione (tempi e dinamica).
   Espone matra corrente, vibhag, segno (X, 0, 2…) per la UI dello step 7.
5. **Prove** (regressione: hash, costo, regolarità dei tempi misurata sugli eventi; cambi di tempo) e **WAV** di ogni
   taal per alcuni cicli a tempo medio, più uno lento e uno veloce, e un cambio di tempo.

### Verifica di completamento
- `make test` passa; WAV in `renders/step-6/` con README; costo nel budget (taal + reftone + mixer ≤ 5 %).

### Report step 6 (2026-11-08) — **STOP: in attesa dell'ascolto di ElectroNick**

**Fatto**
1. **Formato e dati** — `data/taal/FORMAT.md`; 10 file in `data/taal/` con la fonte citata in ognuno (D. Courtney,
   chandrakantha.com; Wikipedia per Jhoomra e Dhamar; NIOS per Chautaal; tablatheka.com). Aggiungere un taal = scrivere
   un file e rigenerare.
2. **Script** `tools/taal_data/build_taal.py`: valida (vibhag che sommano alle matra, segni, lunghezza della theka,
   bol esistenti: altrimenti errore con file, matra e bol disponibili) e genera `taal/taal_tables.h`.
3. **Modulo `taal/`**: posizione in interi esatti (nessuna deriva), eventi al campione verso `tabla_trigger`, tempo
   20–600 matra/min senza salti, cambio di taal alla matra successiva dal sam, accenti, irregolarità. Espone matra
   corrente, segno del vibhag (X, 0, 2…), parti della matra: quello che servirà al display (step 7).
4. **Prove** — ogni evento entro 1 campione dall'istante esatto per tutti i 10 taal a 20, 137 e 600 mpm; cambio di
   tempo a metà matra: il colpo successivo cade dove deve (124609 contro 124608,5); quattro scenari nuovi
   (Teentaal 160, cambi di taal e tempo, Ektaal a 600 a Sa F#4, Teentaal con tanpura). `make test` passa.

**Misure**
| Voce | Misura | Budget |
|---|---|---|
| CPU taal + tabla + mixer (stima, primo core) | **≈3,4 %** (Teentaal 160), 3,2 % (Ektaal a 600) | 30 % + 5 % |
| Flash | +2,1 KB (immagine 340.820 B) | |
| RAM | +48 B | |

**Domande per ElectroNick**
1. **Ascolto** (`renders/step-6/`): i taal suonano giusti? Il passo è regolare, gli accenti (sam più forte) ti piacciono?
2. **Theka con varianti** (ho scelto io; dimmi se la tua scuola le suona diversamente):
   - Keherwa: Dha Ge Na Ti | Na Ka Dhi Na (varianti: "Na Ke Dhi Na", "Dha Ge Na Tin").
   - Dadra: Dha Dhi Na | Dha Ti Na (variante: "Na Ti Na").
   - Ektaal: … Dhin Na in fondo (variante "Dhi Na": qui suonano uguali); Kat = Ke.
   - Rupak: il sam cade sul khali (segni 0, 1, 2).
   - Jhoomra: Dhin S-Dha Ti-Ra-Ki-Ta | Dhin Dhin Dha-Ge Ti-Ra-Ki-Ta | Tin S-Ta Ti-Ra-Ki-Ta | … (Wikipedia).
   - Deepchandi: con le pause (S) alla 3ª, 7ª, 10ª e 14ª matra.
   - Dhamar e Chautaal sono taal da pakhawaj: ho tradotto i bol sul tabla (Ka = Ke, Ga = Ge, Di = Dhin, Ga-Di = Ge + Dhin…).
     Vanno bene così?
3. **Tempo**: 20–600 matra al minuto ti basta? A 40 la theka è suonata nuda, senza riempitivi: va bene per esercitarti?
4. **Cambio di taal mentre suona**: alla matra successiva, ripartendo dal sam del nuovo taal. Oppure preferisci che
   aspetti la fine del ciclo?
5. **Irregolarità**: 30 di default (`3_changes/human_0_then_100.wav` mette a confronto 0 e 100). Va bene?

### Risposte di ElectroNick (2026-11-08)
1. I taal suonano bene. 2. Theka e mappature dei bol: ok. 3. Tempo 20–600, vilambit senza riempitivi: ok.
4. **Cambio di taal: aspettare la fine del ciclo** → fatto (`taal/taal.c`): il nuovo taal parte dal sam successivo;
   regressione aggiornata (hash di `taal_switch_tempo`), `3_changes/taal_changes.wav` rigenerato. 5. Irregolarità 30: ok.

**Step 6 completo dal mio lato: attendo che ElectroNick lo dichiari chiuso per passare allo step 7.**

---

## Step 5 — Tabla, libreria completa di colpi (solo host) — **chiuso** (ElectroNick, 2026-11-08)

### Piano
1. **Dati dei campioni** (`tools/tabla_data/build_samples.py` → `tabla/tabla_samples.h`, generato): dalle registrazioni
   CC0 di mmiron: inizio esatto, coda tagliata (−60 dB o ~1 s con dissolvenza), livello normalizzato per colpo,
   frequenza di memorizzazione, **compressione** (ADPCM IMA 4 bit, confrontata con altre) e **misura della qualità**
   dopo la compressione (rapporto segnale/errore, spettro, ascolto A/B). I campioni stanno nell'immagine dell'app come
   dati costanti (posto: 103 KB usati su 581).
2. **Tabella bol → colpi nei dati** (`data/tabla/bols.json` → `tabla/tabla_bols.h`, generato e validato: un colpo
   sconosciuto è un errore chiaro), con i bol che servono ai 10 taal; le corrispondenze dubbie segnalate a ElectroNick.
3. **Modulo `tabla/`** (stesso codice per host e target, interfaccia comune + un ingresso per gli eventi del taal):
   voci per tamburo (il colpo nuovo spegne il precedente in 8 ms, il bayan continua sotto il dayan), prese a
   rotazione, dinamica (volume e un poco più scuro se piano), dayan accordato sul Sa per ricampionamento (finestra
   d'ottava 190–380 Hz: La → La3), bayan alla sua altezza. Niente allocazioni, niente divisioni per campione.
4. **Prove**: regressione (hash, costo), stress (tutti i bol, tutti i Sa, colpi rapidissimi), costo stimato.
5. **Deliverable**: WAV di ogni colpo e di ogni bol a tre dinamiche e più accordature; A/B originale/compresso.

### Verifica di completamento
- `make test` con gli scenari del tabla; WAV in `renders/step-5/` con README; costo CPU, flash, RAM nel budget.

### Report step 5 (2026-11-08) — **STOP: in attesa dell'ascolto di ElectroNick**

**Fatto**
1. **Dati** — `data/tabla/strokes.json` (12 colpi, 23 prese del pacchetto CC0 di mmiron) e `data/tabla/bols.json`
   (28 bol), convertiti e validati da `tools/tabla_data/build_samples.py` in `tabla/tabla_samples.h`,
   `tabla_bols.h`, `tabla_strokes.h` (generati). Un colpo sconosciuto, due colpi sullo stesso tamburo o un nome
   ripetuto fanno fallire lo script con un errore chiaro.
2. **Memorizzazione** — ADPCM 4 bit; **dayan a 44 kHz, bayan a 22 kHz**: **220 KB** (dayan 150, bayan 70). Tabella del
   confronto fra formati e motivi in `DECISIONS.md`. Errore della compressione, presa per presa: 22–48 dB sotto il
   segnale (dayan), 26–45 dB (bayan); i peggiori sono dhec (22), na_sharp (25), re (26).
3. **Modulo `tabla/`** — 6 voci, il colpo nuovo spegne il precedente sullo stesso tamburo in 8 ms, prese a rotazione,
   dinamica (volume e timbro), ±5 % casuale, dayan accordato sul Sa per ricampionamento (Hermite 4 punti), bayan alla
   sua altezza. Eventi al campione esatto con `tabla_trigger` (per lo step 6). Niente allocazioni né divisioni per
   campione.
4. **Prove** — decoder C identico bit per bit al Python (`tabla_demo -selftest`, in `make test`); uscita del modulo
   uguale a un'interpolazione Hermite indipendente in Python (93,5 dB: solo l'arrotondamento a intero); altezza del
   Tun misurata 220,1 / 196,2 / 261,9 / 370,3 Hz per Sa A3 / G3 / C4 / F#3 (attesi 220,0 / 196,0 / 261,6 / 370,0: lo
   0,1 % è la stima dei 315,2 Hz delle registrazioni). Tre scenari nuovi in regressione (tutti i bol, drut a Sa F#4
   con un colpo ogni 46 ms, stop e riaccordatura): `make test` passa.

**Misure**
| Voce | Misura | Budget |
|---|---|---|
| CPU tabla + mixer (stima, primo core) | **≈2,7–2,9 %** (184–198 istruzioni host/campione, caso drut) | 30 % + 5 % |
| Flash: campioni | 225.596 B | ~160 KB stimati allo step 4 (**superato**: vedi sotto) |
| Flash: immagine app | **338.700 B** (era 108.912) su 581.564 | margine ≥150 KB: restano 243 KB |
| RAM `.data+.bss` | 71.316 B su 98.304 (+0,6 KB) | |

La stima CPU non conta la lettura dei campioni dalla flash sul dispositivo (cache): sono pochi byte al campione, ma
va misurata allo step 7. Il firmware compila con il tabla dentro (solo come prova di dimensione: **nessun `.fwsc` da
installare** in questo step).

**Cosa non va ancora / limiti**
- 220 KB invece di ~160: il dayan a 44 kHz costa il doppio. Alternative pronte se lo spazio servirà (in `DECISIONS.md`).
- I **livelli fra i colpi** li ho messi io a orecchio (Te/Re/Ti/Ne −6 dB, Ke −5, Tas −3…).
- Il dayan su Sa sotto Sol3 (es. Fa#3) salta in cima alla finestra (370 Hz): è una scelta di finestra, vedi domanda 4.
- Le prese del pacchetto sono di un solo tablista e un solo tabla: per Tin ci sono solo una presa, per Re/Ti/Ne una.

**Domande per ElectroNick**
1. **Ascolto dei colpi** (`1_strokes/`, `2_bols/`): qualche colpo suona male, troppo forte o troppo piano rispetto
   agli altri? Qualche presa da togliere (es. una delle 5 del Ge)?
2. **Compressione** (`4_AB_compression/`): senti differenza fra A e B? Se sì, su quali prese?
3. **Bol dubbi** (scelte mie, da confermare): Ta = Na; Dhi/Di = Dhin (Ge + Tin); Ki/Kat/Kit = Ke (bayan chiuso);
   Tit = Te; "Dhet" = il dhec del pacchetto, senza Ge; "Tas" e "Meend" sono nomi di comodo per due colpi del pacchetto
   (tas chiuso; Ge con glissando). Il Ge nel Dha/Dhin è a −1,4 dB rispetto al Ge da solo.
4. **Finestra del dayan**: oggi 190–380 Hz (Sa G3 → 196, Sa F#3 → 370). Ti va bene, o preferisci che sotto un certo Sa
   il dayan resti sotto (es. finestra 175–350)?
5. **Dinamica**: il pianissimo (40) è abbastanza piano e scuro? E il forte (127)?
6. **Spazio**: va bene 220 KB per i campioni, o preferisci risparmiare (meno prese, oppure solo l'attacco del dayan a
   44 kHz)?

### Risposte di ElectroNick (2026-11-08)
1. Colpi e livelli: vanno bene. 2. Nessuna differenza udibile fra originale e compresso. 3. Bol dubbi: ok come sono.
4. Finestra del dayan 190–380 Hz: ok. 5. Dinamica: ok. 6. Spazio: lascia a me la scelta → **restano 220 KB** (dayan
44 kHz, bayan 22 kHz): il margine dello slot (243 KB) supera i 150 richiesti; le alternative restano in
`DECISIONS.md` per lo step 7–8 se la UI ne avesse bisogno.

**Step 5 completo dal mio lato: attendo che ElectroNick lo dichiari chiuso per passare allo step 6.**

---

## Step 4 — Tabla, valutazione dell'approccio — **chiuso** (ElectroNick, 2026-11-08): colpi registrati

### Piano
1. **Fonti**: acustica del tabla (Raman; Rossing & Sykes; descrizioni dei colpi) per rapporti dei modi, decadimenti,
   effetto della posizione del colpo e del dito che smorza, glissando del Ge sul bayan. Le fonti vanno citate.
2. **Prototipi su host** (`tools/tabla_proto/`, Python: è una valutazione, il codice per il firmware arriva allo step 5)
   dei colpi **Na, Tin, Ge, Ke**, in sintesi modale: eccitazione (impulso + breve rumore del contatto, durata e
   brillantezza per colpo) in un banco di modi smorzati; dayan quasi armonico (effetto dello syahi), accordato sul Sa;
   bayan grave con glissando di pitch sul Ge; Ke chiuso e smorzato.
3. **Due livelli di dettaglio** per confrontare costo e suono: semplice (pochi modi) e ricco (più modi, rumore
   modellato). Una variante ibrida (transiente campionato + corpo modale) richiederebbe registrazioni di tabla con
   licenza adatta: la propongo solo se la sintesi non convince.
4. **Stima dei costi**: istruzioni per modo e campione, polifonia necessaria al tempo più veloce, flash e RAM.

### Verifica di completamento
- WAV di confronto in `renders/step-4/`: ogni colpo da solo, a più dinamiche, e brevi frasi (es. Dha Dhin Dhin Dha)
  con il Sa del tanpura; tabella dei costi.
- STOP: la scelta dell'approccio la fa ElectroNick.

### Report step 4 (2026-11-08) — **STOP: la scelta dell'approccio è di ElectroNick**
- **Fatto**: `tools/tabla_proto/tabla_proto.py`; 20 WAV e spettrogrammi in `renders/step-4/` (README con fonti e
  misure): Na, Tin, Ge, Ge con glissando, Ke, Dha, Dhin, una frase con Ke, Teentaal a 120 da solo e con il tanpura;
  ognuno in due livelli di dettaglio (simple, rich) e tre dinamiche.
- **Verifiche**: Na ha la 3ª armonica più forte e la fondamentale −19/−21 dB; Tin la 2ª; Ke −20 dB in 70 ms; il Ge
  con glissando sale da 108 a 140 Hz. Corretto un difetto del primo tentativo (gli zeri spettrali del contatto a coseno
  rialzato cadevano su singole armoniche: Tin h5 a −65 dB).
- **Costo stimato sul firmware** (un modo = risonatore a due poli, ~8 istruzioni per campione; 2 voci per tamburo:
  il colpo nuovo e la coda del precedente; rumore ~12 istruzioni mentre dura; glissando: coefficienti aggiornati a
  ogni sotto-blocco di 64):

| Variante | Modi al massimo | Istruzioni/campione | Core 0 (1,0–1,3 cicli/istr.) | Flash | RAM |
|---|---|---|---|---|---|
| simple | 2×6 + 2×3 = 18 | ~190 | **2,3–3,0 %** | < 10 KB | < 2 KB |
| rich | 2×14 + 2×8 = 44 | ~420 | **5,1–6,7 %** | < 15 KB | < 4 KB |
| rich, 4 voci per tamburo | 88 | ~800 | 10–13 % | < 15 KB | < 6 KB |
| ibrido (transiente campionato ADPCM + corpo modale) | come rich | +~10 per voce | ~6–8 % | +~20 KB di campioni | |

  Tutte nel budget del 30 %: la scelta è di suono, non di costo. L'ibrido richiede registrazioni di tabla con una
  licenza adatta (oggi non ne ho): lo propongo solo se la sintesi non convince.

**Domande per ElectroNick**
1. **Ascolto**: Na, Tin, Ge, Ke (01–05) suonano come tabla? Quale livello preferisci, simple o rich?
2. **Approccio**: la sintesi modale ti convince, o vuoi provare l'ibrido con transienti campionati?
3. **Accordatura del dayan**: con il tuo Sa (La) l'ho messo a La4 (440 Hz), il bayan a La2. Va bene, o preferisci un
   dayan più grave (La3, un dayan grande e raro)?
4. **Ge**: con o senza glissando di default? Quanto (ho messo +4 semitoni)?
5. **Tin**: l'ho fatto con la 2ª armonica dominante (ipotesi). Ti suona giusto?

### Ascolto di ElectroNick (2026-11-08) e nuova direzione
- "Questi suoni non mi convincono per niente, sembrano usciti da un videogioco." ElectroNick non ha un tabla né un
  tablista; chiede campioni liberi e lascia a me l'approccio, senza il vincolo di CLAUDE.md §7 sui campioni
  (DECISIONS.md, 2026-11-08). **Approccio scelto: colpi registrati**, compressi e intonati sul Sa.
- **Fonti verificate**: pacchetto Freesound "tabla bols" di mmiron, **CC0** (26 colpi, 44,1 kHz / 16 bit mono:
  na, na_sharp, na-open, tun ×3, te ×4, tas ×3, re, ke ×3, ghe ×8, dhec; manca il Tin); dataset Four-Way Tabla
  (IIT Bombay, **CC BY 4.0**), scaricato e verificato (md5) in `docs/references/tabla_fourway/`: 237 registrazioni di
  10 tabla, istanti dei colpi in 4 categorie (smorzato, risonante acuto, risonante grave, entrambi), senza il nome del
  bol. Scartato il pacchetto di msarkar ("Sampling+").
- **In attesa**: Freesound richiede un accesso (gratuito) per scaricare gli originali: ElectroNick scarica il pacchetto e
  lo salva in `docs/references/tabla_mmiron/` (da Windows: `\\wsl.localhost\Ubuntu-26.04\home\dox\FM-1\TeenTaal-FM1\docs\references\tabla_mmiron`).
- **Fatto (2026-11-08)**: pacchetto ricevuto e verificato (26 file, tutti CC0 nel file di licenza del pacchetto);
  `tools/tabla_proto/tabla_samples.py`; 33 WAV in `renders/step-4/samples/` (README lì): ogni colpo con tutte le prese
  e tre dinamiche, i due sostituti del Tin, Dha, Dhin, una frase, Teentaal a 120 da solo e col tanpura, con il dayan a
  La4 e a La3. Accordatura verificata: Tun a 440,5 / 220,3 Hz (+2 cents; le prese differiscono tra loro di ±1,5).
- **Costi stimati sul firmware**: CPU trascurabile (decodifica ADPCM e ricampionamento, ~25 istruzioni per campione e
  voce; 4 voci ≈ 100: ~1,5 % del core 0). **Flash**: tutte le 23 prese usate, tagliate a −60 dB: 28,7 s → **309 KB**
  in ADPCM 4 bit a 22 kHz; tagliate a 1 s con dissolvenza → 159 KB; a 0,7 s → 119 KB. L'area dati disponibile è
  292 KB + 16 + 92 KB: il budget di 200 KB della §6 va rivisto se si tengono tutte le prese lunghe.

**Domande per ElectroNick**
1. **Ascolto**: questi suonano come un tabla vero? (Partirei da `16_teentaal_120_TinStandinA_dayanA4_with_tanpura`.)
2. **Dayan La4 o La3?** (440 Hz più piccolo e brillante; 220 Hz più grande e scuro: entrambi a 6 semitoni dall'originale.)
3. **Tin**: A (na-open) o B (tun) è più vicino a ciò che ti aspetti in Dhin? Se nessuno, estraggo Tin veri dal dataset
   Four-Way Tabla (IIT Bombay, CC BY 4.0).
4. **Code lunghe**: i Ge lunghi (fino a 3,6 s) costano flash; tagliarli a ~1 s ti va bene?

### Risposte di ElectroNick (2026-11-08)
1. Suonano come un tabla vero: **sì**. 2. Dayan **La3**. 3. Tin: **A (na-open)**. 4. Code a ~1 s: **ok**.
→ **Approccio scelto: colpi registrati** (DECISIONS.md; budget del tabla aggiornato in CLAUDE.md §6).
**In attesa della chiusura dello step 4.**

---

## Step 3 — Tanpura su target — **chiuso** (ElectroNick, 2026-11-08)
**Punto aperto**: la misura reale della CPU sul dispositivo (ElectroNick non ha un FM-1 ora): alla prima installazione, al
più tardi allo step 8.

### Piano
1. **Secondo core** (`fm1.toml` `cpu2 = true`, `cpu2_fallback = true`): il tanpura del blocco *successivo* è un job del
   core 1 mentre il core 0 suona il blocco corrente (un blocco di latenza, 5,8 ms, irrilevante per un bordone): il
   core 0 non aspetta. Le riaccordature preparate dal ciclo principale si applicano sul core 0 tra un job e l'altro
   (core 1 fermo): i due core non toccano mai gli stessi dati insieme. Se il secondo core manca (fallback), il
   tanpura gira sul core 0 e lo schermo lo dice.
2. **Misura del costo reale**: il job si cronometra da solo con `plat_ticks24` (24 MHz, leggibile da entrambi i
   core): percentuale media e massima del blocco sul core 1, `plat_cpu_pct()` sul core 0, profondità dello stack del
   core 1; tutto sullo schermo.
3. **Registro del tanpura** (il punto aperto dallo step 2a): il Sa resta quello del suonatore; il tanpura può stare
   un'ottava sotto ("grande", default). Default: Sa A3 → tanpura A2, tono madhya A3, tar A4 (il Sa del flauto in La).
   Se l'ottava sotto esce dall'estensione (F#2), il tanpura resta all'altezza del Sa.
4. **UI minima del tanpura**, dalla proposta approvata (`docs/CONTROLS.md`): SEQ avvio/arresto tanpura, ARP tono,
   SCL + tasto = Sa, PRESETS = Sa fine (cents), ALGORITHM = jawari, KNOB2 volume tanpura, KNOB4 volume tono,
   OCT−/+ ottava del tono; EDIT: pagina con prima corda, registro, La, ciclo, irregolarità (SELECT sceglie la riga,
   PRESETS cambia il valore, HOME torna). Niente salvataggio (step 8), niente tabla.
5. Test: simulatore host con scenario, regressione, build target; **prova in fm1-emu (ElectroNick)**; poi, se ElectroNick
   vuole, installazione sul dispositivo per la misura vera (prima: V15 a portata e backup).

### Verifica di completamento
- Simulatore: scenario del firmware che usa i controlli nuovi; regressione e stress ok.
- `.fwsc` provato da ElectroNick in fm1-emu (suono, controlli, indicatore CPU).
- Costo reale misurato sul dispositivo (se ElectroNick installa) e confrontato con la stima 54–79 %.

### Stato (2026-11-06) — **in attesa della prova di ElectroNick in fm1-emu**
- Fatto: `tanpura_take` (unico punto che cambia lo stato delle corde dall'esterno del render) e registro del tanpura
  (`TANPURA_P_REGISTER`, default "un'ottava sotto"); engine diviso per due core (`engine_tanpura_take`,
  `engine_tanpura_render`, `engine_render_with`); `app.c` con la pipeline sul secondo core, il ripiego su un core e la
  misura del job (`plat_ticks24`); `ui/ui.c` nuova (pagina principale e pagina EDIT); `fm1.toml` con `cpu2 = true`,
  `cpu2_fallback = true`.
- Verifiche: autotest, stress, regressione (3 hash del tanpura riscritti: il tanpura non parte più con una
  riaccordatura grande a t = 0; qualità invariata: centroide 75 Hz, inviluppo 1,05 dB, spettro 2,74 dB), simulatore
  (`tools/test/fw_basic.txt`: 25 controlli, 962 job sul secondo core simulato). Firmware: 108 KB, RAM 70,7 KB di 96.
- Da provare: `renders/step-3/teentaal-step3-test.fwsc` (sha256 `beefe51a…ad91fa0`) in fm1-emu.

### Prove (2026-11-06/07)
- **fm1-emu nel browser** (ElectroNick): due core → muto (256 blocchi/s invece di 172), un core (`diag_B`) → suona.
  Riga di diagnostica sullo schermo: il secondo core parte e completa un job per blocco.
- **Emulatori locali in WSL** (approvati da ElectroNick, DECISIONS.md): A = `tools/web.sh` (WebAssembly, localhost:8790),
  B = `tools/emu.sh` (fm1-emu senza finestra, il `.fwsc` vero).
- **B, un core**: tanpura acceso a 2,5 s → audio presente; core 0 nell'interrupt audio il 41,8 % del tempo
  (~5100 istruzioni per campione con il tanpura).
- **B, due core**: core 0 nell'interrupt solo il 5,1 %; 167 blocchi/s (normali); **l'uscita è identica bit per bit a
  quella a un core, ritardata di esattamente un blocco** (la pipeline come progettata). Il silenzio nel browser è un
  limite dell'emulatore nel browser.
- **B, due core, modalità strict** (cicli misurati, a 240 MHz fissi nell'emulatore): 744 render, 743 job tutti sul
  core 1; job del tanpura 121646 tick (ultimo) e 124643 (massimo) su 139264 per blocco = 87–90 % a 240 MHz →
  **≈ 58 % (medio) / 60 % (massimo) a 360 MHz**, dentro la stima (54–70 %) e il budget (85 %). Prudenza: i miss della
  cache della flash costano tempo fisso, quindi a 360 MHz qualche punto in più. Misura vera: sul dispositivo.
- **Difetto da correggere** (screenshot di ElectroNick nell'emulatore A, 2026-11-07): tornando dalla pagina EDIT alla
  principale restano residui della pagina EDIT negli spazi tra le fasce dello schermo (le fasce non coprono tutto):
  pulire lo schermo intero al cambio di pagina. **Corretto** (2026-11-07): schermo pulito al cambio di pagina; scenario del simulatore
  con una schermata dopo il ritorno (`screen_main_after_settings.png`).

### Report step 3 (2026-11-07)
- **Deliverable**: `renders/step-3/teentaal-step3.fwsc` (sha256 `54895f3c…85cd7f35`, 108,9 KB, RAM 70,7 KB di 96).
  Provato in fm1-emu sul pacchetto vero (B: due core, tanpura acceso a 2,5 s, audio presente, livelli identici alla
  prova precedente); controlli e suono provati da ElectroNick nell'emulatore browser locale (A: "come i WAV").
  Test: autotest, stress, regressione, simulatore (26 controlli). I pacchetti di prova e diagnostica sono stati tolti.
- **Costo**: stima in modalità strict ≈ 58–60 % del secondo core a 360 MHz (budget 85 %); primo core ~5 % per l'audio.
- **Non fatto**: la misura reale sul dispositivo — ElectroNick non ha un FM-1 a disposizione per ora. Il firmware la mostra
  sullo schermo (`TANPURA x% MAX y%`) e la riga gialla di diagnostica resta fino ad allora. Da fare alla prima
  installazione (prima: V15 a portata, backup, passaggio per V15 tra firmware diversi).
- **Domanda per ElectroNick**: chiudere lo step 3 con la misura sul dispositivo rimandata (annotata come punto aperto per
  lo step 8, o per quando avrai un FM-1), e passare allo step 4 (tabla, valutazione dell'approccio)?

---

## Step 2b — Tanpura, versione per il target — **chiuso** (ElectroNick, 2026-11-06)

### Piano
1. `tanpura/`: il modello in **float32**, stesso schema del riferimento (modi esatti, contatto in forma chiusa,
   stabilità), con il numero di modi per corda come parametro di configurazione. Vincoli §6: niente allocazione
   dinamica, niente libm (solo `fastmath.h` di fm1-base), niente divisioni o trascendenti per campione:
   - forza al ponte senza divisione: dalla (48) segue F_b = (s_b + u)/θ in tutti e quattro i casi, con 1/θ costante;
   - radice quadrata con `fm_sqrtf`; pizzico da tabella; coefficienti dei modi calcolati con formule stabili in
     float (1−cos e 1−R senza cancellazione).
   Parametri del modulo (§7): Sa e accordatura fine per corda, swara della prima corda, jawari (posizione del filo),
   velocità del ciclo, irregolarità umana, volume per corda, acceso/spento.
2. **Corpo in IIR**: biquad adattati alle risposte misurate del ponte e del capotasto (script Python che genera i
   coefficienti in un header C).
3. **Misure per ogni variante** (numero di modi per corda, eventuali altre riduzioni): costo (istruzioni host per
   campione; istruzioni pi32v2 dal disassemblato del ciclo interno) e differenza dal riferimento
   `REF_sa_A2_large_2x` (spettro di lungo periodo a terzi d'ottava, centroide nel tempo, spettrogrammi affiancati,
   WAV A/B alla stessa accordatura). Tabella costo/qualità.
4. **Robustezza in float**: tutta l'estensione di Sa (F#2–G4), tutte le posizioni del filo, esecuzioni lunghe:
   nessuna instabilità, overflow, click; energia che non cresce.
5. Iterazione all'ascolto con ElectroNick.

### Verifica di completamento
- Test di regressione del tanpura (hash, costo, robustezza) nel `run_tests.sh`.
- WAV ≥ 30 s per Pa, Ma, Ni; Sa diversi; jawari leggero e marcato; confronti A/B; tabella costo/qualità.
- Costo stimato entro il 35 % (o proposta motivata di usare il secondo core, se serve).

### Report step 2b (2026-11-06) — **STOP: in attesa dell'ascolto e di una decisione di ElectroNick**

**Fatto**: `tanpura/tstring.[ch]` (la corda), `tanpura/tanpura.[ch]` (4 corde, ciclo, irregolarità, corpo,
interpolatore, parametri §7), `tanpura/body_coefs.h` e `upsample_coefs.h` (generati da `tools/reference/body_iir.py`
e `upsample_fir.py`). Strumenti: `tools/test/tanpura_var.c`, `tstring_dump.c`, `tanpura_stress.c`,
`tools/reference/compare2b.py`, `rate_test.py`, `level_grid.py`, `deliver2b.py`. `make test`: autotest del
riferimento, stress, regressione (5 scenari del tanpura in più): tutto ok. Il firmware compila (103 KB, RAM 64 KB).

**Riduzioni, una alla volta** (Sa A2; "spettro" = scarto RMS a terzi d'ottava dal riferimento d'oro fino a ~7 kHz)

| Passo | Modi | Aggiornamenti/campione | Costo stimato FM-1 | Qualità |
|---|---|---|---|---|
| float, 44,1 kHz, tutti i modi | 638 | 638 | ~300 % | 2,7 dB (tutte le bande) |
| troncamento a 8 kHz | 323 | 323 | ~155 % | ≈ uguale |
| metà frequenza, 8 kHz | 323 | 162 | ~77 % | uguale sotto 10 kHz |
| troncamento per corda 6/8/8/3 kHz | 252 | 126 | ~67 % | uguale (2,0–2,4 dB) |
| ciclo interno 39 → 31 istruzioni | 252 | 126 | **54–70 %** | identico bit per bit |

**Tabella costo/qualità delle varianti finali** (Sa A2; costo = 31,6 istruzioni per modo e passo + contatto, corpo,
interpolatore, a 1,0–1,3 cicli per istruzione: **da misurare sul dispositivo**)

| Variante | Aggiornamenti/campione | Costo stimato | Spettro (≤7 kHz) | Errore del centroide | Errore dell'inviluppo |
|---|---|---|---|---|---|
| **Q** qualità 6/8/8/3 kHz | 126 (142 con Sa F#2) | **54–70 %** (61–79 % a F#2) | 2,65 dB | 74 Hz | 1,1 dB |
| M intermedio 4/5/5/2 kHz | 84 | 37–48 % | 4,58 dB | 190 Hz | 2,7 dB |
| E economico 3/4/4/1,5 kHz | 66 | **30–39 %** | 5,21 dB | 183 Hz | 2,2 dB |

Q è vicino al riferimento (spettrogrammi affiancati); E ha un fruscio tra 4 e 10 kHz (con pochi modi il contatto col
ponte diventa impulsivo).

**Robustezza** (`tanpura_stress`): Sa da F#2 a G4 × jawari 0/50/100 × prima corda Pa/Ma/Ni (40 s ciascuno),
riaccordature ogni 0,4 s mentre suona, 10 minuti di fila, una corda per 60 s: nessun valore non finito, nessuna
saturazione, energia mai crescente (scende a ~10⁻²⁶), passo massimo tra campioni 0,2 del fondo scala.

**Non va ancora / limiti**
- **Budget**: Q supera il 35 % di un core; E ci sta. I cicli per istruzione dell'FM-1 per questo codice non sono
  noti: la misura vera (`plat_cpu_pct`) richiede il dispositivo (step 3).
- Sopra ~10 kHz il tanpura del firmware non ha contenuto (il riferimento ne ha −43 dB): un po' meno "aria".
- Q è un po' più scuro del riferimento tra 4 e 6 kHz (−4…−6 dB).

**Domande per ElectroNick**
1. **Ascolto**: `AB_reference_then_Q_every_8s.wav` (riferimento e firmware alternati ogni 8 s): senti la differenza?
   E tra Q, M ed E (`01`, `02`, `03`)?
2. **Decisione: secondo core.** Q non entra nel 35 % di un core. Propongo di **far girare il tanpura sul secondo core**
   (oggi inutilizzato; fm1-base lo supporta): il primo core resta libero per tabla, taal, tono e interfaccia. Il costo
   reale si misura allo step 3. In alternativa: E (o M) su un solo core, con il suono più ruvido che senti nei file.
3. Gli altri file (Ma, Ni, Sa diversi, jawari leggero/marcato) vanno bene?

### Decisione di ElectroNick (2026-11-06)
- **Secondo core con Q.** Registrata in DECISIONS.md e CLAUDE.md §6 (budget del tanpura: 85 % del core 1). Il test di
  regressione ora verifica la stima contro questo budget (peggior caso Sa F#2: 61–79 %). L'uso effettivo del secondo
  core si implementa allo step 3.
- Ancora aperti: l'ascolto (domande 1 e 3) e la chiusura dello step.

### Ascolto A/B di ElectroNick (2026-11-06) e correzione
- "Q è piuttosto buono, ma sento bene la differenza: il riferimento è meno scuro e ha alte frequenze molto più pulite."
- **Diagnosi** (`tools/reference/diagnose2b.py`, un fattore alla volta, livelli per terzo d'ottava rispetto al
  riferimento): la causa principale è la **metà frequenza** (−5 dB a 5–6 kHz e +17 dB di sporco vicino a 10 kHz anche
  con tutti i modi); il troncamento dei modi scurisce un po' 5–8 kHz; il corpo IIR non c'entra (±1 dB). Le Sa a
  frequenza piena e le altre corde a metà non risolvono (lo sporco viene anche dalle corde a metà). Tutto a frequenza
  piena con tutti i modi combacia, ma costa ~160 % di un core: fuori portata anche col secondo core.
- **Correzione**: un filtro a campana a 5,5 kHz (+6 dB, Q 1,1) alla metà frequenza, ~5 istruzioni per campione:
  tutte le bande fino a 8 kHz entro ~1,5 dB dal riferimento (prima fino a −5,9 dB). Non toglie la ruvidità residua
  delle alte (che viene dalla metà frequenza). Ora è il default; test aggiornati (cambio voluto del suono).
- File: `AB_1_reference.wav`, `AB_2_Q_as_before.wav`, `AB_3_Q_brighter.wav` (stesso volume), e l'alternanza con pause.

**Domanda per ElectroNick**: AB_3 (corretto) è meglio di AB_2? È abbastanza vicino al riferimento da chiudere lo step 2b,
o vuoi che continui a lavorare sulla pulizia delle alte?

### Risposta di ElectroNick (2026-11-06)
- Non sente differenza tra AB_2 e AB_3 → **correzione spenta per default**, Q resta come al primo ascolto (AB_2).
  Test aggiornati, file rigenerati (01–10 = default). Pulizia delle alte: **proposta rimandata** (DECISIONS.md).
- Domanda sul Sa per flauti diversi: risposta data (vedi sotto, "Punto aperto" dello step 2a sul registro).
- **In attesa della conferma esplicita di chiusura dello step 2b.**

---

## Step 2a — Tanpura, modello di riferimento — **chiuso** (ElectroNick, 2026-11-06)

### Materiale
- Paper: `docs/references/vanwalstijn-dafx16-tanpura.pdf` (letto per intero).
- Dalla pagina degli autori (copia su web.archive.org, 2017-02-13), in `docs/references/dafx16a/`:
  risposte all'impulso **misurate** del corpo al ponte e al capotasto (`hb.wav`, `hn.wav`, 2048 campioni),
  esempi sonori della corda C3 (`ex1` normale x_c = 5 mm, `ex2` senza rigidità, `ex3` senza smorzamento,
  `ex4` senza ponte, `ex5a`/`ex5c` filo a 4 e 8 mm, `ex6` senza filo), segnali `fd fn pd pn`, derivazioni A–C.
  Il drone a 4 corde degli autori è solo su SoundCloud: non scaricato.

### Piano
1. `tools/reference/tanpura_ref.c`: il modello del paper **completo, in double**, separato da `dsp/` e `tanpura/`:
   modi fino a Nyquist (eq. 16–20), coefficienti esatti (33), aggiornamento vettoriale (40–42), forze di contatto
   ponte/filo in forma chiusa (43–48, Fig. 4), pizzico (14–15), forza al capotasto (22), forze motrici (13, 56),
   energia numerica (49) per il controllo di stabilità. Uscita: segnali grezzi (F_d, F_n, y_b) a qualunque fs.
2. `tools/reference/analyze.py` (numpy/scipy/matplotlib, venv `~/.venvs/teentaal`): radiazione con le risposte
   all'impulso misurate (come nel paper; l'IIR è compito dello step 2b), sovracampionamento 2× → decimazione,
   WAV, spettrogrammi, centroide spettrale, confronti con gli esempi degli autori.
3. Verifiche (riprodurre il paper): f1 = 130,81 Hz e 133 modi sotto 20 kHz (C3, Tabella 1); compressione al ponte
   < 3·10⁻⁸ m (§4.1); forme d'onda di y_b come Fig. 7 e di F_d/F_n come Fig. 8; EI = 0 → niente precursore;
   energia mai crescente senza eccitazione; 44,1 kHz contro sovracampionato (Fig. 9); drone G2-C3-C3-C2 con
   pizzichi a 0; 1,2; 1,8; 2,4 s (Fig. 11); confronto con ex1/ex2/ex5a/ex5c/ex6 degli autori.
4. Deliverable in `renders/step-2a/`: Do3 singola (filo a 4, 5, 6, 8 mm; senza rigidità; senza filo), pattern a
   4 corde (quello del paper e uno con Sa A3 per il bansuri di ElectroNick), versioni 2×, spettrogrammi, report.

### Verifica di completamento
- Le verifiche del punto 3 sono riportate con numeri e figure, con gli scostamenti spiegati.
- STOP: ElectroNick ascolta e confronta.

### Report step 2a (2026-11-06) — **STOP: in attesa dell'ascolto di ElectroNick**

**Fatto**: `tools/reference/tanpura_ref.c` (modello completo in double, con autotest `-selftest`),
`tools/reference/analyze.py` (radiazione, WAV, figure, confronti), `renders/step-2a/` (11 WAV, 10 figure,
`results.json`, README). Comandi:
```
gcc -std=c99 -O2 -o build-host/tanpura_ref tools/reference/tanpura_ref.c -lm && build-host/tanpura_ref -selftest
~/.venvs/teentaal/bin/python tools/reference/analyze.py
```

**Verifiche**
| Verifica | Risultato |
|---|---|
| Stabilità: energia senza perdite, con contatto col ponte (8862 passi) | costante a 2·10⁻¹² (eq. 50) |
| Energia con perdite | mai crescente |
| y_b dalla soluzione del contatto vs stato modale | 10⁻¹⁸ m |
| g (eq. 20) vs integrale numerico | 10⁻⁹ |
| Do3, Tabella 1: f1, tensione, EI | 130,811 Hz; 33,08 N (paper 33,1); 6,03·10⁻⁵ |
| Modi sotto 20 kHz (paper: 133) | **133** |
| Compressione massima al ponte (paper: < 3·10⁻⁸ m) | 2,1·10⁻⁸ m |
| Fig. 7 (y_b): arco iniziale, poi pacchetto ad alta frequenza che rallenta | sì, ampiezze 0–20 µm come nel paper |
| Fig. 8: il suono radiato cresce nei primi ~500 ms | sì (polarità di F_d opposta: convenzione, non udibile) |
| Fig. 9: a 44,1 kHz il segnale del ponte è troppo forte sopra 15 kHz | +10,7 dB vs 8×; con 2×: +1,7 dB (il paper: 2× basta) |
| Fig. 11: formanti discendenti, jvari continuo tra le corde | sì |
| Esempi degli autori (spettro di lungo periodo, scarto RMS) | normale 1,2 dB; filo 4 mm 1,2; 8 mm 1,0; senza rigidità 0,9; senza smorzamento 0,8 |
| Tempo di discesa del jvari (centroide al 60 %) | normale 1,38 s (autori 1,40); filo 4 mm 1,26 s (1,30) |

**Scostamenti dal paper** (dettagli in DECISIONS.md)
- "Senza filo" degli autori non riprodotto (ricetta non documentata; non ci riguarda).
- Segnale del capotasto a 44,1 kHz attenuato sopra 15 kHz (effetto della media a n+½, inudibile).
- Parametri delle corde G2, C2 e del pattern in A3: non dati dal paper, scelti da me (DECISIONS.md).
- Il drone degli autori è solo su SoundCloud: confronto diretto non fatto.

**Utile per lo step 2b**: a 44,1 kHz le corde del pattern in A3 hanno 95–147 modi ciascuna (≈455 in tutto);
il budget del tanpura (~2860 cicli/campione) ne permette molti meno: la riduzione sarà il lavoro principale.

**Domande per ElectroNick**
1. **Ascolto**: `03_drone_sa_A3_Pa.wav` (il tuo Sa) e `02_drone_paper_G2_C3_C3_C2.wav`. Ti sembra un tanpura?
   Cosa manca o cosa disturba? (Nota: sinistra = ponte, destra = capotasto; in cuffia il timbro dei due lati è
   diverso, è voluto come nel paper.)
2. **Filo** (jvari): ascolta `01_c3_xc4`, `xc5`, `xc6`, `xc8`. Quale ti piace? È il parametro "jawari" della UI.
3. **Ciclo e corde**: ho messo un ciclo di 4 s e un tanpura femminile (corde di 0,85 m) per il Sa A3. Vuoi un
   ciclo diverso? Vuoi che il pattern in A3 usi corde più lunghe (suono più "maschile", più grave e ricco)?
4. **Sovracampionamento**: senti differenze tra `03_drone_sa_A3_Pa.wav` e la versione `_2x_oversampled`? Se no,
   allo step 2b possiamo puntare direttamente alla versione a 44,1 kHz.

### Primo ascolto di ElectroNick (2026-11-06)
1. "Sembra decisamente un tanpura, ma il suono evolve come se qualcuno girasse il cutoff di un Minimoog, e il timbro
   ricorda un'onda a dente di sega del Minimoog. Il decadimento forse è un po' troppo veloce."
2. Filo: **6 mm**. 3. Suono **più profondo e più ricco**. 4. Il sovracampionato **sembra più ricco** → il riferimento
   per lo step 2b è la versione 2×.

### Analisi e secondo giro (`renders/step-2a/round2/`, README lì)
- Decadimento: il suono radiato scende di 20 dB in ~1 s qualunque sia lo smorzamento: lo determina il jvari stesso
  (il ponte trasferisce energia verso parziali alte che si smorzano presto). La forza del pizzico non conta: con
  la legge di contatto del paper (lineare, ponte a quota 0) il modello è invariante di scala. Allungano il suono:
  pizzico a dito + σ₀ 0,3 + r_c 0,6 → −20 dB in 1,7 s, −30 dB in 2,7 s (prima 0,95 e 1,6 s).
- "Minimoog": il pizzico del paper eccita solo il primo modo; la corda parte da un seno e diventa un dente di sega
  con un formante discendente. Varianti: pizzico a dito, irregolarità umana, Sa leggermente scordati.
- Più profondo: tanpura grande con Sa A2 (variante E).
- Codice: `tanpura_ref` accetta l'ampiezza per singolo pizzico (`-p I:T:A`); il pizzico di default resta identico
  bit per bit; autotest ok. Script: `tools/reference/round2.py`.

### Domande per ElectroNick (secondo giro)
1. Ascolta A → B → C → D: quali modifiche migliorano, quali no? Lo "sweep" è ancora troppo da synth?
2. E (Sa A2, tanpura grande) contro D (Sa A3): quale preferisci come suono di base per il tuo flauto in La?
3. Se lo sweep resta il problema principale, il passo successivo uscirebbe dal modello del paper (contatto
   distribuito sul ponte, le due polarizzazioni della corda: estensioni proposte dagli autori stessi), con un
   costo maggiore anche per il firmware. Vuoi che ci provi, o il carattere attuale è accettabile?

### Secondo ascolto di ElectroNick (2026-11-06)
- A come prima; B meglio (note più lunghe); C decisamente meglio ma forse "troppo forte troppo a lungo"; D piuttosto
  buono; **E molto buono**. Preferenza leggera per E (Sa A2), vanno bene entrambi. **Il carattere attuale è accettabile.**

### Riferimento finale (`renders/step-2a/final/`, README lì)
- Impostazioni: tanpura grande Sa A2 (corde 1,1 m), filo 6 mm, pizzico a dito, σ₀ 0,4 e r_c 0,8 (−20 dB a ~3,3 s,
  tra B e C), irregolarità umana. **Riferimento d'oro per lo step 2b: `REF_sa_A2_large_2x.wav`.**
- Per lo step 2b: a 44,1 kHz il tanpura grande ha ~520 modi in tutto (≈135 per corda), contro un budget di
  ~2860 cicli/campione: la riduzione dei modi sarà decisiva.

### Punto aperto (non urgente: riguarda gli step 3 e 8)
Con il tanpura su A2 e il flauto su A4, il tono di riferimento (oggi ottave −1…+1 attorno al Sa del tanpura)
non arriva al Sa del flauto. Proposta: il Sa resta quello del suonatore (A3/A4) e il tanpura ha un'impostazione
"registro" (normale / grande = un'ottava sotto); oppure si allarga l'ottava del tono a −1…+2. Da decidere allo step 3.

### Domanda per ElectroNick
- **Posso chiudere lo step 2a** con questo riferimento e passare allo step 2b (riduzione per il firmware)?

---

## Step 1 — Scheletro — **chiuso** (ElectroNick, 2026-11-06)

### Risposte di ElectroNick alla chiusura, e cosa è cambiato
1. Timbro: seno puro. — nessun cambiamento.
2. **Temperamento equabile** — `dsp/tuning.c` ora usa 100 cents a semitono; test e hash aggiornati;
   `renders/step-1/05_swaras_equal_temperament_sa_Csharp3.wav` rigenerato (esatto a 0,001 cents).
3. Varianti di re e ni: non più rilevanti con il temperamento equabile.
4–5. **Bansuri in La e in Sol** — Sa di default **A3 (220 Hz)**; estensione del Sa medio F#2–G4 confermata per i
   bansuri. Nuovi WAV 08 (Sa A3 → tar A4 = 440 Hz) e 09 (Sa G3 → tar G4 = 392 Hz). Convenzione assunta, da
   confermare: il Sa del bansuri in La è A4 e tanpura e tono stanno un'ottava sotto (DECISIONS.md).
6. La 415–466 Hz: ok. 7–11. Controlli: risposte in `docs/CONTROLS.md` (PLAY/SEQ/ARP sì; tono solo a tasto premuto;
   cambio di taal subito; salvataggio con SAVE; niente tap tempo).

Firmware: quello provato in fm1-emu (`renders/step-1/teentaal-step1.fwsc`, `a6a91137…`) è precedente a 2 e 4.
La build attuale (`build/teentaal.fwsc`, `fa287da6…`, test e simulatore ok) **non è consegnata**: non è stata
provata nell'emulatore. Il prossimo firmware da provare sarà quello dello step 3.

### Piano originale dello step 1

### Piano
1. **Base**: copia di fm1-base `317cb86` (bsp `5b0879b`) in `platform/fm1-base/` (esportata con `git archive`, nessuna
   operazione git nel progetto). La radice del progetto è la "APP_DIR" di fm1-base: `fm1.toml`, `app.c`, `app.h`,
   `words.tsv`. `app.c` è solo colla tra piattaforma, mixer, moduli e UI.
2. **Moduli** (§6) con l'interfaccia comune `*_init / *_set_param / *_render(int32_t *out, 64)`:
   `tanpura/`, `tabla/` vuoti (rendono silenzio, nessun parametro); `taal/` vuoto (solo l'header degli eventi, nessun
   suono); `reftone/` completo; `mixer/` (somma, volumi con rampa, saturazione Q23, copia stereo, MASTER);
   `dsp/` (tabella del seno, accordatura: riferimento La, Sa in cents, swara, ottava → frequenza).
3. **Tono di riferimento**: seno da tabella con interpolazione, fase a 32 bit; parametri: swara, ottava, volume, on/off;
   accordatura condivisa (La di riferimento, Sa in cents, cents fini). Nessun click: rampa di volume a ogni blocco,
   dissolvenza in entrata e in uscita, fase continua al cambio di nota.
4. **Tool host `tools/render`** (gcc, stesso DSP): scrive WAV a 44117,647 Hz... *nota*: l'header WAV vuole un intero →
   scritto a 44118 Hz (scarto di 0,008 %, 0,1 cents), documentato.
5. **Test di regressione `tools/test/run_tests.sh`**: scenari fissi → hash FNV-1a 64 dei campioni (`golden.txt`),
   istruzioni host per campione (`cpu_baseline.txt`, tolleranza +25 %) con stima del costo sul target rispetto al
   budget; più verifiche numeriche (frequenza misurata del tono, assenza di salti tra campioni, livello).
6. **Firmware**: UI provvisoria minima solo per provare il tono (non è la proposta di UI); build host (simulatore con
   scenario) e target; prova nell'emulatore online di shcherbakov7 (fatta da ElectroNick).
7. **Proposta di mappatura dei controlli**: solo documento, `docs/CONTROLS.md`.

### Verifica di completamento
- `tools/test/run_tests.sh` passa (hash, costo, controlli numerici); scenario del simulatore passa.
- WAV in `renders/step-1/`: Sa a diverse accordature (cents), swara diversi, ottave, cambio di nota e volume senza click.
- `.fwsc` provato da ElectroNick nell'emulatore online.
- `docs/CONTROLS.md` scritto.

### Report step 1 (2026-11-06) — **in attesa della prova nell'emulatore e dell'ascolto di ElectroNick**

Nota: il PC si è spento a metà lavoro. Al riavvio tutti i sorgenti erano integri; solo il binario
`build-host/regress` era vuoto (0 byte) ed è stato ricompilato.

**Fatto**
- Struttura §6: `dsp/` (costanti, accordatura, seno, rampe), `reftone/`, `tanpura/`, `tabla/`, `taal/` (vuoti,
  interfaccia comune), `mixer/` (mixer + `engine`, l'unico percorso audio), `ui/` (schermata di prova), `data/`
  (vuota), `tools/` (`render`, test), `platform/fm1-base/` (copia fissata).
- Tool host: `make` → `build-host/render OUT.wav SECONDI [T:nome=valore ...]` (eventi in `tools/common/scen.h`).
- Test: `tools/test/run_tests.sh` → **regress ok (0 failed)**, stabile su più esecuzioni. 9 scenari: hash,
  costo, frequenza (errore ≤ 0,0003 cents), armoniche (≤ −135 dB), click, silenzio, saturazione.
- Firmware: `tools/fw.sh` → `build/teentaal.fwsc` (copia in `renders/step-1/teentaal-step1.fwsc`, sha256
  `ee052a66…a3b97`). Tutti i controlli di fm1-base ok. Simulatore: `tools/fw.sh --host` → scenario
  `tools/test/fw_basic.txt` ok (PLAY, Sa ±semitono/cent, swara, ottava, volume, La, suono presente e assente).
- WAV: `renders/step-1/` (7 file, elenco e misure nel README). Verifica indipendente con `tools/test/wavcheck.py`:
  passi di cents e intervalli giusti esatti a 0,001 cents.
- Proposta dei controlli: `docs/CONTROLS.md`.
- CLAUDE.md §4: ingressi corretti (7 encoder, 14 pulsanti, MASTER; non 2 encoder e 2 ruote).

**Misure**
| Voce | Valore | Budget |
|---|---|---|
| CPU, tutto il percorso audio (tono + mixer + moduli vuoti) | ~71 istruzioni host/campione → **~1,05 % di un core** (stima) | 5 % (taal, reftone, mixer) |
| CPU a riposo (tutto spento) | ~56 istr./campione → ~0,8 % | |
| Flash: app | 94.652 B (piattaforma ~79 KB + nostro ~16 KB) | slot 581.564 B |
| RAM `.data+.bss` | 29.848 B | 96 KB |

**Emulatore (2026-11-06)**: ElectroNick ha approvato la modifica B e ha provato il firmware in fm1-emu: **suono e
controlli funzionano**. `renders/step-1/teentaal-step1.fwsc` = build del progetto, sha256 `a6a91137…839f5`.

**Non ancora fatto / limiti**
- **Emulatore scelto da ElectroNick (2026-11-06): fm1-emu di joakimeriksson**,
  https://joakimeriksson.github.io/fm1-emu/?fw=local&mhz=240 (non quello di shcherbakov7). Con fm1-base attuale
  resta muto (step 0). Opzioni: (A) fissare fm1-base a `3c4a57e`; (B) modifica locale di una riga in
  `bsp/boards/fm1/fm1_audio.h`: il controllo del PLL audio lascia passare anche il PLL mai impostato (entrambe le
  parole a 0), stato che solo un emulatore senza bootloader produce; sul dispositivo nessun cambiamento.
  **Scelta B, approvata da ElectroNick e applicata** (DECISIONS.md).
- La schermata del firmware è provvisoria; nessun salvataggio delle impostazioni (step 8).

**Rischi dell'emulatore solo online** (domanda di ElectroNick)
1. *Costo CPU*: nessun emulatore online misura i cicli reali. Mitigazione: stima su host con margine ampio
   (oggi 1 % contro 5 %) e, prima dello step 3 (tanpura, il modulo pesante), una misura sul dispositivo.
2. *Differenze dal dispositivo*: un emulatore può mancare pezzi dell'hardware (come il PLL audio) o essere più
   permissivo. Superare l'emulatore non garantisce il dispositivo; è una prova necessaria, non sufficiente.
3. *Le parti rischiose per il brick non sono emulate*: aggiornamento e scrittura in flash vengono da fm1-base, usata
   da altri firmware sui dispositivi; noi non le tocchiamo.
4. *Prova manuale*: ogni prova la fai tu, quindi è più lenta e meno ripetibile. Lo compensano i test automatici
   su host e il simulatore di fm1-base, che girano a ogni build.
5. *Privacy*: i due emulatori dichiarano che il firmware resta nel browser.
Nessuno di questi rischi è grave per gli step 1–2 (quasi tutto su host).

**Domande per ElectroNick**
1. **Ascolto**: i WAV vanno bene come tono di riferimento? Il seno puro su uno speaker piccolo si sente poco nel
   registro grave (Sa ~140 Hz): vuoi provare una variante con qualche armonica leggera, o resta seno puro?
2. **Intonazione degli swara**: giusta (5 limiti, come nei WAV) o temperamento equabile? Per re e ni: 16/15 e 9/5
   (come ora) o 256/243 e 16/9?
3. **Sa di default**: ho messo C#3 (138,6 Hz) provvisoriamente. Qual è il tuo Sa abituale? Estensione prevista per
   il Sa: da F#2 a G4 (92–392 Hz). Va bene?
4. **La di riferimento**: 415–466 Hz a passi di 0,1 Hz. Va bene?
5. **Controlli**: le cinque domande in fondo a `docs/CONTROLS.md`.
6. ~~Emulatore~~: fatto, suono e controlli ok.

---

## Step 0 — Ricognizione — **chiuso** (approvato da ElectroNick il 2026-11-06)
Risposte finali: nessuna segnalazione pubblica (issue) ai progetti esterni; emulatore online preferito.

### Esito finale nell'emulatore (2026-11-06, fm1-emu di joakimeriksson, probe su fm1-base `3c4a57e`)
- Avvio, schermo, K1/SEL: OK. Tono a 440 Hz udibile.
- `RENDERS/S` = **172** (= 44117,6 / 256): percorso audio OK, blocco 256 confermato.
- `FPU MAIN` = `FPU AUDIO` = **5B28687C** = valore host → **FPU verificata**, bit per bit, anche nell'interrupt audio.
- `CPU %` = 2 con 8 oscillatori Q15. È il modello di temporizzazione dell'emulatore (clock emulato di default a 240 MHz),
  non una misura del dispositivo; ordine di grandezza coerente con la stima (8 × 0,2–0,4 % più l'overhead della piattaforma).
- **Causa del silenzio con fm1-base attuale, confermata**: fm1-emu non modella il PLL audio (0x119A0/0x119A4 leggono 0,
  perché li imposta solo il bootloader), e il controllo aggiunto in felucca-bsp il 2026-11-05 rifiuta l'avvio dell'audio
  in silenzio. fm1-emulator di simonjohansson precarica invece i valori stock (`src/hw/pi32v2/fm1-system.c:366`).
  Sull'FM-1 reale il controllo passa.

### Risposte di ElectroNick (2026-11-06)
1. Emulatore: lo prova ElectroNick nell'emulatore online.
2. float32: OK. 3. Interfaccia int32 Q23: OK. 4. Blocco di piattaforma 256.
5. Secondo core: lasciato a me → in riserva, si parte con un core (DECISIONS.md).
6. Nessuna operazione git per ora.

### Prova del probe nell'emulatore (da fare: ElectroNick) — versione 2
**Prima prova (2026-11-06)**: avvio OK, K1 e SEL funzionano, ma `CPU %` resta a 0. Causa: fm1-base misura la CPU
cronometrando il blocco audio con TIMER4 (24 MHz), e l'emulatore browser non fa avanzare quel timer insieme alle
istruzioni eseguite. Non è un errore del firmware, ma la misura non è disponibile nell'emulatore. Il probe ora
mostra due prove che non dipendono dal timer:
- `RENDERS/S` ≈ **172** (44117/256): il percorso audio gira davvero
- `FPU MAIN` e `FPU AUDIO` = **5B28687C** (valore calcolato su PC con lo stesso codice): la FPU esegue correttamente
  i calcoli float, sia nel programma principale sia nell'interrupt audio

**Seconda prova (2026-11-06, emulatore https://joakimeriksson.github.io/fm1-emu/?fw=local&mhz=60)**: le righe FPU
non comparivano (errore mio: la tela di disegno ha al massimo 144 righe, ne avevo chieste 190) e `RENDERS/S` = 0,
quindi il codice audio non girava. Il percorso audio di fm1-base non ha condizioni che lo saltino; ipotesi: il clock
emulato a 60 MHz o l'audio del browser non avviato. Corretti anche due errori del probe: l'uscita float si
arrotondava a zero, e gli oscillatori Q15 leggermente scordati battevano e si cancellavano a vicenda. Ora entrambe
le modalità producono un tono udibile a ~−18 dBFS (440 Hz Q15, ~220 Hz float), verificato su host; la schermata è
in due strisce, con `RENDERS` (contatore grezzo).

Versione attuale: `tools/step0/cpuprobe/build/cpuprobe.fwsc`, sha256 `297852bc…a2821`.

**Terza prova (2026-11-06, senza mhz=60)**: `FPU MAIN` = **5B28687C**, identico all'host → **FPU verificata**
(nel contesto principale). `RENDERS` = 0, `FPU AUDIO` = 0, nessun suono; altri firmware nello stesso emulatore
suonano. Diagnosi: felucca-bsp dal 2026-11-05 (`e7a22e3`, `4d39207`) controlla che il PLL audio (0x119A0/0x119A4)
abbia i valori lasciati dal bootloader stock e altrimenti **rifiuta l'avvio dell'audio in silenzio**, mentre
`main_fm1.c` di fm1-base ignora l'esito. Ipotesi: l'emulatore carica l'app senza bootloader e il PLL non ha quei
valori. I firmware che suonano sono stati compilati prima di quel controllo.
Verifica proposta: `tools/step0/cpuprobe-old.fwsc` (sha256 `98b60971…9ebae`) è lo stesso probe compilato su
fm1-base `3c4a57e` (bsp `41a6e86`, prima del controllo). Se questo suona e l'altro no, l'ipotesi è confermata.
Conseguenze da discutere: il problema riguarda l'emulatore, non l'FM-1 reale (i valori stock sono stati letti da
dispositivi veri), ma noi dobbiamo provare tutto nell'emulatore → serve una soluzione (vedi domande dopo la prova). La misura reale della CPU
resta da fare sul dispositivo, in un momento successivo e solo se ElectroNick lo vuole.

Istruzioni della prima versione (ancora valide per avvio, K1 e SEL):
- File: `tools/step0/cpuprobe/build/cpuprobe.fwsc` (identità FM-1_7700)
- Emulatore: https://shcherbakov7.github.io/m-wave-fm1-browser-emulator/ → pulsante «Свой файл» (il tuo file)
- Cosa controllare:
  1. avvio: splash, poi la schermata con `Q15 TABLE+LERP`, `OSC 8`, `CPU %`, `MHZ`, `BLOCK 256`
  2. K1 cambia il numero di oscillatori (0–128); SEL alterna `Q15 TABLE+LERP` / `FLOAT RESONATOR`
  3. annota `CPU %` con OSC = 0, 32, 64, 128 per entrambi i modi (nell'emulatore il valore riflette le istruzioni
     eseguite, non i cicli reali, ma è utile come confronto)
  4. la modalità FLOAT funziona senza blocchi o crash → conferma che le istruzioni FPU vengono eseguite
  5. (opzionale) la riga «% реального» sotto lo schermo indica la velocità dell'emulatore
- **Non installarlo sull'FM-1**: serve solo a chiudere lo step 0. Se vuoi una misura reale della CPU sul dispositivo,
  ne parliamo prima (backup della flash, V15 a portata).

### Piano (scritto all'inizio dello step)
1. Leggere awesome-fm-1 e individuare fm1-base, felucca-bsp, mvave-fm1, Felucca, FM-1-RE.
2. Clonare fm1-base, verificare licenza e stato.
3. Installare toolchain JieLi e SDK AC79 nella versione indicata dalla base.
4. Compilare host e target.
5. Far girare l'emulatore.
6. Misurare: core, FPU, RAM/flash liberi, frequenza e blocco reali, costo CPU di un oscillatore di prova.
7. Aggiornare §4 e §6 di CLAUDE.md.

Criteri: build host e target riuscite, emulatore avviato, tabella misure con il metodo di ciascun valore,
licenza verificata, nessuna funzione musicale.

---

## Report step 0 (2026-11-06)

### 1. Base: fm1-base — adatta, si consiglia di adottarla
| Voce | Esito |
|---|---|
| Repository | https://github.com/charlesvestal/fm1-base, clonato in `refs/fm1-base` (commit del 2026-11-06, 175 commit) |
| Licenza | **MPL-2.0** file per file. Il nostro codice può avere qualunque licenza; le modifiche ai file di fm1-base vanno pubblicate MPL-2.0. Nessun problema. |
| Stato | **Beta**, molto attivo (ultimo commit oggi). L'interfaccia `include/plat.h` e `fm1.toml` possono cambiare prima della 1.0 → **va fissata a un commit** (submodule). Usata da CTL-1, Radio, LOOM-8, LUOGU, TRIO, DelRio e altri. |
| BSP | felucca-bsp (MPL-2.0) come submodule `bsp/`, commit `5b0879b` |
| Flash nuova (UC25HQ80) | **Supportata**: `device/fm1_flash_ok.h` accetta sia Puya P25Q80 (0x856014) sia UC25HQ80 (0xB36014), con test. Era l'avvertimento principale di awesome-fm-1. |
| Aggiornamento | App-only: il pacchetto contiene app + loader OTA; bootloader e SPL non vengono toccati. Il pacchetto risponde ancora all'update (si può sempre tornare a V15). |

### 2. Toolchain e SDK — installati e verificati
| Voce | Valore |
|---|---|
| Toolchain | JieLi `jieli-linux-toolchains-20260730.1`, clang 4.0.1 pi32v2, in `~/.jieli/toolchain` (da `tools/get_toolchain.sh`), nativo su WSL, niente Docker |
| SDK | `fw-AC79_AIoT_SDK` tag `AC79NN_SDK_V1.2.1_2023-12-13` in `~/fw-AC79_AIoT_SDK`; SHA-256 di `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` **identici** a quelli attesi da `tools/build.py` |
| Python | serve `fontTools` solo per alcuni font (la build del template funziona anche senza) |
| Emscripten | non installato: serve solo per l'emulatore browser di fm1-base (`--web`), non indispensabile |

Comandi (da `refs/fm1-base`):
```
export JIELI_TOOLCHAIN=~/.jieli/toolchain AC79_SDK=~/fw-AC79_AIoT_SDK
python3 tools/build.py APP_DIR --host     # simulatore host
python3 tools/build.py APP_DIR            # APP_DIR/build/NAME.fwsc
```

### 3. Build eseguite
| Build | Esito |
|---|---|
| template, host | OK; scenario `template/tests/basic.txt` superato (13 s simulati, render 0,3% del tempo reale) |
| template, target con BLE | OK, app 225 KB, `.fwsc` 615 KB |
| template, target `--no-ble` | OK, app 87,7 KB |
| probe CPU (`tools/step0/cpuprobe`), host | OK, scenario superato |
| probe CPU, target | OK, app 79 KB, `cpuprobe.fwsc` identità FM-1_7700 (ricompilato con blocco 256: sha256 `ff774ee9…2030c`). **Non provato nell'emulatore → non va installato.** |

Le build target eseguono controlli automatici: nessuna routine soft-double, nessun accesso diretto ai registri fuori da felucca-bsp, limiti di RAM/flash.

### 4. Misure hardware
Legenda metodo: **[M]** misurato da me; **[S]** letto dal sorgente della base, a sua volta misurato su FM-1 dagli autori; **[C]** calcolato.

| Voce | CLAUDE.md diceva | Valore verificato | Metodo |
|---|---|---|---|
| CPU | 240 MHz | **360 MHz** (bootloader M-VAVE); 320 MHz sotto m-bootloader 1.2. La base riporta il clock a 360 MHz all'avvio in ogni caso | [S] `bsp/chip/fm1_clock.h`, misurato su FM-1 il 2026-10-23 |
| Core | da verificare | **2 core**. Il secondo è utilizzabile tramite fm1-base (`cpu2 = true`, `plat_cpu2_run`/`wait`), da avviare solo in `app_init`, con fallback a un core. Usato da Melodee, X0X, Album | [S] `device/cpu2.c`, `plat.h` |
| FPU | probabilmente assente | **Presente, singola precisione.** clang emette istruzioni FPU native (`r4 = r7 * r6 (f)`, `ftoi`) con `-mcpu=r3 -mfprev1`, senza chiamate soft-float; la build di fm1-base verifica "no soft-double routines (single-precision FPU only)". Il firmware stock usa soft-float per scelta del suo compilatore, non per mancanza di hardware | [M] disassemblato di `tools/step0/osc_probe.c`; [S] flag della base. Esecuzione su silicio: da confermare con emulatore/dispositivo |
| Double | — | **Niente double**: solo float a 32 bit (i double sarebbero emulati in software, lentissimi; la build li rifiuta) | [S] |
| Frequenza | 44118 Hz | **44117,647 Hz** = 24 MHz / 544 (non 44100) | [S] `bsp/boards/fm1/fm1_audio.h` |
| Formato uscita | int16 mono | **stereo interleaved, Q23 in int32** (24 bit), via codec I2S esterno | [S] |
| Blocco | 64 campioni | **Configurabile** (`APP_BLOCK`); default di fm1-base **256** (5,8 ms). Il probe gira con 64 | [S], [M] build |
| Flash: slot app | ~580 KB | **581.564 B** (0x8DFBC) | [S] `tools/fm1pkg_make.py` |
| Flash: piattaforma | — | ~79 KB (probe, senza BLE, con audio USB e screen reader) → **~490 KB liberi per la nostra app** | [M] build |
| Flash: area dati | "molto limitata" | **292 KB** (0x97000–0xDFFFF, condivisa con lo store delle impostazioni) + 16 KB (XDATA0) + 92 KB (XDATA1) | [S] `plat.h` |
| RAM `.data+.bss` | da misurare | 96 KB totali, 26,6 KB usati dalla piattaforma → **~70 KB liberi** | [M] build |
| RAM pool (buffer grandi) | — | 336 KB totali, 69 KB usati → **~267 KB liberi** | [M] build |
| RAM per codice veloce | — | 32 KB (`.ram_text`), 5 KB usati | [M] build |
| Cicli per campione | — | 360 MHz / 44117,6 Hz = **8160 cicli per campione per core** | [C] |

### 5. Costo CPU dell'oscillatore di prova
Due oscillatori sinusoidali di prova (`tools/step0/osc_probe.c`, gli stessi nel probe firmware):

| Oscillatore | Istruzioni pi32v2 per campione (ciclo interno, dal disassemblato) | Istruzioni host x86 per campione (contatori del kernel) | Stima su target |
|---|---|---|---|
| Q15, tabella 1024 + interpolazione lineare | 16 | 20,6 | ~16–30 cicli ≈ **0,2–0,4 %** di un core |
| float, risonatore a 2 poli | 9 | 12,4 | ~9–20 cicli ≈ **0,1–0,25 %** |
| float, biquad (filtro del corpo) | 18 | 19,4 | ~18–35 cicli ≈ **0,2–0,4 %** |

- Il passaggio da istruzioni a cicli (1–2 cicli/istruzione, cache della flash XIP) è **una stima**. La misura vera serve dal dispositivo: il probe mostra `plat_cpu_pct()` (quota del tempo di blocco usata dal render) variando il numero di oscillatori con K1 e passando Q15/float con SEL.
- **L'emulatore non misura cicli**: conta 8 ns per istruzione per convenzione (`fm1-emulator/docs/architecture.md`). Dà quindi il numero di istruzioni sul target, ma non il costo reale.
- **Metodo di regressione su host funzionante**: `perf_event_open` (contatore di istruzioni) funziona sotto WSL2 → possiamo copiare il controllo di Felucca (istruzioni per campione, tolleranza +25 %, hash FNV-1a dei render).

Conseguenza per il tanpura (ordine di grandezza): il 35 % di un core sono ~2860 cicli per campione. Con 4 corde che suonano insieme e ~12–20 cicli per modo, stanno ~35–60 modi per corda su un core. Il paper usa tutti i modi fino a Nyquist (per una corda a ~130 Hz, oltre 150). Quindi: riduzione dei modi, oppure secondo core. Lo valuteremo allo step 2b.

### 6. Bloccato: l'emulatore
Il passaggio "far girare l'emulatore" **non è stato fatto**. Il sistema di permessi ha negato lo scaricamento e l'esecuzione del binario precompilato di [fm1-emulator](https://github.com/simonjohansson/fm1-emulator) (emulatore QEMU di pi32v2, con modalità `--headless`). Non ho cercato di aggirare il blocco. Opzioni nella sezione domande.

### 7. Differenze rispetto a CLAUDE.md da decidere (vedi domande)
- **Virgola mobile vs fissa.** CLAUDE.md prevede la virgola fissa "salvo che lo step 0 dimostri una FPU utilizzabile". La FPU c'è (verifica statica completa; manca solo la prova di esecuzione).
- **Interfaccia dei moduli.** La piattaforma chiede `app_render(int32_t *lr, n, master)` stereo Q23. L'interfaccia di §6 (`int16_t *out`, somma in `out`) farebbe saturare la somma a 16 bit.
- **Blocco 64 vs 256.** Per uno strumento di pratica la latenza conta poco.

### Domande per ElectroNick
1. **Emulatore.** Come preferisci? (a) Mi autorizzi a scaricare ed eseguire il binario precompilato di fm1-emulator (Linux x86-64, licenza GPL-3.0) in `~/fm1-emu`. (b) Lo compiliamo dai sorgenti con `mise` (scarica comunque toolchain e QEMU da Internet). (c) Lo provi tu nell'[emulatore browser](https://github.com/shcherbakov7/m-wave-fm1-browser-emulator), che esegue i `.fwsc` senza modifiche. Senza emulatore non posso chiudere lo step 0, né consegnare nessun `.fwsc`.
2. **Float o virgola fissa?** Propongo **float32 nel DSP di tanpura e tabla** (con `-ffp-contract=off` su host e target, come fa fm1-base, per avere gli stessi campioni) e **virgola fissa solo dove conviene** (oscillatori a tabella, mixer). Motivo: il modello del paper con contatto risolto in forma chiusa è molto più robusto in float, e la riduzione dello step 2b si concentra sul numero di modi invece che sulla stabilità numerica. Va bene?
3. **Interfaccia dei moduli.** Propongo `mod_render(m, int32_t *out, n)`, che somma in un buffer mono Q23 a 32 bit (spazio per sommare senza saturare). Il mixer satura e copia sui due canali; `n` = blocco interno di 64, chiamato 4 volte per blocco di piattaforma da 256. Va bene, o preferisci tenere `int16_t`?
4. **Blocco.** 256 di piattaforma (default fm1-base, meno overhead) con sotto-blocchi di 64 per i moduli, oppure 64 anche per la piattaforma?
5. **Secondo core.** Lo teniamo come riserva per il tanpura (step 2b/3) o preferisci restare su un core per semplicità?
6. **Repository e licenza.** Posso inizializzare git in questa cartella, con fm1-base come submodule fissato al commit di oggi? Che licenza vuoi per il nostro codice (es. MPL-2.0 come la base, GPL-3.0, privata)?
7. **Nota di sicurezza**, da ricordare prima di ogni installazione: tieni a portata il V15 ufficiale (`FM-1_015.fwsc`, link in awesome-fm-1) e fai il backup della flash prima del primo firmware custom. Quando cambi firmware custom, passa sempre da V15: farlo direttamente ha già mandato in brick alcuni dispositivi.

### Note
- I repository di riferimento sono in `refs/` (fm1-base, felucca-bsp, Felucca, FM-1-RE, mvave-fm1, fm1-emulator): solo consultazione, da escludere dal nostro git. Le cartelle `IndianMusic-fm1` e `sloop-fm1` non sono state usate, come richiesto.
- In awesome-fm-1 esiste **DelRio** (fm1-base, MPL-2.0), che include un drone di tanpura. Non l'ho guardato né usato; lo segnalo solo perché esiste.
