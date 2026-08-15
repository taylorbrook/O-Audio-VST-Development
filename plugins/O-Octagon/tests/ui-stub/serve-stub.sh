#!/bin/bash
# Serve Source/ui/public with js/juce/index.js swapped for the bridge stub.
# The page under test is otherwise BYTE-IDENTICAL to production — no import map,
# no edited HTML — so what renders here is what renders in the WebView.
#
# Mechanism copied verbatim from plugins/O-ReverseDelay/tests/ui-stub/serve-stub.sh.
# O-Octagon has no preset-manager.js line to carry at Phase 3.1: the preset store
# is 3.2 work, and the whole UI tree lives under Source/ui/public.
#
# For a human iterating on the page. The automated gate
# (tests/ui_layout_check.js) builds and serves the same tree itself so it does
# not depend on a shell script being running.
#
# Usage: tests/ui-stub/serve-stub.sh [port] [root]
set -euo pipefail

PORT="${1:-8741}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLIC="$(cd "$HERE/../../Source/ui/public" && pwd)"
ROOT="${2:-$(mktemp -d)/ui-stub-root}"

rm -rf "$ROOT"
mkdir -p "$ROOT"
cp -R "$PUBLIC"/. "$ROOT"/
cp "$HERE/juce-stub.js" "$ROOT/js/juce/index.js"

echo "Serving $ROOT on http://localhost:$PORT"
exec python3 -m http.server "$PORT" --directory "$ROOT" --bind 127.0.0.1
