# Stage 3 — GUI · Phase 3.1 (Layout + controls + bindings) — Plan

**Plugin:** O-Granulator
**Stage:** 3 of 4 (GUI)
**Phase:** 3.1 of 3
**GSD phase:** plan
**Date:** 2026-11-06
**Branch:** `main` (trunk-based) @ `4bc8a31e`
**Inputs:**
- `CONTEXT-3.1.md` (D26–D29 locked)
- `RESEARCH-3.1.md` (R104–R116). **Where it differs from the integration checklist, ROADMAP.md or CONTEXT-3.1, RESEARCH-3.1 wins**, as amended by the sign-offs and P97–P112 below.
- `mockups/v2-integration-checklist.md` §1–9 (the contract), `mockups/v2-PluginEditor.h/.cpp` (template), `Source/ui/public/index.html` (== `v2-ui.html`, byte-identical)
- Read at plan: `PluginProcessor.h/.cpp` (state codec `:402-541`, `createEditor` `:314`), `PluginEditor.h/.cpp` (Stage 1 resource map), `CMakeLists.txt` (WebView block present, `:39-40, 97, 138`), `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` (callbacks `:112`, exclusion `:133`, validated apply `:146`, WR-01 reset `:384-405`, customLoad on every apply `:437-442`), `scripts/ui-stub/generic-juce-stub.js` (overrides `:130-237, 413`), `scripts/serve-ui.js:338-370`, `scripts/check-ui-labels.js:558-645`

---

## Sign-offs (Taylor, 2026-11-06, at plan)

| # | Item | Decision |
|---|---|---|
| 1 | **R105** — seed in presets | **Carry it.** The seed travels in `customState`; a preset reproduces its texture. |
| 2 | **R105** — `output_gain` | **Excluded** (`setExcludedParameters ({ "output_gain" })`): a preset never writes it, and a load never resets it. |
| 3 | **R108** — setter bumps | **Keep the verified processor.** Amend checklist §2 to read "bumped on every scale-state change" (the `PluginProcessor.h` comment already says so). Hands-on watch: paint-drag across 53-EDO chips, no visible flicker. |

---

## Entry Check

- Re-hash the six contracts (`shasum -a 256`) against the STATUS.md frontmatter (brief `f9c748a8…`, requirements `eda58c1d…`, param-spec draft `d0b1457e…`, param-spec `f5ce3595…`, architecture `f0cf753b…`, roadmap `1060dae0…`). 3.1 edits none of them.
- `git diff --stat 9831f498 HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt modules scripts` is empty (the Stage-2-verified tree; RESEARCH-3.1 confirmed it at `ba80f6e3`).
- `cmp Source/ui/public/index.html .planning/mockups/v2-ui.html` and `cmp Source/ui/public/js/i18n.js .planning/mockups/v2-i18n.js` clean.

---

## Goal

**UI-01:** the v2 WebView editor ships in the plugin, with all 220 parameters reachable and bound both ways, all 21 native functions live, and host automation / preset recall reflected in the page.

Concretely:
- `GranulatorEditor` from the template, fixed 1200 × 800, relays → `webView` → attachments, 140 / 59 / 21 by ID **and** type, 21 native functions, resource provider = the Stage 1 `granulatorUiResource`, Windows WebView2 block verbatim.
- Processor gains exactly three things: an `OuariconPresetManager` (files only), a `Telemetry` struct + `readTelemetry`, and `getWindowCurve` backed by a new const Engine method. **Audio does not move.**
- Gates that cannot pass vacuously: C++-generated stub overrides (R112), the interaction-state file (D29), a static four-way closure and a 220-assertion two-way page probe (R113), each with a failing negative control.

Not in 3.1: wave ring, grain seqlock (3.2); meters, counts, LFO phase, onset counter, loop gain (3.3); the 16-item DAW pass (3.3 verify, D27).

---

## Plan-phase decisions

- **P97 — baseline B first, on the unmodified tree (T0).** Harness default set (463, background) → carried extract (sorted `[PASS]/[FAIL]`, INFO/timing rows dropped) = **B**; N3 / N2 / N1 / N0 `--strict` + R12 rc 0; HA host-cache gate PASS; `O-Granulator-param-dump` vs `.planning/params.tsv` diff-clean. Also run the five UI gates on the current tree and record the result as the pre-state (the stub is under-seeded, so these are expected to "pass"; that is the R112 point). No commit.

- **P98 — one non-parameter apply path (R105 step 3).** Factor `applyNonParameterState (const juce::var& custom, const juce::var& masks, const juce::var& seed)` out of `setStateInformation`. Both the session codec and `customLoad` call **reset → apply what is present → `bumpScaleRevision()`**. The session codec keeps its XML encoding (it adapts the ValueTree children into the helper's inputs, or the helper takes a small neutral struct — executor's choice, but there is ONE apply implementation). Bitwise-identical session behaviour is the gate: HA + the S-rows + a new two-reopen row (P104).

- **P99 — seed in presets (sign-off 1).** `customSave` writes `seed` as an **int64 decimal STRING**. A JSON number loses precision above 2^53 (the same class as critical_juce_getlargeintvalue_clamps_unsigned_64). `customLoad`: present + well-formed → `setSeed`; **absent → keep the live seed** (there is no default seed; `resetNonParameterState` does not own it today and does not start to). Parse rule = the session codec's (`containsOnly ("-0123456789")`, `getLargeIntValue`).

- **P100 — preset manager wiring.** `OuariconPresetManager presetManager { parameters, "O-Granulator" };` declared after `parameters` and before `engine`'s users; `getPresetManager()`. Constructor: `setCustomStateCallbacks` (P98/P99), `setExcludedParameters ({ Ids::outputGain })` (sign-off 2), `setValidatedApply (true)`. `customState` = `{ seed, custom: { name, text }, masks: [ { stream, collection, bits: 16-hex } … non-default only ] }`. Session state stays on the Stage 1 codec: **never** call `getStateAsXml` / `setStateFromXml`. No preset-manager module change.

- **P101 — `Engine::windowCurve` (R107).** `void windowCurve (float window, float skewRaw, float* out, int n) const noexcept` — `step = WindowMorphBank::stepFor (clamp (window, 0, 1))`, `tilt = WindowLut::makeTilt (0.5f + 0.45f · clamp (skewRaw, −100, 100) / 100)`, `out[i] = WindowMorphBank::read (bank.table (step), tilt, i / (n − 1))`. Read-only on the immutable bank; not called on the audio thread. `getBank()` stays under `OGRAN_TEST_HOOKS`. Processor `getWindowCurve (int stream, float* out, int n) const` reads the BASE `sN_window` / `sN_window_skew` raw values.

- **P102 — `Telemetry` now, filled by phase (R106, R116).** Declare every checklist field. 3.1 fills `scaleRev`, `sources[11]`, `lfo[4]` (= `sources[0..3]`), `env` (= `sources[4]`), `pitchMidi / pitchCents / pitchClarity`; everything else zeroed, `loopGainDb` = quiet NaN. `readTelemetry` returns **true** in 3.1 (no seqlock yet). The editor's `buildTelemetry` **omits** keys whose fields are not live (§3 of RESEARCH: `lfoPhase` may be sent as zeros; `onset`, `onsetFired`, `inPeak`, `out`, `counts`, `limit`, `loopGainDb`, `wave`, `spanSec`, `grains` omitted). `modNorm` = object keyed by destination index, routed destinations only, routing read from APVTS on the message thread. `pitch` = object `{ midi, cents, clarity }`, omitted when `pitchMidi ≤ 0`. Copy-only; the editor never touches the pool or scheduler.

- **P103 — editor = template with three deltas.** (a) `withResourceProvider ([] (const auto& u) { return granulatorUiResource (u); })` and the template's member `getResource` deleted (R114). (b) `getScaleState`: collection 31 reads `processorRef.getCustomTable()` (a locked copy), never `getCollectionTable (31)` (R109). (c) `buildTelemetry` per P102. Otherwise the template's shapes are the contract: rename only to the Stage 1 names. `createEditor()` → `new GranulatorEditor (*this)` under `#if JUCE_WEB_BROWSER` (include under the same guard); consoles keep the generic editor.

- **P104 — new harness rows (processor-only, all in a new `engine_gates_31.h`).** The harness compiles the processor (not the editor), so the new processor surface is gated there:
  - **PM1** preset round trip via `savePresetToFile` / `loadPresetFromFile` in a temp dir on a fresh instance: all parameters except `output_gain`, the custom scale (name + text), three non-default masks, the seed — all equal; scale revision moved.
  - **PM2** `output_gain` set to −12 dB, then load a preset saved at 0 dB → still −12 dB; the preset JSON has no `output_gain` key.
  - **PM3** a preset with no `customState`: custom scale back to 12-EDO, masks all-on, **seed unchanged** (P99).
  - **PM4** two-reopen: preset load → `getStateInformation` → fresh instance `setState` → change mask → `getState` → fresh instance `setState`: final state == the changed one; no `CustomState` / duplicate custom child in the live tree after either load (critical_preset_manager_stale_customstate_child).
  - **PM5** seed precision: seed `0xFEDC'BA98'7654'3211` (> INT64_MAX as uint64) round-trips exactly through a preset.
  - **WC1** `windowCurve (0, 0, …, 64)` == closed-form Hann within 1e-6; **WC2** skew ±100 and window 1 produce finite, in-[0, 1], endpoint-0 curves and differ from WC1 (liveness).
  - **TL1** `readTelemetry` returns true; `sources` == the engine's `getSrcTelemetry`; pitch fields == `getPitchNote / getPitchConfident`; all 3.2/3.3 fields zero / NaN.
  - Negative controls: PM2n (exclusion off → −12 dB lost), PM3n (seed reset on absent → differs), PM5n (seed written as a number → mismatch). Each NC must FAIL its predicate.

- **P105 — audio-neutral proof (R115).** After T1–T3: harness default set → carried extract **diff vs B empty**; N3 / N2 / N1 / N0 `--strict` + R12 rc 0; HA host-cache PASS; param-dump diff-clean. Expected harness count 463 + P104 rows (executor records the exact number).

- **P106 — stub overrides from C++ (R112).** `--dump-ui-overrides <path>` in the render-harness `main.cpp` (early exit, no gates run). Emits `tests/ui-stub/generic-overrides.json`:
  - `_why` (one sentence + the regeneration command);
  - `combos`: all 59 `{ choices, def }` from `AudioParameterChoice::choices` / default index;
  - `sliders`: every Float with `start / end / def / skew / interval` from `getNormalisableRange()` (all 140, not just skewed — one rule, no hand list);
  - `natives`: `getScaleState` (the editor's encoding at the default state — share the encoder: P107), `getTelemetry` (widest-realistic frame, RESEARCH §7: counts 128 ×4, limit 128, `loopGainDb` −0.4, pitch `{61, −49, 1}`, every 3.2/3.3 key present, `scaleRev` == the fixture's), `getParameterDefaults`, `getWindowCurve` (P101, 64 points).
  - Deterministic: generated twice → byte-identical (no seed, no clock in the output). Committed; the gate step regenerates it and `git diff --exit-code` must be clean.

- **P107 — one scale-state encoder.** `buildScaleState` (tables 1–31, custom, masks) lives in a header both the editor and the harness include (e.g. `Source/ScaleStateJson.h`, `juce_core` only, no WebBrowser types), so the fixture is produced by the same code the page talks to (pattern_ui_fixture_state_must_be_producible_by_cpp). Same for the telemetry → `var` builder if the executor finds it shares cleanly; otherwise the widest frame is built in the harness from the `Telemetry` struct through the editor's documented key names, and T9's closure check pins the key set.

- **P108 — `tests/i18n-states.json` (D29, R111).** RESEARCH §6 order, plus one matrix state (slots 1–3 set to the three longest localized destination labels through the page's own `<select>` `change`), popover last. While writing it, confirm every click state with `elementFromPoint(centre) === target` once (pattern_forced_click_under_popover); use `eval` for overlap-prone targets. Runner banner must print `states: default + 8 from tests/i18n-states.json`.

- **P109 — static closure `tests/ui_frontend_check.js` (R104, R113 layer 1).** Port from `O-ReverseDelay/tests/ui_frontend_check.js` (§3 bridge closure, §11 member order, §15 four-way closure), adapted:
  - APVTS side from a fresh `O-Granulator-param-dump` (not a regex — the layout is table-driven);
  - fresh dump == editor `kSliderIds/kComboIds/kToggleIds` == page tables, by ID and TYPE, both directions, no duplicates;
  - 21 page/preset-manager.js native names == 21 `withNativeFunction`;
  - member order relays → `webView` → attachments (→ `webView2RuntimeLink`), constructor in the same order;
  - `node --check` on the inline module, one `init()` as the last statement, `Juce` namespace passed, no viewport units, every `juce_add_binary_data` SOURCE ↔ a `granulatorUiResource` branch, `Source/ui/public/modules/*` untracked;
  - the telemetry keys `buildTelemetry` can emit ⊆ the keys the page reads (guards against a typo'd key that silently reads as "default");
  - **NC:** a scratch copy with one ID moved `kSliderIds` → `kToggleIds` FAILS; a scratch copy with one `withNativeFunction` removed FAILS.

- **P110 — two-way page probe `tests/ui_binding_probe.js` (R113 layer 2).** Playwright via `serve-ui.js` with P106's overrides; quiesce on `init()` + first telemetry tick. For each of the 220 ids: host → UI (set stub state + fire `valueChangedEvent`; assert the control's DOM moved: `val-<id>` text == `FORMAT[id](scaled)` / `choices[idx]`, `aria-pressed`, pressed segment, `selectedIndex`, scale trigger label) and UI → host (ArrowUp / click / `change`; assert the stub value changed and `sliderDragStarted/Ended` paired for continuous controls). **Liveness: assertion count == 220 each direction.** Hidden tabs: switch tab before probing its ids (the probe drives `#tab-*`). **NC:** a page copy with one `bindKnob` removed FAILS; exit 77 (no Playwright) is not a pass.

- **P111 — gate run order (T11).** Regenerate overrides (diff-clean) → `ui_frontend_check.js` → `ui_binding_probe.js` → check-i18n → fr-lint → zh-lint → check-ui-labels (banner must say `seed=param-dump (220 rows) + overrides` and `states: default + 8`) → boot-all-uis `--strict-tips`. Any 77 = stop, install Playwright, re-run.

- **P112 — hands-on doc, not run in execute.** `HANDS-ON-3.1.md` (R113 layer 3 + checklist §4–§7, §9): Standalone + one DAW; one control per kind per page; one automation lane per relay type; preset save/load (bar + chips + custom name re-pull + seed reproduces texture); output_gain survives a load; two reopens; Inspector console clean, no 404; EB Garamond on canvas; Reduce Motion; fr / zh-Hans no clipping; editor close/reopen ×10; the R108 paint-drag watch; expected 3.1 stub artefacts listed (RESEARCH §3: "0 sounding", "0 / 128", "Open", LFO dot at the left edge, flat level lane, empty buffer strip) so verify does not flag them. Run at **3.1 verify** by Taylor.

---

## Source layout (deltas)

| File | Change |
|---|---|
| `Source/PluginProcessor.h/.cpp` | `presetManager` + `getPresetManager()`; `applyNonParameterState` (P98); callbacks / exclusion / validated apply (P100); `Telemetry` + `readTelemetry` (P102); `getWindowCurve` (P101); `createEditor` (P103) |
| `Source/dsp/Engine.h` (`.cpp`) | `windowCurve (…) const noexcept` (P101) — the only `dsp/` change; never on the audio path |
| `Source/ScaleStateJson.h` (new) | shared scale-state encoder (P107) |
| `Source/PluginEditor.h/.cpp` | `GranulatorEditor` from the template + P103 deltas; `granulatorUiResource` kept |
| `tests/render-harness/engine_gates_31.h` (new), `main.cpp` | P104 rows; `--dump-ui-overrides` (P106) |
| `tests/ui-stub/generic-overrides.json` (new, generated) | P106 |
| `tests/i18n-states.json` (new) | P108 |
| `tests/ui_frontend_check.js` (new) | P109 |
| `tests/ui_binding_probe.js` (new) | P110 |
| `.planning/mockups/v2-integration-checklist.md` | §2 R108 wording (sign-off 3); §1 settings-popover item closed (D28) |
| `CMakeLists.txt` | expected unchanged (WebView block already landed in Stage 1); add `ScaleStateJson.h` only if a source list needs it |

Unchanged: `Source/ui/public/**` (v2 locked, D28 — defects only, each one a recorded deviation), `modules/**`, `scripts/**`, every contract file.

---

## Tasks

### Wave 0 — precondition

1. **T0 — Entry Check + baseline B** (P97). Harness default set + N-suites + R12 + HA + param-dump diff + UI-gate pre-state, all logged under `${TMPDIR:-/tmp}/ogran-31/`. No commit.
   - Depends on: none

### Wave 1 — processor surface (audio-neutral)

2. **T1 — preset manager** (P98, P99, P100). Shared apply helper, callbacks, seed string, exclusion, validated apply.
   - Files: `PluginProcessor.h/.cpp`
   - Depends on: T0
3. **T2 — window curve** (P101).
   - Files: `dsp/Engine.h/.cpp`, `PluginProcessor.h/.cpp`
   - Depends on: T0
4. **T3 — telemetry struct** (P102) + `ScaleStateJson.h` (P107).
   - Files: `PluginProcessor.h/.cpp`, `Source/ScaleStateJson.h`
   - Depends on: T0
5. **T4 — harness rows + neutral proof** (P104, P105). `engine_gates_31.h` with PM1–PM5, WC1–WC2, TL1 and the three NCs; then the full default set: carried diff vs B empty, N3/N2/N1/N0 `--strict` + R12, HA, param-dump clean. **Commit** (path-scoped).
   - Files: `tests/render-harness/engine_gates_31.h`, `main.cpp`
   - Depends on: T1, T2, T3

### Wave 2 — editor

6. **T5 — `GranulatorEditor`** (P103). Template → `Source/PluginEditor.*` with the three deltas; `createEditor` switch. Build VST3 + AU + Standalone, **0 warnings**.
   - Files: `PluginEditor.h/.cpp`, `PluginProcessor.cpp`
   - Depends on: T4
7. **T6 — install + validate.** `./scripts/build-and-install.sh O-Granulator` (cache clear, dual-variant sweep); `auval -v aufx …`; pluginval 10 VST3 + AU (opens the editor); Standalone launch smoke: window appears, not blank (one `screencapture` of the Standalone window for the SUMMARY). **Commit.**
   - Depends on: T5

### Wave 3 — UI gates (no `Source/` change unless a gate finds a defect)

8. **T7 — `--dump-ui-overrides` + `generic-overrides.json`** (P106, P107). Generate twice, byte-identical; commit the JSON.
   - Files: `tests/render-harness/main.cpp`, `tests/ui-stub/generic-overrides.json`
   - Depends on: T4 (needs the processor surface; independent of the editor build)
9. **T8 — `tests/i18n-states.json`** (P108). Needs T7's real choices for the matrix state.
   - Depends on: T7
10. **T9 — `tests/ui_frontend_check.js`** (P109) + its two NCs.
    - Depends on: T5
11. **T10 — `tests/ui_binding_probe.js`** (P110) + its NC.
    - Depends on: T7
12. **T11 — full gate run** (P111). All green, exit codes recorded; any page defect fixed under D28 as a recorded deviation; then re-run the whole T11 sequence. **Commit.**
    - Depends on: T6, T8, T9, T10

### Wave 4 — docs + handoff

13. **T12 — docs.** Checklist §1/§2 amendments; `HANDS-ON-3.1.md` (P112); `SUMMARY-3.1.md` (rows, counts, deviations, negative-control results, timings); STATUS.md (`stage_3_phase_3_1_execute_complete`, `next_action: stage_3_phase_3_1_verify`); REQUIREMENTS UI-01 → `partial` (hands-on pending). **Commit** path-scoped: `git commit -- plugins/O-Granulator`.
    - Depends on: T11

---

## Dependency graph

```
T0 ─┬─ T1 ─┐
    ├─ T2 ─┼─ T4 ─┬─ T5 ─┬─ T6 ───────────┐
    └─ T3 ─┘      │      └─ T9 ───────────┤
                  └─ T7 ─┬─ T8 ───────────┼─ T11 ── T12
                         └─ T10 ──────────┘
```

---

## Gate criteria (3.1)

| Gate | Pass |
|---|---|
| Harness default set | all PASS; carried extract diff vs B **empty**; P104 rows PASS; PM2n / PM3n / PM5n each FAIL their predicate |
| N3 / N2 / N1 / N0 `--strict`, R12 | rc 0 |
| HA host-cache | PASS |
| param-dump | `git diff --exit-code .planning/params.tsv` clean |
| Build | VST3 + AU + Standalone, 0 warnings |
| auval, pluginval 10 VST3 + AU | PASS (pluginval opens the WebView editor) |
| `generic-overrides.json` | regenerated twice byte-identical; diff-clean vs committed |
| `ui_frontend_check.js` | 0 failures; both NCs FAIL |
| `ui_binding_probe.js` | 220 / 220 host → UI, 220 / 220 UI → host; NC FAILS |
| check-i18n · fr-lint · zh-lint | 0 · 0 · 0 |
| check-ui-labels | 0; banner `+ overrides`, `states: default + 8` |
| boot-all-uis `--strict-tips` | completes, 0 DEAD, 0 late |

Exit 77 anywhere = not run = not a pass.

---

## Risks / watch items

- **Codec refactor (P98) is the one risk to verified behaviour.** The session path must stay byte-for-byte: compare `getStateInformation` output before/after on the S-row fixtures (T4) in addition to HA.
- **Preset apply on a parameter listener:** WR-01 resets every parameter with `setValueNotifyingHost` on the message thread. No listener writes another parameter (verified Stage 2 constraint) — keep it that way; if anything fires a nested write, stop (critical_vst3_nested_param_write_in_listener_never_reaches_host).
- **Stub-only probe ≠ real WebView.** Layer 2 proves the page binds every id both ways under a faithful stub; JUCE's relay ↔ attachment plumbing is only proven by the hands-on (P112) and pluginval's editor pass.
- **Page defects under D28.** Fix only breakage (clipping, dead bindings, contrast/fit). Each fix: deviation entry + `v2-ui.html` kept in sync (the byte-identical relation is part of the Entry Check of later phases).
- **WebView completions while hidden** — never `setVisible (false)` (critical_webview_completion_gated_on_isvisible).
- **Windows block** compiles only on Windows; not exercisable until Stage 4 COMPAT-03. Keep verbatim, `withBackend` + `withUserDataFolder` present (grep in T9).

## Deviations protocol

As in Stage 2: record every departure from this plan in SUMMARY-3.1 (what, why, evidence, which gate). Stop and ask only for: an audio-moving change, a contract edit, a module change, or a v2 design change beyond a defect.

## Carried (not 3.1)

Env 1e30 pin → Stage 4. Harness items: C7 machine-speed calibration, event-aware `renderPool` (Z6a). W2o — widen seeds, never the threshold. DAW pass (DAW-2.5a 1–8, DAW-2.5b 1–5, DAW-2.6 1–3) at 3.3 verify (D27).

## Success criteria (gate for verify)

- [ ] `GranulatorEditor` builds and loads in Standalone / AU / VST3; fixed 1200 × 800; pluginval 10 + auval PASS
- [ ] 220 relays/attachments match APVTS by ID and TYPE (static closure, NC fails)
- [ ] 21 = 21 native functions; no promise can hang
- [ ] Page binds all 220 both ways under the C++-seeded stub (probe 220/220 ×2, NC fails)
- [ ] Preset manager live: round trip incl. scale + masks + seed; `output_gain` untouched; absent custom state resets scale, keeps seed; two-reopen clean
- [ ] `getWindowCurve` is the engine's curve (WC1); telemetry fills the 3.1 fields, omits the rest
- [ ] All five UI gates green with overrides + 8 states
- [ ] Audio unchanged: carried diff empty, N-suites + R12 + HA PASS
- [ ] HANDS-ON-3.1.md ready for verify (UI-01 completes there)
