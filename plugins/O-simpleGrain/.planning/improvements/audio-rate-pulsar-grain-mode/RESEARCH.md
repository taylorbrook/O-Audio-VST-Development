# Research: Audio-Rate Pulsar Grain Mode

**Plugin:** O-simpleGrain
**Milestone:** audio-rate-pulsar-grain-mode
**Created:** 2026-10-28
**Phase:** Research

---

## Context Summary

From CONTEXT.md:
- **Core requirement:** a second synthesis mode, Pulsar, beside the existing Cloud engine. The key sets the pulse rate f_p, Formant sets f_form/f_p (= 1/duty), and the pulsaret content is a sine or the loaded sample. The Nyquist clamp, Blackman-Harris pulsarets under ~1 ms and fractional onsets keep it alias-free. Extras: deterministic Mask, random Skip, sine rate FM, Glisson.
- **Scope:** Pulsar-only fractional clock. 7 new parameters appended. UI with EN/FR/zh. Cloud mode must stay bit-exact vs 1.9.0.
- **Success criteria:** Cloud bit-exact. f_p within 0.1 cent. Alias ≤ −60 dB. BH crossfade with no level step. Pulsar block-size null ≤ −120 dBFS. 0 allocations. UI gates, pluginval 10 and auval pass.

**Corrections to CONTEXT.md found while researching**
1. **Polyphony is 8 voices, not 16** (`kMaxVoices = 8`, PluginProcessor.h:305). Measure CPU at 8.
2. **The §3.6 clamp `d ≥ f_p/(0.45·fs)` does not meet the −60 dB alias criterion** (measured below). It keeps only the formant *centre* below Nyquist, while a one-cycle windowed pulsaret's spectrum reaches 4.6–10× the formant. The clamp has to be window-aware: f_form ≤ κ·fs with κ ≈ 0.10.
3. **The voice ignores pitch bend** (`pitchWheelMoved` is empty). f_p = note Hz × 2^(GrainPitch/12) only, the same as Cloud. Pitch bend stays out of scope.

---

## Measurements (numpy, scratch scripts; re-check in plan task 0)

Window moments (continuous, 200k-point midpoint):

| Window | μ₁ | μ₂ |
|---|---|---|
| Hann | 0.5000 | 0.3750 |
| Blackman-Harris 4-term (0.35875, 0.48829, 0.14128, 0.01168) | 0.35875 | **0.25796** |
| Triangle | 0.5 | 0.3333 |
| Welch | 0.667 | 0.5333 |
| Gaussian σ = 0.18 (truncated) | — | 0.319 |

These confirm the research table's μ₁/μ₂ for Hann, Triangle and Welch, and add BH (μ₂ = a₀² + (a₁²+a₂²+a₃²)/2).

**Spectral extent of a one-cycle pulsaret** w(t/L)·sin(2πt/L), the classic pulsaret with duty = 1/formant. The highest frequency still above −60 dB re the peak, in multiples of f_form:

| Window | −60 dB | −80 dB | −90 dB |
|---|---|---|---|
| **Blackman-Harris** | **4.61×** | 4.86× | 5.77× |
| Gaussian | 4.9× | — | — |
| Hann | 6.62× | 11.6× | 15.6× |
| Welch | 10.1× | — | — |
| Triangle | 10.4× | — | — |

⇒ For no component above −60 dB to fold back, the formant ceiling is **f_form ≤ fs / (2 · 4.61) ≈ 0.108·fs** with BH. That is 4.8 kHz at 44.1 kHz and 10.4 kHz at 96 kHz. Hann would allow only 0.076·fs. At the ceiling the pulsaret is 1/(0.108·fs) = 9.3 samples (0.21 ms at 44.1 kHz). That is always inside the BH region, so a single BH-derived κ covers every case. Above the crossfade band (L ≥ 1.25 ms, so f_form ≤ 800 Hz), even Triangle's 10.4× extent stays at 8.3 kHz, so the user window never aliases there.

**Recommendation:** `kFormantCeil = 0.10 · fs`, giving ~1 dB of margin under 0.108. Apply it to the *highest content rate the pulsaret will reach*: f_form × 2^(max(0, glisson)/12) × 2^(pitchSpray/12). Clamp the duty to d ∈ [f_p / kFormantCeil, 1]. If f_p alone exceeds the ceiling (about C#8 + Grain Pitch at 44.1 kHz), cap f_p at the ceiling too and document it. This affects the top 1–2 keys at 44.1 kHz and none at 96 kHz.

---

## Implementation Approach

### Recommended Approach

**Strategy:** a separate pulsar scheduler and pulsaret render path inside `GrainVoice`, sharing the existing grain pool.

**Description:**
`GrainVoice::renderNextBlock` keeps its Cloud scheduler and its Cloud-grain overlap-add **verbatim**. A per-block `params.grainMode` picks which scheduler runs: the existing integer countdown, or a new double-precision pulse phase accumulator. Grains gain a `pulsaret` flag. The render loop branches on it, and Cloud grains run exactly the same float expression sequence as now. This gives Cloud bit-exactness by construction, and the only extra cost is one well-predicted branch.

Both kinds of grain live in the same `std::array` pool (24 + 4 fade slots). A mode switch mid-note therefore needs no crossfade of its own. In-flight Cloud grains window out naturally, the first pulsaret starts at the window's zero, and the normalisation gain change goes through the existing 20 ms `outputGain` ramp. Going back to Cloud works the same way. Neither switch clears the pool.

**Pulsar clock (per sample, double):**
```
fInst   = fp * (1 + D * sin(2π φm))        // linear rate FM, mean rate = fp exactly
φm     += ratio * fp / fs                   // wrapped to [0,1)
φp     += fInst / fs
if φp >= 1:                                 // at most one onset per sample (fInst ≤ 0.10·fs·(1+D) < fs)
    φp -= 1
    δ   = φp / (fInst / fs)                 // samples elapsed since the crossing, ∈ [0,1)
    if !masked(k) && !skip(k): spawnPulsaret(δ)
    ++k (= spawnTick)
```
- At note-on, φp = 1 − ε so that the first pulse fires on the note's first sample with δ ≈ 0. Seed φm = 0 there too.
- `spawnPulsaret(δ)` starts the window phase at δ·phaseInc, the read position at start + rate·δ and the content phase at δ·contentInc. The three offsets come straight from §3.1, and render then reads the grain in the same sample.
- The clock depends only on the sample count since note-on and on static parameters, so it is independent of block size. Formant and FM Depth are smoothed per sample through a per-voice linear ramp. A ramp retargeted at block boundaries is block-size-dependent while it moves, the same caveat as the existing `scanSmoothed`. The null gate uses static parameters, as the Cloud gate does.
- **FM depth units:** linear deviation, 0–100 % mapped to D ∈ [0, 0.95]. The mean rate stays at f_p, so pitch stays centred and the 0.1-cent check holds, and the instantaneous rate never reaches 0. Semitone (exponential) FM shifts the mean pitch with depth, so it was rejected. Sidebands fall at h·f_p ± k·ratio·f_p, with index h·D/ratio on harmonic h.
- `fmRatio` is a float from 0.25 to 8, skewed with centre 1, default 1. Integer ratios give harmonic spectra. Snapping is left to the UI.
- D = 0 must skip the FM arithmetic (branch) so that "depth 0 = bit-exact vs FM absent" holds trivially.

**Pulsaret length and Formant:**
L = d / f_p seconds with d = clamp(1/formant, f_p/kFormantCeil, 1). Formant is a float from 1 to 32×, log-skewed, default 4×. L comes from the **nominal** f_p, not fInst, so FM moves the onsets and leaves the formant alone. When fInst > f_p, pulsarets can overlap (at most ~2 at D = 0.95), and the pool absorbs this.

**Content (`pulsaretSource`: Sine | Sample, default Sine):**
- **Sine:** one cycle across the pulsaret, `sin(2π·φc)` with φc += contentInc (double) and contentInc = 1/L_samp. The waveform is zero at both ends and has no DC. Using `std::sin` per sample per pulsaret costs ≤ ~2 live pulsarets × 8 voices, which is negligible. Sine mode never touches `sourcePtr`/`sourceMip`.
- **Sample:** read the source at rate r = f_form / f_C3 (f_C3 = 130.81 Hz, the plugin's recorded-pitch reference, `kRootNote` = 60). Every pulsaret then reads one C3 period (7.6 ms) of source, transposed to fit L. That makes Formant a true transposition of the source content, consistent with Cloud's key tracking. The read start is the playhead plus Position Spray, which reuses the existing `playheadAt()` and spray code, so Position, Scan and Freeze still work. Rates above 1 use `BandlimitedReader`, and the key range stays within the 6 mip levels (r ≤ 0.1·fs/130.8 ≈ 34 at 44.1 kHz). Rates up to 1 keep Lagrange.
- **Sample-content guard:** the window also spreads the *source's own* bandwidth by ±B/L (B ≈ 4 bins for BH). For short pulsarets the reader's fs/2 cutoff is too high. So latch the mip level and sinc set from an over-filtered rate r·γ, with γ = 0.5 / max(0.05, 0.5 − 4/L_samp), while still advancing readPos by r. Above about 50-sample pulsarets this is a no-op. Below that it trades some brightness for no aliasing. The alias gate verifies it.

**Windows and Blackman-Harris:**
- BH becomes a 6th internal table: `tables` grows to 6, with `kShapeBH = 6` as an internal index that is not in the user Choice. The 5 existing tables are unchanged, so Cloud reads are bit-exact. `read()` clamps the shape to `kNumShapes − 1`, so a separate `readBH(phase)` (one lerp) avoids loosening that clamp for Cloud.
- Blend: a = smoothstep((1.25 − L_ms)/0.5) over [0.75, 1.25] ms, and env = (1−a)·g_u·w_user(φ) + a·g_bh·w_bh(φ). The g are per-window power compensations relative to Hann (√(μ₂_Hann/μ₂_w); BH = √(0.375/0.258) = 1.206; the user window's comes from `windowLuts.meanSquare`, cached per spawn). With them the level has no step across the band. a is fixed per pulsaret at spawn.
- **Tilt** is inert below 6 ms by its own 3 ms floor, so every pulsaret is untilted. Skip `tiltFor` in Pulsar mode and document it. Taper and Rect guard reuse `taperEndFor(L)`. At these lengths Rect becomes a full Hann through its 1 ms guard, which is consistent.

**Level law (Pulsar):** the pulsarets don't overlap (N = duty ≤ 1), and the research rule says never boost when N < 1. So Pulsar's normGain is 1 × the per-pulsaret window-power term above. There is no duty compensation: narrower pulses are thinner and quieter, which is the instrument's character. **Open question 1** proposes an optional mild d^(−¼) lift. Cloud's `normGain` block is computed only in Cloud mode, unchanged.

**Masking:**
- **Mask** is a Choice (Off, 2…8). Pulse k is muted when k mod N == N − 1 (**mute 1 of N**). This adds a stable f_p/N subharmonic line while f_p stays the perceived pitch. Keep-1-of-N would mostly duplicate a pitch-down with a fixed formant; it is noted as an alternative in open question 2.
- **Skip** reuses `skipThisGrain()` on kSlotSkip with tick = pulse index k. Mask Off + Skip 0 draws nothing and evaluates no modulo.

**Glisson (±24 st, default 0):** the content rate is multiplied per sample by m = 2^(glisson/(12·L_samp)), a geometric glide from start to start + glisson. Sample mode latches the mip level and sinc set from the **maximum** rate over the glide. A glisson of 0 takes a branch that never multiplies. The Nyquist ceiling includes 2^(max(0, glisson)/12).

**RNG slots:** Pulsar uses the existing slots (position 0, pitch 1, pan 2, skip 4) with tick = k. No new slots are needed: scatter (3) is unused in Pulsar, and FM and Mask are deterministic. Cloud's draw order is untouched.

**What the Cloud controls do in Pulsar mode**

| Control | Pulsar behaviour | UI |
|---|---|---|
| Density, Grain Size, Scatter, Vel→Density | ignored (clock = key) | dimmed (`.is-inactive`), not hidden, so layout widths stay pinned |
| Grain Tilt | inert (3 ms floor) | dimmed |
| Position, Scan, Freeze, Position Spray | Sample content read start | live |
| Pitch Spray | per-pulse content detune (formant jitter); f_p unaffected | live |
| Pan Spray | per-pulse pan | live |
| Grain Pitch | transposes f_p (and so the formant, which is a ratio) | live |
| Window, Taper | user window above 1.25 ms | live |
| Grain Skip | random pulse masking | live |
| ADSR, Output | unchanged | live |

**Viz decimation:** at 4 kHz × 8 voices × 512-sample blocks, ~370 events would overflow `kMaxEvents = 256`. Overflow drops are safe, but they bias the display toward block starts and starve later voices. So a per-voice pulsaret publish countdown emits at most one GrainEvent every ~10 ms (480 samples at 48 kHz). The countdown is audio-independent and never touches the RNG or the clock. Cloud spawns publish as now.

**Pros:**
- Cloud is bit-exact by construction: the Cloud code path, RNG ticks and normGain code are not edited, only branched around.
- One pool, so mode switches are click-free without new crossfade machinery.
- Measured, window-aware alias ceiling instead of the loose 0.45·fs.

**Cons:**
- `GrainVoice.h` grows by ~250 lines. The pulsar spawn and render should go in member functions to keep `renderNextBlock` readable.
- The formant ceiling of 0.10·fs (4.4 kHz at 44.1 kHz) is lower than the research text implies. It is honest, but it limits brightness at 44.1 kHz. Users at 96 kHz get 9.6 kHz.

### Alternative Approaches Considered

**Alternative 1: move simpleGrain onto the shared grain-engine scheduler.**
- Description: use the module's fractional scheduler for both modes.
- Why not chosen: out of scope (CONTEXT), and it would break Cloud bit-exactness.

**Alternative 2: separate PulsarVoice class / second Synthesiser.**
- Description: a dedicated voice type, with mode switching between synths.
- Why not chosen: a mid-note mode switch would need a cross-synth crossfade and duplicated ADSR and voice-steal logic. It also doubles voice memory, which the shared pool avoids.

**Alternative 3: keep the §3.6 0.45·fs clamp and rely on BH.**
- Why not chosen: measured above. A one-cycle BH pulsaret has −60 dB content out to 4.6× f_form, so 0.45·fs aliases by ~30 dB.

**Alternative 4: multi-cycle pulsarets (C > 1) to raise the ceiling.**
- Description: the content has C cycles, so the band narrows to f_form(1 ± 4.6/C) and the ceiling rises toward 0.45·fs.
- Why not chosen: that is the grainlet/trainlet direction, which is out of scope. Noted as a v2.1 candidate.

---

## Affected Components

### Files to Modify

| File | Changes | Complexity |
|------|---------|------------|
| `Source/GrainVoice.h` | `GrainVoiceParams` pulsar fields. Pulse clock state (φp, φm, k), `spawnPulsaret()`, `renderPulsaret()` branch, ceiling/duty clamp, viz decimation, mode-switch handling (no pool clear) | **High** |
| `Source/dsp/Grain.h` | `pulsaret`, `sineContent`, `contentPhase` (double), `contentInc`, `glideMul`, `bhMix`, `gUser`, `gBh` fields. POD, safe to copy into the fade slots | Low |
| `Source/dsp/WindowLuts.h` | 6th table (BH), `readBH()`, `kShapeBH`, header doc. The IN-05 contract only extends if the UI inset draws the pulsaret window | Low–Med |
| `Source/PluginProcessor.cpp` | 7 params appended with `ParameterID{…, 2}` (O-TextureForge v1.8.0 precedent). Load/push into `GrainVoiceParams`. `normGain` gated to Cloud. Pulsar gain path | Medium |
| `Source/PluginProcessor.h` | param pointers, ParamIDs, `kFormantCeil` constant | Low |
| `Source/ui/public/index.html` | Mode select (Grain group), new `group-pulsar` section: Formant, Content, Mask, FM Depth, FM Ratio, Glisson | Medium |
| `Source/ui/public/js/app.js` | bindings, mode-driven `.is-inactive` dimming, readout formatting (×, st, %), viz handling of sub-ms sizes | Medium |
| `Source/ui/public/js/i18n.js` | EN/FR/zh labels + tooltips for 7 params, 2+8 choice labels | Medium |
| `Source/ui/public/css/styles.css` | pulsar group, inactive state, widest-language width pins | Low |
| `CMakeLists.txt` | version 2.0.0 | Low |
| `CHANGELOG.md`, `BRIEF.md`/`parameter-spec.md` | docs | Low |

### New Files

| File | Purpose |
|------|---------|
| `tests/pulsar-gate/gate.py` | Pedalboard gate on the installed VST3: f_p 0.1 cent (A3 at 44.1/48/96 kHz), formant moves while f_p stays, alias ≤ −60 dB (C8 + 32× + FM max + glisson +24), BH band no-step sweep + sidelobe vs Hann at 0.3 ms, Mask f_p/N line, FM sidebands at ratio 1/2, glisson spectrogram slope, Sine mode with no source (source unloaded still sounds), mode-switch click probe |
| `tests/blocksize-null/` (extend) | Pulsar configurations: Sine/Sample, FM on, Mask 3, Skip 30 %. Cloud configs untouched |
| `tests/render-harness/` (extend) | 0-alloc check in Pulsar mode + CPU at 8 voices, C8, Sample content |
| null vs 1.9.0 | reuse the `null_v180.py` pattern (seeded out-of-tree pair via `git archive`; memory: `pattern_null_test_needs_seeded_out_of_tree_pair`, `pattern_git_archive_ui_tree_misses_gitignored_module_copies`) |

### Dependencies

- `juce_audio_basics` (`MidiMessage::getMidiNoteInHertz`), existing `SynthesiserVoice` and ADSR.
- grain-engine 1.1.0 `BandlimitedReader` (already vendored: `latchLevel`, `SincBank::setFor`). Plan task: confirm `setFor` accepts the over-filtered residual in [1, 2) and that `read()` takes positions in level-0 units.
- preset-manager 1.1.0 resets every parameter to its default before applying a preset (`OuariconPresetManager.h:386`), so old presets without the new keys load as Cloud. Sessions go through `apvts.replaceState`. A fresh instance defaults to Cloud.

---

## Pattern Analysis

### Existing Patterns in Codebase

- **Branch-around for bit-exactness:** `g.tilted ? readTilted : read` (GrainVoice.h:330) and the `rate > 1` Lagrange/sinc split (:333). The pulsaret branch follows the same idiom.
- **Counter RNG per tick:** `bipolarDraw(slot)` / `skipThisGrain()` (GrainVoice.h:519–532). Pulsar reuses these with tick = pulse index.
- **Spawn-time latching:** taper, tilt, mip level and sinc set are all fixed in `spawnGrain` (:436–493). BH mix, gains, ceiling and glide get latched the same way.
- **Steal fade slots:** `moveToFadeSlot` (:574) copies the POD, so new Grain fields come along automatically.
- **Window-power normalisation:** `meanSquare()` relative to Hann (PluginProcessor.cpp:1131–1157). The Pulsar per-pulsaret g comes from the same reference.
- **Param version hint 2 for appended params:** O-TextureForge `PluginProcessor.cpp:172`.

### Similar Implementations

- O-GrainScatter / O-TextureForge: the counter RNG and steal fade are already shared. No pulsar mode exists in the suite. This is the first one, and the grain-engine fractional scheduler pseudocode (§3.1) is the reference.
- Roads, *Microsound* (pulsar synthesis); Grainferno/Phonon (2026) for UX: Formant ratio + Mask are the common surface.

---

## Complexity Assessment

### Overall Complexity: High

- Files affected: ~11
- DSP changes: Significant (new scheduler, render path, window, clamp)
- UI changes: Significant (new group, mode-driven dimming, 3 languages, width pins)
- Parameter changes: Add 7 (appended, version hint 2)
- Breaking changes: No

### Risk Areas

1. **Cloud regression** (the user's top frustration)
   - Risk: an edited shared expression in the render loop, RNG tick or normGain changes Cloud output by 1 ULP.
   - Mitigation: no edits inside the Cloud expressions, only `if (g.pulsaret) { …; continue; }` ahead of them. Null all existing blocksize-null configs plus `null_v180`-style state nulls vs a seeded 1.9.0 build **before** any UI work lands. Negative control: an intentionally 1-ULP-perturbed build must fail.
2. **Aliasing at the top of the keyboard**
   - Risk: the ceiling misses glisson, pitch spray or FM overlap, or the Sample-content window spread.
   - Mitigation: the ceiling is computed from the max content rate. The gate sweeps C8 × Formant 32 × FM 100 % × Glisson +24 × Pitch Spray 12 for both content types. Stimulus check: the gate must FAIL with the ceiling set back to 0.45·fs (memory: `pattern_gate_stimulus_below_threshold_is_vacuous`).
3. **Block-size dependence**
   - Risk: the clock or FM phase uses float, or smoothing retargets per block.
   - Mitigation: double φp and φm, per-sample updates, static-param null at 32/441/7 vs 512. The `pattern_block_timer_fires_one_block_early` trap is avoided because the onset test happens before render in the same sample.
4. **Level jump on mode switch / BH band**
   - Risk: Cloud normGain vs Pulsar gain differ by many dB, giving an audible jump.
   - Mitigation: the 20 ms outputGain ramp plus pool drain. The gate measures a mid-note switch click (step ratio vs a same-motion control, `pattern_step_click_gate_needs_same_motion_control`). Report the steady-state level difference, which is acceptable but documented.
5. **UI width / i18n**
   - Risk: new labels overflow in FR (Formant, Glissando, Masque d'impulsion).
   - Mitigation: glossary first, pin widths at the widest language, check-ui-labels + scrollWidth with nowrap (`pattern_scrollwidth_fit_gate_misses_wrapped_caption`).
6. **Viz flood**
   - Mitigation: per-voice 10 ms publish countdown, independent of the audio path.

---

## Domain Detection

| Domain | Score | Keywords Found |
|--------|-------|----------------|
| DSP | 10 | processblock, filter, gain, dsp, algorithm, buffer, sample, frequency, realtime, latency |
| GUI | 7 | webview, ui, css, html, interface, editor, layout |
| Polish | 4 | preset, validation, pluginval, cpu |

**Detected Domain:** mixed (DSP > 0 and GUI > 0)
**Recommended Execute Agent:** general-purpose, with the plan split into waves: **Wave A, DSP** (dsp-agent-style tasks plus Cloud null gate, which must pass before Wave B), **Wave B, UI/i18n** (gui-agent-style), **Wave C, gates/docs**.

---

## Version Impact

### Recommended Version Bump: MAJOR (2.0.0) as chosen in Discuss

- [ ] Bug fix only (PATCH)
- [x] New feature, backward compatible. Semver would allow MINOR, but the user chose 2.0.0 for a headline new synthesis mode.
- [ ] Breaking changes

**Breaking Change Analysis:**
- Parameter IDs: unchanged, 7 appended
- Parameter ranges: unchanged
- Preset format: unchanged (missing keys → defaults → Cloud)
- Public API: n/a

---

## Open Questions

1. **Duty level compensation.** Recommended: none (physical pulsar character; the formant 32× pulse train sits ~15 dB under formant 1×). The alternative is a mild d^(−¼) lift (+3.8 dB at 32×) so the Formant knob reads as timbre more than volume. → Decide in plan. Default to none unless you want the lift.
2. **Mask convention.** Recommended: mute 1 of N (adds the subharmonic, keeps the pitch). The alternative is keep 1 of N (the pitch drops to f_p/N with a fixed formant).
3. **Default content.** Recommended: Sine, so the first switch to Pulsar gives the textbook vowel-like pulsar sound. With the fire sample, a one-C3-period slice reads as a noise burst.
4. **Formant ceiling at 44.1 kHz (4.4 kHz).** Accept it, or plan multi-cycle pulsarets (Alternative 4) for v2.1.
5. **Lesson presets.** One or two Pulsar lesson/concept entries ("Pulsar vowel", "Subharmonic mask")? Recommended: yes, 2 entries, decided in plan.

---

## Next Phase

This research feeds the **Plan** phase, which will:
- Break Wave A (DSP + Cloud null gate) / B (UI + i18n) / C (gates, CPU, docs) into atomic tasks
- Make "Cloud null vs 1.9.0 passes" a hard dependency before Wave B
- Set per-task verification (pulsar-gate items, block-size null, 0 alloc)
- Settle open questions 1–5

---

*Generated by improve-milestone research phase*
