#!/bin/bash
# The web installer for the FM-1 (fm1-base's tools/make_site.py), built and served from WSL on this computer only
# (127.0.0.1), for Chrome or Edge on Windows: http://localhost:8791/install/   (INSTALL.md)
#   tools/site.sh VERSION        e.g. tools/site.sh 1.0: the site for release/teentaal-VERSION.fwsc (built if missing)
#                                in build/site, served until Ctrl-C
# m-bootloader is left out of the site on purpose: it would replace the FM-1's bootloader (CLAUDE.md §3).
# The site (build/site) is also what GitHub Pages can publish (docs/DEVELOPMENT.md, "Publishing a release").
# This script only builds and serves the page: nothing is written to the FM-1 until Install is pressed there.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
V="${1:?usage: tools/site.sh VERSION (e.g. 1.0)}"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}" AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
cd "$ROOT/platform/fm1-base"
if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then                  # the browser emulator for the site's emu/ (optional)
    source "$HOME/emsdk/emsdk_env.sh" >/dev/null 2>&1
    python3 tools/build.py "$ROOT" --web | grep -E "^emu .*wasm" || true
fi
PKG="$ROOT/release/teentaal-$V.fwsc"                        # the package tested in fm1-emu, if there is one
if [ ! -f "$PKG" ]; then
    echo "no $PKG: building it (a new package: run it in fm1-emu, tools/emu.sh, before installing it)"
    python3 tools/build.py "$ROOT" --release "$V" | grep -E "^(package|app) "
    mkdir -p "$ROOT/release" && cp "$ROOT/build/teentaal-$V.fwsc" "$PKG"
fi
rm -rf "$ROOT/build/site"
python3 tools/make_site.py "$ROOT" "$PKG" "$V" "$ROOT/build/site"
rm -f "$ROOT"/build/site/firmware/m-bootloader-*           # (no m-bootloader: the page then does not offer it)
sha256sum "$PKG"
echo "open http://localhost:8791/install/ in Chrome or Edge on Windows (Ctrl-C to stop)"
exec python3 -m http.server --bind 127.0.0.1 8791 -d "$ROOT/build/site"
