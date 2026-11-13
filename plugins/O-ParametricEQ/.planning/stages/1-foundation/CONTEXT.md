# O-ParametricEQ — Stage 1 (Foundation) — CONTEXT

**Date:** 2026-11-12
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:** `parameter-spec.md` (196 parameters, not the draft), `BRIEF.md`, `REQUIREMENTS.md`, `research/ARCHITECTURE.md` ("Parameter Mapping", "State Persistence"), `ROADMAP.md` § Stage 1, `mockups/v1-CMakeLists-SNIPPET.txt`, `mockups/v1-integration-checklist.md`

## Goal

A buildable plugin shell (VST3 + AU + Standalone) with the frozen 196-parameter layout and the v1 state format. Audio passes through unchanged. pluginval strictness 10 and `auval` pass (COMPAT-01); the layout and state gate G11 passes (DSP-07).

## Decisions

### D1 — Band IDs are zero-padded (user, this session)
`b01_freq` … `b24_freq`, as `parameter-spec.md` already has them. Host-visible names stay unpadded (`B1 Freq` … `B24 Freq`). Closes Stage 0 "Needs the user's call" item 6. IDs are frozen from this stage on: never renamed, never removed, ranges and skews never changed.

### D2 — Analyzer mode is UI state (user, this session)
Not a host parameter. The count is 196. The mode is stored in the state's `UI` child. Closes Stage 0 item 3.

### D3 — The six DSP calls move to Stage 2 discuss (user, this session)
Stage 0 items 1 (slope on Bell / Notch / Band Pass), 2 (v1 accuracy level), 4 (tilt convention), 5 (Q 1.0 = Butterworth on cuts and shelves), 7 (above-Nyquist behaviour), 8 (listen filter) do not touch the parameter layout. They are raised in `/plugin-discuss O-ParametricEQ` for Stage 2 with the Stage 0 recommendations as defaults. Stage 1 must not encode an answer to any of them. `bNN_slope` is registered for every band regardless of type.

### D4 — Shell editor (Claude, by suite precedent; not asked)
`createEditor()` returns `juce::GenericAudioProcessorEditor` until Phase 3.1. The WebView build plumbing (`NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, the module lines) is wired now so Stage 3 is an editor swap. The UI files stay in `mockups/`; nothing is copied into `Source/ui/` in Stage 1. A half-landed UI tree with a comment-only editor fails repo `check-i18n` rule 8. ROADMAP's alternative (a placeholder editor that carries the resource map) is acceptable only if the planner finds a gate that needs a WebView editor in Stage 1.

### D5 — Identity (Claude, by O-AnalogEQ pattern; not asked)
Target `OuariconParametricEQ`, `PLUGIN_CODE OuPQ` (unused in the repo on 2026-11-12; re-check at plan time), `VERSION 0.1.0`, `FORMATS VST3 AU Standalone`, `IS_SYNTH FALSE`, no MIDI.

## Requirements carried from ROADMAP / ARCHITECTURE / spec

- **CMake** on the O-AnalogEQ pattern; modules `preset-manager` 1.1.1, `eb-garamond` 1.0.0, `vu-meter` 1.0.0. No comment in the file may contain the JUCE plugin-declaration command name followed by an opening parenthesis (`scripts/resolve-target.sh` greps for it).
- **Parameters:** 196, registered in the spec's order: group `global` (`bypass`, `output_gain`, `gain_scale`, `auto_gain`), then groups `band01` … `band24` (`enabled`, `bypass`, `type`, `freq`, `gain`, `q`, `slope`, `placement`).
  - `juce::ParameterID { id, 1 }` on every parameter.
  - No lambda `NormalisableRange`. `setSkewForCentre (1000)` on frequency, `setSkewForCentre (1.0)` on Q.
  - Interval 0 on every float range.
  - The 72 reserved dynamic-EQ IDs (`bNN_dyn_range`, `bNN_dyn_threshold`, `bNN_dyn_auto`, version hint 2) are not registered and are listed in a source comment next to the layout.
- **Processor:** `getBypassParameter()` returns `bypass`; `isBusesLayoutSupported` accepts mono→mono and stereo→stereo only (`(2,1)` and `(1,2)` refused); `setLatencySamples (0)` in `prepareToPlay()`; `ScopedNoDenormals`; pass-through audio.
- **State:** `copyState()` → XML → `copyXmlToBinary`; root attributes `stateVersion="1"` and `pluginVersion`; a `UI` child (analyzer mode, UI language); tolerant restore (unknown parameter ignored, missing parameter defaults, `stateVersion="2"` loads without error). Values read back from XML are parsed by type.
- **No parameter is written from another parameter's listener**, including in `setStateInformation` and preset apply.
- **Gate G11:** `param-dump` target and `tests/golden/params.tsv` (ID, index, version hint, range, skew, default, automatable), committed and diff-clean; exact state round trip; the three foreign-state cases.
- The first-order `juce::dsp::IIR::Filter` default constructor is not used anywhere.

## Out of scope for Stage 1

- Any DSP (designer, bands, router, output stage, analyzer): Stage 2.
- WebView editor, relays, UI resources: Stage 3.
- Presets and factory content: Stage 4.

## Open for the planner

- Whether a suite `param-dump` tool exists to reuse or the target is plugin-local (ARCHITECTURE.md calls it "the suite `param-dump` tool").
- Whether `vu-meter` 1.0.0 (no consumer in the registry) configures cleanly through `ouaricon_add_module` before Stage 3 needs it.
