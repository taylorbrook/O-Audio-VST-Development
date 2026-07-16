# Stage 3: GUI — Context

## Discussion Summary

**Date:** 2026-06-27
**Participants:** User, Claude
**Stage goal (from ROADMAP):** A single-page WebView UI, signal-flow left→right, all 17 params bound and resonator-aware, plus the four pedagogical visuals (loop/flow diagram, scope, spectrum, modal stems) and tooltips — consuming the live viz taps wired in Stage 2.

## Inputs Reviewed

- **BRIEF.md** — UI Concept section: single clear page, classroom/projector-readable, left→right signal flow (Excitation → Resonator → Material/Damping → Amp/Output), prominent central viz panel; clean/instructional aesthetic consistent with O-simpleFM/O-simpleAdditive; all four pedagogical visuals + on-hover tooltips selected as first-class features.
- **parameter-spec.md** — 17 locked params (zero-drift IDs); resonator-/exciter-specific controls identified (String-only: `stringModel`; Modal-only: `inharmonicity`, `modeBrightness`; Bow-only: `bowForce`).
- **ROADMAP.md** — Stage 3 split into 3 phases (3.1 layout+binding, 3.2 scope+spectrum, 3.3 diagram+stems+tooltips).
- **Stage 2 VERIFICATION.md** — DSP verified; viz taps live and confirmed.
- **VizTap.h** — the available data surface (see below).
- **Sibling templates** — O-simpleFM (`Source/ui/public/` — full template **with** preset-manager + spectrum analyzer) and O-simpleAdditive (same template **without** preset manager).

## Available Viz Data (Stage-2 contract — `processor.getVizTap()`)

The audio thread is copy-only into pre-allocated lock-free structures (PERF-01). The Stage-3 editor Timer (message thread) reads these:

| Tap | Type | Drives |
|-----|------|--------|
| `waveform` | `VizRing`, 8192-sample lock-free ring of master output | Scope (UI-03) **+** message-thread FFT → spectrum (UI-04) |
| `loopEnergy` | atomic `float` (KS circulating-energy scalar) | Loop/flow diagram dampening (UI-02) |
| modal stems | 8 × (`stemFreq`, `stemAmp`) atomic pairs | Modal stem display (UI-05) + Modal-mode diagram skin |

> The spectrum is **computed on the message thread** from the `waveform` ring (O-simpleFM pattern) — no FFT on the audio thread.

## Requirements Confirmed (Stage-3 scope)

- **UI-01** (should) — single clear page, projector-readable, left→right signal flow.
- **UI-02** (must) — animated loop/flow diagram reflecting ACTUAL circulating energy (`loopEnergy`), visibly dampening each pass, re-skinning per resonator.
- **UI-03** (should) — live waveform + decay scope.
- **UI-04** (must) — live spectrum showing harmonics fade top-down + inharmonic spacing in Modal.
- **UI-05** (should) — modal stem display (mode freqs + amplitudes) in Modal mode.
- **UI-06** (nice) — on-hover pedagogical tooltips on every control.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **D1 — Build path** | **Build direct from brief + sibling template** (no separate ui-mockup round-trip) | Aesthetic + layout already established in the brief and proven in O-simpleFM/Additive; the only novel element (loop diagram) is iterated live during execute. Fastest path; user decides fast. |
| **D2 — Headline loop/flow diagram (UI-02)** | **Literal block diagram, animated** — the class's own EXCITATION→RESONATOR→MATERIAL boxes; energy enters, circulates a loop arrow, visibly dims each pass; re-skins to modal stems in Modal mode | Matches BRIEF's "class's own figure brought to life"; maximally pedagogical — what students see maps 1:1 to the lecture figure. Driven by the real `loopEnergy` scalar (not canned). |
| **D3 — Frequency viz (UI-04)** | **Spectrum analyzer (live)** — message-thread FFT off the `waveform` ring, reusing O-simpleFM's analyzer | Proven, low-risk, fast; shows harmonic vs inharmonic spacing clearly and harmonics dropping top-down on decay. The loop diagram already carries the "dampens over time" temporal story, so an instantaneous spectrum is sufficient. (Scrolling spectrogram considered and deferred — richer but new code; reconsider for v1.1.) |
| **D4 — Preset selector** | **Include preset bar in Stage 3** — wire the shared `preset-manager` module + `OuariconPresetManager` C++ backend now (shell + save/load/navigate); factory presets populate it in Stage 4 | Supports the brief's "preset tour" use case from the start; `persistence/preset-manager` is a registered shared module (add via `/module-add`), so cost is integration, not authoring. (O-simpleFM precedent.) |
| **D5 — Resonator-aware controls** | **Grey out (disable but keep visible)**, not hide | Pedagogical: students see the full control surface and learn which controls belong to which engine. String-only (`stringModel`) greys in Modal; Modal-only (`inharmonicity`, `modeBrightness`) grey in String; `bowForce` greys unless Excitation = Bow. Driven by `resonatorType` / `excitationType` listeners. |
| **D6 — Aesthetic** | **Reuse the established sibling visual language** (O-simpleFM/Additive clean-instructional palette + shared CSS conventions) | Consistency across the pedagogical suite; no new design system needed. |
| **D7 — Tooltips (UI-06)** | **On-hover, plain-language, one per control**; content seeded from BRIEF examples (pitch = SR ÷ delay length; feedback near one sustains; higher modes decay faster; what inharmonicity does) | Brief makes tooltips a first-class teaching feature; on-hover keeps the page uncluttered. |

## Constraints / Binding Facts (carried from earlier stages)

- **Zero param-ID drift** — bind exactly the 17 IDs in `parameter-spec.md`; Material macro UI writes `damping` + `decay` (already a message-thread APVTS write-back from Stage 2 — the two knobs must visibly co-move when Material moves).
- **WebView wiring order** — `unique_ptr` relays → WebView → attachments; 3-arg `WebSliderParameterAttachment` (O-simpleFM/MicrotonalSampler pattern).
- **JUCE WebView namespace gotcha** — pass the `Juce` ES-module namespace (not `window.__JUCE__`) to any module needing `getNativeFunction` (e.g. preset-manager). (Memory: critical_juce_webview_namespace_vs_postmessage.)
- **Binary data** — add the **single** `juce_add_binary_data` UI-resources target in Stage 3 (Stage 1 deliberately omitted it). If a second binary-data target ever appears, give it a distinct `NAMESPACE` (memory: dual_binary_data_namespace_collision).
- **Windows WebView2** — flags already set in Stage 1 (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`); set `withUserDataFolder()` to a temp dir; resource provider receives **bare paths** (memory: resource-provider-paths).
- **Render-harness vs WebView editor** — adding WebView types to `PluginEditor.cpp` breaks the offline harness (it compiles under `JUCE_WEB_BROWSER=0`). Keep `createEditor` under `#if JUCE_WEB_BROWSER` and ensure `PluginEditor.cpp` stays dropped from harness sources. Re-run the harness at the START of Stage 4 (memory/ROADMAP: O-simpleBeatmaker lesson).
- **Window size** — fixed, classroom/projector-readable. Sibling O-simpleFM is 760×980 (tall, keyboard-bearing); the left→right signal-flow + central viz layout here likely wants a **wider** canvas. Confirm exact dimensions during execute/research.

## Open Questions (for research/plan)

- Exact window dimensions + whether an on-screen keyboard is included (siblings do; confirm — note the `uiMidi` native-fn-bridge gotcha if a keyboard is added).
- Loop-diagram render tech: SVG + JS animation vs `<canvas>` (canvas needs the DPR-aware backing-store + explicit width/height fix — memory: canvas-replaced-element gotcha).
- Spectrum analyzer: how much of O-simpleFM's analyzer is liftable verbatim vs needs adaptation for the modal inharmonic-spacing display.
- Modal-stem display: separate panel vs the Modal "skin" of the loop diagram (avoid duplicating the same data in two widgets) — decide in plan.
- Timer rate (O-simpleFM uses 30 Hz) and scope downsample factor.

## Next Phase

Ready for: **research** phase (investigate O-simpleFM analyzer reuse, preset-manager module integration, loop-diagram render approach) → then plan.
