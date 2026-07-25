---
plugin: O-ReverseDelay
stage: 4-polish
phase: verify
phase_status: complete
stage_status: complete
status: complete
last_updated: 2026-07-24
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
workflow_mode: manual
next_action: /publish O-ReverseDelay
ready_for_implementation: true
all_stages_verified: true
ship_ready_version: 1.0.0
human_needed: none - all 7 checklist items closed 2026-07-24 (user DAW sign-off)
v1_0_signed_off: true
contract_checksums:
  brief: sha256:7075c269df79e5dc1cf7f28d6ff4dda88805abb2c8086fc751e737f626efb07e
  parameter_spec: sha256:34f6fdf831f785784354d09ff8df864f9c4df42e0fb2175fe5645be0ade3ef39
  architecture: sha256:28f04b7ddc3e2d6d5dbb20616fde05a9f9e1665d8a4a14b8afd2a7765f0eecfa
  roadmap: sha256:854f43dbdabb6bed6a7f7042d024d72b8a61afd7e95913d65fa12ef62258f98e
---

# O-ReverseDelay Status

## Current Position

Stage: 4 (Polish) — verify ✓ complete (2026-07-24) — **ALL 4 STAGES VERIFIED · SHIP-READY v1.0.0**
Status: 18 of 19 tasks executed (Task 3 correctly skipped — D11 declined). Preset-manager v1.0.5 + 8 factory presets + 940×484 preset bar + tooltips shipped; harness 41/41, ui_frontend_check 76/76, pluginval-10 ×3 both formats, auval SUCCEEDED, CHANGELOG at v1.0.0. **Stage 4 carries ZERO DSP diff.**
Progress: [####################] 100%

## Phase Progress

### Stage 4: Polish
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-07-24 | |
| research | ✓ | 2026-07-24 | |
| plan | ✓ | 2026-07-24 | |
| execute | ✓ | 2026-07-24 | |
| verify | ✓ | 2026-07-24 | |

**Stage 4 verify results:**
- VERIFICATION.md written — verdict ✅ **VERIFIED, no blockers. Plugin ship-ready at v1.0.0.** All 14 requirements re-confirmed complete
- **Every automated gate independently re-run at verify**, not read from SUMMARY: harness **41/41 exit 0**; `ui_frontend_check` **76/76 exit 0**; pluginval-10 **VST3 SUCCESS + AU SUCCESS**; `auval` SUCCEEDED; AU version 65536 / `aufx ORvD OuDv`; build clean, zero warnings
- **Zero-DSP-diff claim tested, not assumed**: `git diff bbefa10 -- PluginProcessor.cpp` = exactly 2 hunks (constructor preset table + `get/setStateInformation`). `processBlock` and every DSP function untouched
- UI re-rendered in a real browser at the shipping viewport: 940×484 exact, overflow 0/0, band 32+12=44, panels 215px, name `Default`, **zero JS console errors** (only the stub favicon 404)
- **C5 re-proven**: all 10 tooltips a full 230px; `mix` clamped to `left:702` (= 940−230−8) with `--arrow-x:157px`; zero overflow. **Gate is viewport-sensitive** — at a 1200px viewport the clamp never fires and the test proves nothing; must resize to 940 first
- UI-02 slot **mode-invariant** at `x:117 y:242 w:86 h:100`; bar cycles all 8 names + wraps; knobs unregressed (drag 35→62%, dblclick→35%); Save/Load/**Delete** labels survive binding (C6)
- C1 skew confirmed on disk (`delayTime 0.60037` for 500 ms vs linear 0.231); probe N worst delta **0.0000**; C3/C8/C9 all hold
- Missing `~/Library/O-ReverseDelay/Presets/User/` is **not** a defect — `savePreset()` creates it lazily (`OuariconPresetManager.h:348`)
- **2 documentation issues found and fixed** (no shipped-behaviour defects): (1) frontend-check count was 77/77, actually **76/76** — the 77th grep match is the `check()` definition; corrected in CHANGELOG + SUMMARY. (2) `CMakeLists.txt:77` comment misdocumented the binary-data symbol as `preset_manager_js`; real symbol is `presetmanager_js` (hyphen **stripped**) — corrected with the memory-pattern name cited inline
- **Human checklist COMPLETE** — user tested and listened in DAW 2026-07-24 and signed off v1.0 ("complete for 1.0, improvements later"). All 7 items closed; nothing gates `/publish` on macOS. Windows deferred to CI with static prerequisites in place

**Stage 4 execute results:**
- **D11 CLOSED — makeup constant explicitly DECLINED.** User auditioned at feedback=100; wash length is right as shipped. Task 3 skipped ⇒ **Stage 4 carries ZERO DSP diff**; harness 33 entry / 41 exit (not 34/42)
- Phase 4.1: preset-manager v1.0.5 via CMake include (ONE binary-data target); 8 factory presets in engineering units + `convertTo0to1`; 11-fn bridge with **hoisted** SafePointer (MSVC-safe) + bare-return null path; probe N audits all 8 through the real `loadPreset()` — **worst round-trip delta 0.0000 on every param of every preset**
- Phase 4.2: 940×484 (band and height increase are the same 44px, so panels/footer unmoved); 5-control bar (Delete confirmed by user over D15's 4-control sketch); tooltips on all 10; stub repaired (`__JUCE__` shim + serve-stub copy line); `ui_frontend_check.js` §3/§9 **repaired** + §12–14 added → **77/77 exit 0**
- Phase 4.2 gate driven in a real browser: 940×484 exact, zero overflow, **zero JS console errors**, all 8 preset names cycle + wrap, Save→Load→Delete round-trip with label restored from `data-label`, `.time-slot` box identical across modes (y 198→242 = the predicted +44), **`mix` tooltip full 230px** clamped at left:702 with arrow tracking, knob drag + dblclick reset unregressed
- Phase 4.3: harness **41/41 exit 0**; pluginval-10 **VST3 3/3 + AU 3/3** zero failures (incl. Plugin state restoration); `auval` SUCCEEDED; AU version **65536**; CHANGELOG authored at v1.0.0
- **4 plan corrections found during execution** (full list in SUMMARY.md §Deviations): binary-data symbol is `presetmanager_js` not `preset_manager_js` (JUCE strips the hyphen); highCut tolerance set to 0.5 Hz **from the 0.0000 measurement** not the assumed 2.0; probe N's wash window moved to [2..4 s] because the plan's [6..8 s] would fail correct low-feedback presets; `ui_frontend_check` needed a THIRD repair — its binary-block regex truncates at the first `)`, so a parenthesis in a comment silently FAILs correct code
- Human checklist (7 items) batched in SUMMARY.md — item 1 (audition) already done; items 2–7 outstanding

**Stage 4 plan results:**
- PLAN.md: **19 tasks across 4 phases** — 4.0 entry gate (audition + 33/33 baseline + conditional makeup constant) / 4.1 preset system (CMake, processor+factory table, 10 native fns, harness probe N) / 4.2 bar + tooltips (geometry, markup+CSS, TDZ-safe init, tooltips, stub repair, ui_frontend_check repair) / 4.3 validation + release
- All 7 RESEARCH findings **re-verified against the live tree** this session: preset-manager.js:89-98 = 10 fns · module.yaml v1.0.5 (its own native-functions list is stale at 9) · editor at 1 fn (PluginEditor.cpp:114 ≡ app.js:266) · harness inherits includes (render-harness/CMakeLists.txt:37-38) · ui_frontend_check.js:110-131 asserts surface==1 on appJs only · :209/:226 regexes miss module paths + dynamic import · serve-stub.sh:16 copies public/ only · juce-stub.js:106-110 rejects all but getParameterDefaults
- Exact edit sites confirmed: PluginEditor.cpp:168 · styles.css:54+88 · index.html:18/21 · PluginProcessor.cpp:361 · CMakeLists.txt:25-28, 59-68
- **RESEARCH.md §8 declared authoritative over CONTEXT.md** wherever they conflict (11≡11 not 10≡10; 41/41 exit not 33/33; ~/Library/O-ReverseDelay/Presets/ not Application Support)
- 4 open items closed: makeup constant = conditional task, k=2.0f pencilled · highCut tolerance set from measured delta, not assumed · Near-Infinite renders 30 s · **preset bar ships 5 controls incl. Delete** (D15's 4-control sketch is schematic; human-checklist item 6 requires the delete round-trip) — flagged for user
- Risk register: 9 failure modes that pass build/auval/pluginval silently, each mapped to its owning task

**Stage 4 research findings (RESEARCH.md — 6 corrections to CONTEXT):**
- **F1:** `preset-manager.js` requires **10** native fns, not 9 — D12 omitted `savePresetWithDialog` (the fn the Save button actually calls). Total bridge surface is **11**, not 10 → C4 gate is `11 ≡ 11`
- **F2:** All 10 preset `getNativeFunction` calls live in `preset-manager.js`, not `app.js`. `ui_frontend_check.js` §3 scans `appJs` only and hard-asserts surface==1 → must scan both files
- **F3:** `ui_frontend_check.js` §9 three-way closure FAILs on correct code — its regex is `Source/ui/public/…` and the new binary-data entry is a `modules/…` path; also dynamic `import()` isn't harvested into `refs`
- **F4:** Browser stub is unusable for the bar as-is: no `window.__JUCE__` → `_waitForNative()` polls 5 s then `console.error`s (fails the zero-console-errors gate); and `serve-stub.sh` never copies `preset-manager.js` (it's not under `Source/ui/public`) → 404
- **F5:** Preset path is `~/Library/O-ReverseDelay/Presets/{Factory,User}/`, NOT `~/Library/Application Support/…` as D12 states
- **F6:** O-Contrabass's dialog lambdas use the **MSVC-breaking** nested `SafePointer(this)` init-capture — must hoist to a local before `launchAsync` or Windows CI won't compile (`critical_msvc_safepointer_init_capture_nested_lambda`)
- **F7:** `.factory-version` sentinel is keyed on `JucePlugin_VersionString`; at a frozen 1.0.0 **factory-table edits never re-seed** — `rm -rf ~/Library/O-ReverseDelay/Presets/Factory` after every edit
- **F8:** C2 needs **zero** plugin-side code — v1.0.5 `applyPresetJson` already resets all params to defaults (meta-first) before applying
- **F9:** Harness inherits the preset-manager include free via `$<TARGET_PROPERTY:…,INCLUDE_DIRECTORIES>` → preset audit runs through the **real** `loadPreset()` (true skew round-trip), not a re-typed table. Answers open question #4. Harness goes **33 → 41** checks
- Resolved: 8-preset table authored in engineering units (7 Free + 1 Sync 1/8D); 10 tooltip copy lines; Naturalist bar styling = borrow O-Contrabass *structure*, dress in local `.segment`/`.division-select` vocabulary (O-Contrabass's dark chrome bar would double the title); makeup-constant derivation (recommend `k=2.0f`/+6 dB if the audition asks, pre-`tanh` at the tap)
- Suggested split: 4.0 entry gate (audition + 33/33) → 4.1 preset system → 4.2 bar + tooltips → 4.3 validation + CHANGELOG

**Stage 4 discuss decisions:**
- D11: Wash decay — **audition first, then decide**. The makeup constant at the feedback tap is NOT pre-authorized; user auditions Standalone and reports. If implemented, gated on harness + probe G (60 s @ fb=100)
- D12: Presets = **full OuariconPresetManager v1.0.5** (module.yaml authoritative; registry.yaml stale) + preset bar; CMake-include pattern per O-Bowed/O-Wind; 9 native fns added (1 → 10)
- D13: Extra scope = **tooltips on all 10 controls only**. No CPU pass; Windows deferred to publish/CI
- D14: Release = **ship-ready v1.0.0, publish separately** (`/publish` is a later user-triggered step; Windows pluginval fuzz not a Stage-4 blocker)
- D15: Preset bar = **grow to 940×484**, new 44px band under the header; panels + footer untouched. `PluginEditor.cpp:168` setSize change; `.time-slot` y shifts +44 (re-verify asserts *identical box across modes*, not the old absolute y)
- D16: **8 factory presets** — Reverse Bloom, Guitar Swell, Vocal Halo, Slow Wash, Tight Smear, Dark Cavern, Near-Infinite (doubles as DSP-03 stability check), Rhythmic Reverse (Sync 1/8D — name avoids "/", `critical_preset_name_slash_path_separator`)
- Open for research: makeup-constant value (if needed), Naturalist bar styling, tooltip copy, preset-audit harness placement

### Stage 3: GUI
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-07-24 | |
| research | ✓ | 2026-07-24 | |
| plan | ✓ | 2026-07-24 | |
| execute | ✓ | 2026-07-24 | |
| verify | ✓ | 2026-07-24 | |

**Stage 3 verify results:**
- VERIFICATION.md written — verdict ✅ VERIFIED, no blockers; UI-01 + UI-02 marked complete in REQUIREMENTS.md (all 14 requirements now complete)
- Harness independently re-run at verify: 33/33 PASS, exit 0 — WebView editor caused zero DSP regression; harness links with JUCE_WEB_BROWSER=0, proving the createEditor guard
- ui_frontend_check.js re-run: 45/45; native-fn grep-diff 1 ≡ 1 (app.js:266 ≡ PluginEditor.cpp:114)
- Stub fixtures compared line-by-line against createParameterLayout() — all 8 ranges/skew-centres/defaults and both combo defaults match, so the stub gate is representative
- Browser render: exactly 940×440, zero overflow, zero JS console errors (only a stub-server favicon 404, absent in the WebView path)
- Skew proof at normalised 0.5: delayTime 316 ms, grainSize 158 ms, lowCut 200 Hz, highCut 3.2 kHz; linear params 50 % — exact setSkewForCentre values
- UI-02 proof: .time-slot box identical in both modes (x:117 y:198 w:86 h:100), correct control visible each way, FREE/SYNC labels + aria-pressed intact
- Dblclick-reset returns all 8 knobs to exact engineering defaults from off-default positions
- Real WKWebView confirmed in Standalone (942×498 window) — renders identically to the stub and restored a NON-default saved state (FREE / 317 ms), proving the C++→JS direction in the real bridge
- auval SUCCEEDED; pluginval@10 SUCCESS on VST3 and AU with Editor / Automation / Editor Automation tests included
- Known non-blocker: synthetic click into the live Standalone was refused by macOS accessibility (error -25208), so the interactive JS→C++ leg was not re-driven at verify; covered by execute-session evidence + pluginval Editor Automation

**Stage 3 plan results:**
- PLAN.md: 10 tasks — 3.1 (frontend authoring, browser-stub render gate, CMake wiring, editor with 10 relays, createEditor guard, build+harness gate); 3.2 (interaction/UI-02 swap/dblclick-reset, stub re-check + native-fn grep-diff, ui_frontend_check.js port, full exit gate)
- 12 success criteria incl. harness re-run, ui_frontend_check exit 0, pluginval-10 both formats (single run; ×3 stays Stage 4), Standalone open
- Corrections vs earlier docs: 4 skewed params (delayTime/grainSize/lowCut/highCut) per APVTS, not 6; delayTime skew-centre ≈316 ms not 315
- Open in-task items: AU subtype `aufx ORvD OuDv` to confirm before auval; botanical PNG pick (birds vs flora) resolved in Task 1 (CMake list + resource-provider list together)

**Stage 3 research findings:**
- Mockup path: **direct-author production index.html** (skip ui-mockup workflow — no recent sibling used it; D8/D9/D10 fully constrain design); browser render against ~20-line JUCE-bridge stub replaces the mockup gate and doubles as TDZ/label check
- Window: **fixed 940×440**, single row of 4 framed group panels TIME | GRAIN | FEEDBACK | OUTPUT
- noteDivision: Naturalist `<select>` via **WebComboBoxRelay**, options from `st.properties.choices`; **syncMode is also Choice** (WebComboBoxRelay, FREE|SYNC segment pair); UI-02 swap = pure-JS hidden-class toggle on shared fixed slot, both controls always bound
- Actual param contract: 8 sliders + 2 combos, 0 toggles; only 4 skewed params (delayTime, grainSize, lowCut, highCut) — not 6 as CONTEXT assumed
- One native fn only: getParameterDefaults (dblclick-reset)
- CMake: NAMESPACE **UIBinaryData**; NEEDS_WEB_BROWSER/NEEDS_WEBVIEW2 + JUCE_WEB_BROWSER=1 + WEBVIEW2 static-linking flag; harness already excludes editor sources — only edit is `#if JUCE_WEB_BROWSER` guard in createEditor()
- Reference implementation: **O-simpleGrain editor** minus drag-drop/keyboard/viz/Timer
- Phase 3.2 gate: port O-Contrabass `tests/ui_frontend_check.js`

**Stage 3 discuss decisions:**
- D7: D6 Standalone audition **deferred** — GUI touches no DSP; audition (incl. −7.3 dB/gen wash-length call + possible feedback-tap makeup constant) is now a REQUIRED Stage-4 entry check
- D8: Aesthetic = **Ouaricon Naturalist** (ouaricon-naturalist-001)
- D9: Layout = **grouped sections** as signal flow: TIME (syncMode, noteDivision/delayTime swap per UI-02) / GRAIN (grainSize, density) / FEEDBACK (feedback, lowCut, highCut) / OUTPUT (width, mix)
- D10: Visualization = **none** — knobs + readouts only (no C++→JS polling bridge)
- Open for research: mockup production path (ui-mockup workflow vs direct authoring), window size, noteDivision control style (13 divisions)

### Stage 2: DSP
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-07-23 | |
| research | ✓ | 2026-07-23 | |
| plan | ✓ | 2026-07-23 | |
| execute | ✓ | 2026-07-24 | |
| verify | ✓ | 2026-07-24 | |

**Stage 2 verify results:**
- VERIFICATION.md written — verdict ✅ VERIFIED, no blockers; 11/11 stage-2 requirements complete in REQUIREMENTS.md
- Harness independently rebuilt + re-run at verify: 33/33 PASS, exit 0
- pluginval-10 confirmation runs at verify: VST3 SUCCESS, AU SUCCESS (4 total each across execute+verify)
- PERF-01/DSP-02 code review clean: zero alloc/locks/logging in processBlock; ArrayCoefficients in-place; recompute-only cutoff guards; skewed ranges untouched (DSP-04)
- Mono→stereo D4 open item closed (probe L Δ0.0000 dB)
- **D6 Standalone audition outstanding** — required before Stage 3; evaluate wash length vs the −7.3 dB/gen finding (fix if wanted: single makeup constant at feedback tap, then re-run probe G)

**Stage 2 execute results:**
- Phase 2.1 (81603a8): capture ring (D+2n law), Hann LUT, grain pool/scheduler, processBlock REQUIRED order, probes 0+A–E green (density flatness 0.061 dB)
- Phase 2.2 (0ae40e5): feedback wet→gain→HP→LP→tanh→NaN-guard; ArrayCoefficients in-place; probes F–H green (centroid 10008→4136 Hz/gen; 60 s @ fb=100 peak 0.24)
- Phase 2.3: tempo sync (13-division table, COMPAT-02 fallback), width spread (xorshift32 + alternating pan, kPanBias=0.5); probes I–M green (sync latency exact; width-100 corr 0.34; mono→stereo Δ0.0000 dB)
- Harness: 33 hard-exit checks total, ALL PASS; build warning-clean
- pluginval strictness-10 ×3: VST3 3/3, AU 3/3, zero failures; auval lists AU
- Design finding for D6 audition: contract topology has inherent ~−7.3 dB/generation loss at fb=100 pre-damping (Hann² duty + pan→monoSum round trip) — wash decays rather than self-sustains; single makeup constant at the feedback tap is the fix IF audition wants a longer wash
- Gate-infra fix (91c673f): run-gate.sh now resolves the juce_add_plugin target (was hard-coded to folder name)

**Stage 2 discuss decisions:**
- D4: Grain stereo source under width = **mono-sum** (grain reads 0.5(L+R); equal-power pan places the mono grain) — resolves the rule ARCHITECTURE.md deferred to Stage 2
- D5: Render harness = **hard pass/fail exit codes**; every acceptance criterion an automated assertion; phase advancement requires green
- D6: Listening checkpoint = **one Standalone audition after Phase 2.3**, before verify; 2.1/2.2 advance on harness alone
- Open tuning items (harness-resolved, not decisions): overlap-compensation constant, pan bias amount, mono→stereo 0.5(L+R) identity check

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-07-23 | |
| research | ✓ | 2026-07-23 | |
| plan | ✓ | 2026-07-23 | |
| execute | ✓ | 2026-07-23 | |
| verify | ✓ | 2026-07-23 | |

**Stage 1 verify results:**
- VERIFICATION.md written — verdict ✅ VERIFIED, no blockers
- COMPAT-01 marked complete in REQUIREMENTS.md (auval + pluginval-10 both formats)
- Verify-phase pluginval-10 confirmation runs: VST3 SUCCESS, AU SUCCESS (4 total each across execute+verify)
- AU component version confirmed 65536 = 1.0.0
- Human checks outstanding (non-blocking): DAW mono→stereo listen, GenericEditor param review, session save/reload

**Stage 1 execute results:**
- CMakeLists.txt + PluginProcessor.h/.cpp created (no PluginEditor files — GenericAudioProcessorEditor)
- Clean build all 3 formats; installed via build-and-install.sh
- auval lists AU; component version 65536 (1.0.0)
- pluginval strictness 10: VST3 3/3 + AU 3/3, zero failures (COMPAT-01)
- 0→1 gate bypassed with --force (build check demands a target Stage 1 itself creates; logged in gate-bypasses.log)

**Stage 1 discuss decisions:**
- D1: Bus layouts mono→mono, mono→stereo, stereo→stereo (user choice; deviates from suite stereo-only)
- D2: pluginval strictness 10 from Stage 1 (COMPAT-01 gate)
- D3: Render harness deferred to Phase 2.1 per roadmap
- PLUGIN_CODE `ORvD`; target `OuariconReverseDelay`; VERSION 1.0.0

## Completed So Far

**Ideation:** ✓ Complete
- Creative brief, 14 requirements, 10 draft parameters

**Stage 0:** ✓ Complete — Research & Planning
- Plugin type: stereo audio effect (granular reverse delay, time-domain, stateful)
- Complexity tier 3, research depth MODERATE
- 7 features researched: capture buffer, reverse grain engine, tempo sync, damping filters, feedback stability, width, mix
- Professional references: Strymon TimeLine (REV/grain), ValhallaDelay Reverse, Red Panda Particle 2, Boss DD-7 (contrast), yaleD
- JUCE APIs verified against local 8.0.14 source: `dsp::IIR::ArrayCoefficients`, `AudioPlayHead::PositionInfo::getBpm()`, `dsp::DryWetMixer` (rejected), `SmoothedValue`
- In-suite prior art mapped: O-GrainScatter (DelayBuffer, GrainScheduler), O-simpleGrain (WindowLuts, harness)
- Every HIGH/MEDIUM-risk feature has a documented fallback (dual crossfaded reverse heads; RMS energy limiter)
- Complexity score: 5.0 (capped) — **Staged implementation** (DSP phases 2.1–2.3, GUI phases 3.1–3.2)
- ARCHITECTURE.md and ROADMAP.md documented

## Next Steps

**All four stages verified · human checklist complete · v1.0.0 SIGNED OFF (2026-07-24).**

1. `/publish O-ReverseDelay` — cross-platform GitHub Actions release. The only
   remaining unknown is **Windows**, deferred to CI by D13/D14. Static
   prerequisites are verified (both WebView2 flags set; both `FileChooser`
   completions hoist `SafePointer` to a local, which is what MSVC needs) but the
   first Windows CI build is the real test — expect to fix forward there, not here.
2. Improvements are deferred to a later version per user sign-off. Use
   `/improve O-ReverseDelay` when picking that up — and run the **4-way version
   check first** (`pattern_uncommitted_improve_versions_lost`).
3. Note for any future improve/verify session: the **C5 tooltip gate is
   viewport-sensitive** — resize the browser to 940×484 before testing, or the
   clamp never fires and the test silently proves nothing
   (`pattern_tooltip_clamp_gate_viewport_sensitive`).

## Context to Preserve

**Key Decisions:**
- Granular reverse smear engine (reverse read offset D+2n over 3.5 s capture ring); per-grain parameter latching for click-free changes
- Feedback through shared capture buffer (alternating-direction regenerations = intended character); tanh loop stability at unity gain
- Damping: 2nd-order Butterworth IIR + ArrayCoefficients in-place (DSP-02); cutoff clamp 0.49·fs
- Custom equal-power mix (DryWetMixer rejected — zero latency); density compensation before feedback tap
- Render harness FIRST in Phase 2.1 — all Stage-2 acceptance criteria are offline-render assertions

## Files Created (Stage 0)
- plugins/O-ReverseDelay/.planning/research/ARCHITECTURE.md
- plugins/O-ReverseDelay/.planning/ROADMAP.md
- plugins/O-ReverseDelay/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-ReverseDelay/NOTES.md
