# Stage 1 (Foundation) — VERIFICATION

**Plugin:** O-simpleGrain · **Stage:** 1 Foundation · **Date:** 2026-06-24
**Verdict:** ✅ **PASS** — silent synth shell builds, validates, and exposes the full 18-param APVTS with state persistence.

## Goal-backward check

**Stage goal:** A silent, valid 8-voice synth shell (VST3 + AU + Standalone) on macOS, full 18-param APVTS, state persistence (incl. loaded-source identity), correct cross-platform WebView/CMake config. No audio (Stage 2), no WebView UI (Stage 3).

| # | Success criterion | Result | Evidence |
|---|-------------------|--------|----------|
| 1 | Builds clean (VST3 + AU + Standalone) | ✅ PASS | `ninja O-simpleGrain_VST3 O-simpleGrain_AU O-simpleGrain_Standalone` — 64 steps, 0 errors, all three linked. |
| 2 | Auto-discovered by root CMake | ✅ PASS | `cmake -B build` configured O-simpleGrain (glob discovery; no manual registration). |
| 3 | All 18 params present, correct | ✅ PASS | `auval` → **"18 Global Scope Parameters"**, every `-parameter PASS`. IDs/ranges/defaults per parameter-spec.md. |
| 4 | AU validates structurally | ✅ PASS | `auval -v aumu OsGr OuDv` → **"AU VALIDATION SUCCEEDED."** Registered as `aumu OsGr OuDv` (instrument). |
| 5 | State round-trips (params + source identity) | ✅ PASS | APVTS XML + custom `"SOURCE"`/`"identity"` child (`get/setStateInformation`); auval state/property checks PASS. Default `"embedded:fire"` preserved when child absent. |
| 6 | Loads as instrument, accepts MIDI, silent, no crash | ✅ PASS | `aumu` (music device) registered + validated; `processBlock` clears buffer + drains MIDI. Builds as Standalone instrument. |
| 7 | `processBlock` allocation-free | ✅ PASS | Silent path: `ScopedNoDenormals` + `buffer.clear()` + `ignoreUnused(midi)` — no heap ops, no locks. |

## Technical validation (auval)

```
VALIDATING AUDIO UNIT: 'aumu' - 'OsGr' - 'OuDv'
PUBLISHED PARAMETER INFO:  # # # 18 Global Scope Parameters  → all -parameter PASS
--------------------------------------------------
AU VALIDATION SUCCEEDED.
--------------------------------------------------
```

## Self-review (foundation critic, inline)

- **Contract fidelity:** 18/18 params with exact IDs; no drift, no extras. `freeze` bool, `sourceSample`/`windowShape` choices (Hann default idx 4) correct. ✅
- **JUCE 8 gotchas honored:** `getLatencySamples()` NOT overridden; `setLatencySamples(0)` in prepare; synth-only bus; both WebView2 flags present for Stage 3 inheritance. ✅
- **Scope discipline:** no DSP, no WebView, no `juce_add_binary_data`/`ouaricon_add_module`, no invented `.wav` blobs — deferrals marked with comments (`# TODO(Stage 2.3)` + Stage 3 UI marker). ✅
- **Reuse:** faithful structural mirror of shipped O-simpleFM Stage-1 shell. ✅

## Notes / accepted items

- `outputLevel` uses a −60 dB floor as the "−inf" endpoint (standard, within spec).
- `ampSustain` stored 0–1 (default 0.8); UI scales ×100 in Stage 3 (matches spec "0–100% (0–1)").
- Bundle-ID-contains-spaces CMake warning is **suite-wide and pre-existing** (O-simpleFM shares it); benign, not introduced here.
- VST3 ad-hoc signature replacement is JUCE's standard local-build behavior; benign.
- Mockup deferred to Stage 3 per user decision — APVTS built against research-locked spec; minor reconciliation risk tracked in parameter-spec.md gate_note.

## Installed artifacts (for manual DAW check)
- `~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3`
- `~/Library/Audio/Plug-Ins/Components/O-simpleGrain-dev.component`
