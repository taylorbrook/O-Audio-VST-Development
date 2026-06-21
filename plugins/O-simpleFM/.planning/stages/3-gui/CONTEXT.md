# Stage 3 (GUI) — CONTEXT

> **Source:** Auto-generated (express mode) from BRIEF.md UI Concept, ROADMAP Stage 3 phases,
> parameter-spec-draft.md, and Stage 2 VERIFICATION carryover hooks — folded with one user
> decision captured at the Stage 2→3 handoff (2026-06-20).

## Decision captured at handoff

**GUI path: DIRECT INTEGRATION (no separate `/design-ui` mockup-iteration loop).**
The gui-agent builds the WebView UI directly during execute.

**Aesthetic: Ouaricon Audio Naturalist** (`.claude/aesthetics/ouaricon-naturalist-001/`) — the
official botanical brand system. Aged-paper background, Garamond serif, seed cross-section knobs,
green botanical toggles, one botanical overlay (transparent PNG, right side, ~0.35 opacity),
fleuron accents, wide letter-spacing labels.

## Goal

Replace the `GenericAudioProcessorEditor` placeholder with a single-page WebView UI that:
1. Binds all 17 APVTS parameters two-way (drag→DSP and host-automation→UI).
2. Renders the two headline teaching visuals (live FFT spectrum + oscilloscope) from the
   already-running message-thread analyzer.
3. Adds the pedagogical layer: live operator-routing diagram, per-parameter hover tooltips,
   and an educational preset tour.
4. Works cross-platform (macOS VST3+AU, Windows VST3 — no blank UI).

## Parameter inventory (17) → control mapping

**Knobs — seed cross-section (15 float):**
`ratio`, `modIndex`, `feedback`, `modFixedHz`, `modEnvToIndex`, `velToIndex`,
`modAttack`, `modDecay`, `modSustain`, `modRelease`,
`ampAttack`, `ampDecay`, `ampSustain`, `ampRelease`, `outputLevel`

**Toggles — green botanical (2 bool):** `ratioSnap`, `modFixedMode` (Ratio ↔ Fixed)

> IDs are the single source of truth in `Source/PluginProcessor.h` → `OSimpleFM::ParamIDs`.
> Relays/attachments MUST use these exact strings.

## Layout (single page, classroom-readable)

- **Visualization panel (prominent):** spectrum analyzer (top) + oscilloscope (below) — the
  headline. Discrete sidebands must be clearly separable.
- **Operators group:** Ratio + Ratio Snap (toggle) · Mod Index · Feedback · Mod Fixed Mode
  (toggle) + Mod Fixed Hz · Mod Env→Index · Vel→Index
- **Modulator Envelope group:** Attack / Decay / Sustain / Release
- **Amplitude Envelope group:** Attack / Decay / Sustain / Release
- **Output group:** Output Level
- **Routing diagram:** MOD → CAR with self-feedback loop, reflecting feedback/ratio state.
- **Tooltips:** every parameter, plain-language ("Ratio 2:1 → octave-related harmonics;
  integer = harmonic, irrational = bell-like").
- **Preset tour:** named concept-isolating patches (E-Piano, Tubular Bell, Brass, Clarinet,
  Clang Bell).

## Stage 2 carryover hooks (already in place — do NOT rebuild)

- `OSimpleFMAudioProcessor::getAPVTS()` → bind relays/attachments.
- `getVizRing()` + `getCurrentSampleRate()` → analyzer input (audio-thread copy-only ring).
- Editor is ALREADY a `juce::Timer` at 30 Hz; `timerCallback()` ALREADY calls
  `vizAnalyzer.process(...)`. Stage 3 adds the WebView + the two `emitEventIfBrowserIsVisible`
  calls right after `process()`.
- `FmVizAnalyzer::getSpectrum()` → 256 log-frequency dB bins, range ≈ [-100, 0] dB
  (rise-fast/fall-slow smoothed). `getScope()` → 128 points, [-1, 1], max-abs sign-preserving.
- WebView2 CMake flags (`NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`,
  `JUCE_WEB_BROWSER=1`) ALREADY set at Foundation — only the binary-data + editor wiring is new.
- Render harness (`-DOUARICON_BUILD_TESTS=ON`) is a permanent DSP regression gate — must stay green.

## Constraints / known gotchas (from project memory)

- **JS namespace:** pass `Juce` (ES-module, `import * as Juce from './js/juce/index.js'`) to anything
  needing `getNativeFunction`/`getSliderState`; `window.__JUCE__` has NO `getNativeFunction`. Use
  `window.__JUCE__.backend.addEventListener(...)` for the spectrum/scope events.
- **Resource provider receives BARE PATHS** (`/`, `/index.html`, `/js/app.js`) — compare by direct
  equality; do NOT strip scheme/host.
- **Member declaration order:** relays → WebView → attachments (reverse-destruction safety).
- **Canvas:** DPR-aware backing store (`canvas.width = clientWidth*dpr`, `ctx.setTransform(dpr,…)`);
  size with explicit `width/height` (replaced-element gotcha), not `right/bottom`.
- **Windows:** `withUserDataFolder(tempDir)` or WebView2 is denied in DAW hosts → blank.
- **Module-load failures are silent** to C++ build/auval — a JS ReferenceError kills the whole UI;
  verify all helper refs after writing.

## Out of scope (v1.0)

Non-sine operator waveforms, fine detune, master tune, LFO/vibrato, A/B compare. (Deferred to v1.1.)

## Success criteria (goal-backward)

A curious student reaches "oh, THAT's how FM works" in 5 min: turn Mod Index → sidebands bloom in
the spectrum; change Ratio 1:1→2:1→1.41:1 → harmonic↔inharmonic snap + scope morphs; Mod Env→Index
makes the timbre evolve; Feedback smears toward saw/noise. All visible live, every control annotated,
routing always visible.
