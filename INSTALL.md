# Installing TeenTaal on the FM-1

> [!WARNING]
> TeenTaal is **AI-generated** and has been **tested only in emulators, never on a real FM-1**. The first installation
> is also its first real test. A failed update can leave an FM-1 unable to start. **You do this at your own risk**
> (see the [Disclaimer](README.md#disclaimer)).

The installation writes **only the FM-1's application area**: M-VAVE's bootloader is left as it is, so you can always
go back to the official firmware the same way you came (see below).

## Before you start

1. **Have the official V15 firmware at hand**: `FM-1_015.fwsc` (the link is in
   [awesome-fm-1](https://github.com/cicloid/awesome-fm-1)). You need it to go back.
2. **A USB data cable** (not a charge-only one).
3. **Chrome or Edge** on a computer (Web MIDI: Firefox and Safari do not have it). Close other apps and tabs that use
   MIDI (editors, other installers).
4. If the FM-1 runs **another unofficial firmware**, put the official V15 back first ("Going back" below) and install
   TeenTaal from there: going straight from one unofficial firmware to another has bricked some FM-1s.
5. The FM-1 **charged** or powered over USB for the whole install. **Do not unplug the cable** while it writes.

## Way 1: the installer page (recommended)

1. Open the installer page in Chrome or Edge:
   - online, if the repository's page is published: **<https://elenick97.github.io/TeenTaal-FM1/install/>**
   - or locally, from the source (Linux or WSL with fm1-base's toolchain, see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)):
     `tools/site.sh 1.0`, then **<http://localhost:8791/install/>** (Ctrl-C stops it when you are done).
2. Switch on the FM-1 with its current firmware and connect it to the computer.
3. Allow the browser's access to MIDI devices when it asks.
4. Press **Install**. The page checks the package, puts the FM-1 in update mode and writes the firmware; the FM-1
   restarts by itself. Wait until the page says it has finished.
5. The TeenTaal screen appears (Sa, taal, tanpura, tone). If the page reports a problem, **"Report a problem"** shows
   the details to send (open an Issue in the repository).

The page built by this project **does not offer m-bootloader** (`tools/site.sh` removes it on purpose): m-bootloader
would replace the FM-1's own bootloader, which this project never does.

## Way 2: the command line (fm1-base's installer)

With Python, `mido` and `python-rtmidi`, on a computer that sees the FM-1 as a USB-MIDI device:

```bash
python3 platform/fm1-base/tools/fm1_install.py teentaal-1.0.fwsc
```

(The package is attached to the [Releases](https://github.com/ELENICK97/TeenTaal-FM1/releases); check its sha256
against the release notes.)

## Going back to the official firmware

- On the installer page: **"Back to the stock firmware"**, choosing the `FM-1_015.fwsc` file you downloaded.
- Or with M-VAVE's own updater, **M-UPGRADE** (M-VAVE's site, Downloads → PC Software).

TeenTaal's saved settings may be lost when you go back.

## If something goes wrong

- **The page does not find the FM-1**: check the cable (a data cable), close other MIDI apps, press Install again.
- **The FM-1 stayed in update mode**: plug the cable in again and press Install again; it stays in update mode until
  the write is complete.
- **The FM-1 does not start any more**: do not try other tools at random. Recovery may need a hardware adapter; ask
  in the repository's Issues or in the FM-1 community ([awesome-fm-1](https://github.com/cicloid/awesome-fm-1))
  first.

## Trying it without installing

- In the browser, from the source: `tools/web.sh` (WSL or Linux with Emscripten), then **<http://localhost:8790/>**:
  the same code as the firmware, compiled for the browser.
- The real package in the fm1-emu emulator (writes a WAV, no live sound):
  `tools/emu.sh release/teentaal-1.0.fwsc 8000 '2600:PLAY' /tmp/try`.
