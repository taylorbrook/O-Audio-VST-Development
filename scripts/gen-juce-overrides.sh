#!/bin/bash
# ==============================================================================
# gen-juce-overrides.sh — vendored override REGENERATION gate
#
# Usage:
#   ./scripts/gen-juce-overrides.sh                          # check (default), pristine base from network
#   ./scripts/gen-juce-overrides.sh --check --juce-root DIR  # check against a pristine JUCE tree (CI)
#   ./scripts/gen-juce-overrides.sh --write                  # rewrite the vendored overrides (humans only)
#   ./scripts/gen-juce-overrides.sh --check-wiring           # assert CI runs --check before every copy
#
# WHAT THIS GATE CLAIMS
#   vendored/JUCE-overrides/modules/ is exactly what
#   scripts/juce-patches/note-expression-juce-<pinned>.patch produces when it is
#   applied to pristine JUCE at the version in .github/juce-version.txt.
#
#   The local fork has TWO representations of the same change: a ~5 KB patch that
#   only the local machine applies (scripts/apply-juce-patches.sh), and two
#   ~170 KB whole files that only CI copies (cp -R vendored/JUCE-overrides/...).
#   Before this gate the only thing linking them was a human remembering to
#   update both. Edit one and nothing anywhere noticed. This gate re-derives the
#   whole files from the patch and fails when the two drift apart.
#
#   It is complementary to, not a replacement for, check-juce-overrides.sh: that
#   guard proves the overrides were cut from THIS JUCE version; this one proves
#   they were cut BY THE PATCH. Both run, in that order, before every copy.
#
# WHAT THIS GATE DOES NOT CLAIM
#   * It is BLIND TO LINE-ENDING-ONLY CHANGES, deliberately. Comparison is over
#     CR-stripped bytes (tr -d '\r'), the same convention MANIFEST.txt documents.
#     There is no .gitattributes entry for vendored/, so an autocrlf checkout is
#     an uncontrolled axis; `cp -R` is byte-preserving and neither CMake nor the
#     compiler cares about CR, so normalizing costs nothing real.
#     (--write still re-emits CRLF, so a regeneration produces a content diff
#     rather than thousands of lines of line-ending churn.)
#   * It does not verify the JUCE download itself, only the override surface.
#   * It does not stop a motivated insider: a human can run --write to launder an
#     edited override, exactly as they can re-run check-juce-overrides.sh
#     --update to launder a manifest. Both show up as plain-text diffs in review.
#     This gate targets SILENT drift, not a deliberate attacker.
#
# NOTES
#   Targets bash 3.2 (the macOS system bash) — no bash-4 constructs.
#   --write refuses to run when CI is set: a write in CI would rewrite the
#   vendored tree to match whatever the patch currently says and report green,
#   which is precisely the drift this gate exists to catch.
# ==============================================================================

set -euo pipefail

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OVERRIDE_ROOT="$REPO_ROOT/vendored/JUCE-overrides"
VERSION_FILE="$REPO_ROOT/.github/juce-version.txt"
PATCH_DIR="$REPO_ROOT/scripts/juce-patches"
RAW_BASE="https://raw.githubusercontent.com/juce-framework/JUCE"
WORKFLOWS="$REPO_ROOT/.github/workflows/build-and-release.yml $REPO_ROOT/.github/workflows/ci-tests.yml"

TAG="[gen-juce-overrides]"

die() {
    echo -e "${RED}${TAG} $*${NC}" >&2
    exit 1
}

usage() {
    sed -n '3,9p' "$0"
}

# ------------------------------------------------------------------------------
# Argument parsing
# ------------------------------------------------------------------------------
MODE="check"
JUCE_ROOT=""

while [ $# -gt 0 ]; do
    case "$1" in
        --check)        MODE="check" ;;
        --write)        MODE="write" ;;
        --check-wiring) MODE="wiring" ;;
        --juce-root)
            shift
            [ $# -gt 0 ] || die "--juce-root requires a directory argument"
            JUCE_ROOT="$1"
            ;;
        --juce-root=*)  JUCE_ROOT="${1#--juce-root=}" ;;
        -h|--help)      usage; exit 0 ;;
        *)              usage >&2; die "Unknown argument: $1" ;;
    esac
    shift
done

if [ -n "$JUCE_ROOT" ]; then
    [ -d "$JUCE_ROOT" ] || die "--juce-root path is not a directory: $JUCE_ROOT"
fi

# --write is refused OUTRIGHT under CI — before any fetch, any patch, and any
# write. Threat T-hno-03: a --write in CI would rewrite vendored/JUCE-overrides/
# to match whatever the patch currently says and then report green, laundering
# the exact drift this gate exists to catch.
if [ "$MODE" = "write" ] && [ -n "${CI:-}" ]; then
    die "--write is refused because CI is set (CI=${CI}).
  CI must only ever run --check. A --write in CI would rewrite
  vendored/JUCE-overrides/ to match whatever the patch currently says and then
  report green — laundering the exact drift this gate exists to catch.
  Nothing was written; no network call was made."
fi

# ------------------------------------------------------------------------------
# Wiring mode — pure text analysis over the two workflows. No network, no JUCE
# tree, no patch application.
# ------------------------------------------------------------------------------
if [ "$MODE" = "wiring" ]; then
    WIRING_FAILURES=0
    for wf in $WORKFLOWS; do
        rel="${wf#$REPO_ROOT/}"
        [ -f "$wf" ] || die "workflow not found: $rel"

        wf_ok=1

        # --- Assertion 1: no workflow may invoke --write ----------------------
        # Live enforcement of threat T-hno-03. --write refuses at runtime when CI
        # is set; asserting its absence here is the second, independent leg.
        write_hits="$(grep -n 'gen-juce-overrides\.sh.*--write' "$wf" | cut -d: -f1 || true)"
        if [ -n "$write_hits" ]; then
            wf_ok=0
            echo -e "${RED}${TAG} $rel: invokes gen-juce-overrides.sh --write at line(s): $(printf '%s' "$write_hits" | tr '\n' ' ')${NC}" >&2
            echo -e "${RED}${TAG}   CI must only ever run --check. A --write in CI would rewrite the${NC}" >&2
            echo -e "${RED}${TAG}   vendored tree to match whatever the patch currently says and then${NC}" >&2
            echo -e "${RED}${TAG}   report green — laundering the exact drift this gate exists to catch.${NC}" >&2
            WIRING_FAILURES=$((WIRING_FAILURES + 1))
        fi

        # --- Assertion 2: a --check precedes the FIRST copy site --------------
        # NOTE — deliberately "at least one --check before the first copy", NOT
        # the 1:1 guard/copy pairing that check-juce-overrides.sh --check-wiring
        # uses. This check is a property of REPO CONTENT (patch vs vendored
        # tree), not of the runner, so one invocation per workflow run discharges
        # it and it is wired into the macOS jobs only (keeping the release
        # workflow free of a dependency on `patch` existing in Windows git-bash).
        # A 1:1 pairing would therefore false-fail by construction — do not
        # "fix" this into a pair.
        copy_lines="$(grep -n 'cp -R vendored/JUCE-overrides' "$wf" | cut -d: -f1 || true)"
        check_lines="$(grep -n 'gen-juce-overrides\.sh --check' "$wf" | cut -d: -f1 || true)"

        if [ -z "$copy_lines" ]; then
            wf_ok=0
            echo -e "${RED}${TAG} $rel: no override-copy site found — has the copy moved or been renamed?${NC}" >&2
            WIRING_FAILURES=$((WIRING_FAILURES + 1))
        else
            first_copy="$(printf '%s\n' "$copy_lines" | head -1)"
            n_copy="$(printf '%s\n' "$copy_lines" | wc -l | tr -d ' ')"

            first_check=""
            [ -n "$check_lines" ] && first_check="$(printf '%s\n' "$check_lines" | head -1)"

            if [ -z "$first_check" ]; then
                wf_ok=0
                echo -e "${RED}${TAG} $rel: $n_copy override copy site(s) but NO gen-juce-overrides.sh --check invocation.${NC}" >&2
                echo -e "${RED}${TAG}   Add a step before the first copy running:${NC}" >&2
                echo -e "${RED}${TAG}     bash scripts/gen-juce-overrides.sh --check --juce-root JUCE${NC}" >&2
                WIRING_FAILURES=$((WIRING_FAILURES + 1))
            elif [ "$first_check" -ge "$first_copy" ]; then
                wf_ok=0
                echo -e "${RED}${TAG} $rel: first --check at line $first_check does NOT precede the first copy at line $first_copy.${NC}" >&2
                echo -e "${RED}${TAG}   The check must run against the PRISTINE JUCE tree; the copy destroys it.${NC}" >&2
                WIRING_FAILURES=$((WIRING_FAILURES + 1))
            fi
        fi

        # Only vouch for a workflow that cleared EVERY assertion.
        if [ "$wf_ok" -eq 1 ]; then
            echo -e "${GREEN}${TAG} $rel: regeneration check present before the first override copy, no --write.${NC}"
        fi
    done

    [ "$WIRING_FAILURES" -eq 0 ] || die "CI wiring check FAILED ($WIRING_FAILURES problem(s))."
    echo -e "${GREEN}${TAG} CI wiring OK — --check precedes the first override copy in both workflows, and no workflow writes.${NC}"
    exit 0
fi

# ------------------------------------------------------------------------------
# Step 1 — pinned JUCE version
# ------------------------------------------------------------------------------
[ -f "$VERSION_FILE" ] || die "pinned version file not found: .github/juce-version.txt"
PINNED_VERSION="$(tr -d '[:space:]' < "$VERSION_FILE")"
[ -n "$PINNED_VERSION" ] || die ".github/juce-version.txt is empty — cannot determine the pinned JUCE version."

# ------------------------------------------------------------------------------
# Step 2 — patch file, DERIVED from the pinned version. Never a hardcoded
# literal: that second literal is exactly the mirror this task exists to close.
# Raised before any network call so a bump fails instantly and offline.
# ------------------------------------------------------------------------------
PATCH_FILE="$PATCH_DIR/note-expression-juce-$PINNED_VERSION.patch"
if [ ! -f "$PATCH_FILE" ]; then
    die "no patch for the pinned JUCE version.
  .github/juce-version.txt pins JUCE $PINNED_VERSION, so this file must exist:
      scripts/juce-patches/note-expression-juce-$PINNED_VERSION.patch
  It does not. Cut the note-expression patch against a pristine JUCE
  $PINNED_VERSION tree and commit it under exactly that name, then re-run:
      bash scripts/gen-juce-overrides.sh --check"
fi

command -v patch >/dev/null 2>&1 || die "\`patch\` is not on PATH — cannot regenerate the overrides.
  A missing tool is a HARD ERROR, never a skipped check."

# ------------------------------------------------------------------------------
# Step 3 — the target path set, derived FROM THE PATCH (not from the vendored
# tree). This is what makes a newly patched third file surface as a mismatch
# instead of being silently ignored.
# The awk field pick is required: the 8.0.9-era patch format carries a
# tab-separated timestamp after the path.
# ------------------------------------------------------------------------------
PATCH_PATHS="$(grep '^--- a/' "$PATCH_FILE" | sed 's|^--- a/||' | awk '{print $1}' | LC_ALL=C sort -u || true)"
[ -n "$PATCH_PATHS" ] || die "no '--- a/<path>' lines found in $PATCH_FILE — cannot determine which files it patches."

# ------------------------------------------------------------------------------
# Step 4 — stage the pristine bytes.
# ------------------------------------------------------------------------------
WORK="$(mktemp -d -t genjuceov)"
REJ="$(mktemp -t genjuceovrej)"   # OUTSIDE $WORK — a reject inside the work tree
rm -f "$REJ"                      # would be walked as a generated artifact.
cleanup() { rm -rf "$WORK"; rm -f "$REJ"; }
trap cleanup EXIT

if [ -n "$JUCE_ROOT" ]; then
    echo -e "${YELLOW}${TAG} pristine base: $JUCE_ROOT (on-disk tree)${NC}"
else
    echo -e "${YELLOW}${TAG} pristine base: $RAW_BASE/$PINNED_VERSION${NC}"
fi
echo -e "${YELLOW}${TAG} patch:         ${PATCH_FILE#$REPO_ROOT/}${NC}"

while IFS= read -r rel; do
    [ -n "$rel" ] || continue
    mkdir -p "$WORK/$(dirname "$rel")"
    if [ -n "$JUCE_ROOT" ]; then
        src="$JUCE_ROOT/$rel"
        [ -f "$src" ] || die "pristine base is missing a patched path: $src
  The patch touches $rel, so a JUCE $PINNED_VERSION tree must contain it.
  Either --juce-root points at the wrong tree, or the patch targets a path that
  does not exist at this JUCE version."
        # Copy — NEVER read-modify the JUCE tree in place; in CI it is the build input.
        cp "$src" "$WORK/$rel"
    else
        url="$RAW_BASE/$PINNED_VERSION/$rel"
        if ! curl --fail --show-error --silent --location --proto '=https' --tlsv1.2 "$url" -o "$WORK/$rel"; then
            die "failed to fetch the pristine baseline: $url
  A fetch failure is a HARD ERROR, never a skipped check. Either the tag
  $PINNED_VERSION does not exist upstream, the path is wrong, or the network is
  unavailable. Offline? Re-run with:
      --juce-root <pristine JUCE $PINNED_VERSION tree>"
        fi
    fi
done <<EOF
$PATCH_PATHS
EOF

# ------------------------------------------------------------------------------
# Step 5 — CRLF → LF. The patch context lines are LF and patch(1) rejects hunks
# whose context line endings differ; every upstream source measured is CRLF.
# Portable sed-in-place (-i.bak works on both BSD/macOS and GNU sed), the same
# form apply-juce-patches.sh step 3.5 uses.
# ------------------------------------------------------------------------------
find "$WORK" -type f -print | while IFS= read -r f; do
    sed -i.bak 's/\r$//' "$f" && rm -f "${f}.bak"
done

# ------------------------------------------------------------------------------
# Step 6 — apply.
#   -F0  forbids fuzz: a fuzzed apply silently produces a shifted result, which
#        is the exact drift this gate exists to catch.
#   -N   forbids REVERSE application. Without it, `patch -t` against a tree that
#        ALREADY carries the patch happily reverse-applies it and exits 0,
#        yielding a pristine tree from a patched one — the gate then fails with a
#        "drifted apart" message that blames the wrong thing. Observed on the
#        real patched /Users/taylorbrook/JUCE tree (negative control NC6).
#        With -N the already-applied patch is skipped and patch exits non-zero,
#        so the failure is diagnosed as a non-pristine base, which it is.
#   -t   batch: never prompt. A prompt in CI is a hang.
# ------------------------------------------------------------------------------
PATCH_RC=0
PATCH_OUT="$(cd "$WORK" && patch -p1 -F0 -N -t -r "$REJ" -i "$PATCH_FILE" 2>&1)" || PATCH_RC=$?

STRAY="$(find "$WORK" -type f \( -name '*.rej' -o -name '*.orig' \) | LC_ALL=C sort || true)"

ALREADY_APPLIED=""
if printf '%s' "$PATCH_OUT" | grep -qi 'reversed\|previously applied'; then
    ALREADY_APPLIED="  DIAGNOSIS: patch reports the change is ALREADY PRESENT in the base.
  The tree supplied as the pristine baseline is not pristine — it has the
  note-expression patch applied already, so there is nothing left to derive.
"
fi

if [ "$PATCH_RC" -ne 0 ] || { [ -f "$REJ" ] && [ -s "$REJ" ]; } || [ -n "$STRAY" ]; then
    die "the patch did not apply cleanly to the pristine base (patch exit $PATCH_RC).

$ALREADY_APPLIED  patch output:
$(printf '%s\n' "$PATCH_OUT" | sed 's/^/    /')
$( [ -n "$STRAY" ] && printf '  stray reject/backup files:\n%s\n' "$(printf '%s\n' "$STRAY" | sed 's/^/    /')" || true )
  The base this ran against is NOT pristine JUCE $PINNED_VERSION, or the patch no
  longer matches it. The two usual causes:
    1. --juce-root points at a tree that ALREADY has the patch applied (the
       local /Users/taylorbrook/JUCE working tree is patched in place by
       scripts/apply-juce-patches.sh — it is not a valid base). Use a freshly
       unpacked JUCE $PINNED_VERSION tree, or drop --juce-root to fetch the
       pristine files from the tag.
    2. JUCE $PINNED_VERSION changed the context lines the patch anchors on, so
       the patch needs re-cutting against $PINNED_VERSION.
  Fuzz is forbidden (-F0) on purpose: a fuzzed apply shifts the result silently."
fi

# ------------------------------------------------------------------------------
# Step 7 — LF → CRLF re-emit, so --write stays byte-stable against the committed
# tree (see WHAT THIS GATE DOES NOT CLAIM).
# NOT `sed 's/$/\r/'`: BSD/macOS sed does not interpret that escape in the
# replacement and would emit a literal letter r.
# ------------------------------------------------------------------------------
while IFS= read -r rel; do
    [ -n "$rel" ] || continue
    awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$WORK/$rel" > "$WORK/$rel.crlf"
    mv "$WORK/$rel.crlf" "$WORK/$rel"
done <<EOF
$PATCH_PATHS
EOF

# ==============================================================================
# WRITE MODE
# ==============================================================================
if [ "$MODE" = "write" ]; then
    # (The CI refusal fires at argument-parse time, above — before any fetch.)
    while IFS= read -r rel; do
        [ -n "$rel" ] || continue
        dest="$OVERRIDE_ROOT/$rel"
        mkdir -p "$(dirname "$dest")"
        cp "$WORK/$rel" "$dest"
        echo -e "${GREEN}${TAG} wrote vendored/JUCE-overrides/$rel${NC}"
    done <<EOF
$PATCH_PATHS
EOF

    echo -e "${GREEN}${TAG} Regenerated from ${PATCH_FILE#$REPO_ROOT/} against pristine JUCE $PINNED_VERSION.${NC}"
    echo -e "${YELLOW}${TAG} MANIFEST.txt was NOT touched. Re-fingerprint it now:${NC}"
    echo -e "${YELLOW}${TAG}     bash scripts/check-juce-overrides.sh --update${NC}"
    exit 0
fi

# ==============================================================================
# CHECK MODE
# ==============================================================================
[ -d "$OVERRIDE_ROOT/modules" ] || die "override tree not found: vendored/JUCE-overrides/modules/"

# MANIFEST.txt sits ABOVE modules/ and is out of scope here — it is the other
# guard's artifact.
DISK_FILES="$(cd "$OVERRIDE_ROOT" && find modules -type f | LC_ALL=C sort)"

FAILURES=0
fail_check() {
    echo -e "${RED}${TAG} $*${NC}" >&2
    FAILURES=$((FAILURES + 1))
}

ONLY_GENERATED="$(comm -23 <(printf '%s\n' "$PATCH_PATHS") <(printf '%s\n' "$DISK_FILES") || true)"
ONLY_ON_DISK="$(comm -13 <(printf '%s\n' "$PATCH_PATHS") <(printf '%s\n' "$DISK_FILES") || true)"
IN_BOTH="$(comm -12 <(printf '%s\n' "$PATCH_PATHS") <(printf '%s\n' "$DISK_FILES") || true)"

if [ -n "$ONLY_GENERATED" ]; then
    fail_check "INVENTORY: the patch produces path(s) that vendored/JUCE-overrides/ does not carry:
$(printf '%s\n' "$ONLY_GENERATED" | sed 's/^/    /')
  CI copies only what is under vendored/JUCE-overrides/modules/, so a patched
  file that is not vendored never reaches the JUCE tree in CI.
  Fix: bash scripts/gen-juce-overrides.sh --write"
fi

if [ -n "$ONLY_ON_DISK" ]; then
    fail_check "INVENTORY: vendored/JUCE-overrides/ carries path(s) the patch does not produce:
$(printf '%s\n' "$ONLY_ON_DISK" | sed 's/^/    /')
  An unlisted file cannot ride along into the JUCE tree — everything under
  modules/ is copied over the build input, so every file there must be
  accounted for by the pinned patch.
  Fix: remove the file, or add it to scripts/juce-patches/note-expression-juce-$PINNED_VERSION.patch."
fi

while IFS= read -r rel; do
    [ -n "$rel" ] || continue
    gen="$WORK/$rel"
    ven="$OVERRIDE_ROOT/$rel"

    if diff -u <(tr -d '\r' < "$gen") <(tr -d '\r' < "$ven") > /dev/null 2>&1; then
        echo -e "${GREEN}${TAG} OK  $rel${NC}"
    else
        echo -e "${RED}${TAG} BAD $rel${NC}" >&2
        fail_check "REGENERATION: vendored/JUCE-overrides/$rel is NOT what the pinned patch produces.
  Left  (-) = generated from pristine JUCE $PINNED_VERSION + ${PATCH_FILE#$REPO_ROOT/}
  Right (+) = vendored/JUCE-overrides/$rel as committed
$(diff -u --label "generated/$rel" --label "vendored/$rel" <(tr -d '\r' < "$gen") <(tr -d '\r' < "$ven") | head -40 | sed 's/^/    /')
  The patch and the vendored whole file have drifted apart. Either the patch was
  edited without re-cutting the overrides (fix: bash scripts/gen-juce-overrides.sh --write),
  or the override was hand-edited and the patch never learned about it (fix: update
  the patch, then --write)."
    fi
done <<EOF
$IN_BOTH
EOF

if [ "$FAILURES" -ne 0 ]; then
    echo -e "${RED}${TAG} FAILED — $FAILURES check(s) did not pass.${NC}" >&2
    echo -e "${RED}${TAG} vendored/JUCE-overrides/ is NOT reproducible from ${PATCH_FILE#$REPO_ROOT/}.${NC}" >&2
    exit 1
fi

echo -e "${GREEN}${TAG} PASS — vendored/JUCE-overrides/ regenerates exactly from${NC}"
echo -e "${GREEN}${TAG}   ${PATCH_FILE#$REPO_ROOT/}  applied to pristine JUCE $PINNED_VERSION.${NC}"
exit 0
