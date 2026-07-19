#!/bin/bash
# ==============================================================================
# verify-suite-battery.sh — post-JUCE-bump verification battery (resilient sweep)
#
# Gates the whole O-* plugin suite after a JUCE version bump and writes a
# machine-readable per-plugin result TSV. For each plugin it:
#   1. Ensures a FRESH install via scripts/build-and-install.sh (dual-variant
#      sweep + cache clear). Incremental ninja is a near-no-op when the suite
#      was already built at the target JUCE version.
#   2. Runs the AU-link gate scripts/verify-au-link.sh (auval), reused verbatim.
#   3. Runs pluginval on the installed VST3 (strictness 8, --skip-gui-tests).
#      Strictness 8 exercises the thorough state save/restore test that surfaces
#      the JUCE 8.0.11 `var` deep-equality change.
#
# RESILIENCE: every plugin runs in a subshell that NEVER aborts the outer loop.
# Exit codes are captured per-gate. macOS has no `timeout`/`gtimeout`, so a
# portable background-PID + kill watchdog bounds each step.
#
# O-TextureForge is a KNOWN-FAIL fresh build (umappp/irlba transitive drift,
# JUCE-independent, deferred DEF-L26-01). It is recorded as KNOWN-FAIL in every
# column and never built, never "fixed", never allowed to abort the sweep.
#
# Usage:
#   ./scripts/verify-suite-battery.sh                 # full suite
#   ./scripts/verify-suite-battery.sh --plugin O-Bells  # single plugin (re-run)
#   ./scripts/verify-suite-battery.sh --strictness 5    # lower pluginval strictness
#
# Output:
#   .planning/quick/260719-m5p-post-juce-8-0-14-verification-battery/battery-results.tsv
#   Columns: plugin  cmake_target  install_rc  auval_rc  pluginval_rc  note
# ==============================================================================

set -u

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

QUICK_DIR="$REPO_ROOT/.planning/quick/260719-m5p-post-juce-8-0-14-verification-battery"
TSV="$QUICK_DIR/battery-results.tsv"
PLUGINVAL="/Applications/pluginval.app/Contents/MacOS/pluginval"
VST3_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/VST3"

# Per-plugin logs land in scratchpad (kept out of the repo + out of the caller's
# context). Override with BATTERY_LOGDIR if desired.
LOGDIR="${BATTERY_LOGDIR:-$QUICK_DIR/battery-logs}"
mkdir -p "$LOGDIR"

# Wall-clock guards (seconds).
BUILD_TIMEOUT=420       # build-and-install.sh (fresh install; near-no-op rebuild)
AUVAL_TIMEOUT=120       # verify-au-link.sh
PLUGINVAL_WALL=180      # pluginval overall wall-clock (its own --timeout-ms is 60s/test)
PLUGINVAL_TIMEOUT_MS=60000
STRICTNESS=8

SINGLE_PLUGIN=""
while [ $# -gt 0 ]; do
    case "$1" in
        --plugin)     SINGLE_PLUGIN="$2"; shift 2 ;;
        --strictness) STRICTNESS="$2"; shift 2 ;;
        *) echo "Unknown flag: $1" >&2; exit 2 ;;
    esac
done

# --- portable wall-clock watchdog -------------------------------------------
# run_with_timeout <seconds> <logfile> <cmd...>
# Returns the child's exit code, or 124 if the watchdog killed it.
run_with_timeout() {
    local secs="$1"; shift
    local logfile="$1"; shift
    ( "$@" ) >>"$logfile" 2>&1 &
    local pid=$!
    local waited=0
    while kill -0 "$pid" 2>/dev/null; do
        if [ "$waited" -ge "$secs" ]; then
            kill -9 "$pid" 2>/dev/null
            # kill any lingering children of the timed-out step
            pkill -9 -P "$pid" 2>/dev/null || true
            wait "$pid" 2>/dev/null
            return 124
        fi
        sleep 3
        waited=$((waited + 3))
    done
    wait "$pid"
    return $?
}

# --- resolve juce_add_plugin() target (folder != target for 11/37 plugins) ---
resolve_cmake_target() {
    local cmakelists="plugins/$1/CMakeLists.txt"
    local raw
    raw=$(tr '\n' ' ' < "$cmakelists" \
        | grep -oE 'juce_add_plugin[[:space:]]*\([[:space:]]*[^ )]+' \
        | head -1 \
        | sed -E 's/juce_add_plugin[[:space:]]*\([[:space:]]*//')
    if [[ "$raw" =~ ^\$\{([A-Za-z0-9_]+)\}$ ]]; then
        local var="${BASH_REMATCH[1]}"
        raw=$(grep -oE "set[[:space:]]*\([[:space:]]*${var}[[:space:]]+[^ )]+" "$cmakelists" \
            | head -1 \
            | sed -E "s/set[[:space:]]*\([[:space:]]*${var}[[:space:]]+//")
    fi
    if [ -z "$raw" ] || [[ "$raw" == *'${'* ]]; then
        raw="$1"
    fi
    echo "$raw"
}

# --- resolve installed VST3 product name from build artefacts ----------------
resolve_product_name() {
    local plugin="$1" target="$2"
    local vst3_dir="build/plugins/$plugin/${target}_artefacts/Release/VST3"
    local path=""
    if [ -d "$vst3_dir" ]; then
        path=$(find "$vst3_dir" -maxdepth 1 -name '*.vst3' -type d 2>/dev/null | head -1)
    fi
    if [ -n "$path" ]; then
        local base; base=$(basename "$path"); echo "${base%.vst3}"
    fi
}

# --- one plugin --------------------------------------------------------------
gate_plugin() {
    local plugin="$1"
    local target install_rc auval_rc pluginval_rc note
    target="$(resolve_cmake_target "$plugin")"
    install_rc=""; auval_rc=""; pluginval_rc=""; note=""

    # KNOWN-FAIL: never build, never gate.
    if [ "$plugin" = "O-TextureForge" ]; then
        printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
            "$plugin" "$target" "KNOWN-FAIL" "KNOWN-FAIL" "KNOWN-FAIL" \
            "KNOWN-FAIL: umappp/irlba transitive drift (DEF-L26-01), JUCE-independent; not built" >> "$TSV"
        echo "[battery] $plugin -> KNOWN-FAIL (skipped)"
        return 0
    fi

    local blog="$LOGDIR/${plugin}.build.log"
    local alog="$LOGDIR/${plugin}.auval.log"
    local plog="$LOGDIR/${plugin}.pluginval.log"
    : > "$blog"; : > "$alog"; : > "$plog"

    # 1. Fresh install (build-and-install.sh does dual-variant sweep + cache clear).
    echo "[battery] $plugin -> build-and-install"
    run_with_timeout "$BUILD_TIMEOUT" "$blog" bash scripts/build-and-install.sh "$plugin"
    install_rc=$?
    [ "$install_rc" -eq 124 ] && note="build/install watchdog-timeout"

    if [ "$install_rc" -ne 0 ]; then
        [ -z "$note" ] && note="BUILD/INSTALL-FAIL (rc=$install_rc)"
        printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
            "$plugin" "$target" "$install_rc" "SKIP" "SKIP" "$note" >> "$TSV"
        echo "[battery] $plugin -> install_rc=$install_rc (downstream gates skipped)"
        return 0
    fi

    # 2. AU-link gate (auval) — reuse verify-au-link.sh verbatim.
    echo "[battery] $plugin -> auval"
    run_with_timeout "$AUVAL_TIMEOUT" "$alog" bash scripts/verify-au-link.sh "$plugin"
    auval_rc=$?
    [ "$auval_rc" -eq 124 ] && note="${note:+$note; }auval watchdog-timeout"

    # 3. pluginval gate on the installed VST3.
    local product; product="$(resolve_product_name "$plugin" "$target")"
    local vst3_path=""
    if [ -n "$product" ] && [ -d "$VST3_INSTALL_DIR/$product.vst3" ]; then
        vst3_path="$VST3_INSTALL_DIR/$product.vst3"
    else
        # fall back: glob the dev-suffixed bundle by folder name
        vst3_path=$(ls -d "$VST3_INSTALL_DIR/${plugin}"*-dev.vst3 2>/dev/null | head -1)
        [ -z "$vst3_path" ] && vst3_path=$(ls -d "$VST3_INSTALL_DIR/${plugin}".vst3 2>/dev/null | head -1)
    fi

    if [ -z "$vst3_path" ] || [ ! -d "$vst3_path" ]; then
        pluginval_rc="NOVST3"
        note="${note:+$note; }pluginval skipped: installed VST3 not found (product='$product')"
    else
        echo "[battery] $plugin -> pluginval ($(basename "$vst3_path"))"
        run_with_timeout "$PLUGINVAL_WALL" "$plog" \
            "$PLUGINVAL" --strictness-level "$STRICTNESS" --skip-gui-tests \
                         --timeout-ms "$PLUGINVAL_TIMEOUT_MS" --output-dir "$LOGDIR" \
                         --validate "$vst3_path"
        pluginval_rc=$?
        [ "$pluginval_rc" -eq 124 ] && note="${note:+$note; }pluginval watchdog-timeout"
    fi

    [ -z "$note" ] && note="ok"
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$plugin" "$target" "$install_rc" "$auval_rc" "$pluginval_rc" "$note" >> "$TSV"
    echo "[battery] $plugin -> install=$install_rc auval=$auval_rc pluginval=$pluginval_rc"
    return 0
}

# --- driver ------------------------------------------------------------------
mkdir -p "$QUICK_DIR"

if [ -n "$SINGLE_PLUGIN" ]; then
    # single-plugin re-run: append (do not clobber a full-suite TSV)
    if [ ! -s "$TSV" ]; then
        printf 'plugin\tcmake_target\tinstall_rc\tauval_rc\tpluginval_rc\tnote\n' > "$TSV"
    fi
    gate_plugin "$SINGLE_PLUGIN"
    echo "[battery] single-plugin run complete: $SINGLE_PLUGIN"
    exit 0
fi

# full sweep: fresh TSV with header
printf 'plugin\tcmake_target\tinstall_rc\tauval_rc\tpluginval_rc\tnote\n' > "$TSV"

for cml in $(ls plugins/*/CMakeLists.txt | sort); do
    plugin=$(echo "$cml" | sed -E 's|plugins/([^/]+)/CMakeLists.txt|\1|')
    gate_plugin "$plugin"
done

echo "[battery] sweep complete. Results: $TSV"
