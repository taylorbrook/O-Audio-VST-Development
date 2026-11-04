# Context: Audio-Rate Pulsar Grain Mode

**Plugin:** O-simpleGrain
**Milestone:** audio-rate-pulsar-grain-mode
**Created:** 2026-10-28
**Phase:** Discuss

---

## Original Request

> audio-rate pulsar grain mode: duty/formant control, Nyquist duty clamp, Blackman-Harris pulsarets, fractional onsets (research §3.6)

Source: `research/granular-synthesis-2026-update.md` §3.6 (pulsar / grain-rate modulation), §3.1 (fractional onsets), roadmap item #8 ("O-simpleGrain v2 candidate").

---

## Requirements Gathered

### Core Requirements

1. **Mode switch: Cloud / Pulsar**
   - Description: New choice parameter `grainMode` (Cloud default, Pulsar). Cloud is the existing engine, untouched.
   - Acceptance: Cloud mode (and every pre-2.0.0 session/preset, which has no `grainMode`) renders **bit-exact vs 1.9.0** on the existing null configurations.

2. **Keyboard-tracked pulse rate f_p**
   - Description: In Pulsar mode the MIDI note sets the pulse rate f_p = noteHz × 2^(GrainPitch/12). Density/Scatter do not drive the clock in this mode (decide in research whether Density/Size are hidden, repurposed or ignored).
   - Acceptance: a held A3 with Formant 1× produces a fundamental at 220 Hz ± 0.1 cent (FFT peak), at 44.1/48/96 kHz.

3. **Formant as a ratio (duty)**
   - Description: `formant` knob = f_formant / f_p = 1/duty (range ~1–32×, log). Pulsaret length = 1/(formant·f_p) of one cycle of its content; duty d = 1/formant. Timbre tracks the keyboard.
   - Acceptance: spectral centroid/formant peak moves with the knob at fixed f_p; f_p (fundamental) does not move.

4. **Nyquist duty clamp**
   - Description: d ≥ f_p / (0.45·fs) so the formant stays below 0.45·fs, evaluated per pulse (f_p moves under FM). Clamp, never NaN/inf; also clamp d ≤ 1.
   - Acceptance: max formant at C8 + Formant 32× + FM depth max stays < 0.45·fs at 44.1 kHz; no energy above Nyquist folded back beyond the gate's threshold (alias gate style, ≤ −60 dB).

5. **Pulsaret content: Sample or Sine** (`pulsaretSource` choice)
   - Sample: each pulsaret reads the loaded source at Position (+ Position Spray / Scan), played at a rate so its content cycles at the formant — reuse the v1.7.0 BandlimitedReader for rate > 1.
   - Sine: analytic sine burst of one-or-more cycles at the formant frequency; no source reads.
   - Acceptance: both selectable live without clicks; Sine mode does not touch the source buffer.

6. **Blackman-Harris pulsarets, auto under ~1 ms**
   - Description: Pulsar mode uses the user's Window choice, but pulsarets shorter than ~1 ms switch to 4-term Blackman-Harris; the switch is a crossfade over a small length band (e.g. 0.75–1.25 ms), not a hard swap. BH is a new LUT table (closed form, μ₁/μ₂ checked numerically per research §6).
   - Acceptance: sidelobe/aliasing measurement shows BH region improves vs Hann at 0.3 ms pulsarets; no step discontinuity in level when sweeping duty across the band.

7. **Fractional onsets (Pulsar mode only)**
   - Description: phase-accumulator pulse clock; each onset offsets BOTH window phase and read position by δ ∈ [0,1) (§3.1 pseudocode). Cloud mode keeps its integer scheduler (bit-exact requirement).
   - Acceptance: block-size null (32 / 441 / 7 vs 512, fresh instances) ≤ −120 dBFS in Pulsar mode, incl. FM on; integer-vs-fractional comparison at 5 kHz formant shows spur reduction.

8. **Pulse masking**
   - Grain Skip keeps its random role in Pulsar mode (counter-RNG, per pulse).
   - New deterministic `pulseMask` (Off, 2…8): every Nth pulse sounds / is muted → stable subharmonics at f_p/N. Define exact convention in research (mute 1 of N vs keep 1 of N).
   - Acceptance: Mask N gives a spectral line at f_p/N; Mask Off + Skip 0 is bit-exact with masking code absent.

9. **Rate FM (sine, ratio to f_p)**
   - Description: `fmDepth` + `fmRatio` — sine modulator at ratio × f_p modulating the pulse rate inside the phase accumulator (audio-rate). Integer ratios harmonic. Depth units TBD in research (index vs semitones).
   - Acceptance: depth 0 = bit-exact vs FM code absent; sidebands at f_p ± k·ratio·f_p at ratio 1 and 2.

10. **Glisson**
    - Description: `glisson` (± st) — pitch glide of the pulsaret content across its own length (start → start + glisson). Applies to both Sample and Sine content. Nyquist clamp must account for the glide's end rate.
    - Acceptance: glisson 0 bit-exact vs absent; non-zero glide visible in a spectrogram of a slow pulse train.

### Scope Boundaries

**In scope:**
- New parameters: `grainMode`, `formant`, `pulsaretSource`, `pulseMask`, `fmDepth`, `fmRatio`, `glisson` (names provisional).
- New BH window table; fractional-onset pulse clock; per-pulse Nyquist clamp.
- UI: mode switch + pulsar controls in the WebView (hide/disable cloud-only controls in Pulsar), EN/FR/zh tooltips and labels, width pinning at the widest language.
- Level normalisation for Pulsar mode (coherent law, g ∝ 1/(N·μ₁), N = f_p·pulsaret length ≤ 1) so Pulsar sits near Cloud's level.
- Viz: pulsar grains still publish GrainEvents (rate-limited, they will arrive at audio rate — must not flood the TripleBuffer/cloud frame).
- Lesson/concept presets: optional one or two Pulsar lesson entries (decide in plan).

**Out of scope:**
- Fractional onsets in Cloud mode.
- Grainlets (length = c/f), trainlets (BLIT content).
- Moving simpleGrain onto the shared `grain-engine` scheduler/pool (beyond reusing BandlimitedReader already in use).
- Changes to Cloud-mode loudness, windows, steal behaviour.

---

## User Preferences

### Interaction Style
- [x] Real-time parameter response (all pulsar params automatable, smoothed where zipper-prone: formant, FM depth)
- [x] Automation-friendly
- [ ] Preset-based switching

### Quality Priorities
1. Cloud mode untouched — bit-exact regression vs 1.9.0 (user's top frustration is regressions).
2. Alias-free audio-rate output (Nyquist clamp, BH windows, fractional onsets) — the point of the research.
3. RT safety: 0 allocations in processBlock; CPU stays bounded at 16 voices × audio-rate pulses.

### Breaking Changes
- [ ] OK to break existing presets
- [x] Must maintain preset compatibility (new params default to Cloud / off; preset-manager JSON without the keys loads as Cloud)
- [x] Must maintain automation compatibility (no existing parameter ID, range or order changed; new params appended)

---

## Plugin Context

### Current State
- **Version:** 1.9.0 (2026-10-28)
- **Stage:** 4, status human_needed (DAW-listen checklist pending — independent of this milestone)
- **Last change:** user presets + sample library (preset-manager 1.1.0); engine untouched since 1.8.0.

### Relevant Existing Features
- `GrainVoice.h` per-voice scheduler: integer countdown `samplesUntilNextGrain`, `spawnGrain()`, 24-grain pool + 4 spare steal-fade slots (v1.5.4).
- `dsp/CounterRng.h`: seeded counter RNG, slots 0–4 used (position, pitch, pan, scatter, skip). Pulsar draws need new slots.
- `dsp/WindowLuts.h`: 5 tables + Tukey remap + tilt (v1.8.0). BH would be a 6th table; check the IN-05 contract comment and tilt interaction.
- `dsp/GrainSource.h` + BandlimitedReader (v1.7.0): mip levels latched per grain at spawn.
- Grain Skip / Grain Tilt (v1.8.0); density normalisation (v1.5.5, live-N, window μ₂-weighted).
- Gates: `tests/blocksize-null/`, `tests/alias-gate/`, `tests/skip-tilt-gate/`, `tests/state-persistence/`, `tests/preset-library-gate/`.
- preset-manager 1.1.0 with `setExcludedParameters` — new params are included in presets by default.

### Known Limitations
- Grain size floor 2 ms and density ceiling 200 g/s — Pulsar mode needs its own length/rate derivation, not wider ranges on the existing params.
- Per-grain cost: window LUT read + Lagrange/16-tap sinc read. At audio rate with duty ≤ 1 there is ≤ ~1 live pulsaret per voice (plus steal fades), so the pool size is not the constraint; per-sample work is.
- Tilt's 3 ms click floor means tilt is inert for audio-rate pulsarets (consistent; document).
- Scheduler's per-grain GrainEvent publish will run at up to kHz rates in Pulsar mode — must decimate for the viz.

---

## Questions Answered

**Q1: What should a pulsaret contain?**
> Sample + analytic — a choice between reading the loaded sample and an analytic sine burst.

**Q2: How should pulsar pitch and formant be controlled?**
> Key = f_p, Formant as a ratio (f_formant/f_p = 1/duty), so timbre tracks the keyboard.

**Q3: Fractional onsets in Cloud mode too?**
> Pulsar mode only. Cloud stays bit-exact vs 1.9.0.

**Q4: Which §3.6 extras?**
> Pulse masking, Rate FM / glide, Glisson (all three).

**Q5: Blackman-Harris selection?**
> Auto under ~1 ms, crossfaded; the user's Window choice applies above.

**Q6: Pulse masking design?**
> Grain Skip = random masking, plus a deterministic Mask pattern (every Nth pulse) for stable subharmonics.

**Q7: Rate FM modulator?**
> Sine modulator at a ratio to f_p (FM Depth + FM Ratio).

**Q8: Version?**
> 2.0.0 — headline new synthesis mode, still additive.

---

## Success Criteria

The improvement is successful when:

1. [ ] Cloud mode + pre-2.0.0 sessions render bit-exact vs 1.9.0 (blocksize-null configs, null_v180-style state nulls).
2. [ ] Pulsar fundamental = keyboard f_p within 0.1 cent; formant moves independently with the Formant knob.
3. [ ] No formant above 0.45·fs under any combination of note, Formant, FM depth and Glisson (alias gate ≤ −60 dB).
4. [ ] BH auto-switch below ~1 ms crossfades with no level step; measurable sidelobe improvement vs Hann.
5. [ ] Pulsar block-size null ≤ −120 dBFS at blocks 32 / 441 / 7 vs 512 (fractional onsets, counter RNG).
6. [ ] Mask N → stable f_p/N subharmonic; Skip random masking works per pulse; FM sidebands at expected frequencies; Glisson glide audible.
7. [ ] 0 allocations in processBlock in Pulsar mode; CPU measured at 16 voices.
8. [ ] UI: mode switch + pulsar controls with EN/FR/zh; check-i18n, fr/zh lint, check-ui-labels pass.
9. [ ] Build succeeds without warnings; pluginval strictness 10 and auval pass; state-persistence passes with the new params.

---

## Next Phase

Research should resolve:
- Pulsar clock: phase accumulator design with FM inside, fractional δ applied to window phase + read position; how Grain Pitch / pitch bend feed f_p.
- What Density, Grain Size, Scatter, Position Spray, Pitch Spray, Pan Spray and Tilt do in Pulsar mode (ignore, repurpose, hide).
- Sample pulsaret read rate: how "formant" maps to source playback rate when the source isn't a single cycle (e.g. rate = formant·f_p·L_cycle vs a fixed rate with length = d/f_p).
- BH table + crossfade band; μ₁/μ₂ for BH; interaction with Tukey/tilt.
- Mask convention, FM depth units, glisson clamp.
- Level law for Pulsar mode and the mode-switch crossfade (avoid a click when switching mid-note).
- Viz decimation of GrainEvents at audio rate.
- CPU at 16 voices × sinc reads at kHz pulse rates.

---

*Generated by improve-milestone discuss phase*
