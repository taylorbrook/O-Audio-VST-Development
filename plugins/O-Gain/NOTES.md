# O-Gain Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.9.2
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

- **2026-09-12 (v1.5.0):** Metering read-out overhaul. A held peak cap now rides over
  the average bar in every mode (reusing the existing decayed-peak atomics), each column
  gained a 14 px dB scale gutter with gridlines at 0/-6/-12/-18/-24/-36/-60, the -18 to
  -12 dBFS staging band and the -6 dBFS mix-bus line are drawn on the bar, and the single
  rounded readout became a peak/average pair. Fixed VU mode comparing a 300 ms ballistic
  input against a per-block RMS output by adding post-gain VU ballistics (STEP 7). Frame
  350 x 500 -> 380 x 500. No parameter, range, type or state format changed.
- **2026-09-12 (v1.6.0):** LUFS meter mode shows LUFS. The BS.1770 K-weight chain ran
  only inside the Learn branch, so with Learn idle `case 3` drew RMS under a LUFS label on
  both columns. The chain now runs continuously as a `MomentaryLoudnessMeter` struct with
  two instances — pre-gain (Learn consumes its closed blocks via `onLearnHop()`) and a new
  post-gain one (STEP 8) — publishing `momentaryLufsIn` / `momentaryLufsOut`. Offline
  `tests/lufs-harness` (console target, independent BS.1770 reference) proves both columns
  track -18 LUFS pink noise to 0.2 dB with Learn idle, and -12 at +6 dB gain. No parameter,
  range, type or state format changed.
- **2026-09-12 (v1.7.0):** Target presets. Six buttons under the Target Level knob
  (dBFS -18/-16/-20, LUFS -14/-16/-23 — BRIEF.md's unshipped "Target level selector with
  presets") write `target_level` through the knob's own relay as a gesture; the lit preset
  is the one `getScaledValue()` matches within 0.05 dB. UI-only, keyed in en/fr/zh-Hans,
  check-ui-labels 0 FAIL at 380 x 500. No parameter or state change.
- **2026-09-12 (v1.8.0):** Dial legibility and placement. The three dials now share one
  220 px row — Trim | Gain | Target, the large dial in the middle — top-aligned so the
  captions sit on one baseline and the readouts on the next; the Target readout moved from
  beside LEARN to under its own dial and LEARN is centred alone. Captions 9 → 10 px in
  #5C4033 (were the #8B7355 tan), readouts 10 → 11 px semibold. `label.targetLevel` and
  the `target-group` tooltip retired (dead after the merge). check-i18n / fr / zh lints
  and check-ui-labels all 0 FAIL at 380 x 500. UI-only; no parameter or state change.
- **2026-09-12 (v1.8.1):** Presets title centred; peak / avg meter readouts 8 → 10 px
  (caption column re-pinned 20 → 25 px, v1.8.0 colours); Measure / Meter row and the
  Learn Analysis panel centred with `align-self: center` — `.container` never centred its
  children, so both 300 px blocks sat at the left. check-ui-labels 0 FAIL. UI-only.
- **2026-09-12 (v1.9.0):** The four meter methods made visibly different. A Peak | RMS | VU |
  LUFS method strip of live In / Out readouts under the presets (all four values were already
  in every payload), the lit column the mode the bars draw; the ruler and marks follow the
  mode — dBFS with the staging band and bus line in Peak / RMS only, VU with 0 VU at -18 dBFS
  and a dashed reference line, LU against `target_level` with a target line on the output
  bars in LUFS; the second readout caption is the mode's name and unit (RMS in Peak mode, VU
  signed). `label.avg` retired. check-i18n / fr / zh lints and check-ui-labels (11 states)
  all 0 FAIL at 380 x 500. UI-only; no parameter or state change.
- **2026-09-12 (v1.9.1):** Measure / Meter captions and the Settings cog (glyph and ring) black
  (`#000000`, were the `#8B7355` tan); cog hover is an 8 % black wash instead of the brown
  swap. CSS-only.
- **2026-09-12 (v1.9.2):** Input / output dB readouts 10 -> 13 px and black (`#000000`,
  were `#3C2F2F`), line-height pin re-derived to 15.00 / 13 = 1.1538. The meter columns
  had to go 58 -> 72 px to carry it: at 58 the value cell was 30 px and its widest
  reachable string -- the signed VU reading `+24.0`, not `-60.0` as the v1.8.1 note
  assumed -- measured 30.09 px of ink, so the readout had 0.01 px of room. Caption column
  25 -> 27 px (`LUFS` was overrunning it by 1.02 px). Side effect: `.meter-pair` is
  `flex: 1`, so the bars went 19 -> 26 px wide and the centre section 232 -> 204 px.
  60 readout cells measured across 4 meter modes x en / fr / zh-Hans, minimum slack
  2.89 px. CSS-only.

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
