---
title: "O-MicrotonalSampler Stage 4 (Polish) — Plan"
created: 2026-04-28
stage: 4-polish
phase: plan
status: ready_for_execute
inputs:
  - .planning/stages/4-polish/CONTEXT.md
  - .planning/stages/4-polish/RESEARCH.md
  - .planning/REQUIREMENTS.md
  - .planning/STATUS.md
  - .planning/stages/3-gui/VERIFICATION.md
verifies_requirements:
  - PERF-02   # 16 voices ≤ 5 % CPU, partial → complete
  - QUAL-01   # No clicks / zipper / aliasing across vel·poly·retune, partial → complete
---

# Stage 4 (Polish) — Execution Plan

## Goal

Close out v1.0 of O-MicrotonalSampler. Three concrete deliverables in
strict order:

1. **Version-pill plumbing** — replace the hard-coded `v0.1.0` literal
   in `index.html` `.about-card` with a runtime-resolved
   `JucePlugin_VersionString` via a `getPluginVersion` native function
   (mirror of `O-FreqPulse/Source/PluginEditor.cpp:215`).
2. **PERF-02 → complete** — run the Logic Pro CPU-meter protocol from
   `RESEARCH.md §RQ4-3`, log measured numbers in
   `VERIFICATION.md`, and flip `REQUIREMENTS.md` `PERF-02` from
   `partial` to `complete`.
3. **QUAL-01 → complete** — run the targeted artifact-pass listening
   checklist (Stage 2 `VERIFICATION.md` Human Verification +
   ±50 c retune sweep + voice-steal stress), log subjective verdict,
   and flip `QUAL-01` from `partial` to `complete`.

Final stage gate (Phase 4.4) raises the automated bar to
`pluginval --strictness-level 10` with seed pinning, plus AU validation,
Logic + Dorico smoke, and invariant greps. Stage 4 closes only when
4.4 is green.

**No edits to Stage 2 audio-thread code paths.** The version pill
touches editor + JS + HTML only. PERF-02 and QUAL-01 are
**measurement** tasks — no source change unless a defect surfaces, in
which case the relevant Stage 2 sub-phase reopens (per Stage 3 verify
pattern).

## Open-Question Resolutions (from RESEARCH.md)

| # | Decision |
|---|---|
| RQ4-1 | `JucePlugin_VersionString` is a compile-time string-literal macro available via `<JuceHeader.h>`. Use a `withNativeFunction("getPluginVersion", ...)` returning `juce::var(JucePlugin_VersionString)` — exact shape from `O-FreqPulse/Source/PluginEditor.cpp:215`. No build-time codegen, no `'v'` prefix in C++. |
| RQ4-2 | Correct flag is `--strictness-level`, NOT `--strictness`. Prior Stage 3 runs were silent strictness-5 fallback. Pin `--random-seed 0xC0FFEE` and `--timeout-ms 120000` so a fuzz failure is deterministically replayable. Run two variants: `--skip-gui-tests` AND with-GUI. VST3 only at strictness-10 (pluginval doesn't validate AU bundles). |
| RQ4-3 | Logic Pro per-track CPU is not isolated. Measure as **delta from baseline-with-transport** in the aggregate Performance Meter. Apple Silicon must be on power. 8-step protocol with VERIFICATION fields specified in RESEARCH §RQ4-3. |
| RQ4-4 | Dorico smoke: 11-step manual UI procedure (no `.doricoexpmap` distribution). Critical step: duplicate Default expression map and set Microtonality to **"VST3 Note Expression"** — Dorico ignores `INoteExpressionController`, Auto-mode silently uses pitch-bend. Test passage: C4 / ¼♯C4 / C4 / ¼♭C4 quarter-tone alternation. |

## Sub-stage Map

| Phase | Goal | Verifies | Gate |
|---|---|---|---|
| 4.1 | Version-pill native function + JS wire-up + HTML edit | (polish) | Triple build green; About pill reflects `JucePlugin_VersionString` (`1.0.0` for current build); pluginval-10 SUCCESS; no `v0.1.0` literal in `index.html` |
| 4.2 | PERF-02 — Logic Pro CPU-meter measurement run per RQ4-3 protocol | PERF-02 → complete | `delta_CPU_pct ≤ 5.0 %` at 16 voices / 48 k / 256 / Apple Silicon on power; results table written to VERIFICATION.md |
| 4.3 | QUAL-01 — targeted artifact-pass listening checklist | QUAL-01 → complete | All checklist items signed off subjectively; defects (if any) reopen Stage 2 sub-phase before 4.4 |
| 4.4 | Final stage gate + atomic commit + STATUS update | (closure) | pluginval --strictness-level 10 (skip-gui + with-gui) SUCCESS; auval SUCCEEDED; Logic smoke pass; Dorico smoke pass; invariant greps; cache-clear+install per CLAUDE.md |

Sub-stage order is **strict: 4.1 → 4.2 → 4.3 → 4.4.** A failure in
4.2 or 4.3 reopens the relevant Stage 2 sub-phase rather than being
absorbed. Only 4.4 closes Stage 4.

Each sub-stage commits atomically with a `gate-report.json` and
`PHASE-4.N-SUMMARY.md`, matching the Stage 2/3 cadence.

---

## Tasks

### Phase 4.1 — Version-pill plumbing

#### Task 1 — Add `getPluginVersion` native function
- [ ] Insert a new `.withNativeFunction("getPluginVersion", ...)` block in `Source/PluginEditor.cpp` between the existing `getOctaveStretch` (line 127) and `getEmbeddedTuningList` (line 137) blocks — keeps it grouped with the read-side accessors.
- [ ] Body returns `complete(juce::var(JucePlugin_VersionString));` — verbatim from `O-FreqPulse/Source/PluginEditor.cpp:215`. No `'v'` prefix; no fallback string.
- [ ] Confirm `<JuceHeader.h>` (or transitively included) provides `JucePlugin_VersionString` — the file already includes JUCE headers via `PluginEditor.h`.
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** none

#### Task 2 — HTML: empty version pill (defensive render)
- [ ] `Resources/ui/index.html:100` — replace `<div class="about-version">v0.1.0</div>` with `<div class="about-version" id="about-version"></div>`.
- [ ] Verify no other `v0.1.0` literal exists in `Resources/ui/`. Grep `grep -rn "v0\.1\.0" plugins/O-MicrotonalSampler/Resources/`.
- **Files:** `Resources/ui/index.html`
- **Depends on:** Task 1 (function must exist before HTML strips its fallback)

#### Task 3 — JS: `refreshAboutVersion` + wire to mount
- [ ] Add `async function refreshAboutVersion()` to `Resources/ui/js/sampler-app.js`, modeled on `refreshTuningReadout` (line 333). Body: `const fn = window.__JUCE__.backend.getNativeFunction("getPluginVersion"); const value = await fn(); const el = document.getElementById("about-version"); if (el) el.textContent = "v" + String(value);`.
- [ ] Call `refreshAboutVersion()` once at the same JUCE-init site that calls `refreshTuningReadout` (around line 256) — single shot, no cost, no need to gate on About-tab activation.
- [ ] Defensive: if `getPluginVersion` resolves to empty/undefined, leave the pill empty (no `v` prefix appended to nothing). Guard with `value && value.length`.
- **Files:** `Resources/ui/js/sampler-app.js`
- **Depends on:** Tasks 1, 2

#### Task 4 — Phase 4.1 gate
- [ ] Triple build (VST3 + AU + Standalone): `ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone` from `build/`.
- [ ] Cache-clear + install per CLAUDE.md (kill `AudioComponentRegistrar`, `rm -rf` AU caches + old bundles, copy fresh VST3 + AU into `~/Library/Audio/Plug-Ins/`).
- [ ] Open the Standalone — visually confirm About tab pill shows `v1.0.0` (the `PLUGIN_VERSION` from `CMakeLists.txt:14`).
- [ ] Grep guard: `grep -rn "v0\.1\.0" plugins/O-MicrotonalSampler/Resources/ plugins/O-MicrotonalSampler/Source/` returns zero hits.
- [ ] `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests` SUCCESS (smoke, full strictness-10 lives in 4.4).
- [ ] `auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED.
- [ ] Write `PHASE-4.1-SUMMARY.md` + `gate-report.json` (phase 4.1).
- [ ] Atomic commit: source + UI + summary + gate report in one commit.
- **Files:** `.planning/stages/4-polish/PHASE-4.1-SUMMARY.md`, `.planning/stages/4-polish/gate-report.json`
- **Depends on:** Tasks 1, 2, 3

---

### Phase 4.2 — PERF-02 CPU-meter measurement

No source-code edits in this phase. Pure measurement.

#### Task 5 — Configure Logic Pro per RQ4-3 protocol
- [ ] Logic Pro 11.x, Apple Silicon machine **plugged in** (battery throttling forbidden).
- [ ] `Settings → Audio → Devices`: Sample rate **48 kHz**, I/O Buffer **256**, Process Buffer Range **Default (Medium)**, Multithreading **on**.
- [ ] Disable Logic auto-backup, iCloud sync, and all unrelated background tasks.
- [ ] `View → Show Performance Meter` (or `⌥X`).
- **Depends on:** Phase 4.1 gate green (latest bundle installed)

#### Task 6 — Capture baseline + loaded readings (3 runs)
- [ ] New empty Logic project. Note **baseline aggregate CPU %** with transport stopped (target ≤ 1 %).
- [ ] Press play with no instruments — note **baseline-with-transport CPU %** (typically 1–2 %).
- [ ] Add one Software Instrument track. Insert `O-MicrotonalSampler (AU)`. Load a representative sample folder (≥ 60 cells; reuse Phase 2.5 verification fixture if present, else any folder with full vel-xfade coverage).
- [ ] Programme a 16-note held chord (4 octaves × 4 notes) at velocity 90, each held 4 bars, region looped.
- [ ] Press play. Wait 30 s for steady-state. Read aggregate **average** + **peak** CPU % from Performance Meter.
- [ ] Stop. Repeat play/read cycle twice more (3 readings total).
- [ ] Compute `delta_CPU_pct = loaded_avg − baseline_with_transport_avg` for each run.
- [ ] Take a confirmatory reading with the heaviest-cost active tuning (high-stretch microtonal preset) for comparison.
- **Depends on:** Task 5

#### Task 7 — Log results in VERIFICATION.md
- [ ] Create `.planning/stages/4-polish/VERIFICATION.md` (or extend existing structure) with the PERF-02 results table per RQ4-3 schema: hardware (chip, core count, power state), Logic version, sample rate / buffer, sample folder + cell count + total MB, tuning active (12-EDO + microtonal-stretch), baseline-with-transport %, loaded average %, loaded peak %, delta vs baseline, verdict.
- [ ] Decision: if `delta_CPU_pct ≤ 5.0 %` across all 3 runs → flip `REQUIREMENTS.md` row `PERF-02` from `partial` to `complete`, `verified at` = `stage-4`.
- [ ] If any run exceeds 5 % → **do not flip**; file the regression and route to the appropriate Stage 2 sub-phase (likely 2.4 voice-steal or 2.5 loop-detect for hot-path investigation). Stage 4 halts until reopened-phase ships green.
- **Files:** `.planning/stages/4-polish/VERIFICATION.md`, `.planning/REQUIREMENTS.md`
- **Depends on:** Task 6

#### Task 8 — Phase 4.2 gate
- [ ] PERF-02 row = `complete` in REQUIREMENTS.md.
- [ ] VERIFICATION.md PERF-02 section populated with all RQ4-3 fields.
- [ ] Write `PHASE-4.2-SUMMARY.md` + `gate-report.json` (phase 4.2).
- [ ] Atomic commit (data only — no source change).
- **Files:** `.planning/stages/4-polish/PHASE-4.2-SUMMARY.md`, `.planning/stages/4-polish/gate-report.json`
- **Depends on:** Tasks 5, 6, 7

---

### Phase 4.3 — QUAL-01 listening pass

No source-code edits. Subjective audit.

#### Task 9 — Targeted artifact pass (~10 min)
- [ ] Sustained sine: load a sustained-tone fixture, play C4 at vel 90, listen for zipper / DC / clicks across full envelope. Verify no artifact.
- [ ] Cello vibrato (or any organic legato source): play melodic phrase across loaded zone; listen for loop-boundary clicks, vel-xfade discontinuities, voice-steal pops.
- [ ] Transient (plucked / percussive): play repeated short hits at varying velocities; listen for one-shot fallback correctness (no looping artefact on transient material).
- [ ] ±50 c retune sweep: with NE-capable host (Dorico or NE-aware test rig), sweep retune from −50 c to +50 c on a sustained note; listen for zipper, alias, or pitch-tracking glitch.
- [ ] Voice-steal stress: play 24-note cluster (exceeds 16-voice cap) at vel 100, listen for the 5 ms steal ramp — should be inaudible at moderate velocity, audible-but-clean at extreme stress.
- [ ] Mixed-SR fixture (44.1 + 48 + 96 kHz files in same folder): play across pitches; verify Lagrange resample on load + Cubic-Hermite at runtime produce no audible degradation.
- [ ] Short-region loop edge case (loop length < 1024 samples): manually edit a loop to a short region via the loop-point editor, hold note, listen for buzz / artefact.
- **Depends on:** Phase 4.2 gate green

#### Task 10 — Log verdict in VERIFICATION.md
- [ ] Add QUAL-01 section to `.planning/stages/4-polish/VERIFICATION.md`. Each checklist item gets a `pass` / `defect` line. Defects (if any) include reproduction steps + the Stage 2 sub-phase that owns the fix.
- [ ] If all items pass → flip `REQUIREMENTS.md` row `QUAL-01` from `partial` to `complete`, `verified at` = `stage-4`.
- [ ] If any item fails → file the defect and reopen the relevant Stage 2 sub-phase (e.g. 2.1 Voice DSP for sine zipper; 2.5 Loop detect for boundary click). Stage 4 halts.
- **Files:** `.planning/stages/4-polish/VERIFICATION.md`, `.planning/REQUIREMENTS.md`
- **Depends on:** Task 9

#### Task 11 — Phase 4.3 gate
- [ ] QUAL-01 row = `complete` in REQUIREMENTS.md.
- [ ] VERIFICATION.md QUAL-01 section populated with the seven checklist outcomes.
- [ ] Write `PHASE-4.3-SUMMARY.md` + `gate-report.json` (phase 4.3).
- [ ] Atomic commit (data only).
- **Files:** `.planning/stages/4-polish/PHASE-4.3-SUMMARY.md`, `.planning/stages/4-polish/gate-report.json`
- **Depends on:** Tasks 9, 10

---

### Phase 4.4 — Final stage gate

#### Task 12 — Refresh artefacts at gate-time bundle
- [ ] Triple build green from a clean tree (no stale objects): `ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone` from `build/`.
- [ ] Cache-clear + install per CLAUDE.md (kill `AudioComponentRegistrar`, remove caches + old bundles, copy fresh VST3 + AU).
- **Depends on:** Phase 4.3 gate green

#### Task 13 — `pluginval --strictness-level 10` (skip-gui)
- [ ] Run: `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate-in-process --skip-gui-tests --random-seed 0xC0FFEE --timeout-ms 120000 ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3`.
- [ ] **Expect SUCCESS.** This is the first true strictness-10 run in this codebase (prior runs were silent strictness-5 fallback). Budget for 1–2 re-runs with the same seed if `FuzzParametersTest` surfaces a transient — investigate before declaring regression.
- **Depends on:** Task 12

#### Task 14 — `pluginval --strictness-level 10` (with GUI)
- [ ] Run: `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate-in-process --random-seed 0xC0FFEE --timeout-ms 120000 ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3`.
- [ ] **Expect SUCCESS.** Exercises `EditorTest`, `EditorWhilstProcessingTest`, `EditorAutomationTest` against the WebView shell at full strictness — primary risk surface per RQ4-2 risk table.
- **Depends on:** Task 13 (run skip-gui first to isolate audio-side fuzz failures from editor-side)

#### Task 15 — `auval` AU validation
- [ ] Run: `auval -v aumu OMtS OuDv`.
- [ ] **Expect:** `AU VALIDATION SUCCEEDED`.
- [ ] DEF-24-01 static-check finding (per memory: `o_lyrica_spike_reference.md`) is benign — does not block.
- **Depends on:** Task 12

#### Task 16 — Logic Pro AU smoke
- [ ] Open Logic, load O-MicrotonalSampler (AU) on a Software Instrument track.
- [ ] Load a sample folder; play a 16-voice held chord; audition tuning switch + loop-edit.
- [ ] **Expect:** no crash, no AU revalidation prompt, no GUI hang, audio renders cleanly.
- **Depends on:** Task 15

#### Task 17 — Dorico microtonal smoke (RQ4-4 procedure)
- [ ] Confirm `~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3` is the gate-time bundle (or `O-MicrotonalSampler-dev.vst3` if using dev build).
- [ ] Launch Dorico 6. `File → New from Template → Solo → Piano`.
- [ ] In Endpoint Setup, replace HSSE on the Piano track with **Ouaricon Audio Development → O-MicrotonalSampler**.
- [ ] In the plugin GUI, load a sample folder covering at least one octave around C4.
- [ ] `Library → Tonality Systems…` → add **24-EDO Equal Temperament**. Set the active flow's key signature to a 24-EDO key.
- [ ] `Library → Expression Maps…` → duplicate **Default** → rename `O-MicrotonalSampler Smoke (NE)` → set Microtonality method to **"VST3 Note Expression"**. Save.
- [ ] In Endpoint Setup, set the Piano channel's Expression Map to `O-MicrotonalSampler Smoke (NE)`.
- [ ] In Write mode, enter four quarter notes at bar 1: C4 / ¼♯C4 / C4 / ¼♭C4.
- [ ] Play the passage.
- [ ] **Acceptance:** beats 2 + 4 audibly different in pitch from beats 1 + 3 (quarter-tone offset unmistakable); no clicks / zipper / glitches at accidental boundary; no CPU dropouts.
- **Depends on:** Task 15
- **Pitfall guard:** if the Microtonality dropdown is left at `Auto`, Dorico silently routes pitch-bend → 12-TET output → false-fail. Step 6 above is mandatory.

#### Task 18 — Invariant greps
- [ ] Latency-zero: `grep -rn setLatencySamples plugins/O-MicrotonalSampler/Source/` returns exactly one comment-only hit at `PluginProcessor.cpp:133`.
- [ ] Cross-platform WebView2 flags present: `grep -n "NEEDS_WEBVIEW2\|JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING\|withUserDataFolder" plugins/O-MicrotonalSampler/CMakeLists.txt plugins/O-MicrotonalSampler/Source/PluginEditor.cpp` shows all three. (Even though Windows is out of v1.0 release scope per D4-3, the build flags must remain correct so v1.1 Windows is a no-cost flip.)
- [ ] No `v0.1.0` literal: `grep -rn "v0\.1\.0" plugins/O-MicrotonalSampler/Resources/ plugins/O-MicrotonalSampler/Source/` returns zero hits.
- [ ] No new module dependencies in `plugins/O-MicrotonalSampler/modules.json` since Stage 3 (RESEARCH §"Module reuse").
- **Depends on:** Tasks 13–17

#### Task 19 — Final VERIFICATION.md + STATUS update
- [ ] Extend `.planning/stages/4-polish/VERIFICATION.md` with the gate evidence table: pluginval-10 (skip-gui + with-gui) outcomes, auval verdict, Logic smoke result, Dorico smoke result, all four invariant greps, all 22 REQUIREMENTS rows show `complete`.
- [ ] Update `.planning/STATUS.md` to reflect Stage 4 close: status `stage_4_complete; v1.0 ready for internal use`, Stage 4 sub-stage status table populated.
- **Files:** `.planning/stages/4-polish/VERIFICATION.md`, `.planning/STATUS.md`, `.planning/REQUIREMENTS.md`
- **Depends on:** Task 18

#### Task 20 — Phase 4.4 atomic commit + STOP
- [ ] Write `PHASE-4.4-SUMMARY.md` summarising the gate run (what was tested, what passed, deferred items for v1.1).
- [ ] Atomic commit: all `.planning/stages/4-polish/` artefacts + STATUS.md + REQUIREMENTS.md updates in a single commit.
- [ ] Hand off to `/plugin-verify O-MicrotonalSampler 4-polish`.
- **Files:** `.planning/stages/4-polish/PHASE-4.4-SUMMARY.md`, `.planning/stages/4-polish/gate-report.json`
- **Depends on:** Task 19

---

## Dependency Graph

```
4.1: Task 1 ───┐
               ├─→ Task 4 (4.1 gate) ──→ Task 5 ──→ Task 6 ──→ Task 7 ──→ Task 8 (4.2 gate)
       Task 2 ─┤                                                                       │
       Task 3 ─┘                                                                       ▼
                                                                              Task 9 ──→ Task 10 ──→ Task 11 (4.3 gate)
                                                                                                            │
                                                                                                            ▼
                                                                                                       Task 12
                                                                                                            │
                                                                                                            ▼
                                                                                                  Task 13 ─→ Task 14
                                                                                                            │
                                                                                                            ▼
                                                                                                       Task 15
                                                                                                       /     \
                                                                                                  Task 16  Task 17
                                                                                                       \     /
                                                                                                       Task 18
                                                                                                            │
                                                                                                            ▼
                                                                                                       Task 19 ─→ Task 20
```

Tasks 13 and 14 are sequential, not parallel: skip-gui first isolates
audio-side fuzz failures from editor-side issues.

Tasks 16 and 17 are sequential against Task 15 but parallel against
each other (both depend on `auval` clearing the bundle).

---

## Files to Create / Modify

### New
- `.planning/stages/4-polish/PHASE-4.1-SUMMARY.md`
- `.planning/stages/4-polish/PHASE-4.2-SUMMARY.md`
- `.planning/stages/4-polish/PHASE-4.3-SUMMARY.md`
- `.planning/stages/4-polish/PHASE-4.4-SUMMARY.md`
- `.planning/stages/4-polish/VERIFICATION.md`
- `.planning/stages/4-polish/gate-report.json` (overwritten per phase)

### Modified
- `Source/PluginEditor.cpp` — add `getPluginVersion` native function (Task 1)
- `Resources/ui/index.html` — empty `#about-version` div (Task 2)
- `Resources/ui/js/sampler-app.js` — `refreshAboutVersion` + JUCE-init wire-up (Task 3)
- `.planning/REQUIREMENTS.md` — flip PERF-02 + QUAL-01 to `complete`, `verified at = stage-4` (Tasks 7, 10)
- `.planning/STATUS.md` — Stage 4 close-out (Task 19)

### Untouched (invariant)
- `Source/MicrotonalSamplerVoice.{h,cpp}`, `Source/LoopDetector.{h,cpp}`, `Source/SampleLoader.cpp` — Stage 2 audio-thread paths frozen
- `plugins/O-MicrotonalSampler/CMakeLists.txt` — `PLUGIN_VERSION` source of truth, no edit
- `plugins/O-MicrotonalSampler/modules.json` — no new module deps

---

## Constraints / Invariants (must hold at gate)

1. **`setLatencySamples` invariant** — single comment-only hit at `PluginProcessor.cpp:133` (PERF-04).
2. **No edits to Stage 2 audio-thread code paths** — voice DSP, loop-detect, loader audio path frozen. PERF-01.
3. **Cross-platform WebView memory pattern** — `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder` (Windows guard) + path-equality resource provider. v1.1 Windows must be a no-cost flip.
4. **Pluginval flag form is `--strictness-level`, not `--strictness`** — prior gate runs were silent strictness-5; correct form mandatory at 4.4.
5. **Pluginval seed pinning** — `--random-seed 0xC0FFEE` + `--timeout-ms 120000`. Stage 3 did not pin; Stage 4 must.
6. **Apple Silicon power state for PERF-02** — measurement on power, never battery. Document in VERIFICATION.
7. **Dorico expression-map override** — Microtonality method = "VST3 Note Expression". Auto-mode silently drops to pitch-bend → false 12-TET fail.
8. **No hard-coded `v0.1.0`** in `index.html` after 4.1 lands. Empty div is the defensive render if `getPluginVersion` fails.
9. **No `'v'` prefix in C++ return** — `getPluginVersion` returns the bare `JucePlugin_VersionString` (`1.0.0`); JS prepends the `v`.
10. **AU coverage at strictness-10** — pluginval doesn't validate AU bundles; AU coverage comes from `auval` (Task 15). VST3 alone gets the strictness-10 fuzz pass.

---

## Success Criteria

- [ ] Phase 4.1 gate green: triple build, `v1.0.0` visible in About pill, no `v0.1.0` literal, pluginval-5 SUCCESS, auval SUCCEEDED.
- [ ] Phase 4.2 gate green: `delta_CPU_pct ≤ 5 %` across 3 runs at 16 voices / 48 k / 256 / Apple Silicon on power; PERF-02 = `complete`.
- [ ] Phase 4.3 gate green: all 7 listening checklist items pass; QUAL-01 = `complete`.
- [ ] Phase 4.4 gate green:
  - [ ] `pluginval --strictness-level 10 --skip-gui-tests` SUCCESS (with seed pin + 120 s timeout)
  - [ ] `pluginval --strictness-level 10` (with GUI) SUCCESS (with seed pin + 120 s timeout)
  - [ ] `auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED
  - [ ] Logic Pro AU smoke: 16-voice load, no crash, no revalidation prompt
  - [ ] Dorico smoke: C4 / ¼♯C4 / C4 / ¼♭C4 produces audibly correct quarter-tone alternation
  - [ ] Latency-zero grep: single comment-only hit
  - [ ] WebView2 memory-pattern grep: all three flags present
  - [ ] No `v0.1.0` literal in `Resources/` or `Source/`
- [ ] All 22 rows in `REQUIREMENTS.md` show `complete` (or explicit OOS — none expected).
- [ ] STATUS.md reflects Stage 4 close: `stage_4_complete; v1.0 ready for internal use`.
- [ ] Stage 4 atomic commits land in order: 4.1, 4.2, 4.3, 4.4 (one each).

---

## Failure Routing

| Failure point | Action |
|---|---|
| Task 4 (Phase 4.1 gate): build fails or pill empty | Debug version-string plumbing in 4.1; do not advance. |
| Task 4: pluginval-5 regression | Investigate WebView mount / native function registration order. Likely a typo in the new `getPluginVersion` block. |
| Task 6: `delta_CPU_pct > 5 %` on any run | Reopen Stage 2 — likely 2.4 (voice-steal) or 2.5 (loop-detect hot path). Stage 4 halts. |
| Task 9: any listening checklist item fails | Reopen relevant Stage 2 sub-phase (sine zipper → 2.1; loop-boundary click → 2.5; vel-xfade pop → 2.3; voice-steal pop → 2.4). Stage 4 halts. |
| Task 13 (pluginval-10 skip-gui): `FuzzParametersTest` failure | Re-run with same seed twice; if reproducible, RT-path defect — bisect to identify which APVTS parameter mutation triggers it (likely `polyphony`). Reopen Stage 2 sub-phase. |
| Task 14 (pluginval-10 with-GUI): editor test failure | Inspect WebView relay/attachment lifetime. Likely Stage 3 latent issue surfaced by stricter cycling. Reopen Stage 3 phase 3.1. |
| Task 15 (auval): regression | Compare against Stage 3 verify auval (passed). New version-string block is the only source change — revert and re-run. |
| Task 16 (Logic smoke): crash | If crash on plugin instantiation: AU bundle metadata corruption — clean rebuild + cache-clear. If crash on play: RT regression — diff against Stage 3 verify build. |
| Task 17 (Dorico smoke): no quarter-tone offset | First check Microtonality dropdown is "VST3 Note Expression", not "Auto". If correct, inspect NE pipeline: `Source/PluginProcessor.h:22,116`, `Source/MicrotonalSamplerVoice.cpp:508`. |

A failure does **not** auto-rollback. The user diagnoses, files the
defect against the responsible Stage 2 / 3 sub-phase, and reopens that
phase. Stage 4 resumes from the failed task once the responsible
sub-phase ships green again.

---

## Next Phase

Ready for: **execute** — `/plugin-execute O-MicrotonalSampler 4-polish`.
