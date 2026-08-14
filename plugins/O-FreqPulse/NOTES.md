# O-FreqPulse Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.17.0
- **Type:** Audio Effect (Spectral Sequencer)

## Lifecycle Timeline

- **2026-02-03:** Ideation complete — creative brief and requirements documented
- **2026-02-03 (Stage 1):** Foundation + Shell complete — 165 parameters, APVTS, build working
- **2026-02-03 (Stage 2):** DSP complete — FFT spectral processing, step sequencing, Euclidean generator
- **2026-02-03 (Stage 3):** GUI complete — WebView UI with 2D step grid, playhead, naturalist aesthetic
- **2026-02-04 (v1.0.0):** Stage 4 complete — 12 factory presets, pluginval Level 5 passed, auval passed
- **2026-02-04 (v1.1.0):** Added Clear and Random buttons per lane
- **2026-02-04 (v1.1.1):** Fixed playhead moving when no audio signal present
- **2026-02-04 (v1.1.2):** Euclidean patterns now display on step grid
- **2026-02-05 (v1.2.0):** Fixed buzzing artifact at step transitions — per-band time-domain gain application
- **2026-03-05 (v1.13.0):** Added per-band Mute (M) and Solo (S) buttons — surfaces existing band_enable parameter in UI with visual dimming and exclusive solo
- **2026-07-08 (v1.16.3):** Resolved all 11 warnings from the v1.16.2 deep code review (no critical issues). Fixed tooltip persistence (wrong bridge object + one-shot restore race, WR-01), factory step-velocity leak into Euclidean bands (WR-02), save dialog ignoring the chosen directory (WR-03), and hardened DSP/preset edges (Nyquist clamp WR-04, channel-count clamp WR-05, version-string factory sentinel WR-06, non-destructive factory init WR-07, preset-manager.js resync WR-08). Marked freq_low/freq_high non-automatable (display-only, WR-10) and reduced the DAW native program menu to 1 entry (WR-11). Documented approximate LR-tree unity-gain (WR-09).
- **2026-07-08 (v1.16.4):** Resolved the safe/mechanical info-level review findings (IN-02, IN-03, IN-06, IN-10, IN-13). Corrected stale docs that described the removed FFT design + phantom ~46 ms latency (IN-02); scoped the WebView2 user-data folder to a plugin-specific child dir (IN-03); added a self-safe `numSteps <= 0` guard to `calculateCurrentStep` (IN-06); fixed the `"steps"` param version-hint typo `2`→`1` — verified no VST3/AU param-ID impact (IN-10); removed a dead `globalSteps` local and corrected a stale gain-smoothing comment (IN-13). No audio-path behavior change; auval passed.
- **2026-08-02 (v1.16.5):** Licensing release — no functional changes.
- **2026-08-13 (v1.17.0):** Fixed the tooltip shrink-to-fit measurement bug. The surface was measured at its *previous* `left`, so a `position:absolute` box with `width:auto` + `max-width:220px` reported an already-squeezed width near the right edge, and the edge clamp then placed it from that wrong number — self-reinforcing, so it never recovered. Also fixed the vertical placement (tooltips covered the control they described — 46 of 53), a sub-pixel `offsetWidth` rounding issue that re-wrapped the pinned box, and mouseout flicker between a control's own children. Verified in a browser harness at the true 850×550 editor size: overlaps 46→0, offscreen 5→0, squeezed 10→0. auval passed.

## Concept Summary

A rhythmic gate that combines FFT spectral processing with step sequencing. Different rhythmic patterns for different frequency regions, with Euclidean rhythm generation per band.

**Key Features:**
- 4-band spectral processing with configurable crossovers
- Per-band step sequencer (4/8/16/32 steps)
- Euclidean rhythm generation per band
- Visual frequency × time grid
- Tempo-synced with swing control
- Clear/Random buttons per lane (v1.1.0)

**Technical Approach:**
- Time-domain Linkwitz-Riley LR4 crossover tree (4 bands; binary split at c2 → c1/c3)
- Per-band step sequencer → asymmetric attack/release gain envelope → dry/wet mix
- **Zero reported latency** (`setLatencySamples(0)`; IIR LR filters have no fixed reportable latency)

> Historical note: an earlier design was FFT-based (2048-sample, 4× overlap, ~46 ms reported
> latency). That approach was replaced by the current time-domain LR topology — do **not**
> reintroduce a latency report. (Docs corrected v1.16.4, code-review IN-02.)

## Known Issues

None

## Known Limitations

- **Approximate unity-gain at rest (WR-09, v1.16.3).** The crossover engine is a **time-domain**
  Linkwitz-Riley LR4 binary tree (split at c2, then split each half at c1/c3) — *not* the FFT design
  described in the older Concept notes below. Because a single LR4 low+high sum is an allpass rather
  than identity, the tree sums two differently-phased allpass halves, producing a small magnitude
  ripple near the c2 crossover for **closely-spaced** crossover frequencies. It is negligible at the
  default well-separated crossovers (120/500/4000 Hz). This is an accepted tradeoff for a creative
  rhythmic gate; exact reconstruction would require per-path allpass compensation. Reported latency
  is correctly `0` (IIR LR filters have no fixed reportable latency).

## Additional Notes

### Design Decisions
- **Hybrid interaction model:** Discrete bands by default, paint mode planned for v1.2
- **Modular architecture:** Core v1.0 focused, expansion roadmap defined
- **All use cases supported:** Rhythmic gating, spectral animation, polyrhythmic layers

### Complexity Assessment
- **DSP:** High (FFT processing, multi-band, sequencing)
- **UI:** Medium-High (2D grid, real-time visualization)
- **Parameters:** ~165 total (many are step grid states)

### Future Versions
- v1.2: Paint mode for step grid, per-step attack/release
- v1.3: LFO modulation, envelope follower
- v2.0: Spectral freeze functionality
