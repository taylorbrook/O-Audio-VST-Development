# O-Gain Changelog

All notable changes to O-Gain are documented here.

## [1.2.0] - 2026-07-01

Second code-review remediation pass (WR-03, WR-04, WR-05, IN-01…IN-06). Metering
label/behavior changes are user-facing → MINOR bump. No parameter IDs, ranges,
types, or state format changed — v1.0.0 / v1.1.0 sessions and presets load unchanged
(verified with pluginval strictness 5 + auval).

### Fixed

- **WR-03: the "True Peak / dBTP" readout is now labeled "Sample Peak / dBFS".**
  The metric was never oversampled — it is the max absolute *sample* value, so
  inter-sample peaks (which can exceed the sample peak by several dB) were never
  measured, yet it was presented as dBTP and used as a -1 dBTP safety ceiling. The
  label, tooltip, and internal names (`samplePeakMax` / `samplePeakDBFS`) now say
  dBFS, and the Learn safety ceiling drops an extra **3 dB** below -1 dBFS
  (`kInterSamplePeakHeadroomDB`) to cover the un-measured ISPs. No oversampling was
  added — relabel + headroom only.

- **WR-04: the running integrated-LUFS recompute no longer runs every 100 ms hop
  on the audio thread.** `calculateIntegratedLUFS()` is O(gatingBlockCount) (two
  passes over up to 4000 blocks) and was recomputed on every hop, so per-hop RT cost
  grew with learn duration. It is display-only, so it is now throttled to ~1 Hz
  (`kIntegratedRecomputeHops`); the authoritative final value is still computed once
  in `finalizeLearn()` (off the audio thread, per v1.1.0 CR-01).

- **WR-05: the Learn panel is published to the UI as one coherent snapshot.**
  The editor timer previously read ~17 independent atomics, so learnState /
  confidence / integrated / momentary could come from different `processBlock`
  iterations (e.g. "DONE" shown against a stale integrated value). The Learn-panel
  fields are now published together through a seqlock (`LearnSnapshot` +
  `learnSnapshotSeq`) and read coherently by the editor. Plain peak / RMS / VU meters
  remain independent atomics.

- **IN-01: removed the dead/empty branch in the `toggleLearn` native function.**

- **IN-02: `learnState` now clears back to idle when the gain is edited manually.**
  After a Learn completed, the button read "DONE" indefinitely. Editing `gain_offset`
  or `trim` (a real user edit, not Learn's own write — guarded by
  `ignoreLearnGainWrite`) now resets the display to idle.

- **IN-03: meter mode "LUFS" now shows momentary loudness during Learn.**
  It previously always showed RMS. It now drives the input meters from momentary
  K-weighted LUFS while Learn runs, falling back to RMS otherwise; the meter tooltip
  documents this.

- **IN-04: `withBackend(webview2)` (and its WinWebView2 options) are guarded with
  `#if JUCE_WINDOWS`** so the editor no longer references a Windows-only enum on macOS.

### Changed

- **IN-05: the M/S "DEC" tooltip documents the +6 dB behavior.** Decoding a normal
  (un-encoded) L/R signal raises level by +6 dB — the inverse of ENC's -6 dB — so
  ENC→DEC is a matched unity pair. DSP convention unchanged (no rescale).

- **IN-06: promoted magic numbers to named constants.** Gain ramp (0.02 s), VU
  ballistics (300 ms), LUFS block/hop (0.4 s / 0.1 s), gating capacity (4000),
  confidence thresholds (5 s / 15 s / 50 blocks), and the JS meter range (-60…0 dB) /
  clip threshold (-0.5 dB) are now `constexpr` / JS consts with unit comments.

### Notes

- Real-time safety preserved: the per-hop audio-thread path only writes a seqlock
  (no allocation, locking, or host notification).

## [1.1.0] - 2026-07-01

Code-review remediation (CR-01, CR-02, WR-01, WR-02). Adds working mono support
for VU metering and the Learn auto-gain feature (behavior change → MINOR bump).

### Fixed

- **CR-01 (critical): `finalizeLearn()` no longer runs on the audio thread.**
  Stopping Learn previously called `setValueNotifyingHost()` — a locking/allocating
  host + listener notification path — directly from `processBlock`, along with the
  log10/gate loops over up to 4000 gating blocks. This violated real-time safety and
  risked priority inversion / dropouts. The processor now derives from
  `juce::AsyncUpdater`: the audio thread only detects the learn-stop edge, sets a
  `learnFinalizePending` flag, and calls `triggerAsyncUpdate()` (once, not per block);
  the full measurement + gain write happens in `handleAsyncUpdate()` on the message
  thread. Robust even when the editor is closed.

- **CR-02 (critical): Learn can no longer slam `gain_offset` to +40 dB.**
  Learning over silence, near-silence, or (previously) any mono instance produced a
  measured level of −100 dB, driving the computed gain to the +40 dB clamp — a
  full-scale output and hearing-safety hazard. `finalizeLearn()` now bails out
  (confidence → none, `gain_offset` left untouched) when there is no valid
  measurement (`gatingBlockCount == 0` / `rmsSampleCount == 0`), the measured level is
  ≤ −70 dB, or the capture ran shorter than a 1 s minimum. The measured LUFS / true
  peak are still published to the UI so the user sees why nothing changed.

- **WR-01: peak-hold meters no longer freeze.**
  The ~300 ms decay coefficient is per-sample but was applied only once per block
  (~0.6 %/s at 512-sample blocks), so the peak latched at its maximum and its decay
  varied with block size. Decay is now a proper per-block coefficient
  (`pow(perSampleDecay, numSamples)`) in both the input and output metering stages.

### Added / Changed

- **WR-02: VU metering and Learn now function on mono instances.**
  Both were gated on `numChannels >= 2` and were silently dead in mono (the direct
  cause of the CR-02 mono +40 dB slam). Added a mono path that drives the VU
  ballistics filter from channel 0 (mirrored into both meters) and feeds channel 0
  into the K-weight / LUFS and RMS accumulators. The active channel count is snapshotted
  at learn start (`learnChannelsAtStart`) so the RMS divisor and LUFS block power yield
  the correct single-channel loudness rather than a 3 dB error.

### Notes

- No parameter IDs, ranges, types, or state format changed — v1.0.0 sessions and
  presets load unchanged.
- Real-time safety verified: no allocation, locking, or host notification remains in
  `processBlock`.

## [1.0.0] - 2026-03-07

Initial release. Stereo gain-staging utility with channel utilities (phase invert
L/R, channel swap, mono sum, M/S encode/decode), BS.1770 K-weighted LUFS "Learn"
auto-gain, VU / peak / RMS metering, and a WebView UI.
