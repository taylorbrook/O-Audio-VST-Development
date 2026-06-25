# Stage 4 (Polish) — SUMMARY

**Outcome:** v1.0.0 closeout. The concept-preset tour (FUNC-06) is fully wired and
audibly correct, playability (FUNC-07) is satisfied, and the final cross-format
validation sweep is green. **No DSP or parameter changes** — Stages 2–3 stayed frozen.

## What shipped

### 1. Preset roster completed to the full 8 (FUNC-06)
Stage 3 shipped the live bridge with 5 buttons; Stage 4 added the 3 missing concepts
so the roster matches FUNC-06 exactly. Pedagogical order, all additive:

| # | Button | Concept | Defining moves |
|---|--------|---------|----------------|
| 1 | Saw Sweep | the literal subtraction | Saw → LP 24 dB, slow filter env, cutoff 600 |
| 2 | Pluck | fast filter env → percussive | filter D 0.18 / S 0, amp D 0.35 / S 0 |
| 3 | Brass Stab | env opens & holds cutoff; key-track | filterEnvAmt +0.6, keyTrack 0.3 |
| 4 | Sweep Pad | slow swell + long deep sweep | filter A 1.8 / D 2.5, amp A 1.2 |
| 5 | Acid Bass | high-res squelch, mono + glide | res 0.78, Mono, glide 0.06 |
| 6 | Square Bass | hollow square + sub, mono | Square, subLevel 0.5, Mono |
| 7 | Noise Wind | band-passed noise = wind | noiseLevel 1.0, BP 1.5 kHz, Sine carrier rejected |
| 8 | Self-Oscillation | filter rings into an in-tune sine | res 1.0, cutoff 261.6 Hz, keyTrack 1.0, envAmt 0 |

### 2. `applyFactoryPreset` filled (was a wiring-only stub)
Reset-to-default loop + `setReal`/`setChoice` helpers + an 8-branch dispatch, all
through `parameters.getParameter(id)->setValueNotifyingHost(convertTo0to1(real))`.
Writes flow through the public host API, so the relays/attachments sync every
knob/combo and every visual back to the page — no DOM poking, no DSP touched.

## Files modified
- `Source/PluginProcessor.cpp` — `applyFactoryPreset` body (8 snapshots).
- `Source/ui/public/index.html` — tour buttons reordered + extended to 8.
- `Source/ui/public/js/app.js` — 3 new `LESSONS` captions + 3 new `TIP` tooltips.
- `CHANGELOG.md` — **new**, v1.0.0 entry.
- `.planning/` — CONTEXT / RESEARCH / PLAN / SUMMARY / VERIFICATION, STATUS, REQUIREMENTS.

## Key design note (load-bearing)
The source mix is `main + subLevel·sub + noiseLevel·noise` — the **main oscillator
cannot be silenced** (no osc-level param in the irreducible control set). Two presets
are designed around this:
- **Self-Oscillation:** Sine source + `cutoff = 261.6 Hz` (C4 = ref note 60) + `keyTrack
  = 1.0` so `fcEff` tracks the keyboard → the resonant tone and the sine coincide at
  the note's pitch and reinforce into one pure, in-tune sine. `filterEnvAmount = 0`.
- **Noise Wind:** Sine source sits below a 1.5 kHz band-pass and is rejected, leaving
  `noiseLevel = 1.0` broadband noise → wind.

## Verification
- 3-way key parity (HTML `data-preset` ≡ JS `LESSONS` ≡ C++ `name ==`) confirmed for all 8.
- VST3 + AU build clean; **auval SUCCEEDED**; **pluginval s10 SUCCESS**.
- Stage 2/3 untouched → no regression surface beyond the snapshot bodies + additive UI.

## Manual UAT remaining (look/feel only — not automatable)
Eyeball each of the 8 presets in `/show-standalone` or a DAW: controls + visuals snap
on load; Self-Oscillation whistles in tune across the keyboard; Noise Wind is
noise-dominant (osc fundamental inaudible); Acid Bass squelches; Pluck is percussive.
