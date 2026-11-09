#!/bin/bash
# Option A (step 3): our firmware's code compiled to WebAssembly (fm1-base's browser emulator), served from WSL on
# this computer only (127.0.0.1), for listening and trying the controls in Chrome: http://localhost:8790/
# Not the .fwsc: the emulator of the real package is fm1-emu (tools/emu.sh).
#   tools/web.sh            build, then serve until stopped (Ctrl-C)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$HOME/emsdk/emsdk_env.sh" >/dev/null 2>&1
"$ROOT/tools/fw.sh" --web | grep "^emu "
echo "open http://localhost:8790/ in Chrome (Ctrl-C to stop)"
exec python3 -m http.server --bind 127.0.0.1 8790 -d "$ROOT/build/emu"
