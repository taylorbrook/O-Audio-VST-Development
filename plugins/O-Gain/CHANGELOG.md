# O-Gain Changelog

All notable changes to O-Gain are documented here.

## [1.3.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout. PATCH: no
parameter, range, type or state format changed; only French strings, and only
their values.

### Changed

- **19 French entries revised** against the suite glossary and lint, out of 63.
  Terminology: the Learn confidence verdict reads **Faible / Moyen / Élevé**
  rather than Bas / Moy / Haut (French says *confiance faible*, not *confiance
  basse*), and the tooltip that explains it now uses the same three words the
  cell shows; the settings caption is **Aide au survol** rather than the
  abbreviated "Aide", which also ends a disagreement with the tip title on the
  same control; the hover-help switch's accessible name is **Activer ou
  désactiver l'aide au survol**; one French word for "switch" (*interrupteur*)
  where two were in use. Typography: 19 no-break spaces before a colon, before
  a semicolon and between a number and its unit, and 7 typographic minus signs
  (−40 à +40 dB) in place of hyphens. Grammar and idiom: *recommandée* agreeing
  with *sonie*, *norme de l'industrie* for "industry standard", and *par paire*
  for the pleonastic *comme paire appariée*.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

## [1.3.0] - 2026-08-28

The page speaks French, not only the hover help — and the tooltip layer is
replaced rather than translated. Stage J of the repo-wide i18n task (canon v2),
plugin 7 of 7, on the narrowest frame in the suite. MINOR: no parameter IDs,
ranges, types or state format changed; existing sessions and presets load
unchanged, and a session saved before this version simply opens in English.

### Added

- **English + French across every visible string on the page.** 37 label keys
  and 26 tooltip keys in a new `Source/ui/public/js/i18n.js`. Meter captions,
  the gain-offset caption, knob captions, mode-selector captions, the whole
  Learn Analysis panel, the seven utility buttons, the Learn button's four
  faces, the confidence verdict and three accessible names all switch language
  with no reload and no English survivor. Counts parsed out of the RENDERED
  DOM, not grepped: 23 live tooltip anchors (23 unique strings) and 40 text
  nodes, against a plan figure of 23 / 41 and a raw grep of 34 attribute-token
  hits.
- **A settings popover with the language selector**, in the exact absolute slot
  the "?" help button occupied through v1.2.1 (`right: 8px; top: 0` inside the
  22 px header), so nothing on a 350 x 500 layout had to move to make room for
  it. The hover-help switch moves inside it. Every colour, border, radius and
  transition on the gear is the "?" button's own, carried across unchanged.
- **`getUiLanguage` / `setUiLanguage` native functions and session
  persistence.** This plugin had no bridge beyond `toggleLearn`, so the pair is
  new. The choice rides the session XML as a plain `uiLanguage` attribute —
  deliberately NOT an `AudioParameterChoice`, so it never appears in a host
  automation lane and no preset can change which language somebody reads their
  interface in. Read back with `isVoid()` and `toString()`, because a
  ValueTree property restored from XML is a string `var` whatever it was
  written as.
- `tests/i18n-states.json`, driving the settings popover and four Learn states
  so all 28 keyed elements — including the three confidence verdicts and all
  four Learn-button faces — are measured at 350 x 500 rather than shipped
  unmeasured.

### Changed

- **The hover help is the measure-then-pin renderer, ported from
  O-ReverseDelay. There is now ONE tooltip renderer repo-wide.** v1.2.1 had no
  tooltip JavaScript at all: its help was a pure-CSS `[data-tooltip]::after`
  pseudo-element with `content: attr(data-tooltip)` and three hand-picked
  direction override classes. It could not be measured, flipped, clamped or
  re-aimed. The port brings a title/body pair, a 120 ms dwell delay, a width
  RELEASED then MEASURED then PINNED before `left` is applied, an automatic
  vertical flip, a horizontal clamp, and an arrow offset recomputed AFTER the
  clamp so a clamped tip still points at its control. The old layer is DELETED,
  not disabled — `grep -rn 'tooltipHeight\|tooltipWidth\|data-tooltip'
  Source/ui/public/` returns nothing outside comments.
- **The 447-line inline `<script type="module">` is extracted to `js/app.js`.**
  Behaviour moved, not rewritten: the same listeners in the same order. Only
  the import specifier changed, and only because the module's depth did —
  `./js/juce/index.js` became `./juce/index.js`.
- **The Learn button's width is pinned to 121 px and the Target block to
  91 px.** `.learn-section` is 220 px of intrinsic-width button plus a `flex: 1`
  target group, so the button's own text positioned everything to its right.
  That was already a defect in English: the four faces measure LEARN 80.8,
  LEARNING... 120.7, DONE 73.4 and TOO QUIET 109.9 px, so the Target caption
  and its readout slid up to 40 px sideways every time Learn ran or finished.
  121 px is the widest ENGLISH face rounded up, so no English face re-wraps.
  Visible cost, stated rather than hidden: the resting LEARN pill is 40.2 px
  wider than in v1.2.1 and the Target block sits 40.2 px further right. That is
  the ONLY English-at-rest geometry change in this release, measured HEAD vs
  working tree over all 84 rendered boxes.
- `.meter-label` gains `white-space: nowrap`. The column is a hard 44 px and
  ENTRÉE renders 42.4 px inside it. Without nowrap a caption one glyph wider
  does not clip, it takes a second line and shoves the meter bars down. Costs
  English nothing (INPUT 33.3 px, OUTPUT 42.9 px).

### Fixed

- **The page no longer overflows its own frame.** v1.2.1's twenty-three in-flow
  `[data-tooltip]::after` boxes inflated the document's scroll extent to
  435 x 540 inside a 350 x 500 window, in English, at rest. The replacement
  surface is a single `position: fixed` element that is out of flow, so the
  scroll extent is now exactly 350 x 500 in both languages.

### Notes

- **All 63 French strings are machine drafts, every one flagged
  `reviewed: false`.** No native speaker has read them.
  `node scripts/check-i18n.js` prints the worklist.
- **All 23 tooltips were HAND-SPLIT; none split on a `": "`.** O-Gain's copy is
  bare sentences with no title prefix, so every tooltip title is the control's
  own existing English caption reused verbatim. Four strings DO contain a
  `": "` — the two `Range: ` tips, `Sum to mono: ` and the DEC `Note: ` — and a
  mechanical split on the first one would have made most of the sentence into
  the title. All 23 bodies are byte-identical to v1.2.1's attribute values.
- **The M/S captions localize and the meter/measure captions do not**, on the
  byte-identity test against the `AudioParameterChoice` option strings. `Peak`,
  `RMS`, `VU` and `LUFS` match `meter_mode` and `measurement_mode` verbatim and
  are `I18N_EXEMPT` under D-01 — translating them would make the page and the
  host automation lane disagree. `M/S OFF`, `ENC` and `DEC` match none of
  `ms_mode`'s `Off` / `Encode` / `Decode`, so they are page-invented captions
  and they translate.
- The seven-button utility row already renders PH L, PH R and M/S OFF on two
  lines in ENGLISH at this frame — 344.8 px of min-content in a 334 px row.
  That is pre-existing and untouched; every French caption was chosen so its
  widest word matches its English counterpart's, so the row keeps exactly two
  lines and its 32 px height.
- The vertical tooltip clamp is carried in from O-FreqPulse but is **not
  independently reproducible on this page** — measured, with the clamp deleted,
  over 52 tips with the Learn panel open and 40 with it closed: none left the
  frame. The same sweep with the HORIZONTAL clamp deleted instead reports 26
  off-frame tips, worst 89.3 px, so the probe is not blind.
- The hover-help switch is still SESSION-ONLY, exactly as v1.2.1's "?" was.
  This plugin has no `setTooltipsEnabled` bridge and this release does not add
  one.
- The DAW round-trip (pick Français, close the session, reopen) has not been
  executed by hand. It is reasoned from the `isVoid()` guard, not measured.

## [1.2.1] - 2026-07-21

Learn-mode safety fix. No parameter IDs, ranges, types, or state format changed —
v1.0.0 / v1.1.0 / v1.2.0 sessions and presets load unchanged.

### Fixed

- **Learn no longer slams `gain_offset` to the +40 dB max (blasting/clipping the
  output) when engaged over an unrepresentatively quiet passage.**
  Root cause: `finalizeLearn()` normalizes to the loudness measured *only during the
  learn window*, and the inter-sample-peak anti-clip ceiling only protects peaks seen
  *in that same window*. A quiet-but-valid capture — a soft intro, a low noise floor,
  anything above the −70 LUFS "invalid" floor — implied a normalization boost all the
  way to the +40 dB parameter maximum; the ISP ceiling happily allowed it because the
  window's own peaks were low, then real (louder) material clipped hard when it played
  through the boost. Confidence was computed but never gated the magnitude of the write.

  **Fix:** a new `kMaxLearnBoostDB` (**+24 dB**) caps the *automatic* learn-derived
  boost. When the raw normalization boost (`target − measured`, before the ISP ceiling)
  exceeds the cap, Learn treats the capture as too quiet / unrepresentative and **does
  not write `gain_offset`** — it publishes the existing complete + none-confidence
  snapshot instead. The Learn button now reads **"TOO QUIET"** (rather than a
  misleading "DONE") so the user knows to re-run Learn over a louder / representative
  section, or set the gain manually. The manual `gain_offset` slider keeps its full
  ±40 dB range — the cap applies only to Learn's automatic write. The ISP ceiling and
  all prior guards (−70 dB floor, 1 s minimum capture) are unchanged.

### Notes

- UI change is display-only (button label reuses the already-published `learnState` /
  `learnConfidence` fields — no new native-function or snapshot plumbing). A successful
  Learn still keeps its low/medium/high confidence and reads "DONE".

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
