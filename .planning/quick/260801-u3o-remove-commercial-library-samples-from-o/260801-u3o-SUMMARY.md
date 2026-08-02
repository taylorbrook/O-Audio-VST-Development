---
quick_id: 260801-u3o
task: Remove commercial-library samples from O-simpleSampler and document provenance of the remaining (cleared) sample sets
slug: 260801-u3o-remove-commercial-library-samples-from-o
date: 2026-08-01
mode: quick
status: complete
tasks_completed: 5
tasks_total: 6
blocked_on: "Task 6 — blocking human-verify DAW checkpoint (developer only)"
requirements: [L4-2.2]
plugin: O-simpleSampler
version: 1.1.0
subsystem: plugin-assets, licensing, release-readiness
tags: [licensing, provenance, agpl, public-release, apvts-contract, webview-ui]
metrics:
  duration: ~55 min
  auval_global_scope_parameters: 20
  auval_component_version: 1.1.0
  pluginval_runs: 6
  pluginval_failures: 0
  render_harness: 9/9
commits:
  - 22796a4a  # feat(O-simpleSampler)!: withdraw commercial-library built-ins
  - 5b4c4fc3  # feat(O-simpleSampler): remove dead source-selector combo from web assets
  - 54b9ec80  # chore(O-simpleSampler): bump 1.1.0; changelog + parameter-spec
  - ee9a7f81  # test(O-simpleSampler): verification battery for v1.1.0
  - ddd6f6c5  # docs: provenance for cleared sample sets; correct readiness 2.2
key-files:
  created:
    - plugins/O-simpleGrain/Source/samples/LICENSE.md
    - plugins/O-simpleSampler/Source/samples/LICENSE.md
    - plugins/O-MicrotonalSampler/tests/fixtures/4-layer/LICENSE.md
    - .planning/quick/260801-u3o-remove-commercial-library-samples-from-o/260801-u3o-VERIFICATION.md
  deleted:
    - plugins/O-simpleSampler/Source/samples/cello.aif
    - plugins/O-simpleSampler/Source/samples/pizz.aif
    - plugins/O-simpleSampler/Source/samples/hit.wav
  modified:
    - plugins/O-simpleSampler/CMakeLists.txt
    - plugins/O-simpleSampler/Source/PluginProcessor.{h,cpp}
    - plugins/O-simpleSampler/Source/PluginEditor.cpp
    - plugins/O-simpleSampler/Source/ui/public/{index.html,js/app.js,css/styles.css}
    - plugins/O-simpleSampler/tests/render-harness/{main.cpp,CMakeLists.txt}
    - plugins/O-simpleSampler/CHANGELOG.md
    - plugins/O-simpleSampler/.planning/{parameter-spec.md,STATUS.md,stages/4-polish/VERIFICATION.md}
    - PUBLIC-RELEASE-READINESS.md
---

# Remove commercial-library samples from O-simpleSampler — Summary

Three commercial-sample-library assets withdrawn from O-simpleSampler with no replacement,
the source-selector parameter dropped with them (21 → 20 APVTS params), the plugin rebuilt
and fully re-verified at v1.1.0, and provenance documents written for the three sample sets
that were already clear. PUBLIC-RELEASE-READINESS.md §2.2 is closed on its merits — with one
exposure deliberately left open and now impossible to miss.

## What was done

**Tasks 1–5 complete and committed.** Task 6 is a `checkpoint:human-verify` DAW test and was
not attempted — it requires the developer.

| # | Task | Commit |
|---|---|---|
| 1 | Withdraw the three assets; collapse the built-in set to piano (C++, CMake, harness) | `22796a4a` |
| 2 | Strip the dead source-selector from the web assets; surface the active built-in | `5b4c4fc3` |
| 3 | Version 1.1.0, CHANGELOG, parameter-spec, STATUS, stale UAT checkbox | `54b9ec80` |
| 4 | Build, install, and run the full verification battery | `ee9a7f81` |
| 5 | Provenance documents + corrected readiness record | `ddd6f6c5` |

## Verification results (real numbers)

**auval** — `AU VALIDATION SUCCEEDED`, exit 0.

- **20 Global Scope Parameters** (down from 21, exactly as D-01 predicted)
- **Component Version: 1.1.0 (0x10100)** — proves the `VERSION` keyword took effect

**pluginval strictness 10 — six runs, all exit 0:**

| Format | Run 1 | Run 2 | Run 3 |
|---|---|---|---|
| VST3 | exit **0** | exit **0** | exit **0** |
| AU | exit **0** | exit **0** | exit **0** |

Every log was independently scanned: **0** `nan` matches, **0** standalone `inf` matches,
**0** `FAILED` matches, in all six. Each reached and completed the `Fuzz parameters` stage
and terminated `SUCCESS`.

**Render harness — 9/9, 0 failures.** Rebuilt and re-run in Task 4 rather than trusting the
Task 1 run. Figures bit-identical to Task 1; `repitch-tuning` still reports f48 = 131.2 Hz,
confirming the root-48 seed survives with the selector gone.

**Installed bundles** — `-dev` VST3 + AU at 6.2 M each, **no unsuffixed orphan present**.
Zero withdrawn filenames inside either bundle, and `strings` over both Mach-O executables
returns **0** matches for the withdrawn tokens.

**Headless UI render** — `index.html` loaded against a JUCE-bridge stub in headless Chromium,
driven over CDP: **0 uncaught exceptions, 0 console errors/warnings**, 2 populated `<select>`
elements, no `<select>` left in the Source group, status line rendering
`piano — built-in source`, 17/17 knob readouts populated, and a live combo round-trip.

Full detail: `260801-u3o-VERIFICATION.md` in this directory.

## Provenance documents written

| Path | Covers | Generator |
|---|---|---|
| `plugins/O-simpleGrain/Source/samples/LICENSE.md` | `fire/piano/voice/water.wav` | `tools/generate_samples.py`, `SEED = 20260624`, bit-reproducible |
| `plugins/O-simpleSampler/Source/samples/LICENSE.md` | `piano.wav` (+ a record of what was removed) | same generator — byte-identical file, identical MD5 |
| `plugins/O-MicrotonalSampler/tests/fixtures/4-layer/LICENSE.md` | `C4_v1..v4.wav` | `generate.py`, 440 Hz sines; **test fixtures, never embedded** |

Each records a **per-file MD5 measured from disk**. The Task 5 gate recomputes every hash and
requires it to appear in the neighbouring document, so a drifted (and therefore false)
provenance claim fails loudly.

## Readiness record corrected

§2.2 was factually broader than the evidence supported, and the document now says so:

- The real blocker was **three** files, not twelve. Nine were provably self-authored.
- **Four of the twelve were never embedded in any binary** — the O-MicrotonalSampler fixtures
  are referenced by no `juce_add_binary_data` target. The "compiled into distributed plugin
  binaries" claim did not hold for them.
- **Neither affected plugin has ever been released** (38 releases repo-wide, zero for either),
  so no signed or notarised artefact ever contained the commercial samples.

§2.2 and §5.4 are marked resolved in §2.1's style; §1's verdict, §4.6, and the §6 checklist
were updated so the document stays internally consistent. §2.3, §2.4, and all of §3 untouched.

---

## ⚠️ CARRY-FORWARD for `.planning/STATE.md` — open decision, not a task

**The three withdrawn commercial-library samples remain in git history at commit
`4ca27977`** (2026-07-02, "feat: O-simpleSampler v1.0.0"). `git rm` in a later commit does
not remove a file from the commits that already contain it.

**If this repository is published with its current history, the commercial samples are still
public** — recoverable by anyone who clones and checks out that commit. Nothing in this task
changed that, by explicit instruction: **no history rewrite was performed.**

The developer must resolve this before flipping visibility:

- **Rewrite** — `git filter-repo` over those three paths (§4.6 has the exact path list ready
  to paste). Cost: every commit SHA changes; existing clones and links into history break;
  the `plugins/O-Orbit/libs/SAF` gitlink needs post-rewrite verification.
- **Accept** — publish with current history and record the decision. Defensible for three
  assets in one commit of a plugin that never shipped, but it must be a *made* decision.

Recorded in the readiness document as **checklist step 3b** (gating step 15), with
cross-references from §1, §2.2, §4.6, and step 14 — four independent paths to it, so it
cannot be skipped by reading only one section. Also noted in
`plugins/O-simpleSampler/.planning/STATUS.md` and in `Source/samples/LICENSE.md`.

**Side effect worth knowing:** §4.6 previously framed the history rewrite as "genuinely
optional — it reclaims repository size and nothing else". That framing no longer holds
unconditionally, and §4.6 has been amended to say so.

## Known limitation (flagged for the Task 6 DAW check)

**The seeded source-status line is a static string, not a query.** The UI has no native
function returning the live source identity, and Task 2's gate forbids C++ changes, so the
status line is seeded at boot with the literal built-in name. On a **restored session that
had a user file loaded**, the line will read `piano — built-in source` until the next load,
even though the restored user file is what is actually playing (the audio is correct; only
the label is stale). Closing this properly needs a `getSourceIdentity` native function — one
C++ method, one JS call — which was out of scope here. Worth deciding on during Task 6 step 7.

## Deviations from plan

**1. [Rule 1 — Bug] Unused-parameter warning introduced by the Task 1 edit.**
Removing the selector branch from `parameterChanged` left `newValue` unread, producing a new
`-Wunused-parameter` warning that did not exist before. Fixed in the same commit with
`juce::ignoreUnused (newValue)` plus a comment explaining that only the marker IDs matter.
Commit `22796a4a`.

**2. [Rule 2 — Accuracy] Stale parameter counts in prose adjacent to the edits.**
`PluginEditor.cpp` (file header, the `21 APVTS params` comment) and `app.js`'s header both
counted 21 params / 3 combos. These are outside the grep gate (no dead token), so nothing
would have caught them, but leaving a contract document's count wrong next to the code that
changed it is how the next reader gets misled. Updated to 20 / 2. Commits `22796a4a`, `5b4c4fc3`.

**3. [Rule 2 — Consistency] §1 of the readiness document was not in the plan's edit list.**
It asserted "the remaining blocker stands", which the rest of the task made false. The plan
required §2.2 be marked resolved "so the document stays internally consistent"; leaving §1
contradicting §2.2 would have defeated that. §1 now records both blockers closed and carries
the open history decision. §2.3, §2.4, and §3 were left untouched as instructed. Commit `ddd6f6c5`.

**4. [Tooling] Playwright's npm package is not installed; the headless render used CDP directly.**
Chromium binaries are cached (`~/Library/Caches/ms-playwright/chromium-1234`) but the driver
package is absent, and installing a package is not an auto-fixable action. `--dump-dom` has
been removed from Chrome 151, so the render check drives the cached Chromium over the DevTools
Protocol from a `node --experimental-websocket` script instead. Equivalent evidence — it
executes the page and reads the live DOM plus the console/exception streams — and it is
throwaway (scratchpad only, nothing added to the repo).

## Self-Check: PASSED

All created files verified present on disk; all five commit hashes verified in `git log`.
