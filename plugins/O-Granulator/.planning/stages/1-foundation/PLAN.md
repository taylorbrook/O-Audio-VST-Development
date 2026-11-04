---
title: "O-Granulator Stage 1 (Foundation) — Execution Plan"
created: 2026-11-04
juce_version: "8.0.15"
summary: "Four-wave plan: (W1) extract pitch-collections module, (W2) migrate O-GrainScatter to it as 3.6.0 behind a bit-identical null gate, (W3) O-Granulator CMake + 220-param APVTS + pass-through shell + state codec + spec-derived golden harness, (W4) install + COMPAT-01 (pluginval 10 VST3/AU, auval). Order is mandatory: module → GS → Granulator (shared-build configure blast radius)."
stages: [1]
agents: [foundation-shell, build]
---

# O-Granulator — Stage 1 Execution Plan (Foundation)

## Goal

O-Granulator builds as VST3 + AU + Standalone with all **220** parameters registered in exact `parameter-spec.md` order (140 Float / 59 Choice / 21 Bool), a literal append-only **113-entry** mod-destination table, a bitwise pass-through shell in all three bus layouts, the full non-parameter state codec, and passes pluginval 10 (VST3 + AU) and auval. PitchCollections lives in a new shared module `modules/tuning/pitch-collections` 1.0.0, and O-GrainScatter 3.6.0 consumes it with **bit-identical** audio vs 3.5.1.

No DSP, no WebView editor (GenericAudioProcessorEditor), no presets/i18n work for Granulator.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/1-foundation/CONTEXT.md` | Locked decisions D1 (module + GS migration), D2 (Generic editor, WebView plumbing wired), D3 (3 bus layouts) |
| `stages/1-foundation/RESEARCH.md` | Line-level recipes. §1 module, §2 GS gate, §3 CMake review, §4 params, §5 buses, §6 state, §7 harness. **Authoritative over ARCHITECTURE.md where they differ.** |
| `parameter-spec.md` (LOCKED) | The contract: Full ID list `:258-479`, dest table `:201-231`, scale list `:162-193`, non-param state `:240-256`. NOT the draft. |
| `mockups/v2-CMakeLists.txt`, `mockups/v2-ui.yaml` | CMake starting point; host `name:` strings for the 112 dests |
| `plugins/O-GrainScatter/Source/dsp/PitchEngine.h` | Move source (lines 41-305) |
| `plugins/O-ReverseDelay/Source/PluginProcessor.cpp:1518-1531`, `tests/render-harness/main.cpp:613-627` | Bus predicate, MockPlayHead |
| `plugins/O-Strata/Source/StrataParamIds.h` | ID-generation pattern |
| `modules/synthesis/grain-engine/{module.yaml,tests/}` | Header-only module + standalone test template |

---

## Plan-phase decisions (resolve RESEARCH open questions)

- **P1 (O1) Host names for the 108 un-named params** — extend the existing YAML convention (`"S1 · Level"`):
  - Stream: `S<N> · Enable`, `S<N> · Mute`, `S<N> · Solo`, `S<N> · Size Sync`, `S<N> · Density Sync`, `S<N> · Scale`, `S<N> · Onset Trigger`, `S<N> · Filter Type`, plus any other non-dest stream row as `S<N> · <YAML label>`.
  - Global: `Freeze`, `Sync Mode`, `Scale Root`, `Grain Limit`; LFO: `LFO <N> · Shape|Rate Sync|…`; mod slots: `Mod <M> · Source|Dest|Depth`; `Macro <N>`; `Pitch Follow S<N>`; `Onset Threshold`.
  - The executor writes the full 220-name list into `tests/golden/params.tsv` (from `extract_spec.py`). **Checkpoint:** show the user the 108 derived names (one compact list) before T4 compiles them in. Names are display-only (IDs are identity), so a later rename is cheap pre-1.0.
- **P2 (O2) `toScala`** — exactly RESEARCH §1d item 3 (header `! <legal name>.scl`, description = sanitized name or `"Ouaricon custom scale"`, shortest exact cents precision p ∈ 1…10, bare-int tokens get `.0`).
- **P3 (O3) uiLanguage** — write the code string (`"en"/"fr"/"zh-Hans"`, RD precedent); read tolerantly (digit string → int, else `languageIndex`). Note the deviation in `parameter-spec.md:250` with a one-line amendment.
- **P4 (O4) GS copy** — minimum: `badEdo` error row only (en / fr `reviewed:false` / zh-Hans `reviewed:'mt'`). No help-text rewrite.

---

## Tasks

### Wave 1 — pitch-collections module (lands first; nothing else configures against it until done)

**T1. Capture v3.5.1 parse expectations** *(before any move)*
- Scratch console (scratchpad, not committed) against HEAD GS code: dump `error | count | period hex | cents hex` for the U5 corpus (~30 valid/invalid texts + 3 `.scl` bodies).
- Output → hex literals for U5 in T2.
- Depends on: none.

**T2. Create `modules/tuning/pitch-collections` 1.0.0**
- Files: `module.yaml`, `README.md`, `cpp/PitchCollections.h`, `tests/CMakeLists.txt`, `tests/pitch_collections_tests.cpp`.
- `PitchEngine.h:41-305` moved **verbatim** under `namespace ouaricon::tuning::PitchCollections`, includes `<juce_core/juce_core.h>` + std headers. Must compile at **gnu++17**.
- Additions appended (existing bodies byte-identical): 4-arg `tableFromText(..., badToken)` (3-arg becomes a thin wrapper), `edo N [period]` via `equalDivision` (regex, error order, >3-digit → `badEdo` per RESEARCH §1d), `toScala` (P2).
- README documents the frozen Id enum / `names()` contract (GS session compatibility) and the edo grammar.
- Tests U1–U6 (RESEARCH §1e); exit 0 iff all pass. Standalone configure: `cmake -S modules/tuning/pitch-collections/tests -B build-pitch-collections-tests -G Ninja`.
- Depends on: T1.

**T3. Registry entry**
- `modules/registry.yaml`: new entry (grain-engine block shape), header `version` 1.2.18 → 1.3.0, `last_updated: 2026-11-04`.
- Depends on: T2.

**Gate W1:** module tests exit 0. Commit: `git commit -- modules/tuning/pitch-collections modules/registry.yaml`.

### Wave 2 — O-GrainScatter 3.6.0 migration (own commit; any non-zero `cmp` is a BLOCKER)

**T4. Pre-capture baselines** *(before editing PitchEngine.h)*
- Build `O-GrainScatter-param-dump`, save output to scratchpad.
- Add `tests/pitch-null/main.cpp` + CMake block with `pitch-null-v351` twin (`git archive 0e1117b6`, RESEARCH §2b). Jobs R1–R4, P, positive control (`parse-edo.txt` MUST differ), seed liveness, signal RMS > −90 dBFS. Uses only the 3.5.1 public API.
- Run `pitch-null-v351 <dir>` → dumps.
- Depends on: W1.

**T5. Migrate**
- `Source/dsp/PitchEngine.h`: delete 41-305; `#include "PitchCollections.h"` + `namespace PitchCollections = ouaricon::tuning::PitchCollections;`. Engine class untouched.
- `CMakeLists.txt`: `VERSION 3.6.0`; `ouaricon_add_module(OuariconGrainScatter pitch-collections)` beside `:54`.
- UI: `app.js:626-633` add `badEdo` branch; `i18n.js` `label.customErr.badEdo` en/fr/zh-Hans (P4).
- `CHANGELOG.md` `## [3.6.0] - 2026-11-0X` (Added / Changed / Testing).
- `.planning/STATUS.md` version, `PLUGINS.md` row.
- Depends on: T4.

**Gate W2 (all mandatory):**
- `pitch-null` all files byte-equal, positive control differs, seeds pairwise distinct.
- `ninja -t deps` shows old-tree twins resolve their own `dsp/PitchEngine.h`.
- 5 existing gates + their twins/negative controls green (blocksize-null, density-gain(+compoff), alias-gate(v320 fails rows 1,2,4), skip-tilt, adapt-gate(+fixedk)).
- param-dump diff empty.
- `node scripts/check-i18n.js`, fr lint, zh lint, `check-ui-labels --plugin O-GrainScatter` clean.
- `./scripts/build-and-install.sh O-GrainScatter`; pluginval 10 VST3 + AU; `auval -v aufx OuGS OuDv`.
- Commit (temp-index, path-scoped): `plugins/O-GrainScatter/{Source,CMakeLists.txt,CHANGELOG.md,tests/pitch-null,.planning/STATUS.md}` + `PLUGINS.md`. **Do not sweep** the untracked `nielsen-dafx26.txt`. No tag.

### Wave 3 — O-Granulator shell

**T6. CMake + file skeleton**
- `plugins/O-Granulator/CMakeLists.txt` from `v2-CMakeLists.txt`: pitch-collections line uncommented; render-harness console block; `target_compile_features(... cxx_std_20)` after **every** console call; `GOLDEN_DIR` define on the harness. Do NOT call `ReverbUnit::addParameters`.
- UI resources so binary data resolves: `v2-ui.html` → `Source/ui/public/index.html`, `v2-i18n.js` → `Source/ui/public/js/i18n.js`, JUCE `index.js` + `check_native_interop.js` → `Source/ui/public/js/juce/`.
- Placeholder `Source/PluginEditor.{h,cpp}` (comment-only TU; Stage 3 swaps contents).
- Never commit `Source/ui/public/modules/*` (gitignored copies).
- Gate: root configure clean (blocks all sessions if broken — configure before committing).
- Depends on: W2.

**T7. Golden extractor** *(spec-derived, never code-derived)*
- `tests/golden/extract_spec.py` parses `parameter-spec.md` (+ `v2-ui.yaml` names, + P1 convention) → `tests/golden/params.tsv` (idx, id, name, type, start, end, default, skew/centre, choices) and `tests/golden/mod-dests.tsv` (113). Both committed.
- **User checkpoint:** present the 108 derived names (P1).
- Depends on: T6.

**T8. `Source/GranulatorParamIds.h` + `createParameterLayout()`**
- Frozen suffix arrays + loop builders (Strata pattern); `allIdsInOrder()`; **literal** `kModDestIds[113]`; `kModDestTableVersion = 1`.
- `juce::ParameterID{id, 1}`; interval 0; skew via `setSkewForCentre`; no lambda ranges; `s1_enable` default On, s2–s4 Off.
- `scaleChoiceToId()` + static_asserts (RESEARCH §1d).
- Depends on: T7.

**T9. Processor shell**
- Buses: stereo default; predicate accepts mono→mono, mono→stereo, stereo→stereo only (RD reference).
- `processBlock`: `ScopedNoDenormals`; mono→stereo copies ch 0 → 1; clear non-output channels; all loops bounded by `buffer.getNumChannels()`; stereo decision from negotiated layout in `prepareToPlay`.
- `setLatencySamples(0)` in `prepareToPlay`; tail 0.
- Seed: `std::atomic<uint64_t>`, ctor seeds from `Random::getSystemRandom().nextInt64()`; public `getSeed/setSeed`.
- Warm `PitchCollections::builtIns()` in the ctor.
- State codec per RESEARCH §6: write onto `copyState()` copy; `Engine` (seed as int64 string, stateVersion, modDestTableVersion), `CustomScale` (text = source of truth), `DegreeMasks` (16-hex, non-default only, canonicalized), `uiLanguage` (P3). No `id` attribute on custom children. Restore resets non-param state first, `isVoid()` guards, `containsOnly` validation.
- `createEditor()` → `GenericAudioProcessorEditor` under the `JUCE_WEB_BROWSER` guard.
- Grep gate: zero `setValueNotifyingHost` in `Source/`.
- Depends on: T8.

**T10. Render harness `O-Granulator-render-harness`**
- `tests/render-harness/main.cpp`; MockPlayHead (bpm, isPlaying, timeSig, timeInSamples, ppq, `advance()`), null-playhead + no-bpm rows.
- Rows: G1–G6, D1–D4 (RESEARCH §4c); B1 layouts (reject stereo→mono, disabled, 5.1); B2 `memcmp` pass-through in 3 layouts × {defaults, mix 0 %} × blocks {1, 32, 441, 512, 4096, seeded variable}, mono→stereo L == R == in; B3 latency 0; S1–S4 state (seed `0xF00DFACECAFEBEEF`, mask `0x8000000000000001`, custom `edo 19`, uiLanguage 2, two reopens, no Mask children at defaults); P1 scale map/names cross-check.
- Default checks: normalised exact, engineering values with explicit tolerance (skewed defaults are not exact — RESEARCH §4b).
- Gate: exit 0.
- Depends on: T9.

### Wave 4 — install + COMPAT-01

**T11. Install + validate** *(long commands with `run_in_background`)*
- `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep + AU cache clear).
- pluginval `--strictness-level 10 --timeout-ms 600000` on `O-Granulator-dev.vst3` (2–3 runs) and `--validate-in-process` on the `.component`.
- `auval -v aufx OuGn OuDv` (cold rescan → background) + negative control `auval -v aufx ZZZZ OuDv`. Never `auval -a`.
- Standalone launch smoke.
- Depends on: W3.

**T12. Bookkeeping + commit**
- `.planning/params.tsv` from `O-Granulator-param-dump`; `scripts/regen-registry-used-by.sh`; `parameter-spec.md:250` P3 amendment; `PLUGINS.md` row; STATUS.md (execute ✓).
- Re-check `git branch --show-current && git status --short`; temp-index path-scoped commit: `plugins/O-Granulator modules/registry.yaml PLUGINS.md`.
- Depends on: T11.

---

## Dependency graph

```
T1 → T2 → T3 ─┐ (W1 commit)
              └→ T4 → T5 ─┐ (W2 commit)
                          └→ T6 → T7 ─(user names check)→ T8 → T9 → T10 → T11 → T12
```

Strictly serial across waves (configure blast radius: a consumer referencing a non-existent module breaks `cmake` for every concurrent session).

## Execution notes

- `foundation-shell-agent` has no Bash. The orchestrator runs every configure/build/test/install/commit; the agent writes files.
- Builds share `build/` (`OUARICON_BUILD_TESTS=ON` already). Target names ≠ folder: `OuariconGranulator_*`, consoles `O-Granulator-<suffix>`.
- 0→1 gate needs `--force`.

## Success criteria (Stage 1 gate)

- [ ] VST3 + AU + Standalone build; `auval -v aufx OuGn OuDv` PASS; pluginval 10 PASS on VST3 and AU
- [ ] B2: output bitwise equals input in all three layouts at defaults and mix 0 %
- [ ] D1–D4: 113-entry dest table matches spec-derived golden, order frozen, all dests are Float
- [ ] G1–G6: 220 params, exact ID order, 140 / 59 / 21, version hint 1, ranges/defaults/choices match golden
- [ ] S1–S4: state round-trips uint64 seed > 2^63, high-bit masks, custom scale, uiLanguage; two reopens clean
- [ ] pitch-collections U1–U6 pass (edo, badEdo, offending token, toScala round-trip, v3.5.1 corpus)
- [ ] O-GrainScatter 3.6.0: pitch-null byte-identical + positive control, 5 existing gates green, param-dump diff empty, i18n gates clean, pluginval + auval PASS
- [ ] No `setValueNotifyingHost` in O-Granulator `Source/`; no tag created; untracked foreign files untouched

## Risks

- **R1** GS null mismatch → stop; check verbatim-ness (whitespace-intact diff) and include order before any code change.
- **R2** Shared-build configure breakage → land W1 → W2 → W3 in order; configure before every commit.
- **R3** pluginval on 220 params may run long → 600 s timeout, background.
- **R4** Stage 2 must not assume `get()` on skewed/non-endpoint defaults is bit-exact.
