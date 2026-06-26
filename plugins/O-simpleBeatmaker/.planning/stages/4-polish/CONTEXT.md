# Stage 4 (Polish) — CONTEXT

**Source:** Auto-generated (express mode) from existing contracts — BRIEF.md,
REQUIREMENTS.md, ROADMAP.md (Stage 4 line), parameter-spec.md, and the Stage 3
VERIFICATION.md residual list. No interactive discuss session.

**Plugin:** O-simpleBeatmaker · **Stage:** 4 of 4 (Validation / Polish) · **Mode:** express

---

## Goal

Take the verified Stage-3 teaching instrument from "all mechanisms work" to
"ships as v1.0.0": real concept-isolating factory presets, playability tuning so
it doubles as a real 808/909 drum instrument, a full cross-format validation
sweep, a hands-on QUAL-02 audible-vs-visible confirmation, and CHANGELOG v1.0.0.

This stage adds **no new DSP mechanisms and no new UI mechanisms** — it fills the
content/quality residuals deferred from Stages 2–3 and proves the whole is
release-ready. Stability of the verified Stage-3 build is paramount: presets and
playability tweaks must not regress the grid, playhead, timing lane, MIDI
readout, or any param binding.

## Requirements in scope (from REQUIREMENTS.md → Stage 4)

| ID | Priority | What it asks |
|----|----------|--------------|
| FUNC-05 | should | Concept-isolating factory pattern presets: straight/no-feel, backbeat + accents, ghost notes, swing, humanized, quantize demo |
| FUNC-08 | nice | Playable/musical enough to double as a simple 808/909-style drum instrument in real projects |
| (sweep) | must | Final validation: pluginval VST3+AU, auval, no regressions across all prior requirements |
| QUAL-02 | must | Hands-on confirm the timing lane / playhead / velocity / MIDI readout match what is heard (audible-vs-visible audit) |

## The 6 factory presets (content already named in UI + tooltips)

The Stage-3 UI already ships the tour buttons + tooltips (`index.html` line
110–115, `app.js` TIPS `lessonStraight…lessonQuantize`). Stage 4 makes them load
real patterns + param settings. Each preset **isolates exactly one concept** so a
student can reverse-engineer the move:

| # | Button label | Concept isolated | Pattern + param intent |
|---|--------------|------------------|------------------------|
| 0 | **Straight** | baseline / no-feel | 4-on-floor kick + 8th hats + snare on 2&4, **all equal velocity**, swing=0, humanize=0, quantize=100. The dead-flat reference everything departs from. |
| 1 | **Backbeat + Accents** | velocity (accents) | Same skeleton, but **hard accents** on the backbeat snare + downbeat kick, quieter in-between hats. Velocity ALONE turns a march into a groove. swing/humanize still 0. |
| 2 | **Ghost Notes** | velocity (ghosts) | Backbeat + quiet ghost snares tucked between the 2&4 — the pattern "breathes." Still swing=0, humanize=0 so the dynamics are the only variable. |
| 3 | **Triplet Swing** | swing | A hat/kick/snare groove with **swing pushed up (~58–66%)**, humanize=0, quantize=100 — the off-beats slide late into a shuffle, cleanly, with no random scatter. |
| 4 | **Humanized** | humanize | A tight pattern with **humanize ~70%, quantize low (~25%)** — every hit scatters slightly off the grid. Shows "loosely played." |
| 5 | **Quantize Demo** | quantize strength | **humanize high (~85%) + quantize mid (~50%)** with swing also on, so the student can sweep the Quantize knob and watch/hear the scatter pull back toward the grid WHILE the swing stays (the DSP-04 invariant, made interactive). |

Pattern length 16 for all six (one bar). Voices used: Kick/Snare/ClosedHat
(+OpenHat accents where musical). Tasteful, recognizably musical — these double
as the FUNC-08 "real instrument" starting points.

## Key decisions (locked for express execution)

1. **Preset mechanism = lightweight C++ factory table, NOT OuariconPresetManager.**
   FUNC-05 is *read-only concept lesson patterns*, not user save/load. A heavy
   disk-JSON preset manager (à la O-simpleFM Stage 4) is out of scope and would
   add a whole UI surface. Instead: a `BeatPresets.h` data table + a processor
   `applyConceptPreset(int)` + one new `applyPreset` native function. This keeps
   the single-page projector layout uncluttered (UI-06) and matches the existing
   tour hook. `getNumPrograms()` stays 1 (host program list not used for these).
2. **Apply path is message-thread + host-notifying.** `applyConceptPreset` sets
   APVTS params via `setValueNotifyingHost` (so the two-way-bound knobs + host
   automation update for free) and writes the grid via the existing thread-safe
   `setStep`/`clearGrid` atomics, then the JS refreshes the grid from backend.
   No audio-thread work, no new lock.
3. **Playability (FUNC-08) = tuning only, no new params.** Confirm/adjust default
   voice tunings/decays/tones and preset velocities so the kit sounds good out of
   the box. No new APVTS params (the 42-param contract is frozen). Any default
   change must be a value change in `createParameterLayout`, regression-checked.
4. **Validation sweep is the gate.** Build VST3+AU+Standalone clean → cache-clear
   + dual-variant sweep install → `auval` → `pluginval --strictness-level 10` →
   render-harness re-run (all 6 probes still green) → screenshot the UI with a
   preset loaded (not blank) → hands-on QUAL-02 spot check.
5. **CHANGELOG v1.0.0** authored as the first release entry (covers the whole
   staged build, not just Stage 4).

## Constraints / non-negotiables

- **No regression** of the Stage-3 verified build (CLAUDE.md frustrations
  directive + user profile). Grid/playhead/lane/MIDI/all-42-bindings must still
  pass after preset + playability work.
- **42-parameter APVTS contract frozen** — presets set existing params, never add
  params. Grid stays custom `std::atomic<uint8_t>[6×32]` + PATTERN ValueTree.
- **PERF-01 real-time safety unchanged** — preset apply is message-thread only.
- **Cross-platform**: any new native fn must keep JS `getNativeFunction` ⇿ C++
  `withNativeFunction` parity exact (suite critical pattern; memory:
  `pattern_webview_native_fn_bridge_gap`).
- **BinaryData namespace**: still a single `O-simpleBeatmaker_UIResources` target;
  no second `juce_add_binary_data` (O-simpleGrain collision lesson) — preset data
  is C++ source, not a binary-data blob.

## Out of scope (deferred / never)

- User-savable presets, disk JSON, preset browser UI (v1.1+ if ever).
- Song mode / pattern chaining, per-step probability, new voices, FX — all listed
  Out of Scope in REQUIREMENTS.md.
- Any new DSP mechanism or UI panel.

## Success criteria (goal-backward for verify)

1. All 6 tour buttons load a distinct, musically recognizable pattern + the param
   settings that isolate their concept; grid + knobs + lane visibly update.
2. The 6 presets demonstrably isolate their concept (e.g. Quantize Demo: sweeping
   quantize pulls humanize scatter back while swing remains — DSP-04 visible).
3. Kit sounds good out of the box (FUNC-08) — defaults + preset velocities are
   musical, no harshness/clicks.
4. Build clean VST3+AU+Standalone; **auval SUCCEEDED**; **pluginval strictness-10
   SUCCESS**; render-harness 6/6 green; UI screenshot with a preset loaded (not
   blank).
5. No regression: every Stage 1–3 verified behavior still holds.
6. CHANGELOG v1.0.0 written; STATUS/registry → ✅ Working.

## References

- BRIEF.md (preset tour framing: lines 98, 104; playability: line 106)
- ROADMAP.md Stage 4 line (preset roster, validation sweep list)
- REQUIREMENTS.md FUNC-05 / FUNC-08 / QUAL-02
- Stage 3 VERIFICATION.md "Residual (Stage 4)" block
- Sibling preset precedent: O-simpleFM `FactoryPresets.*` (heavier; we go lighter)
- parameter-spec.md (frozen 42-param contract + ranges/defaults)
