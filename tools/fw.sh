#!/bin/sh
# Build the firmware with fm1-base (platform/fm1-base, a pinned copy: platform/FM1-BASE-VERSION.txt).
#   tools/fw.sh            build/teentaal.fwsc (JieLi toolchain, AC79 SDK)
#   tools/fw.sh --host     the host simulator, then runs tools/test/fw_basic.txt
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}" AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
cd "$ROOT/platform/fm1-base"
python3 tools/build.py "$ROOT" "$@"
if [ "$1" = "--host" ]; then
    "$ROOT/build/host/teentaal_host" "$ROOT/tools/test/fw_basic.txt" "$ROOT/build/host"
fi
