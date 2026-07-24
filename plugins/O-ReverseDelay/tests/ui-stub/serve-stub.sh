#!/bin/bash
# Serve Source/ui/public with js/juce/index.js swapped for the ~100-line stub.
# The page under test is otherwise BYTE-IDENTICAL to production — no import map,
# no edited HTML — so what renders here is what renders in the WebView.
#
# Usage: tests/ui-stub/serve-stub.sh [port] [root]
set -euo pipefail

PORT="${1:-8731}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLIC="$(cd "$HERE/../../Source/ui/public" && pwd)"
ROOT="${2:-$(mktemp -d)/ui-stub-root}"

rm -rf "$ROOT"
mkdir -p "$ROOT"
cp -R "$PUBLIC"/. "$ROOT"/
cp "$HERE/juce-stub.js" "$ROOT/js/juce/index.js"

echo "Serving $ROOT on http://localhost:$PORT"
exec python3 -m http.server "$PORT" --directory "$ROOT" --bind 127.0.0.1
