---
phase: quick-260925-r5c
plan: 01
subsystem: O-AnalogEQ
tags: [code-review, read-only, i18n, webview, rt-safety, shared-module]
status: complete
requires: []
provides:
  - plugins/O-AnalogEQ/.planning/CODE-REVIEW.md (v1.5.0, depth thorough)
affects:
  - modules/persistence/preset-manager (WR-10 — finding recorded, NOT fixed)
  - scripts/check-i18n.js (WR-10 — gate scan gap recorded, NOT fixed)
tech-stack:
  added: []
  patterns: []
key-files:
  created: []
  modified:
    - plugins/O-AnalogEQ/.planning/CODE-REVIEW.md
decisions:
  - "All 10 prior findings verdict FIXED — 0 regressions across 13 versions"
  - "CR-02 (isBusesLayoutSupported absent) tiered critical despite PLAUSIBLE reachability: crash mechanism is confirmed in JUCE source and the fix is 4 lines"
  - "IN-07 deliberately NOT escalated to critical — adapterTable is StringRef-keyed, so the per-block map lookup allocates nothing"
  - "WR-10 scoped shared-module + repo-wide gate; must not be fixed plugin-locally"
metrics:
  duration: ~28min
  completed: 2026-09-25
  tasks: 3
  files: 1
actuals:
  tokens: 31000
  tasks: 3
  commits: 3
plan_head_before: 2b27e179
---

# Quick Task 260925-r5c: Thorough Code Review of O-AnalogEQ v1.5.0 — Summary

Replaced the stale `depth: standard` review of v1.1.7 with a `depth: thorough` review of
**v1.5.0** across **10 files**, re-adjudicating all ten prior findings and covering the
four surfaces that shipped in 1.2.0–1.5.0 and had never been reviewed. **Read-only: no
code, test, build file, changelog or shared module was touched.**

## Versions elapsed since the prior review

Prior review: **v1.1.7**, 2026-06-30, `depth: standard`, 6 files.
This review: **v1.5.0**, 2026-09-25, `depth: thorough`, 10 files.

**Thirteen versions elapsed** — 1.1.8, 1.1.9, 1.1.10, 1.1.11, then 1.2.0 (French +
settings popover), 1.3.0 (hover-help), 1.3.1 (French copy revision), 1.4.0 (the
`oaeq.tipsEnabled` switch), 1.4.1 (`Infobulles`), 1.5.0 (Simplified Chinese).

## Verdict split across the ten prior findings

**10 FIXED, 0 STILL OPEN, 0 SUPERSEDED, 0 NOT-A-BUG.** Every one was resolved in the
v1.1.9 / v1.1.10 window and **none has regressed** in the thirteen versions since.

| Finding | Verdict | Fixed in |
|---------|---------|----------|
| CR-01 audio-thread coefficient allocation | FIXED | `1cef1017` — `ArrayCoefficients` + `*Moving` gate |
| WR-01 frequency readouts ignore skew | FIXED | `1cef1017` — `FREQ_SKEW` / `toHz` |
| WR-02 no parameter smoothing | FIXED | `1cef1017` — 8 `SmoothedValue`, 32-sample chunks |
| WR-03 HF cutoff not Nyquist-clamped | FIXED | `1cef1017` — `clampFreq` on **all four** bands |
| WR-04 FileChooser captures `this` | FIXED | `1cef1017` — `SafePointer` + bail |
| IN-01 `output_gain` has no UI control | FIXED | `2f973a53` — decision documented in code |
| IN-02 double-click reset uses 0.5 | FIXED | `1cef1017` — `toNorm` from real C++ defaults |
| IN-03 dead `currentParamName` | FIXED | `2f973a53` — zero grep matches |
| IN-04 `_waitForNative` unbounded poll | FIXED | `2f973a53` — 100 × 50 ms bound |
| IN-05 `promptDelete` uses `confirm()` | FIXED | `2f973a53` — `onConfirmDelete` hook + fail-safe |

Three prior leads were tested and **disproved**, each recorded in the artifact rather than
left unstated:

- **WR-04's bail path is correct, not a hung promise.** Both `SafePointer` null paths
  `return` without calling `complete`. That is what
  `pattern_webview_launchasync_safepointer_no_complete` prescribes — calling `complete`
  on the dead-editor path is itself the use-after-free, because JUCE 8's completion holds
  a raw pointer into the destroyed `WebBrowserComponent::Impl`.
- **`label.level` is not an `output_gain` control.** It is the VU meter caption at
  `index.html:1076`. IN-01's resolution stands.
- **The stale-`<CustomState>` bug class is NOT-APPLICABLE.** O-AnalogEQ registers no
  custom-state callbacks (grep recorded as evidence).

## New findings: 1 critical, 6 warning, 4 info — 11 open

Counts are **new findings only**. A prior finding adjudicated FIXED does not count toward
them; 11-vs-10 is a first pass over four unreviewed surfaces, not a regression.

| Tier | IDs | One-liners |
|------|-----|------------|
| Critical (1) | **CR-02** | `isBusesLayoutSupported` not overridden → a 2-in/1-out negotiation null-derefs `processors[1]` in every band's `ProcessorDuplicator` |
| Warning (6) | **WR-05** | `output_gain` is not smoothed — `dsp::Gain`'s default ramp is **0 s** and `setRampDurationSeconds` is never called. The prior review's WR-02 text asserted the opposite and excluded it on that basis |
| | **WR-06** | Band on/off is a hard bypass over **stale** IIR state — click on toggle, transient burst on re-enable |
| | **WR-07** | "Save Preset" dialog discards the chosen directory; `savePresetWithDialog` calls `savePreset()` instead of the module's `savePresetToFile()`. Can silently overwrite a library preset |
| | **WR-08** | The v1.4.0 hover-help switch is **entirely ungated** — no test drives `tipsEnabled === false` or the `data-tip-always` bypass |
| | **WR-09** | The switch's **Off** label is never geometry-measured in any language — `i18n-states.json` has two states, both opens |
| | **WR-10** | `check-i18n` [12] scans **1** module; the page ships **2**. Five English `title=` in the shared module sit in the blind half |
| Info (4) | **IN-06** | Settings-popover contract comments still describe a one-row panel, two minors after it grew a second row |
| | **IN-07** | 16 `getRawParameterValue` lookups per block — a map walk, **not** an allocation (recorded so it is not re-raised as a blocker) |
| | **IN-08** | A second dialog launch destroys the first `FileChooser` mid-flight |
| | **IN-09** | The page's init log still announces **v1.3.1** |

## Gate exit codes — all four zero

| Gate | Exit | Result |
|------|------|--------|
| `check-i18n --plugin O-AnalogEQ` | **0** | `ALL CHECKS PASS`, 0/35 French unreviewed, canon v2 |
| `i18n-fr-lint --plugin O-AnalogEQ --verbose` | **0** | `CLEAN` — 49 rows, 0 findings |
| `i18n-zh-lint --plugin O-AnalogEQ --verbose` | **0** | `GATE PASSED` — 35 zh entries, 0 findings, 0 at `'mt'` |
| `check-ui-labels --plugin O-AnalogEQ --verbose` | **0** | `ALL CHECKS PASSED` on en/fr/zh-Hans, 920×220 on every arm |

The four zeros **constrain** the review: no finding claims a missing key, a mistyped
string, a clipped label at the shipping frame, or localized copy reaching `innerHTML`.
Two findings (**WR-09**, **WR-10**) are precisely about what these gates never enter —
the switch's Off state, and the second shipped JS module.

## Shared-module rather than plugin-local

`diff modules/persistence/preset-manager/js/preset-manager.js
plugins/O-AnalogEQ/Source/ui/public/modules/preset-manager.js` returns **zero lines** —
the plugin's copy is byte-identical to the canonical module at **v1.0.8**
(`modules/registry.yaml:140`), with ~30 consumers.

- **IN-04** and **IN-05** (prior, both FIXED) are tagged shared-module.
- **WR-10** (new) is shared-module **and** repo-wide: its fix lands in
  `modules/persistence/preset-manager/js/preset-manager.js` **and**
  `scripts/check-i18n.js`, never in the plugin's copy. A plugin-local edit forks the
  module for one consumer and silently diverges the other ~30.

**WR-07** is the mirror case worth noting: it *looks* like a module bug and is not — the
module already exposes the correct `savePresetToFile` API and has since v1.0.8; the
plugin's editor simply calls the wrong one. Fix is plugin-local.

## Artifact-path mismatch — action needed before `/improve-review`

`.claude/commands/improve-review.md` declares a **blocking** precondition on
`plugins/[PluginName]/CODE_REVIEW.md` (underscore, plugin root). **25 plugins** use that
path; O-AnalogEQ and **6 others** (`O-DigiDelay`, `O-Freeze`, `O-Gain`,
`O-MultiBandCompressor`, `O-Polystutter`, `O-Prism`) use `.planning/CODE-REVIEW.md`
(hyphen).

**Consequence:** `/improve-review O-AnalogEQ` will **reject** with "No CODE_REVIEW.md
found for O-AnalogEQ" even though this review exists. `/improve-review-info O-AnalogEQ`
rejects identically. Point either command at `plugins/O-AnalogEQ/.planning/CODE-REVIEW.md`
explicitly, or resolve the path split repo-wide. The frontmatter key set here matches what
those commands parse, so only the path needs adapting. D-03 pinned the write path, so the
file was deliberately **not** moved.

## Deviations from Plan

### 1. [Rule 3 - Blocking] Task 2's three parallel review agents were run inline

- **Found during:** Task 2
- **Issue:** The plan directs dispatching three read-only review agents in a single
  parallel batch. **No Agent/Task tool exists in this executor's toolset** — the available
  tools are Read, Write, Edit, Bash, Skill and the Context7 MCP pair. Subagent dispatch was
  not possible.
- **Fix:** Executed all three briefs directly and sequentially, against the exact scopes,
  pattern-memory citations and gate commands the plan specifies for Agents A, B and C. The
  merge step was unaffected — the plan already names the executor as the single writer.
- **Effect on output:** None on coverage. All three scopes are covered and all four gates
  were run with `--verbose` as specified. The cost is wall-clock, not completeness.
- **Files modified:** none beyond the authorized artifact.
- **Commit:** `c549010a`

### 2. [Rule 3 - Blocking] Stale real index after temp-index commits

- **Found during:** Tasks 1–3
- **Issue:** The concurrent-session protocol mandates committing through a temp
  `GIT_INDEX_FILE`. That leaves the **real** `.git/index` holding the pre-commit blob, so
  `git status` reports the just-committed file as `MM` — misleading, and a foreign
  `git commit` in a sibling session could pick up the stale entry.
- **Fix:** After each temp-index commit, ran a **path-scoped**
  `git reset -q -- plugins/O-AnalogEQ/.planning/CODE-REVIEW.md` to refresh that one index
  entry from HEAD. Path-scoped deliberately: a bare `git reset` would discard a concurrent
  session's staged work.
- **Commit:** n/a (index hygiene, no content change)

### 3. Note — a concurrent session committed mid-run

`HEAD` at dispatch was `2b27e179`. By the first commit, `8936dea4
docs(O-Formant): STATUS.md version 1.33.0` had landed from another session. Expected in a
trunk-based shared checkout; every commit here is path-scoped and verified to carry
exactly one file under `plugins/`. No foreign work was swept in.

## Threat Register Verdicts

All six mitigate-disposition threats were adjudicated and recorded in the artifact.

| Threat | Verdict |
|--------|---------|
| T-r5c-01 resource-provider URL mapping | **Mitigated** — 7 exact string equalities, no wildcard, no path construction, no filesystem access |
| T-r5c-02 preset name → filesystem | **Mitigated** — `sanitizePresetName` strips `/ \ :` at all 6 call sites |
| T-r5c-03 i18n / tooltip copy → DOM | **Mitigated** — `textContent` / `createTextNode` on every path; the one `innerHTML` is a `= ''` clear |
| T-r5c-04 `localStorage['oaeq.tipsEnabled']` | **Mitigated** — single `!== 'false'` test, `try/catch` on both get and set, fail-direction is ON |
| T-r5c-05 shared git index | **Mitigated** — temp-index commits, path-scoped, re-checked immediately before each; verified exactly 1 file under `plugins/` per commit |
| T-r5c-06 unresolved native-fn promise | **Mitigated** — all 12 registrations complete on every path; the 2 that do not are WR-04's dead-editor bails, correct per pattern memory |

No new threat surface was introduced (this task ships no code), so there are no threat
flags.

## Commits

| Task | Commit | Files |
|------|--------|-------|
| 1 (tracer) | `2709aaa7` | `plugins/O-AnalogEQ/.planning/CODE-REVIEW.md` |
| 2 | `c549010a` | `plugins/O-AnalogEQ/.planning/CODE-REVIEW.md` |
| 3 | `5c63d2c7` | `plugins/O-AnalogEQ/.planning/CODE-REVIEW.md` |

Three commits, one file each, all on `main`, none carrying a concurrent session's work.

## Read-only assertion

```
git diff --quiet 2b27e179 HEAD -- plugins/O-AnalogEQ/Source plugins/O-AnalogEQ/tests \
  plugins/O-AnalogEQ/CMakeLists.txt plugins/O-AnalogEQ/CHANGELOG.md modules/
→ D-03 OK: byte-identical to dispatch-time HEAD across the whole run
```

No build, no install, no `auval`, no version bump, no tag, no `PLUGINS.md` edit.

## Known Stubs

None. The artifact is complete — every finding names its current file, its construct, its
failure scenario and its prescribed fix, so a later `/improve-review` run can act without
re-investigating.

## Self-Check: PASSED

- `plugins/O-AnalogEQ/.planning/CODE-REVIEW.md` — **FOUND**
- `.planning/quick/260925-r5c-thorough-code-review-of-o-analogeq-plugi/260925-r5c-SUMMARY.md` — **FOUND**
- Commit `2709aaa7` — **FOUND**
- Commit `c549010a` — **FOUND**
- Commit `5c63d2c7` — **FOUND**
- Task 1 verify (frontmatter keys, 10 prior IDs, verdict tokens, `## New Findings`) — **PASS**
- Task 2 verify (4 gates recorded, 6 surfaces named, labelled findings) — **PASS**
- Task 3 verify (counts reconcile 1/6/4=11, depth thorough, 10 files, status) — **PASS**
- Commit-scope verify (exactly 1 file under `plugins/` in HEAD, tree clean) — **PASS**
- Read-only verify (no source/test/build/changelog/module change, all 3 commits) — **PASS**
