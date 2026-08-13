#!/usr/bin/env bash
# Phase 2.4a R34-pre / Phase 2.4b R35a / Phase 2.4c R36e / Phase 2.6a-bis /
# Phase 2.6b R40d — canonical reproduction of all 17 O-Contrabass render
# goldens. Locks invocations against duration-dependence trap (RESEARCH.md
# §17.10 Risk #10).
# Phase 2.4b extension (PLAN rev-9 pin #3): adds `sub-harmonics` +
# `sub-harmonics-stability` entries; HR-9 IEEE 754 identity arithmetic
# preserves bit-exact regression for the 10 carry-forward goldens at
# SUB_HARMONICS=0 default. Phase 2.4c extension (PLAN rev-10 pin #4): adds
# `saturator-tail-comparison` entry; HR-11 (zero production DSP edits)
# trivially preserves the 12 carry-forward goldens byte-identical.
# Phase 2.6a-bis extension: adds `output-chain` entry covering the master
# output-chain stress harness (sat sweep / limiter ceiling / width sweep /
# clickfree fast automation / peak-overshoot stress).
# Phase 2.6b R40d extension: adds 3 microtonal goldens —
#   `microtonal-12tet` (TuningEngine wire-up at default 12-TET; bit-equivalent
#                       to pre-edit getMidiNoteInHertz path per RP1 algebraic
#                       identity at REFERENCE_PITCH=440)
#   `microtonal-scala` (19-EDO test fixture loaded via loadScalaFile;
#                       exercises the engine's calculateCustomFrequency
#                       lookup → frequencyTable[128] atomic store path)
#   `microtonal-mpe`   (MPE legacy ±24 semitone pitch-bend tracking on
#                       channel 2; verifies notePitchbendChanged Site B
#                       cache re-use per Q17 LOCK)
# Carry-forward bit-equality at TUNING_SYSTEM=12-TET (Gate 8b inv #1):
# all 14 prior goldens preserve byte-identity post-R40 source edits via
# the algebraic identity at default APVTS state (RESEARCH §23.6.6 proof).
# Usage: ./reproduce-goldens.sh              — render to /tmp/repro/, diff vs committed
#        ./reproduce-goldens.sh --quiet      — exit code only, no stdout
#        ./reproduce-goldens.sh --regenerate — RE-BASELINE: overwrite the committed
#                                              goldens + .sha256 from this build.
#                                              Only for an intentional DSP change;
#                                              always verify after (plain re-run).
# Bash 3.2 compatible (macOS system bash) — uses parallel arrays, not associative.
set -euo pipefail

ARG1="${1:-}"
REGEN=""
QUIET=""
case "$ARG1" in
    --regenerate) REGEN=1 ;;
    --quiet)      QUIET=1 ;;
esac
REPO_ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
GOLDEN_DIR="$REPO_ROOT/plugins/O-Contrabass/tests/render-harness/golden"
HARNESS="$REPO_ROOT/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test"
OUTDIR="/tmp/repro"

mkdir -p "$OUTDIR"
cd "$REPO_ROOT/build"

# Build harness if missing.
if [ ! -x "$HARNESS" ]; then
    cmake --build . --target O-Contrabass-render-test --parallel >/dev/null
fi

# Parallel arrays (bash 3.2 compatible). Canonical invocations per RESEARCH §17.1:
# sustain 60 / release 5 default for sustained modes; 3-s notes for note-sequence;
# mode-locked sustain for vibrato/macro/slow/schelleng.
NAMES=(stiffness-zero-pre string-A string-D string-G detune-sweep-A note-sequence voice-recycling crossfade-seed vibrato macro-sweep slow-lfo schelleng-stress sub-harmonics sub-harmonics-stability saturator-tail-comparison output-chain microtonal-12tet microtonal-scala microtonal-mpe note-expression mpe-yz)
INVOCS=(
    "--note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0 --string-stiffness 0"
    "--string A"
    "--string D"
    "--string G"
    "--detune-sweep A"
    "--note-sequence 28:3,33:3,38:3,43:3,28:3"
    # v1.3 voice-recycling regression golden. EIGHT of the SAME note against a
    # 4-voice pool, so notes 5-8 must recycle a voice rather than light a fresh
    # string. This is the probe that catches a dropped note-on: under v1.2.0 the
    # note-ons past the fourth were silently discarded (voice stealing defaulted
    # off and released voices never fell under the energy floor), and segments
    # 5-8 collapsed to 0.0084 RMS against a 0.034 median. The existing
    # `note-sequence` golden CANNOT see this — it plays one note per string, so
    # only its last note-on is dropped and four ringing voices mask it (0.671
    # consistency, a pass). Do not fold these two scenarios together.
    "--note-sequence 40:0.5,40:0.5,40:0.5,40:0.5,40:0.5,40:0.5,40:0.5,40:0.5 --velocity 0.8 --release 4"
    # v1.4 legato string-change golden. Neither `note-sequence` (one note per
    # string, all on fresh voices) nor `voice-recycling` (eight of the SAME note,
    # so the recycled voice keeps its string) ever reaches `needsCrossfade` —
    # both report crossfade_note_ons = 0. JUCE hands a stolen voice back the pitch
    # it last played, so the only way in is to fill the pool on one string and
    # then leap to another, which is what this mode's built-in sequence does.
    # Gates on crossfade_note_ons >= 5 (else the probe is vacuous) AND on the
    # crossfaded notes actually speaking (>= -36 dBFS RMS). Reinstating the v1.2
    # "don't seed across a crossfade" carve-out drops them to -41 dBFS and fails.
    "--crossfade-seed"
    "--vibrato"
    "--macro-sweep"
    "--slow-lfo"
    "--schelleng-stress"
    "--sub-harmonics"
    "--sub-harmonics-stability"
    "--saturator-tail-comparison"
    "--output-chain"
    # Phase 2.6b R40d microtonal goldens (TuningEngine wire-up verification).
    "--microtonal --tuning-system 12tet"
    "--microtonal --tuning-system scala --scl ${REPO_ROOT}/plugins/O-Contrabass/tests/render-harness/fixtures/test-19edo.scl"
    "--mpe-pitch-bend"
    # Phase 2.6c R41d goldens (VST3 Note Expression wire-up + FUNC-05 Y/Z adoption).
    #   note-expression: 3 cells — +0.50-semi seeded offset / exchange-consume
    #                    retrigger / NOTE_EXPRESSION-off gate (D9/D10 proofs).
    #   mpe-yz:          4 segments — baseline / CC74 Y triangle / channel-pressure
    #                    Z triangle / max-Z stress (BOW_PRESSURE=8.0, MAXZ1 cell).
    "--note-expression"
    "--mpe-yz"
)

FAIL=0
TOTAL=0
SKIPPED=0
for i in "${!NAMES[@]}"; do
    g="${NAMES[$i]}"
    invoc="${INVOCS[$i]}"
    # A name with no committed .sha256 is skipped when verifying, but --regenerate
    # MUST be able to create it or a newly-added golden would silently never run
    # (it would sit in NAMES looking covered while contributing nothing, and the
    # summary line would still say "all N reproduce"). Announce the skip loudly.
    if [ ! -f "$GOLDEN_DIR/$g.wav.sha256" ] && [ -z "$REGEN" ]; then
        echo "[SKIP] $g — no committed golden yet; run --regenerate to create it"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi
    wav="$OUTDIR/$g.wav"
    json="$OUTDIR/$g.json"
    # Harness may exit non-zero on quality-gate FAILs (peak / RMS thresholds);
    # WAVs are still bit-deterministic. sha256 is the truth-bar, not exit code.
    $HARNESS $invoc --out "$wav" --json "$json" >/dev/null 2>&1 || true
    TOTAL=$((TOTAL + 1))

    if [ -n "$REGEN" ]; then
        # Re-baseline: promote this render to the committed golden. Driven from the
        # SAME NAMES/INVOCS arrays as verification so the two can never drift — a
        # separate regeneration script would be a fixture mirroring the source.
        cp "$wav"  "$GOLDEN_DIR/$g.wav"
        [ -f "$json" ] && cp "$json" "$GOLDEN_DIR/$g.json"
        (cd "$GOLDEN_DIR" && shasum -a 256 "$g.wav" > "$g.wav.sha256")
        # Only refresh a .json.sha256 that is already tracked; do not create new ones.
        if [ -f "$GOLDEN_DIR/$g.json.sha256" ]; then
            (cd "$GOLDEN_DIR" && shasum -a 256 "$g.json" > "$g.json.sha256")
        fi
        echo "[REGEN] $g  $(awk '{print $1}' "$GOLDEN_DIR/$g.wav.sha256")"
        continue
    fi

    computed=$(shasum -a 256 "$wav" | awk '{print $1}')
    expected=$(awk '{print $1}' "$GOLDEN_DIR/$g.wav.sha256")
    if [ "$computed" = "$expected" ]; then
        [ -z "$QUIET" ] && echo "[PASS] $g  $computed"
    else
        [ -z "$QUIET" ] && echo "[FAIL] $g  computed=$computed  expected=$expected"
        FAIL=$((FAIL + 1))
    fi
done

if [ -n "$REGEN" ]; then
    echo "REGENERATED: $TOTAL goldens re-baselined. Re-run without --regenerate to verify."
    exit 0
fi

if [ "$FAIL" -gt 0 ]; then
    [ -z "$QUIET" ] && echo "FAILED: $FAIL of $TOTAL goldens drifted"
    exit 1
fi
if [ "$SKIPPED" -gt 0 ]; then
    echo "ERROR: $SKIPPED golden(s) in NAMES have no committed baseline and were NOT verified."
    echo "       Run --regenerate to create them; a skipped golden is not a passing one."
    exit 1
fi
[ -z "$QUIET" ] && echo "OK: all $TOTAL goldens reproduce byte-identical"
exit 0
