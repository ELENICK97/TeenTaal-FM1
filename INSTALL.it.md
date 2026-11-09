# Installare TeenTaal sull'FM-1

> Versione italiana. English: [INSTALL.md](INSTALL.md).

> [!WARNING]
> TeenTaal è **generato con l'IA** (Claude) ed è stato **provato solo negli emulatori, mai su un FM-1 vero**: la
> prima installazione è anche la prima prova reale. Un aggiornamento non riuscito può rendere l'FM-1 non avviabile.
> **Lo fai a tuo rischio** (vedi il [Disclaimer](README.md#disclaimer)).

L'installazione scrive **solo l'area dell'applicazione** dell'FM-1: il bootloader di M-VAVE resta com'è, quindi si
può tornare al firmware ufficiale con l'updater di M-VAVE (vedi sotto).

## Prima di cominciare

1. **Il firmware ufficiale V15 a portata di mano**: `FM-1_015.fwsc` (il link è in
   [awesome-fm-1](https://github.com/cicloid/awesome-fm-1)). Serve per tornare indietro.
2. **Un cavo USB per dati** (non solo di ricarica).
3. **Un computer che vede l'FM-1 come dispositivo USB-MIDI**: Windows, macOS o Linux (non WSL, che non arriva
   direttamente ai dispositivi USB). Chiudi le altre app che usano il MIDI (editor, DAW, altri installer, schede del
   browser con Web MIDI).
4. Se sull'FM-1 c'è già un **altro firmware non ufficiale**, rimetti prima il V15 ("Tornare al firmware ufficiale")
   e installa TeenTaal da lì: passare direttamente da un firmware non ufficiale a un altro ha già reso inutilizzabili
   alcuni FM-1.
5. L'FM-1 **carico** o alimentato dall'USB per tutta la durata. **Non staccare il cavo** durante la scrittura.

## Installare (l'installer di fm1-base, da riga di comando)

L'installer è un piccolo programma Python che fa parte di fm1-base, dentro questo repository. Fa lo stesso
aggiornamento di una pagina d'installazione web: porta l'FM-1 in modalità di aggiornamento, scrive il pacchetto e
controlla che l'FM-1 si sia riavviato con il firmware nuovo.

1. **Scarica il progetto**: su <https://github.com/ELENICK97/TeenTaal-FM1>, **Code → Download ZIP**, e scompattalo.
2. **Scarica il firmware**: dalle [Release](https://github.com/ELENICK97/TeenTaal-FM1/releases), il file
   `teentaal-1.0.fwsc`. Mettilo nella cartella del progetto scompattato. Puoi controllare che sia il file pubblicato:
   il suo SHA-256 deve essere quello scritto nella release (Windows: `certutil -hashfile teentaal-1.0.fwsc SHA256`;
   macOS e Linux: `shasum -a 256 teentaal-1.0.fwsc`).
3. **Installa Python 3** se non ce l'hai (<https://www.python.org/downloads/>; su Windows spunta "Add python.exe to
   PATH" nell'installazione).
4. **Installa le due librerie MIDI** che l'installer usa. Apri un terminale (Windows: Prompt dei comandi o PowerShell)
   ed esegui:
   - Windows: `py -m pip install mido python-rtmidi`
   - macOS / Linux: `python3 -m pip install mido python-rtmidi`
5. **Collega l'FM-1**: accendilo con il firmware che ha adesso e collegalo con il cavo USB.
6. **Avvia l'installer** dalla cartella del progetto (prima entra nella cartella con `cd`):
   - Windows: `py platform\fm1-base\tools\fm1_install.py teentaal-1.0.fwsc`
   - macOS / Linux: `python3 platform/fm1-base/tools/fm1_install.py teentaal-1.0.fwsc`

   Trova l'FM-1 e chiede conferma ("Install? … [y/N]"): rispondi `y`. Poi scrive il firmware e l'FM-1 si riavvia da
   solo. Aspetta che l'installer dica che ha finito, senza staccare niente nel frattempo.
7. Compare la schermata di TeenTaal (Sa, taal, tanpura, tono).

Viene scritta solo l'area dell'applicazione: questo installer non installa m-bootloader e non tocca il bootloader.

Se l'FM-1 non viene trovato, l'installer elenca le porte MIDI che vede con `fm1_install.py --info`; se ne sceglie
una con `--port NOME`.

## La pagina d'installazione web (non ancora disponibile)

Una pagina d'installazione che chiede solo Chrome o Edge e un cavo USB, senza Python, può essere pubblicata su
<https://elenick97.github.io/TeenTaal-FM1/install/>. **Non è ancora online**: finché non lo è, usa l'installer qui
sopra. Chi sviluppa può prepararla e servirla in locale dai sorgenti (Linux o WSL con il toolchain di fm1-base, vedi
[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)): `tools/site.sh 1.0`, poi <http://localhost:8791/install/>. Quella pagina
non offre m-bootloader (`tools/site.sh` lo toglie apposta).

## Tornare al firmware ufficiale

- Con l'updater di M-VAVE, **M-UPGRADE** (sito M-VAVE, Downloads → PC Software), e il file `FM-1_015.fwsc`.
- Oppure, quando la pagina d'installazione web sarà online, con il suo pulsante **"Back to the stock firmware"**.

Le impostazioni salvate da TeenTaal possono andare perse tornando al firmware ufficiale.

## Se qualcosa va storto

- **L'installer non trova l'FM-1**: controlla il cavo (deve essere per dati), chiudi le altre app MIDI, riprova;
  `--info` elenca le porte che vede.
- **`No module named mido` o `rtmidi`**: il punto 4 non è stato fatto per il Python che stai usando; ripetilo.
- **L'FM-1 è rimasto in modalità di aggiornamento** (installazione interrotta): avvia di nuovo l'installer, che
  completa la scrittura. L'FM-1 resta in modalità di aggiornamento finché la scrittura non è completa.
- **L'FM-1 non si avvia più**: non provare altri strumenti a caso. Il recupero può richiedere un adattatore hardware;
  chiedi prima negli Issues del repository o nella comunità dell'FM-1
  ([awesome-fm-1](https://github.com/cicloid/awesome-fm-1)).

## Provare senza installare

- Nel browser, dai sorgenti: `tools/web.sh` (WSL o Linux con Emscripten), poi **<http://localhost:8790/>**: è lo
  stesso codice del firmware, compilato per il browser.
- Il pacchetto vero nell'emulatore fm1-emu (scrive un WAV, niente audio dal vivo):
  `tools/emu.sh release/teentaal-1.0.fwsc 8000 '2600:PLAY' /tmp/prova`.
