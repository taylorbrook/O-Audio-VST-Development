# Stage 4: Polish — Execution Plan

**Date:** 2026-06-27
**Plugin:** O-simplePhysicalModelSynth
**Stage:** 4 of 4 (Polish) — final stage before ship
**Inputs:** CONTEXT.md (5 decisions D1–D5), RESEARCH.md (7 §, all 3 open Qs resolved)

---

## Goal

Close the one remaining functional requirement (**FUNC-07** — 6 concept-isolating
factory presets) by seeding `OuariconPresetManager::initializeFactoryPresets()` with
raw-authored values converted to normalized via `convertTo0to1`, re-run the full
validation gate (with the render-harness link seam fixed), write the v1.0.0 CHANGELOG,
install, and publish cross-platform (mac VST3/AU + Windows VST3).

**No new DSP. No scope creep.** Stage 4 is presets + validation + ship only. The entire
code change is one new file pair + 3 edits; all preset infra (10 native fns, state-I/O
swap, `getPresetList`, bar UI) already shipped in Stage 3 — **zero JS / native-fn / param changes.**

---

## Phase Structure

Three phases, each gated. Phase 4.0 is the mandatory Stage-start regression baseline
(D5); 4.1 is the only real code change; 4.2 is validation + ship.

- **Phase 4.0 — Baseline gate (BEFORE any new code):** re-run render-harness @ `JUCE_WEB_BROWSER=0`. Must be ALL PASS. Proves the WebView editor never broke the seam and gives a clean pre-change reference. → **GATE 4.0**
- **Phase 4.1 — Seed factory presets:** the FactoryPresets file pair, ctor wiring, and BOTH CMake edits (main + harness) folded into one change, then one harness rebuild that catches the §3 link seam. → **GATE 4.1**
- **Phase 4.2 — Validate, document, install, publish:** full automated gate, CHANGELOG v1.0.0, `build-and-install.sh`, Standalone screenshot, then tag-driven cross-platform publish after green verify. → **GATE 4.2**

---

## Tasks

### Phase 4.0 — Baseline regression gate (D5)

1. [ ] **Re-run render-harness at Stage-4 START — pre-change baseline**
   - Build `O-simplePhysicalModelSynth-render-test` @ `JUCE_WEB_BROWSER=0` and run it.
   - Expect ALL PASS (22/22) — no DSP touched since Stage 2 verify; this confirms the
     Stage-3 WebView editor did not break the `#if JUCE_WEB_BROWSER` / dropped-`PluginEditor.cpp` seam.
   - Files: none (build + run only)
   - Depends on: none
   - **GATE 4.0:** harness builds + ALL PASS. If red here, STOP — a Stage-3 regression must be fixed before seeding presets.

### Phase 4.1 — Seed the 6 factory presets (FUNC-07)

2. [ ] **Create `Source/FactoryPresets.h`**
   - Adapt O-simpleFM `FactoryPresets.h`: `namespace FactoryPresets { std::vector<OuariconPresetManager::FactoryPresetDef> build(juce::AudioProcessorValueTreeState&); }`.
   - `#include "OuariconPresetManager.h"`.
   - Files: `Source/FactoryPresets.h` (NEW)
   - Depends on: Task 1

3. [ ] **Create `Source/FactoryPresets.cpp` — 6 presets, raw → normalized**
   - Port O-simpleFM's `normalize(apvts, raw)` helper (`p->convertTo0to1(value)`, **skips unknown IDs**) + `makePreset(apvts, name, raw)`.
   - `#include "PluginProcessor.h"` for `ParamIDs` (namespace is `ParamIDs`, top-level in this plugin — NOT `OSimpleFM::ParamIDs`).
   - **Author exactly the 6 named presets, no "Default"** (RESEARCH §2: power-on defaults already cover it; a Default preset would set `material` and stomp damping/decay).
   - Seed the §6 raw-value maps. Choice params authored as **raw indices** (`excitationType` 0/1/2, `resonatorType` 0/1; `stringModel` omitted → KS default 0).
   - **Material authoring convention (RESEARCH §2 — load-bearing):**
     - String presets (Bright Steel, Muted Nylon, Koto/Harp, Bowed String): set `material` (raw %), **OMIT** `damping`/`decay` — the macro derives both.
     - Modal presets (Struck Bar, Bell): set `damping`/`decay` (raw %), **OMIT** `material` — leaving it at default 30 fires no listener, so explicit damping/decay survive.
     - **NEVER co-author `material` AND `damping`/`decay` in one preset.**
   - Files: `Source/FactoryPresets.cpp` (NEW)
   - Depends on: Task 2
   - Seed values (RESEARCH §6, raw units — ear-tune starting points, refined by D3 after install):

     | Param (raw) | Bright Steel | Muted Nylon | Koto/Harp | Struck Bar | Bell | Bowed String |
     |---|---|---|---|---|---|---|
     | `excitationType` | 0 | 0 | 0 | 1 | 1 | 2 |
     | `resonatorType` | 0 | 0 | 0 | 1 | 1 | 0 |
     | `excitationPosition` | 18 | 30 | 22 | 30 | 25 | 30 |
     | `excitationColor` | 75 | 35 | 60 | 55 | 65 | 50 |
     | `bowForce` | — | — | — | — | — | 55 |
     | `inharmonicity` | — | — | — | 10 | 80 | — |
     | `modeBrightness` | — | — | — | 45 | 70 | — |
     | `material` | 8 | 85 | 35 | *(omit)* | *(omit)* | 15 |
     | `damping` | *(macro)* | *(macro)* | *(macro)* | 55 | 70 | *(macro)* |
     | `decay` | *(macro)* | *(macro)* | *(macro)* | 60 | 88 | *(macro)* |
     | `ampAttack` (s) | 0.001 | 0.001 | 0.001 | 0.001 | 0.001 | 0.04 |
     | `ampRelease` (s) | 0.3 | 0.15 | 0.25 | 0.3 | 0.6 | 0.25 |
     | `outputLevel` (dB) | −6 | −5 | −6 | −8 | −8 | −6 |

     `coarseTune`/`fineTune`/`stringModel`/`velToBrightness` omitted everywhere (defaults).

4. [ ] **Wire the constructor call**
   - In `PluginProcessor.cpp` ctor (the empty-stub comment at ~line 151), replace with:
     `presetManager.initializeFactoryPresets(FactoryPresets::build(parameters));`
   - Add `#include "FactoryPresets.h"` to `PluginProcessor.cpp`.
   - Files: `Source/PluginProcessor.cpp` (EDIT)
   - Depends on: Task 3

5. [ ] **Add `FactoryPresets.cpp` to the plugin target**
   - `CMakeLists.txt` `target_sources` (~line 38, alongside `PluginProcessor.cpp`): add `Source/FactoryPresets.cpp`.
   - Files: `CMakeLists.txt` (EDIT)
   - Depends on: Task 3

6. [ ] **⚠ Add `FactoryPresets.cpp` to the render-harness target (CRITICAL link seam — RESEARCH §3)**
   - `tests/render-harness/CMakeLists.txt` `target_sources` (~line 25, alongside the existing `PluginProcessor.cpp`): add `${CMAKE_CURRENT_SOURCE_DIR}/../../Source/FactoryPresets.cpp`.
   - **Why:** the harness compiles `PluginProcessor.cpp`; the moment its ctor calls `FactoryPresets::build()` the harness gets an **undefined-symbol link error** unless `FactoryPresets.cpp` is also linked. Same class of footgun as the Stage-3 WebView/editor seam.
   - Files: `tests/render-harness/CMakeLists.txt` (EDIT)
   - Depends on: Task 3
   - **GATE 4.1:** rebuild plugin (VST3+AU) AND render-harness clean; harness ALL PASS (link seam resolved + no DSP regression); `getPresetList()` returns the **6** named presets; each loads without error, round-trips state, and sets the expected excitation/resonator combo.

### Phase 4.2 — Validate, document, install, publish

7. [ ] **Full automated validation gate**
   - pluginval strictness-10 (VST3 **and** AU).
   - `auval -v aumu OsPM OuDv` SUCCEEDS (dev manufacturer code; shipped AU is `OuAu` — expected per RESEARCH §4, not a defect).
   - Native-fn parity (JS ↔ C++) 12↔12, param-ID parity (APVTS ↔ JS) 17==17 — both unchanged (no new params/fns), tooltip coverage, `node --check`.
   - Standalone renders not-blank — capture screenshot.
   - Files: none (validation)
   - Depends on: Task 6

8. [ ] **Write CHANGELOG v1.0.0**
   - Author `CHANGELOG.md` v1.0.0 entry: 6 concept-isolating factory presets (FUNC-07) completing the physical-modeling synth; summarize the shipped feature set (3 exciters, 2 resonators, Material macro, animated loop diagram + spectrum/scope viz, preset bar, keyboard). Note DSP-06 (waveguide) deferred → v1.1.
   - Files: `CHANGELOG.md` (NEW or EDIT)
   - Depends on: Task 7

9. [ ] **Install via `build-and-install.sh` (dual-variant sweep)**
   - `./scripts/build-and-install.sh O-simplePhysicalModelSynth` — clears AU cache, sweeps both `-dev`/unsuffixed variants, installs fresh VST3+AU.
   - Confirm `auval -a | grep -i OsPM` shows the AU registered.
   - Files: none (install)
   - Depends on: Task 8

10. [ ] **Cross-platform publish (D4) — AFTER green verify only**
    - Tag `O-simplePhysicalModelSynth-v1.0.0` → `build-and-release.yml` (tag-driven; no registration/matrix edit needed per RESEARCH §4).
    - CI builds mac **VST3+AU** + Windows **VST3** with `OUARICON_RELEASE=ON` (unsuffixed bundles, `OuAu`). **First real MSVC exercise** — pre-flight is ALL CLEAR (C3493 clean, single binary-data target, WebView2 static-link flag present) but watch the first Windows CI run.
    - Confirm all three artifacts build green.
    - Files: none (tag + CI)
    - Depends on: Task 9 **and a green `/plugin-verify`** (publish is post-verify; this task is listed for completeness — it executes in the verify/handoff step, not mid-execute).

---

## Files to Create / Modify

| Action | File | Task |
|--------|------|------|
| NEW | `Source/FactoryPresets.h` | 2 |
| NEW | `Source/FactoryPresets.cpp` | 3 |
| EDIT | `Source/PluginProcessor.cpp` (ctor + include) | 4 |
| EDIT | `CMakeLists.txt` (`target_sources`) | 5 |
| EDIT | `tests/render-harness/CMakeLists.txt` (`target_sources`) | 6 |
| NEW/EDIT | `CHANGELOG.md` (v1.0.0) | 8 |

---

## Dependency Graph

```
1 (baseline harness gate) ─ GATE 4.0
        │
        ▼
2 (FactoryPresets.h) ──► 3 (FactoryPresets.cpp) ──┬──► 4 (ctor wire)
                                                  ├──► 5 (main CMake)
                                                  └──► 6 (harness CMake) ─ GATE 4.1
                                                            │
                                                            ▼
                              7 (automated gate) ──► 8 (CHANGELOG) ──► 9 (install) ──► 10 (publish, post-verify)
```

---

## Risk Register

| # | Risk | Likelihood | Mitigation |
|---|------|-----------|------------|
| R1 | **Render-harness undefined-symbol** on `FactoryPresets::build` (forgot the harness CMake edit) | High if missed | Task 6 is mandatory + GATE 4.1 catches it immediately. RESEARCH §3 flags it as the #1 footgun. |
| R2 | **Material macro fights damping/decay** if both co-authored in a String preset | Medium | Task 3 authoring convention: String→`material` only; Modal→`damping`/`decay` only; never both. Verified at preset-load test (GATE 4.1) + D3 audition. |
| R3 | **Raw values >1 silently clamp** if authored as normalized by mistake (the O-Bells latent bug) | Medium | Use O-simpleFM template (raw author + `convertTo0to1`), NOT O-Bells. `normalize()` runs every value through `convertTo0to1`. |
| R4 | **Stage-3 regression** surfaces at GATE 4.0 (WebView editor broke the harness seam) | Low (Stage-3 verify saw it green) | Task 1 baseline gate before any new code; if red, fix the seam first. |
| R5 | **Windows/MSVC first build fails** in publish CI | Low | RESEARCH §4 pre-flight ALL CLEAR (C3493 clean, single binary-data ns, WebView2 static flag). Watch first CI run; publish is post-verify so a failure doesn't block the local ship. |
| R6 | **Preset ear-tune mismatch** (a preset doesn't sound like its name) | Medium | §6 values are starting points; D3 user audition after install refines. Non-blocking for the automated gate (load/round-trip/combo are what's gated). |

---

## Success Criteria

- [ ] GATE 4.0: render-harness builds @ `JUCE_WEB_BROWSER=0` + ALL PASS (pre-change baseline).
- [ ] `FactoryPresets.{h,cpp}` created; 6 named presets, no "Default"; raw-authored + `convertTo0to1`.
- [ ] Material convention honored: String presets set `material` only; Modal set `damping`/`decay` only; never both.
- [ ] Constructor calls `initializeFactoryPresets(FactoryPresets::build(parameters))`; both CMake targets list `FactoryPresets.cpp`.
- [ ] GATE 4.1: plugin (VST3+AU) + render-harness rebuild clean; harness ALL PASS (link seam resolved, no DSP regression).
- [ ] `getPresetList()` returns 6; each preset loads without error, round-trips state, sets the expected excitation/resonator combo.
- [ ] pluginval strictness-10 SUCCESS (VST3 + AU); `auval -v aumu OsPM OuDv` SUCCEEDS.
- [ ] Native-fn 12↔12, param-ID 17==17 (unchanged), `node --check` OK, Standalone renders not-blank (screenshot).
- [ ] CHANGELOG v1.0.0 written; `build-and-install.sh` installs (dual-variant sweep), AU registered.
- [ ] (post-verify) Tag `O-simplePhysicalModelSynth-v1.0.0`; CI green for mac VST3/AU + Windows VST3.

---

## Notes for Execute

- The entire functional change is **Tasks 2–6**. Tasks 1, 7–9 are gates/ship; Task 10 is post-verify.
- Fold Tasks 3–6 into a single edit set, then **one** harness rebuild (GATE 4.1) — that build is the §3 link-seam check.
- Do **not** touch any Stage-2 DSP or Stage-3 JS/native-fn/param code. If a preset needs a value the DSP doesn't expose, that's a §6 ear-tune adjustment, not a code change.
- Manual play-through (D3) is the user's job after install — execute supplies the focused DAW checklist already drafted in CONTEXT.md (audition 6 presets, Material co-move, grey-out tracking, keyboard, loop pulse/scope/spectrum lockstep, no clicks).
