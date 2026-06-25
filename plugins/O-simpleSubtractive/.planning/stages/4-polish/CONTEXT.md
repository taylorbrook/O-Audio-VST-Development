# Stage 4 (Polish) — CONTEXT

**Source:** Auto-generated from contracts (BRIEF.md, REQUIREMENTS.md, ROADMAP.md, Stage 3 VERIFICATION.md) — express mode, no interactive session.

## Goal

Close out v1.0.0: the concept-preset tour (FUNC-06), a playability pass (FUNC-07), and the final cross-format validation sweep. No new DSP, no new parameters — Stages 2–3 are frozen and validated. This stage fills the `applyFactoryPreset` snapshots, completes the preset roster in the UI, and ships the CHANGELOG.

## Requirements in scope

| ID | Description | Priority |
|----|-------------|----------|
| FUNC-06 | Concept-preset tour — 8 named patches: Saw→LP Sweep, Acid Bass (303), Brass Lead, Pluck, Sweep Pad, Self-Oscillation Sine, Hollow Square Bass, Filtered Noise (wind) | should |
| FUNC-07 | Playable/musical enough to double as a simple subtractive instrument (bass, lead, pluck, pad) | nice |
| — | Validation sweep: VST3 + AU build clean, auval SUCCEEDED, pluginval strictness 10 | must |

## Decisions / constraints

1. **Full 8-preset roster.** Stage 3 shipped the live bridge with **5** buttons (Pluck, Sweep Pad, Acid Bass, Self-Oscillation, Brass Stab). FUNC-06 names **8**. Stage 4 completes the roster: add **Saw Sweep**, **Square Bass**, **Noise Wind** to the UI (HTML buttons + `LESSONS` captions + `TIP` tooltips) and author all 8 C++ snapshots. Purely additive — no existing control is altered.

2. **No osc-level parameter (load-bearing constraint).** The source mix is `main + subLevel·sub + noiseLevel·noise` (`OscillatorBank::next`) — the main oscillator is **always at unity** and cannot be silenced. This shapes two presets:
   - **Self-Oscillation Sine:** can't mute the osc, so pick `oscWave = Sine` (single partial), push `resonance → 1.0` (self-osc), and set `cutoff = 261.6 Hz` with `keyTrack = 1.0` so the resonant tone tracks the keyboard. At reference note 60 the cutoff equals C4, so the self-osc whistle and the sine sit at the same pitch and reinforce into one pure, in-tune sine. `filterEnvAmount = 0` (no sweep → sustained tone).
   - **Filtered Noise (wind):** `noiseLevel = 1.0`, `oscWave = Sine`, band-pass with `cutoff ≈ 1500 Hz` so the played sine fundamental sits below the passband and is attenuated, leaving broadband noise → wind. Slow amp env for swell.
   This is a feature of the deliberately irreducible control set, not a defect; the filter-routing workaround is itself a valid lesson.

3. **keyTrack cutoff law (verified in `SubVoice.h`):** `fcEff = cutoff · 2^(keyTrack·(note−60)/12) · 2^(filterEnvAmount·env·octaves)`, reference note **60**.

4. **Preset write path:** `applyFactoryPreset(name)` resets every parameter to default (`setValueNotifyingHost(getDefaultValue())`), then writes the snapshot via `parameters.getParameter(id)->setValueNotifyingHost(convertTo0to1(real))`. Relays/attachments sync every knob/combo back to the page — **no DOM poking**. Model: `O-simpleGrain::applyFactoryPreset`.

5. **No 2nd binary-data target.** Presets are authored in C++, not embedded as files, so the single `O-simpleSubtractive_UIResources` target stands — no `BinaryData` namespace collision risk (O-simpleGrain lesson).

## Out of scope
- New DSP, new parameters, oversampling changes.
- User preset save/recall (A2 activity uses the host's own preset system — out of v1.0 per REQUIREMENTS "Out of Scope").
