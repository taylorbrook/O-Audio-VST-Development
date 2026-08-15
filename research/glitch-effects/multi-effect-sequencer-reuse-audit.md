---
title: "Multi-Effect Glitch Sequencer — Suite Reuse Audit & O-Glitch Architecture"
created: 2026-08-14
last_verified: 2026-08-15
juce_version: "8.0.14"
summary: "Code audit of O-Polystutter v1.12.4 (plus O-ReverseDelay, O-Freeze) mapping reusable components for a dBlue Glitch 2-style multi-effect step sequencer, with a recommended O-Glitch architecture and effort estimates."
domain: dsp
type: reference
keywords:
  - o-glitch
  - o-polystutter
  - step-sequencer
  - multi-effect
  - reuse
  - capture-buffer
  - crossfade
  - tapestop
  - o-tapestop
stages: [0, 2]
agents: [dsp, research]
---

# Multi-Effect Glitch Sequencer — Suite Reuse Audit

Level 3 deliverable (2026-08-14). Line references are against O-Polystutter v1.12.4. Bottom line: **the suite already contains ~60% of a multi-effect glitch sequencer.**

## 1. O-Polystutter reuse map

### Reusable as-is (lift with minor renames)

- **Capture ring + snapshot-at-trigger (WR-05 pattern)** — `DSP/RepeatLane.cpp:96–109` (circular write), `:349–367` (trigger-time bounded two-segment snapshot copy so the live write head can't corrupt long tails), `:37–53` (5 s pre-allocated ring + snapshot). The "freeze audio at step boundary, then mangle" primitive every buffer-reading effect needs. Extract as `CaptureSnapshot`.
- **Anti-click envelope stack** (three independent mechanisms, `RepeatLane.cpp`): loop-boundary crossfade `:219–238` + `getCrossfadeGain` `:476` (linear 5 ms, clamped to effectiveLength/2); trigger fade-in / tail fade-out `:111–168`, `:265–282` (fade-out from *last output sample*, not zeros — v1.12.1 fix); **retrigger crossfade** (old output → new segment) `:284–297`, armed `:322–327` — generalizes directly to *effect-to-effect* transitions at step boundaries.
- **Tempo sync + boundary-edge triggering** — `PluginProcessor.cpp:1691–1824` (`updateBeatSync`): BPM fallback+clamp `:1710–1719`, subdivision-boundary detect via `int(ppq/subdivPPQ)` crossing `:1772–1817`, offline/no-playhead fallback `:1694–1706`, subdivision tables `:1758–1770` (dup at `RepeatLane.cpp:413–429`).
- **Euclidean generator** — Bjorklund `RepeatLane.cpp:692+` (static, dependency-free; wrap-at-steps `:531–570`). Copy verbatim (v1.12.3 fixed leftover accounting — use the fixed one).
- **Varispeed/pitch read loop** — `RepeatLane.cpp:183–217`: fractional position, pitch-ratio read, linear interp, reverse via `captureLength − 1 − pos·ratio` `:198–200`. Already implements retrigger + reverser + pitch; **tapestop = this with ratio ramping 1→0**.
- **Swing / probability / decay-per-repeat** — `calculateSwingOffset` `:571+`, probability gate `:318–320`, per-repeat gain decay + pitch re-randomization `startNewRepeat` `:431–476`.
- **TapeDegrader post-chain** — `DSP/TapeDegrader.cpp:105+`: gain-compensated tanh `:114–137`, wow/flutter via modulated 5-sample DelayLine `:139–176` (≈80% of a tapestop module), hiss `:179–195`, cached-coeff rolloff LPF, dropout with 5 ms fades. Self-contained; drop in as master lo-fi section.
- **MIDI trigger routing** — `DSP/TriggerRouter.cpp:65–83` (notes 60–63 → lanes, 67 → all); consumed `PluginProcessor.cpp:1461–1507`; MIDI-vs-beat-sync exclusion `:1739–1749`. Retarget note → pattern/scene select.
- **UI progress bridge** — atomics `PluginProcessor.cpp:1647–1668` → timer + `emitEventIfBrowserIsVisible("laneProgress",…)` `PluginEditor.cpp:980–1022` → JS listener `parameter-bindings.js:1004–1010`. The playhead/step indicator pattern.
- **Preset layer** — `OPolystutterPresetManager.h` + `ui/public/modules/preset-manager.js` + SafePointer-hardened FileChooser native fns `PluginEditor.cpp:410–523`.

### Adaptable (right idea, wrong shape)

- **Step model**: 16 bool steps/lane (`RepeatLane.h:172–176`; params `pattern_lane{N}_step{1..16}` `PluginProcessor.cpp:676–706`) is a *gate*, not a *program*. O-Glitch needs per-step effect ID + intensity (Choice/Int per step). PPQ→step math, wrap, and grid UI (`index.html:1351+`, toggles `parameter-bindings.js:926`) carry over.
- **Lane mixing**: parallel lanes fed from a saved dry copy, wet-split by active count (`PluginProcessor.cpp:1529–1627`). O-Glitch is serial-per-step → mixer becomes an effect-slot output selector with crossfade. Keep the dry-copy + pre-allocated scratch discipline (`:1169`).
- **Param layout**: ~700 lines of 4x-duplicated lane blocks (`createParameterLayout` `:33+`) + ~80 hand-named relays (`PluginEditor.h:58+`). Keep the conventions (versioned ParameterID, cached raw pointers `:734–792`, prefix pattern `:559`); generate per-slot/per-step params in loops instead of copy-paste.

### Not reusable / warnings

- **`lane{N}_filter` params are shipped no-ops** (declared `:78/203/328/453`, relayed, never read; no filter in RepeatLane) — bug filed 2026-08-14, /improve in progress (implement per-lane LPF, → v1.13.0).
- The 4-discrete-member lane plumbing (`PluginProcessor.h:91–94`, 4x update blocks `:1292–1459`) — architecture to avoid, not port.

### Missing entirely (new work)

1. Per-step effect-slot routing (StepProgram model + slot switcher with boundary crossfade).
2. Bitcrusher, gater, dedicated tapestop modules (reverser/pitch fall out of existing read loop).
3. Pattern/scene banks + MIDI scene switching + preset schema support.
4. Per-step intensity smoothing (`LinearSmoothedValue` ramps — O-Polystutter applies params per-block, unsmoothed).

## 2. Sibling-plugin reusables

**O-ReverseDelay** (`plugins/O-ReverseDelay/Source/dsp/`) — stronger DSP substrate than RepeatLane:
- `CaptureBuffer.h:47+` — absolute-index stereo capture ring (`readStartAbs = totalWritten − D`, no wrap ambiguity). Prefer this ring.
- `ReverseGrain.h:78+` — POD grain with **direction field** (`:139`), equal-power pan, spectral tilt, `1/sqrt(overlap)` gain; `GrainPool` `:181+` fixed-array RT-safe pool.
- `GrainScheduler.h:79+` — sample-counting spawn scheduler with SpawnRequest/Result contracts.
- `WindowLut.h` — window LUTs incl. variable-α Tukey-as-Hann-remap.
→ Right foundation for **reverser and tapestop** (windowed grains with direction/speed beat naked buffer reads). Also the core for a standalone **O-Tapestop**.

**O-Freeze** (`plugins/O-Freeze/Source/PluginProcessor.h:67–123`) — compact freeze engine (GateState, fixed-base grains, freezeBuffer, smoothed gain, drift locked at engage). Extractable as a **freeze** step effect — a 7th slot dBlue doesn't have.

## 3. Recommended O-Glitch architecture

**Signal-flow decision: always-warm effect modules with per-slot output gains, crossfaded ~5 ms at step boundaries** (generalizing the retrigger-xfade `RepeatLane.cpp:284–297`). Not on-demand instantiation — that's where O-Polystutter accumulated three generations of click fixes (v1.1.1/1.2.1/1.12.1). Warm modules also let tails overlap steps (tapestop ringing into the next step). CPU is fine: all effects are cheap; gain-gate processing (skip when slot gain 0 AND tail decayed), don't lifecycle-gate. Snapshot taken **once per step by the host**; all buffer-reading effects share it.

| Module | Source | Effort |
|---|---|---|
| CaptureRing + StepSnapshot | O-ReverseDelay CaptureBuffer + WR-05 copy | 0.5 d |
| StepClock (ppq→step, swing, Euclidean, banks) | updateBeatSync + pattern pos + Bjorklund | 1–2 d |
| StepProgram (per-step effect ID + intensity × N patterns) | new (prefix-loop param scheme) | 1–2 d |
| FX: Stutter/Retrigger | RepeatLane core | 1 d |
| FX: Reverser | O-ReverseDelay grain read | 1 d |
| FX: Pitch | varispeed read, ratio ≠ 1 | 0.5 d |
| FX: Tapestop | varispeed + ratio ramp (curve param) | 1 d |
| FX: Bitcrusher | new (see degradation-dsp-deep-dive.md) | 0.5 d |
| FX: Gater | new (WindowLut shapes) | 0.5 d |
| FX: Freeze (bonus slot) | O-Freeze extraction | 1 d |
| SlotMixer (routing + boundary xfade + tails) | retrigger-xfade generalized | 1–1.5 d |
| MIDI scenes | TriggerRouter retargeted | 0.5 d |
| Post chain | TapeDegrader verbatim | 0.5 d |
| WebView UI (effect-color-coded step grid, playhead) | index.html sequencer + progress bridge, heavy rework | 3–4 d |
| Preset/state incl. banks | PresetManager + schema extension | 1 d |

**Total ≈ 14–17 dev-days.** DSP is mostly assembly of proven parts; the genuinely new work is StepProgram, SlotMixer, and the UI (~half the total).

## Key reference files

- `plugins/O-Polystutter/Source/DSP/RepeatLane.cpp` (capture/snapshot/xfade/varispeed)
- `plugins/O-Polystutter/Source/PluginProcessor.cpp:1691` (updateBeatSync)
- `plugins/O-Polystutter/Source/DSP/TapeDegrader.cpp:105`
- `plugins/O-ReverseDelay/Source/dsp/{CaptureBuffer,ReverseGrain,GrainScheduler,WindowLut}.h`
- `plugins/O-Freeze/Source/PluginProcessor.h:67`
- Tape-start resync (for O-Tapestop): Signalsmith fall-behind → accelerate → crossfade-skip (KVR t=538470); x² deceleration curve for stop.
