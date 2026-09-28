#!/bin/bash
# Serve Source/ui/public with js/juce/index.js swapped for the stub.
# The page under test is otherwise BYTE-IDENTICAL to production — no import
# map, no edited HTML — so what renders here is what renders in the WebView.
#
# No separate preset-manager.js copy step is needed: O-Bitrot vendors the
# module at Source/ui/public/modules/preset-manager.js, which the resource
# provider serves at that same path, so the tree copy below already covers it.
#
# Usage: tests/ui-stub/serve-stub.sh [port] [root]
set -euo pipefail

PORT="${1:-8742}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLIC="$(cd "$HERE/../../Source/ui/public" && pwd)"
ROOT="${2:-$(mktemp -d)/ui-stub-root}"

rm -rf "$ROOT"
mkdir -p "$ROOT"
cp -R "$PUBLIC"/. "$ROOT"/
cp "$HERE/juce-stub.js" "$ROOT/js/juce/index.js"
# v1.17.0 (R5): the bundled face is module-embedded, outside PUBLIC — place it
# where the resource provider serves it (/css/, /fonts/) or the page 404s it.
EBG="$(cd "$HERE/../../../../modules/ui/eb-garamond" && pwd)"
mkdir -p "$ROOT/css" "$ROOT/fonts"
cp "$EBG/css/eb-garamond.css" "$ROOT/css/"
cp "$EBG"/fonts/EBGaramond-{Regular,Italic,Bold}.woff2 "$ROOT/fonts/"

echo "Serving $ROOT on http://localhost:$PORT"
exec python3 -m http.server "$PORT" --directory "$ROOT" --bind 127.0.0.1
