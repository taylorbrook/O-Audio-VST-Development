# Stage 4 (Polish / Validation) — CONTEXT

**Plugin:** O-ReverseDelay
**Stage:** 4 of 4 (Polish / Validation)
**Date:** 2026-07-24
**Mode:** manual (interactive discuss)
**Source:** Interactive discuss session (6 decisions locked, D11–D16) + ROADMAP.md Stage 4 baseline + Stage 3 VERIFICATION.md carried items.

---

## Goal

Close out O-ReverseDelay at ship-ready **v1.0.0**: add the preset system (module + 8 factory presets + UI bar), add tooltips, re-confirm the full validation surface, and author the CHANGELOG. Unlike a pure validation stage, this one ships real UI and persistence code — so the just-verified Stage-3 GUI is a **regression surface**, not a frozen asset.

---

## Entry Gate (must clear before any Stage-4 code)

1. **D7/D6 Standalone audition — REQUIRED, currently OPEN.** Per D11 the audition happens *before* the DSP decision, not after. `/show-standalone O-ReverseDelay` → judge smear, wash length, width by ear.
2. **Render-harness re-run at entry** — `pattern_render_harness_breaks_on_webview_editor`. Build with `-DOUARICON_BUILD_TESTS=ON` and confirm 33/33 before touching anything. Establishes the pre-change baseline for the preset/tooltip diff.

---

## Locked Decisions (this discuss)

### D11 — Wash decay: audition first, then decide

Stage 2 measured an inherent **~−7.3 dB per feedback generation** at `feedback=100` pre-damping (Hann² duty cycle + pan→mono-sum round trip), so the wash decays rather than self-sustains. The fix, if wanted, is a **single makeup constant at the feedback tap**.

**Decision:** do NOT pre-authorize the makeup constant. The user auditions in Standalone and reports; the DSP edit is evidence-based or does not happen.

- If the audition says the wash is too short → makeup constant becomes Stage-4 task #1, gated on re-running the harness **and** probe G (60 s @ fb=100, peak below ceiling, zero NaN/Inf — DSP-03).
- If the audition passes → zero DSP diff; Stage 4 is preset/UI/validation only.
- Either way this closes the last open item carried from Stage 2.

### D12 — Preset scope: full OuariconPresetManager + factory presets

Adopt `modules/persistence/preset-manager` **v1.0.5** (note: newer than the v1.0.4 recorded in project memory; `module.yaml` is authoritative, `registry.yaml` is stale). CMake-include consumer pattern, per O-Bowed / O-Wind:

- `target_include_directories(... ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp)`
- `modules/persistence/preset-manager/js/preset-manager.js` added to the **existing** `OuariconReverseDelay_UIResources` binary-data target (see constraint C3 — do not create a second target)
- 9 native functions: `savePreset`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset` — joining the existing `getParameterDefaults`, taking the editor from 1 → 10 native fns
- User presets land in `~/Library/Application Support/O-ReverseDelay/Presets/`

### D13 — Extra scope: tooltips only

**In:** hover tooltips on all 10 controls (the UI currently has zero `title`/tooltip markup).
**Out:** CPU measurement pass. **Out:** local Windows validation — deferred to publish/CI, exactly as the suite convention. The CMake already carries `NEEDS_WEBVIEW2` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, verified statically in Stage 3.
**Otherwise:** no new code unless validation surfaces a defect.

### D14 — Release target: ship-ready v1.0.0, publish separately

Stage 4 ends with CHANGELOG authored, `VERSION 1.0.0` confirmed in CMakeLists, a fresh dual-variant-swept install, and all validation green. `/publish` (GitHub Actions cross-platform release) stays a **separate later step** the user triggers. Windows pluginval-10 fuzz is therefore not a Stage-4 blocker.

### D15 — Preset bar placement: grow to 940×484, bar under the header

A new **44 px** band between `<header class="header">` and `<main class="groups">`. The four group panels and the footer hint line stay untouched.

```
┌───────────── 940 ─────────────┐
│   O–ReverseDelay              │
│   Granular · A Field Guide    │
├───────────────────────────────┤  ◄ NEW 44px preset bar
│ ◀  Reverse Bloom  ▶   ⤓  ⤑    │
├───────────────────────────────┤
│ TIME  │ GRAIN │ FEEDBACK │ OUT │
│  ○    │ ○  ○  │ ○  ○  ○  │ ○ ○ │
├───────────────────────────────┤
│ ❧ drag · wheel · dblclick ❧   │
└───────────────────────────────┘  height 440 → 484
```

Chosen for lowest regression risk on the verified UI-02 slot geometry. Requires `PluginEditor.cpp:168` `setSize(940, 440)` → `setSize(940, 484)`, and the Stage-3 verify measurement `.time-slot` `x:117 y:198 w:86 h:100` will legitimately shift **y by +44** — the re-verify assertion is *identical box across Sync/Free modes*, not the old absolute y.

### D16 — Factory preset set: 8 presets across the use-case span

| # | Name | Character | Covers |
|---|------|-----------|--------|
| 1 | Reverse Bloom | Default-adjacent, balanced smear | Baseline |
| 2 | Guitar Swell | Volume-swell texture without technique | BRIEF use case 2 |
| 3 | Vocal Halo | Reversed pre-echo wash behind a lead | BRIEF use case 3 |
| 4 | Slow Wash | Long grains, low density, long tail | BRIEF use case 1 |
| 5 | Tight Smear | Short grains, high density, choppier | Contrast pole |
| 6 | Dark Cavern | Heavy in-loop damping | Damping showcase |
| 7 | Near-Infinite | High feedback + heavy damping | BRIEF use case 4 + **doubles as a preset-driven DSP-03 stability check** |
| 8 | Rhythmic Reverse | Sync mode, 1/8 dotted | Only Sync-mode preset (FUNC-02 / COMPAT-02 coverage) |

---

## Constraints / Gotchas (must hold)

**C1 — Factory presets in engineering units + `convertTo0to1`.**
Four params are skewed (`delayTime`, `grainSize`, `lowCut`, `highCut`). A hand-written normalised fraction recalls 10–30× wrong. Reference midpoints from Stage-3 verify: at normalised 0.5 → delayTime **316 ms**, grainSize **158 ms**, lowCut **200 Hz**, highCut **3162 Hz**. (`pattern_factory_preset_normalized_ignores_skew`)

**C2 — `applyPresetJson` must reset ALL params to defaults before applying.**
Otherwise a partial/hand-authored preset silently inherits stale state for omitted keys. (`pattern_preset_apply_needs_reset_to_defaults`)

**C3 — Do NOT create a second `juce_add_binary_data` target.**
`NAMESPACE BinaryData` would duplicate-symbol against the existing `UIBinaryData`. Add `preset-manager.js` to the existing `OuariconReverseDelay_UIResources` SOURCES list. (`critical_dual_binary_data_namespace_collision`)

**C4 — Native-fn bridge gaps fail silently.**
Going 1 → 10 native fns is exactly where an unregistered fn passes build/auval/pluginval while the control is dead. The Stage-3 grep-diff gate (`getNativeFunction` in `app.js` ≡ `withNativeFunction` in `PluginEditor.cpp`) must be re-run and must read **10 ≡ 10**. (`pattern_webview_native_fn_bridge_gap`)

**C5 — Tooltips shrink-to-fit at the viewport edge.**
Pin tooltip width in px *before* placing, or the right-most control (`mix`, in the OUTPUT panel) re-wraps a 230px tip into a ~70px ribbon. Invisible to build/auval/pluginval. (`pattern_fixed_tooltip_shrink_to_fit_edge`)

**C6 — HTML-authored labels must survive JS binding.**
The FREE/SYNC segment text and all `.knob-label` text are authored in HTML and must never be overwritten by a shared state updater. The preset bar's own name field is the new risk surface. (`pattern_js_state_updater_overwrites_html_labels`)

**C7 — TDZ discipline in `app.js`.**
Adding preset-bar init at the top of the module while its state lives lower down throws a ReferenceError that escapes module evaluation and silently kills *every* later initializer — including the currently-working knobs. Build/auval/static-check all pass. (`pattern_module_toplevel_init_tdz`)

**C8 — No "/" in any preset name.**
The manager uses the name verbatim as the JSON filename; "/" is a path separator and the save is silently dropped. Preset #8 must be **"Rhythmic Reverse"**, not "Reverse 1/8". (`critical_preset_name_slash_path_separator`)

**C9 — Any async FileChooser completion (`loadPresetFromFile`) needs a SafePointer,** and on the null path must bail with a bare `return` — never `complete(false)`, which is itself a UAF. (`pattern_webview_launchasync_safepointer_no_complete`)

**C10 — Cache-clear / dual-variant discipline on every install.**
Use `./scripts/build-and-install.sh O-ReverseDelay` — its Phase 4 sweeps both `-dev` and unsuffixed bundles. AU triple is `aufx ORvD OuDv`.

---

## Validation Baseline (ROADMAP Stage 4 + carried)

| Item | Method | Automatable? |
|------|--------|--------------|
| Render harness re-run (entry **and** exit) | `-DOUARICON_BUILD_TESTS=ON`, 33 probes, hard exit code | ✅ yes |
| `ui_frontend_check.js` re-run | `node tests/ui_frontend_check.js` — 45 assertions, extend for preset bar + tooltips | ✅ yes |
| Native-fn grep-diff | `getNativeFunction` ≡ `withNativeFunction`, expect 10 ≡ 10 | ✅ yes |
| Browser-stub render | Stub server + `tests/ui-stub/juce-stub.js`; expect exactly **940×484**, zero overflow, zero JS console errors. Stub must gain the 9 preset native fns or the bar is untestable | ✅ yes |
| Factory preset audit | Render each of the 8 through the harness; assert no NaN/denormal/out-of-range and that skewed params land on their authored engineering values | ✅ yes |
| pluginval strictness 10 **×3** — VST3 | The ×3 repeat gate is Stage 4's | ✅ yes |
| pluginval strictness 10 **×3** — AU | Same | ✅ yes |
| `auval -v aufx ORvD OuDv` | AU validation | ✅ yes |
| CHANGELOG + version confirm | v1.0.0 authored | ✅ yes |
| D7 Standalone audition | Human, **entry gate** | ❌ human |
| DAW listen checklist | Human, batched at the end | ❌ human |

---

## Human Checklist (batched at the END of Stage 4)

Automated first, human listen last and in one pass — carrying the Stage-1/Stage-3 deferrals forward:

1. **D7 audition** (also the entry gate — done first, re-confirmed after any makeup-constant edit): smear / wash length / width by ear.
2. Load in Logic **and** Ableton: WebView renders at 940×484, automation round-trips in-host.
3. Mono→stereo listen (Stage-1 carryover; probe measured Δ0.0000 dB but never heard).
4. Session save/reload round-trip (Stage-1 carryover).
5. All 8 factory presets audibly distinct and none produce runaway/NaN — especially **Near-Infinite**.
6. Save a user preset, reload it, delete it — round-trip through the new bar.
7. All 10 tooltips appear on hover, and the right-most (`mix`) tooltip is **not** shrink-wrapped.

---

## Open Questions (for research/plan)

1. **Makeup-constant value**, only if D11's audition calls for it — magnitude derived from the −7.3 dB/gen measurement, placed at the feedback tap, verified by probe G rather than picked by ear alone.
2. **Preset bar visual treatment** in the Naturalist idiom — the aged-paper/seed-cross-section vocabulary has no established bar component; O-AnalogEQ and O-Bass have `.preset-bar` implementations to borrow *structure* from, but not styling.
3. **Tooltip copy** for all 10 controls — one line each, in the field-guide register of the existing footer hint.
4. Whether the 8 preset renders belong in the existing harness binary or a separate small preset-audit pass.

---

## Success Criteria (Stage 4 complete when)

1. D7 audition performed; the makeup-constant question is **closed** (either implemented + probe-G green, or explicitly declined and recorded).
2. Render harness green at entry **and** exit: 33/33, exit 0 (plus probe G if DSP changed).
3. OuariconPresetManager v1.0.5 integrated; 8 factory presets authored in engineering units + `convertTo0to1`; all 8 audited clean.
4. Preset bar shipped at 940×484; `ui_frontend_check.js` extended and passing; native-fn grep-diff reads 10 ≡ 10.
5. Tooltips on all 10 controls, edge-safe.
6. pluginval strictness 10 **×3** on VST3 **and** AU, zero failures; `auval` SUCCEEDED.
7. Fresh build installed via `build-and-install.sh` with dual-variant sweep; AU cache cleared.
8. CHANGELOG authored at v1.0.0.
9. Human checklist (7 items) handed to the user — Stage-4 verify records these as `human_needed` until confirmed.
10. Windows explicitly marked deferred-to-CI; publish is a separate user-triggered step.

---

## Next Phase

Ready for: **research** phase
