#!/bin/bash
# ==============================================================================
# check-juce-overrides.sh — JUCE override provenance gate
#
# Usage:
#   ./scripts/check-juce-overrides.sh                  # check, upstream from network
#   ./scripts/check-juce-overrides.sh --juce-root DIR  # check, upstream from a pristine JUCE tree
#   ./scripts/check-juce-overrides.sh --update         # regenerate MANIFEST.txt (humans only)
#   ./scripts/check-juce-overrides.sh --check-wiring   # assert CI calls this guard before every copy
#
# WHAT THIS GATE CLAIMS
#   Every file under vendored/JUCE-overrides/modules/ was derived from the
#   upstream JUCE file at the version pinned in .github/juce-version.txt.
#   It proves that by recording, in vendored/JUCE-overrides/MANIFEST.txt:
#     - the JUCE version the overrides were cut from,
#     - the fingerprint of the UPSTREAM file at that version,
#     - the fingerprint of the OVERRIDE file as committed,
#   and re-deriving all three at check time.
#
#   The check that matters is the upstream one. A JUCE bump that edits
#   .github/juce-version.txt without re-cutting the overrides changes the
#   upstream bytes under the recorded fingerprint, and this gate fails —
#   BEFORE `cp -R vendored/JUCE-overrides/modules/.` writes stale sources over
#   a newer JUCE. The pre-existing post-copy `grep -q "JUCE-NE-PATCH"` cannot
#   see that: the marker lives inside the vendored file itself, so it passes
#   for every JUCE version. This gate closes that hole.
#
# WHAT THIS GATE DOES NOT CLAIM
#   * It is BLIND TO LINE-ENDING-ONLY CHANGES. Every fingerprint is taken over
#     CR-stripped bytes (`tr -d '\r'`). That is deliberate and load-bearing:
#     the vendored overrides, the release zip CI unpacks, and the git tag on
#     raw.githubusercontent.com all measured CRLF (2026-09-21), but nothing
#     pins that: there is no .gitattributes entry for vendored/, so any
#     checkout with autocrlf / eol settings is an uncontrolled axis. A
#     byte-level compare would then fail on line endings alone.
#     `cp -R` is byte-preserving and neither
#     CMake nor the compiler cares about CR, so normalizing costs nothing real
#     and is what makes the gate stable across all four axes.
#   * It does not stop a motivated insider. Anyone can re-run --update to
#     launder an edited override. The manifest delta is plain text and shows up
#     in review; this gate targets SILENT drift, not a deliberate attacker.
#   * It does not verify the JUCE download itself, only the two-file override
#     surface layered on top of it.
#
# NOTES
#   Targets bash 3.2 (the macOS system bash) — no bash-4 constructs.
#   Never run --update in CI: it would rewrite the manifest to match whatever
#   is on disk, which is precisely the drift the gate exists to catch.
# ==============================================================================

set -euo pipefail

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OVERRIDE_ROOT="$REPO_ROOT/vendored/JUCE-overrides"
MANIFEST="$OVERRIDE_ROOT/MANIFEST.txt"
VERSION_FILE="$REPO_ROOT/.github/juce-version.txt"
RAW_BASE="https://raw.githubusercontent.com/juce-framework/JUCE"
WORKFLOWS="$REPO_ROOT/.github/workflows/build-and-release.yml $REPO_ROOT/.github/workflows/ci-tests.yml"

TAG="[check-juce-overrides]"

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
        --update)       MODE="update" ;;
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

# ------------------------------------------------------------------------------
# sha256 over stdin. macos-14 runners have shasum but not necessarily sha256sum;
# git-bash on Windows has both. Detect at runtime, never hash a path directly —
# always the CR-normalized stream.
# ------------------------------------------------------------------------------
if command -v shasum >/dev/null 2>&1; then
    sha_of_stdin() { shasum -a 256 | cut -d' ' -f1; }
elif command -v sha256sum >/dev/null 2>&1; then
    sha_of_stdin() { sha256sum | cut -d' ' -f1; }
else
    die "neither shasum nor sha256sum is available — cannot fingerprint"
fi

sha_of_file_normalized() {
    # $1 = path. Strip CR before hashing (see WHAT THIS GATE DOES NOT CLAIM).
    tr -d '\r' < "$1" | sha_of_stdin
}

# ------------------------------------------------------------------------------
# Wiring mode — assert the guard runs before every override copy, in both
# workflows. Pure text/line-number analysis; no network, no JUCE tree.
# ------------------------------------------------------------------------------
if [ "$MODE" = "wiring" ]; then
    WIRING_FAILURES=0
    for wf in $WORKFLOWS; do
        rel="${wf#$REPO_ROOT/}"
        [ -f "$wf" ] || die "workflow not found: $rel"

        guard_lines="$(grep -n 'check-juce-overrides\.sh --juce-root JUCE' "$wf" | cut -d: -f1 || true)"
        copy_lines="$(grep -n 'cp -R vendored/JUCE-overrides' "$wf" | cut -d: -f1 || true)"

        n_guard=0; [ -n "$guard_lines" ] && n_guard=$(printf '%s\n' "$guard_lines" | wc -l | tr -d ' ')
        n_copy=0;  [ -n "$copy_lines" ]  && n_copy=$(printf '%s\n' "$copy_lines" | wc -l | tr -d ' ')

        if [ "$n_copy" -eq 0 ]; then
            echo -e "${RED}${TAG} $rel: no override-copy site found — has the copy moved or been renamed?${NC}" >&2
            WIRING_FAILURES=$((WIRING_FAILURES + 1))
            continue
        fi
        if [ "$n_guard" -ne "$n_copy" ]; then
            echo -e "${RED}${TAG} $rel: $n_copy override copy site(s) but $n_guard guard invocation(s).${NC}" >&2
            echo -e "${RED}${TAG}   Every 'cp -R vendored/JUCE-overrides ...' needs a preceding step running:${NC}" >&2
            echo -e "${RED}${TAG}     bash scripts/check-juce-overrides.sh --juce-root JUCE${NC}" >&2
            WIRING_FAILURES=$((WIRING_FAILURES + 1))
            continue
        fi

        # Pair guard N with copy N in file order and require guard to come first.
        wf_ok=1
        i=1
        while [ "$i" -le "$n_copy" ]; do
            g=$(printf '%s\n' "$guard_lines" | sed -n "${i}p")
            c=$(printf '%s\n' "$copy_lines" | sed -n "${i}p")
            if [ "$g" -ge "$c" ]; then
                wf_ok=0
                echo -e "${RED}${TAG} $rel: guard at line $g does NOT precede its copy at line $c.${NC}" >&2
                echo -e "${RED}${TAG}   The guard must run against the PRISTINE JUCE tree; after the copy${NC}" >&2
                echo -e "${RED}${TAG}   there is no pristine upstream left to fingerprint.${NC}" >&2
                WIRING_FAILURES=$((WIRING_FAILURES + 1))
            fi
            i=$((i + 1))
        done

        # Only vouch for a workflow that cleared every ordering pair.
        if [ "$wf_ok" -eq 1 ]; then
            echo -e "${GREEN}${TAG} $rel: $n_guard guard invocation(s), each before its copy.${NC}"
        fi
    done

    [ "$WIRING_FAILURES" -eq 0 ] || die "CI wiring check FAILED ($WIRING_FAILURES problem(s))."
    echo -e "${GREEN}${TAG} CI wiring OK — guard precedes every override copy in both workflows.${NC}"
    exit 0
fi

# ------------------------------------------------------------------------------
# Pinned JUCE version
# ------------------------------------------------------------------------------
[ -f "$VERSION_FILE" ] || die "pinned version file not found: .github/juce-version.txt"
PINNED_VERSION="$(tr -d '[:space:]' < "$VERSION_FILE")"
[ -n "$PINNED_VERSION" ] || die ".github/juce-version.txt is empty — cannot determine the pinned JUCE version."

[ -d "$OVERRIDE_ROOT/modules" ] || die "override tree not found: vendored/JUCE-overrides/modules/"

# Files actually on disk, relative to vendored/JUCE-overrides/ (so a path maps
# 1:1 onto both <juce-root>/<path> and the upstream repo path).
DISK_FILES="$(cd "$OVERRIDE_ROOT" && find modules -type f | LC_ALL=C sort)"
[ -n "$DISK_FILES" ] || die "no override files found under vendored/JUCE-overrides/modules/"

# ------------------------------------------------------------------------------
# Upstream fingerprint for one override-relative path.
#   --juce-root DIR : read <DIR>/<path> (CI mode — zero network, and it
#                     fingerprints the exact bytes about to be overwritten).
#   default         : fetch the raw file at the pinned tag (local mode — the
#                     local JUCE tree is patched and is not a git repo).
# ------------------------------------------------------------------------------
upstream_sha() {
    _rel="$1"
    if [ -n "$JUCE_ROOT" ]; then
        _src="$JUCE_ROOT/$_rel"
        if [ ! -f "$_src" ]; then
            die "upstream file missing from --juce-root tree: $_src
  A pristine JUCE $PINNED_VERSION tree must contain every overridden path.
  If this path does not exist upstream, the override is not a patch of upstream
  and this manifest entry is wrong."
        fi
        sha_of_file_normalized "$_src"
    else
        _url="$RAW_BASE/$PINNED_VERSION/$_rel"
        _tmp="$(mktemp -t juceov)"
        if ! curl --fail --show-error --silent --location --proto '=https' --tlsv1.2 "$_url" -o "$_tmp"; then
            rm -f "$_tmp"
            die "failed to fetch upstream baseline: $_url
  A fetch failure is a HARD ERROR, never a skipped check. Either the tag
  $PINNED_VERSION does not exist upstream, the path is wrong, or the network is
  unavailable. Offline? Re-run with --juce-root <pristine JUCE $PINNED_VERSION tree>."
        fi
        _sha="$(tr -d '\r' < "$_tmp" | sha_of_stdin)"
        rm -f "$_tmp"
        printf '%s\n' "$_sha"
    fi
}

# ------------------------------------------------------------------------------
# Update mode — regenerate the manifest from the current overrides + upstream.
# ------------------------------------------------------------------------------
if [ "$MODE" = "update" ]; then
    if [ -n "$JUCE_ROOT" ]; then
        SOURCE_DESC="--juce-root $JUCE_ROOT"
        echo -e "${YELLOW}${TAG} --update is reading upstream from $JUCE_ROOT.${NC}"
        echo -e "${YELLOW}${TAG} That tree MUST be pristine (unpatched) JUCE $PINNED_VERSION, or the${NC}"
        echo -e "${YELLOW}${TAG} recorded upstream fingerprints will be worthless.${NC}"
    else
        SOURCE_DESC="$RAW_BASE/$PINNED_VERSION"
    fi

    TMP_MANIFEST="$(mktemp -t juceovman)"
    {
        echo "# vendored/JUCE-overrides provenance manifest"
        echo "#"
        echo "# GENERATED — do not hand-edit. Regenerate with:"
        echo "#     bash scripts/check-juce-overrides.sh --update"
        echo "# Hand-editing this file defeats the gate it exists to feed: the whole point"
        echo "# is that these fingerprints were derived, not asserted."
        echo "#"
        echo "# Every sha256 below is taken over CR-STRIPPED bytes (tr -d '\\r'), so the"
        echo "# same value holds for the release zip, the git tag, and an autocrlf/Windows"
        echo "# checkout. A change to line endings ALONE is invisible here, by design."
        echo "#"
        echo "# Format:"
        echo "#   juce_version <version>"
        echo "#   override <path-relative-to-vendored/JUCE-overrides/> <upstream_sha256> <override_sha256>"
        echo "#"
        echo "juce_version $PINNED_VERSION"
        printf '%s\n' "$DISK_FILES" | while IFS= read -r rel; do
            [ -n "$rel" ] || continue
            up="$(upstream_sha "$rel")"
            ov="$(sha_of_file_normalized "$OVERRIDE_ROOT/$rel")"
            echo "override $rel $up $ov"
        done
    } > "$TMP_MANIFEST"

    mv "$TMP_MANIFEST" "$MANIFEST"
    echo -e "${GREEN}${TAG} Wrote vendored/JUCE-overrides/MANIFEST.txt${NC}"
    echo -e "${GREEN}${TAG}   juce_version: $PINNED_VERSION${NC}"
    echo -e "${GREEN}${TAG}   upstream source: $SOURCE_DESC${NC}"
    grep '^override ' "$MANIFEST" | while IFS= read -r line; do echo "  $line"; done
    exit 0
fi

# ------------------------------------------------------------------------------
# Check mode
# ------------------------------------------------------------------------------
[ -f "$MANIFEST" ] || die "manifest not found: vendored/JUCE-overrides/MANIFEST.txt
  Generate it with:  bash scripts/check-juce-overrides.sh --update"

# The manifest is a tracked text file, so a Windows checkout (core.autocrlf)
# hands it over with CRLF endings. `read` keeps the CR on the LAST field of each
# line, so the recorded override sha compares unequal to a byte-identical actual
# one and the failure prints two hashes that look the same (ci-tests.yml run
# 35659240413, windows-vst3). Every read below goes through this CR-stripped
# copy — the same normalisation the fingerprints themselves already get.
MANIFEST_TEXT="$(tr -d '\r' < "$MANIFEST")"

FAILURES=0
fail_check() {
    echo -e "${RED}${TAG} $*${NC}" >&2
    FAILURES=$((FAILURES + 1))
}

# --- Check 1: manifest version == pinned version (the bump tripwire) ----------
MANIFEST_VERSION="$(printf '%s\n' "$MANIFEST_TEXT" | grep '^juce_version ' | head -1 | awk '{print $2}' || true)"
if [ -z "$MANIFEST_VERSION" ]; then
    fail_check "CHECK 1 (version): manifest has no 'juce_version' line. Re-run --update."
elif [ "$MANIFEST_VERSION" != "$PINNED_VERSION" ]; then
    fail_check "CHECK 1 (version): overrides were cut from JUCE $MANIFEST_VERSION but .github/juce-version.txt pins $PINNED_VERSION.
  The vendored overrides are STALE for the pinned JUCE. Copying them would
  write JUCE $MANIFEST_VERSION sources over a JUCE $PINNED_VERSION tree.
  Fix: re-cut the overrides from JUCE $PINNED_VERSION (apply the note-expression
  patch to a pristine $PINNED_VERSION tree, copy the two files into
  vendored/JUCE-overrides/), then re-run:
      bash scripts/check-juce-overrides.sh --update"
fi

# --- Check 4: set equality, manifest entries <-> files on disk ----------------
MANIFEST_FILES="$(printf '%s\n' "$MANIFEST_TEXT" | grep '^override ' | awk '{print $2}' | LC_ALL=C sort || true)"
if [ "$MANIFEST_FILES" != "$DISK_FILES" ]; then
    fail_check "CHECK 4 (inventory): manifest entries do not match the files on disk.
  Only in manifest (listed but missing from disk):
$(comm -23 <(printf '%s\n' "$MANIFEST_FILES") <(printf '%s\n' "$DISK_FILES") | sed 's/^/    /')
  Only on disk (present but unlisted — an unlisted file cannot ride along):
$(comm -13 <(printf '%s\n' "$MANIFEST_FILES") <(printf '%s\n' "$DISK_FILES") | sed 's/^/    /')
  Fix: re-run  bash scripts/check-juce-overrides.sh --update"
fi

# --- Checks 2 and 3: per-file override and upstream fingerprints -------------
if [ -n "$JUCE_ROOT" ]; then
    echo -e "${YELLOW}${TAG} upstream baseline: $JUCE_ROOT (pristine tree)${NC}"
else
    echo -e "${YELLOW}${TAG} upstream baseline: $RAW_BASE/$PINNED_VERSION${NC}"
fi

while read -r kw rel rec_up rec_ov; do
    [ "$kw" = "override" ] || continue

    if [ ! -f "$OVERRIDE_ROOT/$rel" ]; then
        # Already reported by check 4; skip fingerprinting a missing file.
        continue
    fi

    file_ok=1

    # Check 2 — the override file is what the manifest says it is.
    act_ov="$(sha_of_file_normalized "$OVERRIDE_ROOT/$rel")"
    if [ "$act_ov" != "$rec_ov" ]; then
        file_ok=0
        fail_check "CHECK 2 (override fingerprint): $rel has changed since the manifest was written.
    recorded: $rec_ov
    actual:   $act_ov
  Either the override was edited without re-running --update, or the manifest
  was hand-edited. If the edit is intentional, re-run:
      bash scripts/check-juce-overrides.sh --update"
    fi

    # Check 3 — the upstream file at the pinned version is the derivation base.
    act_up="$(upstream_sha "$rel")"
    if [ "$act_up" != "$rec_up" ]; then
        file_ok=0
        fail_check "CHECK 3 (upstream provenance): $rel was NOT derived from the upstream file now at JUCE $PINNED_VERSION.
    recorded upstream: $rec_up
    actual upstream:   $act_up
  This is the drift the gate exists for. Either JUCE $PINNED_VERSION changed
  this file relative to the version the override was cut from, or the tree
  supplied as the upstream baseline is not pristine (already patched).
  Fix: re-cut the override from pristine JUCE $PINNED_VERSION, then re-run:
      bash scripts/check-juce-overrides.sh --update"
    fi

    # Only vouch for a file that cleared BOTH per-file checks — a green line
    # next to a failure is worse than no line.
    if [ "$file_ok" -eq 1 ]; then
        echo -e "${GREEN}${TAG} OK  $rel${NC}"
    else
        echo -e "${RED}${TAG} BAD $rel${NC}" >&2
    fi
done <<< "$MANIFEST_TEXT"

if [ "$FAILURES" -ne 0 ]; then
    echo -e "${RED}${TAG} FAILED — $FAILURES check(s) did not pass.${NC}" >&2
    echo -e "${RED}${TAG} Refusing to vouch for vendored/JUCE-overrides/ against JUCE $PINNED_VERSION.${NC}" >&2
    exit 1
fi

echo -e "${GREEN}${TAG} PASS — overrides provably derived from JUCE $PINNED_VERSION.${NC}"
exit 0
