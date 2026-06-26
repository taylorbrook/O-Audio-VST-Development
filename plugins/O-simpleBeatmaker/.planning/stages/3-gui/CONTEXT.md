# Stage 3 (GUI) — CONTEXT

> **Source:** Auto-compiled (express mode) from BRIEF.md (UI Concept), ROADMAP.md
> (Stage 3 phase breakdown), parameter-spec.md (the 42-param APVTS contract), and
> the Stage 2 VERIFICATION (the lock-free VizAnalyzer / playhead tap the UI drains).
> Date: 2026-06-25.

## Goal

Replace the Stage-1 `GenericAudioProcessorEditor` shell with the single-page,
projector-readable WebView teaching UI. The headline is the **6×16 step grid with
a live playhead**; the standout teaching visual is the **timing/groove lane** that
shows each hit's *applied* Δt (early/late off its grid line) so swing / humanize /
quantize become things you watch, not just hear. A **live MIDI readout** prints the
note-ons as steps fire ("the sequencer emits MIDI" made visible). Every control
carries a plain-language tooltip. All 42 APVTS params bound two-way.

## Fixed by upstream contracts (do not relitigate)

- **42 APVTS params** (parameter-spec.md): 29 float sliders, 1 choice (`patternLength`
  8/16/32), 12 bool toggles (per-voice Mute/Solo). IDs are the single source of truth
  in `BeatmakerIDs.h::ParamIDs` + `voiceParamID()`.
- **Step grid is NOT APVTS** — `std::atomic<uint8_t>[6×32]`, written by the UI through
  native functions on the message thread, read by the audio thread. Persisted in the
  `PATTERN` ValueTree child. UI reads current cells back via a `getGrid` native fn.
- **QUAL-02 by construction** (Stage 2): the lane reads the `appliedSampleInBar −
  nominalSampleInBar` baked into each `VizEvent` at emit time — never a UI-side
  recomputation of the swing/humanize formula.
- **Viz tap** (`VizAnalyzer`): message-thread `drain(VizEvent*, max)` + continuous
  `getPlayheadStepPhase()` (fractional step index in `[0, patternLength)`).

## Decisions taken at this stage

1. **Voice rows (top→bottom) = grid row order = GM order:** Kick, Snare, Clap,
   Closed Hat, Open Hat, Tom (BeatmakerIDs `kVoiceName`).
2. **Cell interaction:** left-click cycles `off → normal(100) → accent(127) →
   ghost(40) → off` (the "click-again-to-accent, generalised to velocity" from the
   brief). Right-click erases. Velocity shown by cell fill height + brightness.
   Driven by a single `setStep(voice, step, velocity)` native fn (JS owns the cycle).
3. **Grid columns = active `patternLength`** (8/16/32). Changing the selector
   re-renders columns; off-length columns retain their state in C++ (round-trips).
4. **Timing lane is tempo-normalised:** Δt rendered as a fraction of one 16th-note
   step so the picture is stable across tempos. Needs sample-rate + current BPM — add
   two tiny read-only getters to the processor (`getCurrentSampleRate`,
   `getLastKnownBpm`); BPM is stored once/block as a relaxed atomic (no audio-path cost).
5. **Aesthetic:** Ouaricon "simple-family" field-guide — parchment ground, Garamond
   serif, amber `#e8b04a` accent, per-voice hue coding; high-contrast for a projector.
6. **Preset selector is a hook only** (named concept presets selectable; content lands
   in Stage 4 — FUNC-05). No preset persistence engine this stage.

## Non-goals (Stage 4)

- Factory preset *content* (Straight / Triplet Swing / Humanized / Quantize demo /
  Ghost-Note groove), playability tuning, the final validation sweep, CHANGELOG v1.0.0.

## Acceptance (from ROADMAP Stage 3 test criteria)

- Grid renders; click toggles a hit; click-again cycles ghost/normal/accent; per-step
  velocity visible.
- Playhead sweeps in sync with transport (and free-runs in Standalone).
- All knobs/selectors two-way bound (drag→DSP; host automation→UI).
- Renders on macOS (VST3+AU) **and** Windows VST3 (no blank UI) — cross-platform flags.
- Grid state round-trips (save/reload restores the pattern).
- Timing lane offset **matches what is heard** (the applied Δt). Swing pushes off-beats
  later; humanize scatters; quantize pulls scatter back while leaving swing.
- MIDI readout shows note-ons from sequencer playback AND from played MIDI, with velocity.
- No audio-thread allocation/FFT; UI smooth; no event loss under fast patterns.
- Plain-language tooltip on every control + the grid + the lane.
