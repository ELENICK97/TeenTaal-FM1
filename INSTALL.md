# Installing TeenTaal on the FM-1

> [!WARNING]
> TeenTaal is **AI-generated** and has been **tested only in emulators, never on a real FM-1**. The first installation
> is also its first real test. A failed update can leave an FM-1 unable to start. **You do this at your own risk**
> (see the [Disclaimer](README.md#disclaimer)).

The installation writes **only the FM-1's application area**: M-VAVE's bootloader is left as it is, so you can go back
to the official firmware with M-VAVE's own updater (see below).

## Before you start

1. **Have the official V15 firmware at hand**: `FM-1_015.fwsc` (the link is in
   [awesome-fm-1](https://github.com/cicloid/awesome-fm-1)). You need it to go back.
2. **A USB data cable** (not a charge-only one).
3. **A computer that sees the FM-1 as a USB-MIDI device**: Windows, macOS or Linux (not WSL, which cannot reach
   USB devices directly). Close other apps that use MIDI (editors, DAWs, other installers, browser tabs with Web MIDI).
4. If the FM-1 runs **another unofficial firmware**, put the official V15 back first ("Going back" below) and install
   TeenTaal from there: going straight from one unofficial firmware to another has bricked some FM-1s.
5. The FM-1 **charged** or powered over USB for the whole install. **Do not unplug the cable** while it writes.

## Installing (fm1-base's installer, from the command line)

The installer is a small Python program that comes with fm1-base, inside this repository. It does the same update
as an installer web page: it puts the FM-1 in update mode and writes the package, then checks that the FM-1 restarted
with it.

1. **Download the project**: on <https://github.com/ELENICK97/TeenTaal-FM1>, **Code → Download ZIP**, and unzip it.
2. **Download the firmware**: from the [Releases](https://github.com/ELENICK97/TeenTaal-FM1/releases), the file
   `teentaal-1.0.fwsc`. Put it in the unzipped project folder. You can check that it is the published file: its
   SHA-256 must be the one in the release notes (Windows: `certutil -hashfile teentaal-1.0.fwsc SHA256`; macOS and
   Linux: `shasum -a 256 teentaal-1.0.fwsc`).
3. **Install Python 3** if you do not have it (<https://www.python.org/downloads/>; on Windows, tick "Add python.exe
   to PATH" in the installer).
4. **Install the two MIDI libraries** the installer uses. Open a terminal (Windows: Command Prompt or PowerShell) and
   run:
   - Windows: `py -m pip install mido python-rtmidi`
   - macOS / Linux: `python3 -m pip install mido python-rtmidi`
5. **Connect the FM-1**: switch it on with its current firmware and connect it with the USB cable.
6. **Run the installer** from the project folder (`cd` into it first):
   - Windows: `py platform\fm1-base\tools\fm1_install.py teentaal-1.0.fwsc`
   - macOS / Linux: `python3 platform/fm1-base/tools/fm1_install.py teentaal-1.0.fwsc`

   It finds the FM-1, shows what it is about to write and asks for confirmation; answer `y`. Then it writes the
   firmware; the FM-1 restarts by itself. Wait until the installer says it has finished, and do not unplug anything
   meanwhile.
7. The TeenTaal screen appears (Sa, taal, tanpura, tone).

Only the FM-1's application area is written: this installer never installs m-bootloader or touches the bootloader.

If the FM-1 is not found, the installer can list the MIDI ports it sees: `fm1_install.py --info`; pick one with
`--port NAME`.

## The installer web page (not available yet)

An installer page that needs only Chrome or Edge and a USB cable, with no Python, can be published at
<https://elenick97.github.io/TeenTaal-FM1/install/>. **It is not online yet**: until it is, use the installer above.
Developers can build and serve it locally from the source (Linux or WSL with fm1-base's toolchain, see
[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)): `tools/site.sh 1.0`, then <http://localhost:8791/install/>. That page
does not offer m-bootloader (`tools/site.sh` removes it on purpose).

## Going back to the official firmware

- With M-VAVE's own updater, **M-UPGRADE** (M-VAVE's site, Downloads → PC Software), and the `FM-1_015.fwsc` file.
- Or, once the installer web page is online, its **"Back to the stock firmware"** button.

TeenTaal's saved settings may be lost when you go back.

## If something goes wrong

- **The installer does not find the FM-1**: check the cable (a data cable), close other MIDI apps, run it again;
  `--info` lists the ports it sees.
- **`No module named mido` or `rtmidi`**: step 4 was not done for the Python you are running; repeat it.
- **The FM-1 stayed in update mode** (the install was cut off): run the installer again; it finishes the write. The
  FM-1 stays in update mode until the write is complete.
- **The FM-1 does not start any more**: do not try other tools at random. Recovery may need a hardware adapter; ask
  in the repository's Issues or in the FM-1 community ([awesome-fm-1](https://github.com/cicloid/awesome-fm-1))
  first.

## Trying it without installing

- In the browser, from the source: `tools/web.sh` (WSL or Linux with Emscripten), then **<http://localhost:8790/>**:
  the same code as the firmware, compiled for the browser.
- The real package in the fm1-emu emulator (writes a WAV, no live sound):
  `tools/emu.sh release/teentaal-1.0.fwsc 8000 '2600:PLAY' /tmp/try`.
