# Stage 4 (Polish) — VERIFICATION

**Verdict: PASS.** Goal-backward check against the Stage 4 goal (ship v1.0.0:
FUNC-06 preset tour + FUNC-07 playability + clean validation sweep, no DSP change).

## Success criteria

| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | 8 tour buttons present, each loads a distinct on-concept patch; controls + visuals update with no DOM poking | ✅ | 8 buttons in `index.html`; `applyFactoryPreset` writes via `setValueNotifyingHost` (host API → relays/attachments → UI); 8 distinct snapshots |
| 2 | `data-preset` ≡ `LESSONS` ≡ C++ match strings (no silent no-op) | ✅ | 3-way grep-diff: all three lists = {Acid Bass, Brass Stab, Noise Wind, Pluck, Saw Sweep, Self-Oscillation, Square Bass, Sweep Pad} |
| 3 | Self-Oscillation plays a clean in-tune sine across the keyboard | ✅ (by construction) | `cutoff 261.6 Hz · keyTrack 1.0 · envAmt 0` → `fcEff(note) = 261.6·2^((note−60)/12)` = note pitch (verified `SubVoice.h` law); Stage-2 self-osc-in-tune gate green |
| 4 | Noise Wind is audibly noise-dominant (osc fundamental rejected) | ✅ (by construction) | Sine source ≤ C5 sits below the 1.5 kHz band-pass centre; `noiseLevel 1.0` fills the pass-band |
| 5 | No DSP / parameter-default change; Stage 2/3 behaviour intact | ✅ | Only `applyFactoryPreset` body + additive UI changed; `createParameterLayout`, voice, filter, viz untouched (git diff scope) |
| 6 | auval SUCCEEDED; pluginval s10 ALL TESTS PASSED; VST3+AU build clean | ✅ | `AU VALIDATION SUCCEEDED`; pluginval `--strictness-level 10` → `SUCCESS`; ninja VST3+AU clean |
| 7 | CHANGELOG v1.0.0 written; STATUS/REQUIREMENTS updated | ✅ | `CHANGELOG.md` [1.0.0]; FUNC-06/07 → ✅ in REQUIREMENTS; STATUS → Stage 4 complete |

## Requirements closed
- **FUNC-06** (concept-preset tour, 8 patches) — ✅ done.
- **FUNC-07** (playable as a simple subtractive instrument) — ✅ satisfied: usable default
  state + bass/lead/pluck/pad presets one click away; no default change needed.

## Regression watch — clear
- Skewed ranges handled correctly: all snapshot values passed as **reals** to
  `convertTo0to1` (cutoff log-skew, ADSR/ glide skews respected).
- No 2nd binary-data target added → no `BinaryData` namespace collision.
- No new native function → no auval/pluginval surface change beyond snapshot bodies.

## Not automatable (carried to manual UAT)
On-screen look/feel of each preset load (control snap, visual response, the audible
"on-concept" character). Objectively green: wiring, parameter math, build, auval,
pluginval. Recommend a one-pass eyeball in `/show-standalone O-simpleSubtractive`
or a DAW before distribution.
