# Credits, sources and licences

TeenTaal-FM1 is by **ElectroNick** ([github.com/ELENICK97](https://github.com/ELENICK97)), released under the
[Apache License 2.0](LICENSE). If you use it or build on it, keep the [NOTICE](NOTICE) file and cite the repository:
<https://github.com/ELENICK97/TeenTaal-FM1> (a [CITATION.cff](CITATION.cff) is provided).

The code was written by an AI coding assistant (Claude, by Anthropic) under ElectroNick's direction and review.

## What is inside the firmware and this repository

| Part | Author | Licence | Where | What you must do when you redistribute |
|---|---|---|---|---|
| This project's own code, data and documents | ElectroNick | Apache-2.0 | everything outside `platform/` | keep `LICENSE` and `NOTICE` (credit to ElectroNick) |
| **fm1-base** (platform layer, build tools, installer page, simulators) | Charles Vestal and contributors ([CREDITS](platform/fm1-base/CREDITS.md)) | MPL-2.0 (some parts Apache-2.0, BSD-2-Clause; fonts SIL OFL 1.1: [LICENSING.md](platform/fm1-base/LICENSING.md)) | `platform/fm1-base/` | keep its licence files; publish the source of any file of it you change (MPL-2.0) |
| **felucca-bsp** (board support for the FM-1) | Leo Kuroshita (Hugelton Instruments) and contributors | MPL-2.0 | `platform/fm1-base/bsp/` | as above |
| One local change to felucca-bsp | ElectroNick (this project) | MPL-2.0 | `bsp/boards/fm1/fm1_audio.h`, listed in [platform/FM1-BASE-VERSION.txt](platform/FM1-BASE-VERSION.txt) | as above |
| **Tabla recordings**: the Freesound pack "tabla bols" (26 strokes) | [mmiron](https://freesound.org/people/mmiron/) | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) (public domain) | `docs/references/tabla_mmiron/wav/` (originals), `tabla/tabla_samples.h` (trimmed, ADPCM) | nothing required; credit is given here as a courtesy |

The pinned versions: fm1-base `317cb86`, felucca-bsp `5b0879b` ([platform/FM1-BASE-VERSION.txt](platform/FM1-BASE-VERSION.txt)).
Upstream: <https://github.com/charlesvestal/fm1-base>, <https://github.com/hugelton/felucca-bsp>.

## Scientific reference: the tanpura

The tanpura is a reduced version of the model in:

> M. van Walstijn, J. Bridges and S. Mehes, **"A Real-Time Synthesis Oriented Tanpura Model"**, in *Proceedings of
> the 19th International Conference on Digital Audio Effects (DAFx-16)*, Brno, Czech Republic, 2016.

Please cite this paper if you describe or reuse the tanpura synthesis. The paper itself is not included in this
repository; it is available from the DAFx proceedings. The reference implementation in `tools/reference/` and the
firmware version in `tanpura/` are this project's own code, written from the paper.

## Theka sources (the taals)

Each file in `data/taal/` names its sources. They are: D. Courtney, [chandrakantha.com](https://chandrakantha.com);
Wikipedia ([Jhoomra](https://en.wikipedia.org/wiki/Jhoomra), [Dhamar](https://en.wikipedia.org/wiki/Dhamar_(music)));
NIOS, *Hindustani Music, practical book 2*, ch. 4 (Chautaal);
[tablatheka.com](https://www.tablatheka.com). Thekas vary between schools: the ones chosen are listed in
`docs/DEVELOPMENT.md`.

## Tools used, not distributed here

- **fm1-emu** by Joakim Eriksson (<https://github.com/joakimeriksson/fm1-emu>): the emulator every firmware package was
  tested in.
- The JieLi toolchain and AC79 SDK (Apache-2.0), the compilers for the FM-1, installed separately (fm1-base's
  instructions).
- Hardware and protocol knowledge from the community: [awesome-fm-1](https://github.com/cicloid/awesome-fm-1),
  [FM-1-RE](https://github.com/AL-255/FM-1-RE).

M-VAVE and FM-1 are trademarks of their owners; this project is not affiliated with or endorsed by M-VAVE.
