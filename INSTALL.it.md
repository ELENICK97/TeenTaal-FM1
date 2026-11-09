# Installare TeenTaal sull'FM-1

> Versione italiana. English: [INSTALL.md](INSTALL.md). Firmware generato con l'IA (Claude) e provato solo negli emulatori: si usa a proprio rischio (vedi il Disclaimer in [README.md](README.md#disclaimer)).

Questa procedura scrive il firmware **solo nell'area dell'applicazione** dell'FM-1: il bootloader di M-VAVE resta
quello che è. Il firmware è stato provato soltanto negli emulatori (fm1-emu e quello del browser), mai su un FM-1
vero: la prima installazione è anche la prima prova sul dispositivo.

## Prima di cominciare

1. **Il firmware ufficiale V15 a portata di mano**: `FM-1_015.fwsc` (il link è in
   [awesome-fm-1](https://github.com/cicloid/awesome-fm-1)). Serve per tornare indietro.
2. **Un cavo USB per dati** (non solo di ricarica).
3. **Chrome o Edge su Windows** (servono le Web MIDI, che Firefox non ha). Chiudi le altre app e schede che usano
   il MIDI (editor, altri installer).
4. Se sull'FM-1 c'è già un **altro firmware non ufficiale**, rimetti prima il V15 (vedi "Tornare al firmware
   ufficiale") e installa TeenTaal da lì: passare direttamente da un firmware non ufficiale a un altro ha già reso
   inutilizzabili alcuni FM-1.
5. L'FM-1 **carico** o alimentato dall'USB per tutta la durata. **Non staccare il cavo** durante la scrittura.

## Preparare la pagina d'installazione (in WSL)

Nella cartella del progetto, in Ubuntu (WSL):

```bash
tools/site.sh 1.0
```

Lo script compila il firmware di release, prepara la pagina d'installazione di fm1-base in `build/site` e la mette a
disposizione solo su questo computer. Alla fine scrive l'impronta sha256 del pacchetto e resta in ascolto: lascialo
aperto finché non hai finito, poi fermalo con **Ctrl-C**.

La pagina che prepara **non offre m-bootloader** (lo script lo toglie apposta): m-bootloader sostituirebbe il
bootloader dell'FM-1, cosa che questo progetto non fa.

## Installare (Chrome o Edge su Windows)

1. Accendi l'FM-1 con il firmware ufficiale e collegalo al PC con il cavo USB.
2. Apri **http://localhost:8791/install/**.
3. Quando il browser chiede l'accesso ai dispositivi MIDI, consentilo.
4. Premi **Install**. La pagina controlla il pacchetto, porta l'FM-1 in modalità di aggiornamento e scrive il
   firmware; l'FM-1 si riavvia da solo. Aspetta che la pagina dica che ha finito.
5. All'avvio compare la schermata di TeenTaal (Sa, taal, tanpura, tono). Se la pagina segnala un problema,
   **"Report a problem"** mostra i dati da mandare.

## Tornare al firmware ufficiale

- Dalla stessa pagina: **"Back to the stock firmware"**, scegliendo il file `FM-1_015.fwsc` scaricato.
- Oppure con l'updater di M-VAVE, **M-UPGRADE** (sito M-VAVE, Downloads → PC Software).

Le impostazioni salvate da TeenTaal possono andare perse tornando al firmware ufficiale.

## Se qualcosa va storto

- **La pagina non trova l'FM-1**: controlla il cavo (deve essere per dati), chiudi le altre app MIDI, premi di
  nuovo Install.
- **L'FM-1 è rimasto in modalità di aggiornamento**: ricollega il cavo e premi di nuovo Install; ci resta finché
  la scrittura non è completa.
- **L'FM-1 non si avvia più**: non insistere con altri strumenti. Il recupero richiede un adattatore hardware;
  scrivimi prima di fare altro.

## Provare senza installare

- Nel browser, da WSL: `tools/web.sh`, poi **http://localhost:8790/** in Chrome (Ctrl-C per fermarlo). È lo stesso
  codice del firmware, compilato per il browser.
- Il pacchetto vero nell'emulatore fm1-emu (senza audio dal vivo, scrive un WAV):
  `tools/emu.sh release/teentaal-1.0.fwsc 8000 '2600:PLAY' /tmp/prova`.
