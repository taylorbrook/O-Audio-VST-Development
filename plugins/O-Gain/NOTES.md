# O-Gain Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.2.1
- **Type:** Audio Effect (Gain Staging Utility)

## Lifecycle Timeline

- **2026-03-07 (Stages 0–4):** Implemented and installed. Stereo gain-staging utility
  with channel utilities (phase invert L/R, channel swap, mono sum, M/S encode/decode),
  BS.1770 K-weighted LUFS "Learn" auto-gain, VU/peak/RMS metering, WebView UI.
- **2026-07-01 (v1.1.0):** Code-review remediation (CR-01, CR-02, WR-01, WR-02).
  Moved Learn finalization off the audio thread (AsyncUpdater), guarded Learn against
  silence/mono/near-silent captures so it can no longer slam gain to +40 dB, fixed the
  frozen peak-hold meters (per-block decay), and added working mono VU + Learn support.
- **2026-07-01 (v1.2.0):** Second code-review pass (WR-03, WR-04, WR-05, IN-01…IN-06).
  Relabeled "true peak / dBTP" → "sample peak / dBFS" + added 3 dB ISP ceiling headroom;
  throttled the running integrated-LUFS recompute to ~1 Hz on the audio thread; made the
  Learn panel a seqlock-published coherent snapshot; cleared "DONE" on manual gain edit;
  LUFS meter mode shows momentary loudness during Learn; guarded the webview2 backend with
  `#if JUCE_WINDOWS`; documented the +6 dB M/S DEC behavior; promoted magic numbers to
  named constants. Verified pluginval strictness 5 + auval.
- **2026-07-21 (v1.2.1):** Learn safety fix. The v1.1.0 silence/near-silence guard did
  NOT cover a *quiet-but-valid* capture (soft passage / low noise floor above the −70 LUFS
  floor): Learn still derived a boost to the +40 dB max and clipped when louder material
  played through it. Added `kMaxLearnBoostDB` (+24 dB) — when the raw normalization boost
  (`target − measured`, before the ISP ceiling) exceeds the cap, Learn refuses to write
  `gain_offset` and the button reads "TOO QUIET" so the user re-runs over a louder section.
  Manual `gain_offset` keeps its full ±40 dB range. Verified auval PASS (Component Version
  1.2.1 = 0x10201).

## Known Issues

All code-review findings (CR-01/02, WR-01…05, IN-01…06) are now resolved across v1.1.0
and v1.2.0. One conscious non-defect remains:
- **WR-03 scope:** the Learn safety metric is a digital *sample* peak (dBFS), not an
  oversampled true peak (dBTP). Inter-sample peaks are not measured; instead the Learn
  ceiling reserves ~3 dB of extra headroom below -1 dBFS to cover them. If true dBTP is
  ever required, add `juce::dsp::Oversampling` (≥4×) before peak detection.

## Additional Notes

- No parameter IDs, ranges, types, or state format changed in v1.1.0, v1.2.0, or v1.2.1 —
  v1.0.0 sessions and presets load unchanged.
- Learn-panel coherence (WR-05): all Learn readouts are published together via a seqlock
  (`LearnSnapshot` + `learnSnapshotSeq`, single-writer-at-a-time, serialized by
  `learnActive` / `learnDisplayState`). Plain peak/RMS/VU meters stay independent atomics.
- Mono loudness semantics: a mono capture uses single-channel BS.1770 loudness (channel
  0 fed to the L path only), which reads ~3 dB below the same signal presented as
  dual-mono stereo. This is intended.
- AU identifier: `aufx OGan OuDv` (dev build) / `aufx OGan OuAu` (release).
- Backups of pre-change source: `backups/O-Gain/v1.0.0/`, `backups/O-Gain/v1.1.0/`.
