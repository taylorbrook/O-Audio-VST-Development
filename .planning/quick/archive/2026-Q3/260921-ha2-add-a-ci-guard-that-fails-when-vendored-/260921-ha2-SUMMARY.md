---
phase: quick-260921-ha2
plan: 01
subsystem: ci
tags: [ci, provenance, juce, supply-chain, vendored-overrides]
status: complete
requires: []
provides:
  - "scripts/check-juce-overrides.sh (provenance gate, 4 modes)"
  - "vendored/JUCE-overrides/MANIFEST.txt (derivation record)"
  - "guard wired before all four override-copy sites"
affects:
  - .github/workflows/build-and-release.yml
  - .github/workflows/ci-tests.yml
tech-stack:
  added: []
  patterns:
    - "CR-normalized fingerprints (tr -d '\\r') to span CRLF/LF source axes"
    - "gate falsified by negative controls before being wired into CI"
key-files:
  created:
    - scripts/check-juce-overrides.sh
    - vendored/JUCE-overrides/MANIFEST.txt
  modified:
    - .github/workflows/build-and-release.yml
    - .github/workflows/ci-tests.yml
decisions:
  - "Fingerprint CR-stripped bytes, not raw bytes — the release zip, the git tag, and a Windows checkout disagree on line endings, so a byte-level compare would fail on at least one axis every time."
  - "CI uses --juce-root (zero network) and fingerprints the exact bytes about to be overwritten; the network mode exists only for local runs, because the local JUCE tree is patched and is not a git repo."
  - "Guard placed between Setup JUCE and the copy — after the copy there is no pristine upstream left to fingerprint."
  - "Existing post-copy JUCE-NE-PATCH greps left in place as the post-copy fail-fast."
metrics:
  duration: ~35min
  completed: 2026-09-21
actuals:
  tokens: 21000
  tasks: 3
  commits: 3
plan_head_before: 83a5abf6
---

# Quick Task 260921-ha2: JUCE Override Provenance Guard Summary

A recorded derivation manifest plus a four-mode guard script, wired ahead of all four
`cp -R vendored/JUCE-overrides/modules/.` sites, so a JUCE bump that does not re-cut the overrides
fails CI before stale 8.0.14 sources reach a newer JUCE tree.

## What Was Built

**`scripts/check-juce-overrides.sh`** — bash 3.2, `set -euo pipefail`, four modes:

| Mode | Purpose |
|---|---|
| (default) | upstream baseline fetched from `raw.githubusercontent.com` at the pinned tag — local use |
| `--juce-root DIR` | upstream baseline read from a pristine JUCE tree — the CI mode, zero network |
| `--update` | regenerate the manifest (human, during a bump). Never in CI |
| `--check-wiring` | assert the guard precedes every override copy in both workflows |

Four checks, each with a distinct actionable failure message:

1. manifest `juce_version` == `.github/juce-version.txt` (the bump tripwire)
2. every override file's normalized sha == recorded `<override_sha>`
3. every upstream file's normalized sha == recorded `<upstream_sha>` (proves derivation)
4. set equality between manifest entries and files on disk (nothing rides along, nothing vanishes)

**`vendored/JUCE-overrides/MANIFEST.txt`** — `juce_version 8.0.14` plus two `override` rows. Recorded
upstream fingerprints equal the values independently observed at planning time:

| file | upstream sha256 (CR-stripped) | override sha256 (CR-stripped) |
|---|---|---|
| `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` | `4b8ff18d…fec4e2` | `c02f2774…a416ec8c` |
| `modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h` | `5488c7ef…c2a4c0` | `4e2b9023…739ed720` |

**Wiring** — a `Verify JUCE overrides provenance` step (`shell: bash`,
`bash scripts/check-juce-overrides.sh --juce-root JUCE`) inserted between `Setup JUCE` and the
override copy in all four jobs: `build-macos` and `build-windows` (build-and-release.yml),
`probes-macos` and `windows-vst3` (ci-tests.yml). Each carries a comment explaining why it sits
before the copy and not after.

## Negative Controls — Verbatim Evidence

Every control was observed firing. Exit codes and the naming line of each failure message:

**NC1 — version drift** (`juce_version 8.0.14` → `8.0.13`). **EXIT=1**
```
[check-juce-overrides] CHECK 1 (version): overrides were cut from JUCE 8.0.13 but .github/juce-version.txt pins 8.0.14.
  The vendored overrides are STALE for the pinned JUCE. Copying them would
  write JUCE 8.0.13 sources over a JUCE 8.0.14 tree.
[check-juce-overrides] FAILED — 1 check(s) did not pass.
[check-juce-overrides] Refusing to vouch for vendored/JUCE-overrides/ against JUCE 8.0.14.
```

**NC2 — upstream fingerprint drift** (one hex char: `4b8ff18d…` → `4b8ff18e…`). **EXIT=1**
```
[check-juce-overrides] CHECK 3 (upstream provenance): modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp was NOT derived from the upstream file now at JUCE 8.0.14.
    recorded upstream: 4b8ff18ef1d37677cf1be315ae0616fd10d691d4a69ca29a57e2504649fec4e2
    actual upstream:   4b8ff18df1d37677cf1be315ae0616fd10d691d4a69ca29a57e2504649fec4e2
[check-juce-overrides] BAD modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
[check-juce-overrides] FAILED — 1 check(s) did not pass.
```
Confirms check 3 is reached and not short-circuited by check 1.

**NC3 — override fingerprint drift** (`c02f2774418a7335` → `…7336`). **EXIT=1**
```
[check-juce-overrides] CHECK 2 (override fingerprint): modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp has changed since the manifest was written.
    recorded: c02f2774418a73364eb1944888972eba4e474c5fd78a732b8f9c5318a416ec8c
    actual:   c02f2774418a73354eb1944888972eba4e474c5fd78a732b8f9c5318a416ec8c
[check-juce-overrides] BAD modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
[check-juce-overrides] FAILED — 1 check(s) did not pass.
```

**NC4 — non-pristine upstream root** (`--juce-root /Users/taylorbrook/JUCE`, a really-patched tree).
**EXIT=1**, and the strongest control because it uses a real patched tree, not a synthetic edit.
Both files rejected:
```
[check-juce-overrides] upstream baseline: /Users/taylorbrook/JUCE (pristine tree)
[check-juce-overrides] CHECK 3 (upstream provenance): modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp was NOT derived from the upstream file now at JUCE 8.0.14.
[check-juce-overrides] BAD modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
[check-juce-overrides] CHECK 3 (upstream provenance): modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h was NOT derived from the upstream file now at JUCE 8.0.14.
[check-juce-overrides] BAD modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h
[check-juce-overrides] FAILED — 2 check(s) did not pass.
```

### Controls added beyond the plan

The plan listed four; must-have truth 5 (inventory) and the `--check-wiring` gate were unfalsified,
so four more were run. All manifest/workflow-only, all restored.

**NC5a — manifest row deleted** (file on disk, unlisted). **EXIT=1**
```
[check-juce-overrides] CHECK 4 (inventory): manifest entries do not match the files on disk.
  Only on disk (present but unlisted — an unlisted file cannot ride along):
    modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h
```

**NC5b — phantom manifest row** (listed, absent from disk). **EXIT=1**
```
[check-juce-overrides] CHECK 4 (inventory): manifest entries do not match the files on disk.
  Only in manifest (listed but missing from disk):
    modules/juce_core/juce_core.cpp
```

**NC6a — one guard invocation removed** from ci-tests.yml (2 copies, 1 guard). **EXIT=1**
```
[check-juce-overrides] .github/workflows/ci-tests.yml: 2 override copy site(s) but 1 guard invocation(s).
[check-juce-overrides] CI wiring check FAILED (1 problem(s)).
```

**NC6b — guard moved after its copy.** **EXIT=1**
```
[check-juce-overrides] .github/workflows/ci-tests.yml: guard at line 140 does NOT precede its copy at line 139.
[check-juce-overrides]   The guard must run against the PRISTINE JUCE tree; after the copy
[check-juce-overrides]   there is no pristine upstream left to fingerprint.
```

### Positive control — the one that mattered most

Every `--juce-root` observation so far was a *failure*. A gate that can only fail would have been
wired red into four jobs. So the real CI input was fetched — the pristine `juce-8.0.14-osx.zip`
(52,613,143 bytes) from the GitHub release — and the two overridden paths extracted:

```
[check-juce-overrides] upstream baseline: .../scratchpad/JUCE (pristine tree)
[check-juce-overrides] OK  modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
[check-juce-overrides] OK  modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h
[check-juce-overrides] PASS — overrides provably derived from JUCE 8.0.14.
EXIT=0
```

Full CI sequence simulated against that tree: guard (exit 0) → `cp -R` → both `JUCE-NE-PATCH` greps
still pass. The existing post-copy fail-fast is unaffected.

And the headline scenario, end-to-end: pin bumped to `8.0.15` with overrides not re-cut →
**guard exit 1 at check 1, copy never runs**, stale 8.0.14 bytes never reach the JUCE tree.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Guard printed `OK <path>` for a file that had just failed check 2**
- **Found during:** Task 2, NC3
- **Issue:** the per-file green line sat in the `else` branch of check 3 only, so a file failing
  check 2 printed `OK` directly beneath its own failure message — a contradicting green line is
  worse than no line, and in a long CI log it is what a reader skims to.
- **Fix:** per-file `file_ok` flag; print `OK` only when both per-file checks clear, else `BAD` to stderr.
- **Files modified:** `scripts/check-juce-overrides.sh`
- **Commit:** `cf6e6d69`. All controls re-run from the top afterwards, per the plan's instruction.

**2. [Rule 1 - Bug] `--check-wiring` printed its green per-workflow summary under its own failure**
- **Found during:** Task 3, NC6b
- **Issue:** same defect class in the wiring path — after reporting `guard at line 140 does NOT
  precede its copy at line 139`, it still printed `2 guard invocation(s), each before its copy.`
- **Fix:** per-workflow `wf_ok` flag gating the summary line.
- **Files modified:** `scripts/check-juce-overrides.sh`
- **Commit:** `95123893`

### Scope additions

- Four extra negative controls (NC5a/b, NC6a/b) — checks 4 and `--check-wiring` were otherwise
  unfalsified, and must-have truth 5 requires check 4 to be live.
- One positive control against the real release zip (above). Not in the plan; without it the CI
  mode had only ever been observed failing.

## Corrections to Planning-Time Findings

- **F3 is partly wrong.** The plan states "the release zip CI downloads is LF". It is **CRLF**:
  the extracted `juce_audio_plugin_client_VST3.cpp` carries 4062 CR bytes and
  `juce_VST3ClientExtensions.h` carries 305 — exactly their line counts. The CR-normalization
  decision is unaffected (and still necessary — the vendored CRLF vs. an unattributed Windows
  checkout remains uncontrolled), but the stated reason "a byte compare fails on line endings on at
  least one axis" holds for a different axis than the plan named.
- **F6 line numbers** were stale as expected; sites re-grepped before editing. `ci-tests.yml`
  probes-macos copy was at 131, windows-vst3 at 329; `build-and-release.yml` at 160 and 525.

## Verification

| # | Criterion | Result |
|---|---|---|
| 1 | guard exits 0 on unmodified repo | PASS |
| 2 | four negative controls exit non-zero, each naming its check | PASS (8 controls run) |
| 3 | `--check-wiring` exits 0 | PASS |
| 4 | both workflows parse as YAML, guard invoked 2× each before every copy | PASS |
| 5 | `git diff --stat HEAD~3 -- .github/juce-version.txt vendored/JUCE-overrides/modules/` empty | PASS (empty) |
| 6 | no commit contains `.claude/agent-memory/…`; `plugins/O-Orbit/libs/SAF` untouched | PASS (0 / empty) |
| + | guard PASSES against the pristine release zip (CI will be green, not red) | PASS |

Submodule guard run before all three commits: `SUBMODULE_GUARD_OK` each time.

## Commits

| Hash | Message |
|---|---|
| `6236600b` | feat(quick-260921-ha2): JUCE override provenance guard + manifest |
| `cf6e6d69` | fix(quick-260921-ha2): do not print OK for a file that failed check 2 |
| `95123893` | feat(quick-260921-ha2): run the provenance guard before all four override copies |

## Known Stubs

None.

## Disclosed Blindness

Documented in the script header and the manifest header, not only here:

- **Line-ending-only changes are invisible.** Deliberate — `cp -R` is byte-preserving, neither CMake
  nor the compiler cares about CR, and normalizing is what makes one fingerprint valid across the
  vendored CRLF, the release zip, the git tag, and a Windows checkout.
- **Not insider-resistant.** Anyone can re-run `--update` to launder an edited override (T-ha2-03,
  accepted). The manifest delta is plain text and visible in review; this gate targets silent drift.
- **The JUCE download itself is unverified** — only the two-file override surface layered on it.
- **`--check-wiring` is text/line-number analysis**, not YAML semantics. It would not catch a guard
  step placed correctly in the file but disabled by an `if:` condition.

## Threat Flags

None. No new network endpoint, auth path, or schema introduced. The one new network call
(`raw.githubusercontent.com`, local mode only) is pinned to `--proto '=https' --tlsv1.2 --fail`
per T-ha2-02 and is never used in CI.

## Self-Check: PASSED

- `scripts/check-juce-overrides.sh` — FOUND, executable
- `vendored/JUCE-overrides/MANIFEST.txt` — FOUND
- Commits `6236600b`, `cf6e6d69`, `95123893` — all FOUND in `git log --all`
- Measured commit count `83a5abf6..HEAD` = 3, matching `actuals.commits`
- Working tree carries only the pre-existing unrelated
  `M .claude/agent-memory/research-planning-agent.md`
