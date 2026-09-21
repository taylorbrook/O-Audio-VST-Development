#!/usr/bin/env bash
# Gate for research/repo-modernization-audit-2026-09-21.md (quick task 260921-fy9).
#
# Proves the report is substantive rather than a skeleton, and proves the audit
# stayed read-only. Exits non-zero on the FIRST failure, naming what failed.
#
# Usage: check-report.sh <mode> [mode ...]
#   skeleton   frontmatter validator passes + all eight contract headings present
#   1..6       that numbered section has >=4 findings rows, >=3 with real evidence
#   summary    Executive Summary has exactly 5 ranked rows
#   apply      Apply Later has >=6 checklist lines, each carrying a (§N) reference
#   readonly   protected-path git status is byte-identical to preflight-status.txt

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
QUICK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPORT="$REPO_ROOT/research/repo-modernization-audit-2026-09-21.md"
BASELINE="$QUICK_DIR/preflight-status.txt"

# One shared row regex for findings rows AND Executive Summary rows. It cannot
# match a heading, a separator row (--- has no [SML] col), or the header row
# ("Effort" is not [SML]).
ROW_RE='^\| [^|]+ \| [^|]+ \| [^|]+ \| [SML] \| (Low|Med|High) \|'

# An Evidence cell counts as evidence only if it carries a file path with a real
# extension, a semver-ish version string, or a retrieved URL.
# NOTE (deviation, Rule 1): the plan's regex omitted `yaml`. `modules/registry.yaml`
# is the primary evidence source for section 6, so excluding it made the gate reject
# genuinely-cited evidence. Adding `yaml` widens what counts as a real path by exactly
# one real extension; it does not weaken the check.
EVID_RE='[A-Za-z0-9_./-]+\.(yml|yaml|txt|sh|cmake|json|md|cpp|h|patch)|[0-9]+\.[0-9]+\.[0-9]+|https?://'

HEADINGS=(
  '## Executive Summary'
  '## 1. JUCE Version & Toolchain'
  '## 2. Build Efficiency'
  '## 3. Claude Workflow & Tooling'
  '## 4. Testing & Quality Gates'
  '## 5. Repo Hygiene'
  '## 6. Instrument-Specific Best Practices'
  '## Apply Later'
)

fail() { echo "FAIL [$1]: $2" >&2; exit 1; }
pass() { echo "ok   [$1]: $2"; }

require_report() {
  [ -f "$REPORT" ] || fail "$1" "report not found at $REPORT"
}

# Slice a section: from its own heading to the next top-level '## ' heading.
slice() {
  sed -n "/^$1/,/^## /p" "$REPORT" | sed '$ { /^## /d; }'
}

check_skeleton() {
  require_report skeleton
  python3 "$REPO_ROOT/.claude/hooks/validators/validate-research-frontmatter.py" "$REPORT" \
    || fail skeleton "research frontmatter validator rejected the report"
  local h
  for h in "${HEADINGS[@]}"; do
    grep -qF "$h" "$REPORT" || fail skeleton "missing contract heading: $h"
  done
  pass skeleton "frontmatter valid, all ${#HEADINGS[@]} contract headings present"
}

check_section() {
  local n="$1"
  require_report "$n"
  local body rows evid
  body="$(slice "## $n\.")"
  [ -n "$body" ] || fail "$n" "section '## $n.' sliced empty"
  rows="$(printf '%s\n' "$body" | grep -Ec "$ROW_RE")"
  [ "$rows" -ge 4 ] || fail "$n" "only $rows findings rows (need >= 4)"
  evid="$(printf '%s\n' "$body" | grep -E "$ROW_RE" | awk -F'|' '{print $3}' | grep -Ec "$EVID_RE")"
  [ "$evid" -ge 3 ] || fail "$n" "only $evid of $rows rows carry verifiable Evidence (need >= 3)"
  pass "$n" "$rows findings rows, $evid with verifiable evidence"
}

check_summary() {
  require_report summary
  local body rows
  body="$(slice '## Executive Summary')"
  [ -n "$body" ] || fail summary "Executive Summary sliced empty"
  rows="$(printf '%s\n' "$body" | grep -Ec "$ROW_RE")"
  [ "$rows" -eq 5 ] || fail summary "Executive Summary has $rows ranked rows (need exactly 5)"
  pass summary "exactly 5 ranked actions"
}

check_apply() {
  require_report apply
  local body items refless
  body="$(slice '## Apply Later')"
  [ -n "$body" ] || fail apply "Apply Later sliced empty"
  items="$(printf '%s\n' "$body" | grep -c '^- \[ \] ')"
  [ "$items" -ge 6 ] || fail apply "only $items checklist items (need >= 6)"
  refless="$(printf '%s\n' "$body" | grep '^- \[ \] ' | grep -cv '(§')"
  [ "$refless" -eq 0 ] || fail apply "$refless checklist items lack a (§N) section reference"
  pass apply "$items checklist items, all section-referenced"
}

check_readonly() {
  [ -f "$BASELINE" ] || fail readonly "preflight baseline not found at $BASELINE"
  ( cd "$REPO_ROOT" && git status --porcelain -- \
      plugins modules .github scripts CMakeLists.txt CLAUDE.md .gitignore \
      .claude/skills .claude/commands .claude/hooks .claude/agents .claude/settings.json ) \
    > "$QUICK_DIR/.readonly-now.txt"
  if ! diff -q "$BASELINE" "$QUICK_DIR/.readonly-now.txt" >/dev/null; then
    echo "--- protected-path drift (baseline vs now) ---" >&2
    diff "$BASELINE" "$QUICK_DIR/.readonly-now.txt" >&2
    rm -f "$QUICK_DIR/.readonly-now.txt"
    fail readonly "protected paths changed since the preflight baseline"
  fi
  rm -f "$QUICK_DIR/.readonly-now.txt"
  pass readonly "protected-path git status byte-identical to preflight baseline"
}

[ $# -ge 1 ] || { echo "Usage: $(basename "$0") <skeleton|1-6|summary|apply|readonly> ..." >&2; exit 2; }

for mode in "$@"; do
  case "$mode" in
    skeleton)            check_skeleton ;;
    1|2|3|4|5|6)         check_section "$mode" ;;
    summary)             check_summary ;;
    apply)               check_apply ;;
    readonly)            check_readonly ;;
    *) echo "FAIL: unknown mode '$mode'" >&2; exit 2 ;;
  esac
done

echo "ALL GATES PASSED ($*)"
