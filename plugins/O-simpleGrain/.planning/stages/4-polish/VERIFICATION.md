---
phase: 4-polish
verified: 2026-06-25T00:00:00Z
status: human_needed
score: 8/8 automated gates PASS (live re-run) · 16/26 requirements code+automated-verified · 10 runtime/audible criteria DEFERRED to human DAW listen
re_verification: No — initial verification
human_verification:
  - test: "Play a held chord and watch the Grain Cloud canvas while raising Density and Position/Pitch Spray"
    expected: "Sepia dots accumulate (read-position × time); Density thickens the cloud, Spray widens it (UI-01)"
    why_human: "Accumulation needs live MIDI driving the audio-thread grain-event tap; render path verified in code, motion not observable headlessly"
  - test: "Hold a note, set Scatter 0% then 100%, watch the Spectrum (and the Scope)"
    expected: "Discrete sidebands / pitched comb at 0% smearing toward broadband noise at 100% (UI-04, DSP-05 visual); scope moves with output (UI-04)"
    why_human: "Spectrum/scope motion is driven by live output audio through the VizRing → message-thread FFT"
  - test: "Watch the Grain / Overlap / CPU readout while raising Density and Grain Size on a held note"
    expected: "Grains N/192 climbs live, Overlap grows, CPU bar fills (UI-05)"
    why_human: "activeGrainCount is only non-zero with a running voice; the JS render is verified, live values are not"
  - test: "Sweep Position/Scan and toggle Freeze while playing; listen on the transition"
    expected: "Source-waveform playhead tracks Position+Scan; ❄ freeze pin appears and pins the head with a shaded spray band; freeze/unfreeze is click-free (FUNC-03 transition, QUAL-01, UI-02)"
    why_human: "Live playhead motion + the audible click-free crossfade need a running audio thread"
  - test: "Change the Window combo and watch the inset; run all 8 concept presets and listen; hover every control"
    expected: "Inset redraws to the selected window (UI-03); each preset snaps knobs/combos/toggle + caption/active-state AND sounds audibly distinct — Single Grain separated, Pitched Buzz a comb, Rect Click audible clicks, etc. (FUNC-06 audible half); every control surfaces its tooltip (FUNC-07)"
    why_human: "Preset values + apply path + 33≡33 tooltip parity are verified statically; audible character + hover surfacing require a DAW listen"
  - test: "Drag a .wav onto the drop zone AND use the Load… picker in the live build"
    expected: "Both load and granulate a user source; oversized files show the 10 s truncation notice (FUNC-05 runtime)"
    why_human: "macOS WebView content-streaming drag-drop cannot be exercised headlessly; full C++/JS path is verified statically"
  - test: "Open the editor on macOS VST3 + AU, confirm the field-guide page renders (not blank), automate a param from the host"
    expected: "Single-page projector-readable field guide; host-automation moves the on-screen control (UI-06 visual, host→UI round-trip)"
    why_human: "WebView render + host-automation round-trip is a visual/interactive check; code wiring + auval/pluginval are confirmed"
---

# Stage 4 (Validation / Polish): O-simpleGrain Verification Report

**Stage Goal:** Close out O-simpleGrain as a stable, correct, distribution-ready **1.0.0** release.
Stage 4 is a **validation gate, not a feature stage** — prove the shipped granular instrument is
stable/correct/distribution-ready, with the only product-file edits being the version bump and a
new CHANGELOG (no other code unless a validation run surfaces a defect). End by handing the user a
single consolidated DAW-listen checklist (the 7 deferred Stage-3 runtime criteria) and recording
verify as `human_needed`.

**Verified:** 2026-06-25
**Status:** human_needed — all automated gates PASS (re-run live during this verification); the 7-item
DAW-listen checklist is the only remaining gate.
**Re-verification:** No — initial verification.

---

## Goal Achievement Verdict

**The Stage-4 validation goal is achieved at the automated level — every gate was re-run LIVE during
this verification (not trusted from the SUMMARY) and every one passes.** auval reports
`AU VALIDATION SUCCEEDED`, pluginval Tier A (`--skip-gui-tests --strictness-level 10`) exits 0 with
`SUCCESS` and zero FAIL markers across 21 test sections on the *installed* VST3, the offline DSP
harness re-runs **8/8 PASS exit 0**, and all 8 factory presets desk-check in-range / finite /
denormal-free against `parameter-spec.md`. The version is **1.0.0** in both CMake files, CHANGELOG.md
exists with the sibling structure and a Validation section that quotes the real run results, the
installed bundles are the freshly-built `-dev` variants with **no alternate-variant orphan**, and
**D2 held** — the only product-file diff is `CMakeLists.txt` (version) + new `CHANGELOG.md`; the one
defect found (render-harness link) was fixed in *test* CMake only, no `Source/*` touched.

**The remaining gate is human.** Ten requirements have a runtime / audible / visual acceptance half
that cannot be driven headlessly (live viz motion, audible preset character, click-free freeze, WebView
render + host-automation round-trip, live drag-drop). Per locked decision **D1** they are batched into
one DAW-listen checklist (above) and verify is recorded **`human_needed`** until the user confirms —
these are deferrals by design, **not failures**.

---

## Goal-Backward Analysis

### Original Goals (CONTEXT.md success criteria)

1. pluginval passes (VST3) at standard strictness; auval SUCCEEDED (AU).
2. Offline DSP harness re-run: 8/8 PASS (no regression since Stage 2).
3. 8 preset snapshots write valid APVTS (no NaN/denormal/out-of-range).
4. Fresh build installed (`O-simpleGrain-dev`, `aumu OsGr OuDv`), AU cache cleared, both variants swept.
5. CHANGELOG authored.
6. Consolidated DAW-listen checklist (7 criteria) handed over; verify = `human_needed` until confirmed.
7. Windows explicitly marked deferred-to-CI (not a blocker).

### Goal Achievement

| Goal | Status | Evidence (verified live this pass) |
|------|--------|-----------------------------------|
| 1. pluginval VST3 + auval AU | ✅ Achieved | `auval -v aumu OsGr OuDv` → **AU VALIDATION SUCCEEDED**. pluginval `--skip-gui-tests --strictness-level 10` → exit 0, `SUCCESS`, 0 FAIL markers, 21 sections, on `~/Library/.../O-simpleGrain-dev.vst3`. |
| 2. Offline harness 8/8 | ✅ Achieved | Re-ran the built `O-simpleGrain-render-test` → `ALL PASS — 0 failure(s)`, exit 0. No engine regression from the Stage-3 editor rewrite. |
| 3. 8 presets valid APVTS | ✅ Achieved | Desk-checked all 8 `applyFactoryPreset` branches (`PluginProcessor.cpp:813-902`) vs `parameter-spec.md`: every literal inside its declared range; reset-to-default-first + `convertTo0to1`+`setValueNotifyingHost` → bounded [0,1], finite, denormal-free by construction. |
| 4. Fresh install, cache cleared, swept | ✅ Achieved | `O-simpleGrain-dev.{vst3,component}` present (built 11:30); **no unsuffixed orphan** of either type. |
| 5. CHANGELOG authored | ✅ Achieved | `CHANGELOG.md` (4974 B) — sibling structure, single `[1.0.0] — 2026-06-25`, Validation section quotes real results. |
| 6. DAW-listen checklist + `human_needed` | ✅ Achieved | 7-item checklist in this report's frontmatter + handoff; status recorded `human_needed`. |
| 7. Windows deferred-to-CI | ✅ Achieved | Recorded as deferred; cross-platform CMake flags static-verified Stage 1/3; not a Stage-4 blocker. |

---

## Requirements Verification

**Stage:** 4-polish (final). Stage-4 scope = COMPAT-* + roll-up of all remaining. **Total: 26**
(table count; note the REQUIREMENTS.md header line "Total 24 / must 16" undercounts — actual is
must 18 / should 6 / nice 2 = 26).

**Split:** ✅ **16 code + automated-verified** · ⏸️ **10 runtime/audible/visual → human DAW listen**.

| Requirement | Priority | Status | Evidence / Why deferred |
|-------------|----------|--------|-------------------------|
| FUNC-01 overlap-add continuous | must | ✅ complete | harness density→continuity (contLow 0.171 → contHigh 0.900); alloc/lock-free (PERF-01) |
| FUNC-02 MIDI poly transpose | must | ✅ complete | harness pitch-tracks-MIDI C2/C3/C4 exact; 8-voice engine |
| FUNC-03 Freeze pins + sustains | must | ✅ complete | harness freeze-sustains (rms 0.0149). *Click-free transition* → listen (QUAL-01) |
| FUNC-04 built-in sources incl. fire | must | ✅ complete | 4 embedded built-ins via `juce_add_binary_data`; `sourceSample` choice; Granular Fire preset |
| FUNC-05 load-your-own (drag-drop/picker) | should | ⏸️ human | full C++/JS path wired + static-verified; runtime load = listen #6 |
| FUNC-06 concept preset tour | should | ⏸️ human | 8 presets desk-checked in-range + name parity; *audible* isolation = listen #5 |
| FUNC-07 per-control tooltips | should | ⏸️ human | 33 data-tip ≡ 33 TIPS (exact); *hover surfacing* = listen #5 |
| DSP-01 buzz↔fragments continuum | must | ✅ complete | harness pitch probe; Pitched Buzz / Fragments presets; grain-size control |
| DSP-02 density → continuous cloud | must | ✅ complete | harness density→continuity |
| DSP-03 five windows; rect clicks | must | ✅ complete | harness window-rect-clicks hf ratio **905×**. *Inset redraw* = listen #5 |
| DSP-04 pitch/position spray | must | ✅ complete | harness scatter-async; per-grain spray params + RNG (no alloc) |
| DSP-05 scatter sync→async | must | ✅ complete | harness scatter-async flatSync 0.0029 vs flatAsync 0.372. *Spectrum visual* = listen #2 |
| DSP-06 scan / time-stretch / reverse / hold | should | ✅ complete | read-head scan (Granular Fire scan=40); Freeze = hold |
| DSP-07 per-voice amp ADSR | must | ✅ complete | ADSR params; harness makes-sound w/ envelope shaping |
| DSP-08 band-limited up-transpose | should | ✅ complete | harness uptranspose-stable (peak 1.82 rms 0.215); rate-tracking one-pole AA |
| UI-01 grain cloud scatter | must | ⏸️ human | renderer wired; accumulation = listen #1 |
| UI-02 source waveform + playheads/freeze/spray | must | ⏸️ human | renderer + getSourceThumbnail wired; motion = listen #4 |
| UI-03 grain-envelope inset | must | ⏸️ human | JS recompute + redraw-on-change wired; visual = listen #5 |
| UI-04 scope / spectrum sync vs async | should | ⏸️ human | scope/spectrum push + FFT wired; motion = listen #2 |
| UI-05 grain-count / CPU readout | nice | ⏸️ human | grainMeterUpdate wired; live values = listen #3 |
| UI-06 single-page projector-readable | must | ⏸️ human | structure verified; auval render OK; *visual* readability = listen #7 |
| PERF-01 RT-safe (no alloc/locks) | must | ✅ complete | pluginval strictness-10 SUCCESS (no RT violations); Stage-2 inspection; no audio-thread FFT/alloc |
| PERF-02 bounded grain count (no xrun) | nice | ✅ complete | harness stress-bounded peakGrains 148 ≤ cap 192 |
| COMPAT-01 pluginval VST3 + AU | must | ✅ complete | pluginval VST3 SUCCESS @ strictness 10 + auval AU SUCCEEDED |
| COMPAT-02 Windows WebView2 flags | must | ✅ complete | `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder` static-verified; Windows build deferred-to-CI |
| QUAL-01 no unintended artifacts | must | ⏸️ human | engine smoothing verified Stage 2; zipper-free param moves + click-free freeze/unfreeze = listen #4 (intentional rect click is a feature) |

**Requirements Summary:** ✅ Complete: **16** · ⏸️ Human-listen deferred: **10** · ⚠️ Partial: 0 · ❌ Failed: 0.

---

## Automated Checks (all re-run LIVE this verification)

| Check | Result | Notes |
|-------|--------|-------|
| Fresh build installed + dual-variant sweep | ✅ Pass | `O-simpleGrain-dev.{vst3,component}` present; **no unsuffixed orphan** (both types checked) |
| auval AU regression | ✅ Pass | `auval -v aumu OsGr OuDv` → `AU VALIDATION SUCCEEDED` |
| pluginval Tier A (VST3, strictness 10, skip-gui) | ✅ Pass | exit 0, `SUCCESS`, 0 FAIL markers, 21 sections, installed bundle |
| Offline DSP harness (8 gates) | ✅ Pass | `ALL PASS — 0 failure(s)`, exit 0 — makes-sound, density→continuity, pitch-tracks-MIDI, window-rect-clicks (905×), freeze-sustains, scatter-async, stress-bounded (148≤192), uptranspose-stable |
| 8 factory presets state desk-check | ✅ Pass | all literals in declared range; bounded/finite/denormal-free by construction |
| Version bump 0.1.0 → 1.0.0 (both CMake files) | ✅ Pass | `CMakeLists.txt:17` `VERSION "1.0.0"`; harness `JucePlugin_VersionString="1.0.0"` / `VersionCode=0x010000` |
| CHANGELOG.md | ✅ Pass | sibling structure; single `[1.0.0]` entry; Validation section quotes real results |
| D2 minimal-diff discipline | ✅ Pass | product diff = `CMakeLists.txt` + new `CHANGELOG.md` only; harness-link fix is test-CMake only; no `Source/*` change |

---

## Human Verification (the only remaining gate — see frontmatter for full checklist)

Load `O-simpleGrain-dev` (`aumu OsGr OuDv`) in a DAW / Standalone with a MIDI keyboard and confirm:

- [ ] 1 — Grain cloud accumulates; Density thickens, Spray widens (UI-01)
- [ ] 2 — Spectrum: discrete sidebands at Scatter 0% → noise at 100%; Scope moves with output (UI-04, DSP-05)
- [ ] 3 — Grain / Overlap / CPU readout counts `N/192` live (UI-05)
- [ ] 4 — Freeze pins the playhead (❄ + spray band); freeze/unfreeze click-free (FUNC-03, QUAL-01, UI-02)
- [ ] 5 — Window inset redraws on combo change (UI-03); 8 presets snap + sound distinct (FUNC-06); every control shows its tooltip (FUNC-07)
- [ ] 6 — Drag-drop a .wav AND Load… both granulate a user source (FUNC-05)
- [ ] 7 — Editor renders (not blank) on VST3 + AU; host-automation → UI round-trip (UI-06)

---

## Issues Found

- **Render-harness link regression (test-only, FIXED in execute):** Stage-3 `PluginEditor.cpp`
  `getResource()` references `UIBinaryData::*`, but the render-harness `CMakeLists.txt` (Stage 2) only
  linked `O-simpleGrain_Samples` → harness link failed. Fixed by adding `O-simpleGrain_UIResources` to
  the harness `add_dependencies` + `target_link_libraries`. Re-gated → 8/8 PASS. **No product code
  touched** (the shipping VST3/AU already link `UIBinaryData` — proven by auval + pluginval). D2 held.
- No other issues. No defects in shipped DSP / parameters / UI.

---

## Stage Verdict

**Status:** ⚠️ PARTIAL — `human_needed` (all automated gates PASS; one human DAW-listen gate remains)

**Ready for release:** Pending the 7-item DAW listen. The build is installed (`O-simpleGrain-dev`),
1.0.0-stamped, and fully automated-validated — the user can run the listen immediately.

**Blockers:** None automated. The single remaining gate is the human DAW listen (10 runtime/audible/
visual requirement-halves). Windows VST3 is **deferred-to-CI** by decision D3 — not a blocker.

---

_Verified: 2026-06-25 — all gates re-run live against the installed 1.0.0 build_
_Verifier: Claude (plugin-verify / goal-backward)_
