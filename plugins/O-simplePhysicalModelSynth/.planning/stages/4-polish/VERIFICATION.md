# Stage 4: Polish — Verification

## Verification Date

2026-06-27

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

1. Close the one remaining functional requirement — **FUNC-07**: seed the 6
   concept-isolating factory presets (Bright Steel, Muted Nylon, Koto Harp,
   Struck Bar, Bell, Bowed String) covering all 3 exciters + both resonators.
2. Honor the **Material authoring convention** (load-bearing): String presets set
   `material` only; Modal presets set `damping`/`decay` only; never both.
3. Re-run the full automated validation gate with the **render-harness link seam**
   fixed (`FactoryPresets.cpp` linked into the harness so the ctor's
   `FactoryPresets::build()` call resolves).
4. Prove **no Stage-2/3 regression** — DSP untouched, WebView editor never broke the
   `JUCE_WEB_BROWSER=0` seam.
5. Write the **v1.0.0 CHANGELOG**, install via `build-and-install.sh`.
6. (post-verify) Cross-platform publish — tag-driven CI for mac VST3/AU + Windows VST3.

### Deliverables (from SUMMARY.md + code inspection)

1. **`Source/FactoryPresets.{h,cpp}`** — `namespace FactoryPresets { build(apvts) }`;
   6 presets authored RAW → normalized via `convertTo0to1`; no "Default" preset.
2. **Material convention honored** — verified in source AND in the seeded on-disk JSON
   (String presets carry `material`, no `damping`/`decay`; Modal carry `damping`/`decay`,
   no `material`).
3. **`tests/render-harness/CMakeLists.txt`** edited to link `FactoryPresets.cpp` (the
   §3 link seam) — harness builds + links clean.
4. **Ctor wired** — `presetManager.initializeFactoryPresets(FactoryPresets::build(parameters))`;
   `CMakeLists.txt` lists `FactoryPresets.cpp` in the plugin target.
5. **`CHANGELOG.md` v1.0.0** written; installed via `build-and-install.sh`.
6. **Deviation:** `"Koto / Harp"` → `"Koto Harp"` — the preset name is used verbatim as the
   JSON filename; `/` is a path separator that silently dropped the file (5/6 seeded on first
   run). Filesystem-safe rename, same concept. (Matches the known-pattern memory.)

### Goal Achievement

| Goal | Status | Evidence (independently re-verified, not just SUMMARY) |
|------|--------|--------------------------------------------------------|
| FUNC-07 — 6 factory presets | ✅ Achieved | 6 JSON files on disk in `~/Library/.../Presets/Factory/`; harness logs `Factory presets initialized: 6`; combos correct in JSON |
| Material convention | ✅ Achieved | On-disk JSON: String presets have `material`, no `damping`/`decay`; Modal have `damping`/`decay`, no `material` |
| Harness link seam fixed + green | ✅ Achieved | render-harness builds @ `JUCE_WEB_BROWSER=0`, links clean, **22/22 ALL PASS** |
| No Stage-2/3 regression | ✅ Achieved | Harness 22/22 (tuning C1 −0.00 / C7 1.76¢; bow sustains+bounded; modal inharmonicity 1760→2340Hz; state-roundtrip 852B); native-fn 12↔12; param-ID 17==17 |
| CHANGELOG + install | ✅ Achieved | `CHANGELOG.md` v1.0.0 present; AU registered (`auval -a`: `aumu OsPM OuDv`) |
| Cross-platform publish | ⏸️ Post-verify | Tag-driven CI by design — executes after this green verify (Task 10) |

## Requirements Verification

**Stage:** stage-4
**Requirements verified at this stage:** FUNC-07 + roll-up confirmation of COMPAT-01/02
(re-exercised in the shipping build).

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-07: Concept-isolating preset tour | should | ✅ Complete | 6 presets seeded, cover all 3 exciters + both resonators, each loads/round-trips, correct combo + material convention in JSON |
| COMPAT-01: pluginval (VST3 + AU) | must | ✅ Complete | pluginval strictness-10 SUCCESS on both, re-confirmed in shipping build |
| COMPAT-02: Windows WebView2 flags | must | ✅ Complete | `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` present (verified stage-1; survives into shipping build) |

**All prior-stage requirements** (FUNC-01..06, DSP-01..05/07/08, UI-01..06, PERF-01, QUAL-01)
remain **complete** — DSP/UI untouched in Stage 4 and re-confirmed green by the harness +
pluginval + auval pass.

**Requirements Summary (whole plugin, 24 total):**
- ✅ Complete: 23
- ⏸️ Deferred (v1.1): 1 — DSP-06 (waveguide string, `nice`), per D1
- ⚠️ Partial: 0
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + Standalone + render-test) | ✅ Pass | Exit 0, no plugin-code warnings |
| Render-harness @ `JUCE_WEB_BROWSER=0` | ✅ Pass | **22/22 ALL PASS** — link seam resolved, no DSP regression |
| pluginval strictness-10 — VST3 | ✅ Pass | SUCCESS, 0 failures (incl. fuzz parameters) |
| pluginval strictness-10 — AU | ✅ Pass | SUCCESS, 0 failures |
| `auval -v aumu OsPM OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| AU registered (`auval -a`) | ✅ Pass | `aumu OsPM OuDv — O-simplePhysicalModelSynth-dev` |
| Native-fn parity (C++ ↔ JS) | ✅ Pass | 12 ↔ 12 exact, no asymmetry |
| Param-ID count (createParameterLayout + ParamIDs) | ✅ Pass | 17 == 17 |
| `node --check` (app.js, preset-manager.js) | ✅ Pass | Both OK |
| Factory presets seeded on disk | ✅ Pass | 6 JSON files; combos + material convention verified in JSON |
| Standalone renders not-blank | ✅ Pass | `verify-standalone-render.png` (1.3 MB, full UI) |

### Per-preset on-disk verification (normalized JSON)

| Preset | excitationType | resonatorType | material | damping | decay | Convention |
|--------|---------------|---------------|----------|---------|-------|-----------|
| Bright Steel | 0.00 (Pluck) | 0.00 (String) | 0.08 | — | — | ✅ material-only |
| Muted Nylon | 0.00 (Pluck) | 0.00 (String) | 0.85 | — | — | ✅ material-only |
| Koto Harp | 0.00 (Pluck) | 0.00 (String) | 0.35 | — | — | ✅ material-only |
| Struck Bar | 0.50 (Strike) | 1.00 (Modal) | — | 0.55 | 0.60 | ✅ damping/decay-only |
| Bell | 0.50 (Strike) | 1.00 (Modal) | — | 0.70 | 0.88 | ✅ damping/decay-only |
| Bowed String | 1.00 (Bow) | 0.00 (String) | 0.15 | — | — | ✅ material-only |

All 3 exciters (Pluck/Strike/Bow) + both resonators (String/Modal) covered. No preset
co-authors `material` AND `damping`/`decay`.

## Human Verification (owed — closes the Stage-3 play-through, non-blocking)

Audio timbre/lockstep cannot be meaningfully automated; the user auditions after install:

- [ ] Audition all 6 presets — each sounds like its name (steel vs nylon; bar vs bell; bowed sustains).
- [ ] Dragging **Material** visibly co-moves Damping + Decay.
- [ ] Grey-out tracks `resonatorType`/`excitationType` live; selectors never trapped.
- [ ] Keyboard plays (mouse + QWERTY); preset bar navigates/saves/loads/deletes; factory presets non-deletable.
- [ ] **UI-02** loop pulse dims in lockstep with audible decay; Modal skin shows live stems.
- [ ] **UI-03** scope decays after note-off. **UI-04** spectrum: harmonic comb (String) vs inharmonic (Modal).
- [ ] No console errors; no clicks/buzz across the keyboard and parameter ranges.

## Issues Found

- **Preset-name `/` path-separator** (surfaced + fixed in execute): `"Koto / Harp"` silently
  dropped its JSON file (5/6 seeded). Fixed by the filesystem-safe `"Koto Harp"` rename —
  minimal in-scope fix, no module change, same plucked-string concept. All 6 now seed.
  Confirmed on disk this verify.
- **Plugin source not yet committed to git** — `Source/`, `CMakeLists.txt`, `CHANGELOG.md`
  show as untracked. Expected: the GSD workflow commits at the phase boundary (the
  `phase: ... 4-polish/verify complete` commit follows this verification). Not a defect.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next step:** Yes — this is the final stage (4 of 4). Plugin is feature-complete
for v1.0.0 and installed locally.

**Blockers:** None.

**Post-verify (by design, not blockers):**
- Manual play-through (D3) — user auditions after install (audio can't be automated).
- Cross-platform publish (D4 / Task 10) — tag `O-simplePhysicalModelSynth-v1.0.0` → CI
  (mac VST3/AU + Windows VST3). First real MSVC exercise; pre-flight ALL CLEAR (C3493 clean,
  single binary-data target, WebView2 static-link flag present) — watch the first Windows CI run.
