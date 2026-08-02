# Changelog — O-simpleSampler

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.3.0] — 2026-08-02

Header layout repair plus a tooltip on/off switch. UI-only: no DSP, no parameter,
and no preset change. The APVTS contract stays at **20 parameters** with identical
string IDs, ranges and defaults, so sessions, presets and automation written by
1.2.0 restore unchanged.

### Added
- **A "?" button in the header that switches the hover explanations off and on.**
  Tooltips remain **on by default** — a fresh instance behaves exactly as 1.2.0 —
  and the choice is **saved with the DAW session**, so a project reopens the way it
  was left instead of switching the explanations back on every time. Pressed/green
  means explanations are on.

  The button gates **both** hover surfaces: the floating rich tooltip *and* the
  native `title=` fallback the same layer installs. Stripping only the first would
  leave the OS tooltip popping up on hover with the switch visibly "off". The
  authored copy is parked in `data-tip-title` and restored verbatim when tips are
  switched back on (34 elements, round-trip verified).

  The flag is stored as a custom `<UI tipsEnabled="…"/>` child of the saved state
  tree, next to the existing `<SOURCE identity="…"/>` — deliberately **not** an
  APVTS parameter. That keeps the automatable parameter count at 20 (a 21st
  parameter would be a contract change and would show up in every host's automation
  list for a preference), and it keeps `applyFactoryPreset` — which writes
  parameters only — from resetting the user's choice on every concept-preset click.
  State written before 1.3.0 has no `<UI>` child and restores to the default (on).

  The button deliberately carries no `data-tip` of its own: the tooltip layer owns
  the `title` attribute of every `[data-tip]` element and strips it when tips are
  off, which would leave this button unlabelled in exactly the state it exists to
  undo. Its own `title` is authored in the HTML and never touched.

### Fixed
- **The "O – simpleSampler" title no longer breaks across two lines**, and neither
  does the "Keyboard Sampler · A Field Guide" subtitle. `.title-block` is a flex
  item beside the preset bar, which claimed the rest of the header row and squeezed
  the title to 207 px; the hair spaces around the en-dash are break opportunities,
  so the title folded after "O –". Sizing the block to its content (`flex: 0 0 auto`)
  and pinning both lines with `white-space: nowrap` holds the title at its natural
  246 px.
- **All seven concept-preset chips now sit on one row.** The chip bar was capped at
  `max-width: 460px` — 17 px short of the 477 px the row actually measures — which
  orphaned "Filtered" onto a second row. The cap is removed and the bar is
  `flex-wrap: nowrap`.
- **The header row is right-aligned explicitly.** The chip bar used to be padded out
  to its fixed 460 px cap and right-aligned its own contents, which parked it near
  the header's right edge as a side effect. With the bar now exactly its content
  width, `.preset-bar-container` needs `justify-content: flex-end` or the row packs
  left and strands ~140 px of dead space before the header edge.

Net effect: the header is **56.7 px tall instead of 94 px**, and the 37 px it gives
back is why the on-screen keyboard is no longer clipped at the bottom of the 980×720
editor.

### Testing
- Rendered against a JUCE-bridge stub at the editor's real 980×720 and measured:
  title 29.5 px (one line, was 59), subtitle 13 px (one line, was 26), 7 chips on
  1 row, "?" flush to the header's right edge (0 px gap), no horizontal overflow.
- Tooltip toggle, all verified in the rendered page: default on; off strips all 34
  `title` attributes and suppresses the floating tip; back on restores all 34
  verbatim; an already-open tip retracts the moment tips are switched off; a session
  restored with tips off boots with tips off and can still be switched back on; the
  C++ `tipsEnabledChanged` push applies in both bare and array-wrapped `var` shapes;
  clicking a concept preset does not reset the choice.
- WebView bridge grep-diff: 10 `getNativeFunction` calls ↔ 10 `withNativeFunction`
  handlers, no orphans either way; `tipsEnabledChanged` is emitted and listened for.
- **New render-harness test `tips-state-roundtrip`** (harness is now 10/10, was 9/9).
  Neither auval nor any render test can observe a non-parameter state child, so the
  save/restore contract is asserted directly: off→off, on→on, a fresh instance
  defaults to ON, pre-1.3.0 state with no `<UI>` child restores to ON rather than
  leaking the instance's current value, and a witness parameter still round-trips.
- auval: **VALIDATION SUCCEEDED**, 20 Global Scope Parameters.
- pluginval strictness 10, 3 consecutive runs: **SUCCESS** (exit 0, no failures).
- Offline render harness: **10/10 PASS**, 0 failures.
- AU bundle version verified 1.3.0 / 66304.

## [1.2.0] — 2026-08-02

First published release. Neither 1.0.0 nor 1.1.0 was ever distributed — no tag, no
GitHub Release, no binary shipped to anyone. 1.2.0 is the version under which the
1.1.0 content withdrawal reaches its first actual release. No code, DSP, parameter,
or UI change since 1.1.0: the 20-parameter APVTS contract, the piano-only built-in
source set, and the 7 factory presets are exactly as verified for 1.1.0 (auval
20 params, pluginval strictness 10 ×6, offline render harness 9/9).

### Added
- `Source/samples/LICENSE.md` — provenance and licensing for the built-in sample
  set: the surviving piano source is documented as procedurally generated and
  self-authored, and the three commercial-library sources withdrawn in 1.1.0 are
  recorded as removed rather than replaced.

## [1.1.0] — 2026-08-01

Content and contract change ahead of the repository going public. No release ever shipped
v1.0.0, so nothing distributed is affected by any of the removals below.

### Removed
- **Three built-in sources — `cello`, `pizz`, and `hit`.** These assets originated in a
  commercial sample library whose redistribution rights were never established, so they
  could not remain embedded in a plugin binary that is about to be published. They have
  been **removed rather than replaced**: substituting generated or CC0 audio was
  considered and deliberately declined. The built-in source set is now **piano only**
  (recorded root 48), which is procedurally generated and provably self-authored — see
  `Source/samples/LICENSE.md`. No release of this plugin ever shipped the withdrawn
  assets; they were present only in the repository.
- **The `sourceSample` Source parameter** (APVTS contract **21 → 20 parameters**). With a
  single built-in remaining, keeping the selector as a one-entry choice is not an option:
  JUCE's `AudioParameterChoice` asserts `choices.size() > 1`, and a one-entry choice builds
  the degenerate `NormalisableRange {0, 0}` whose `convertTo0to1` is `0/0` — a NaN that
  `jlimit` does not clamp, so the parameter would be born holding NaN in a Release build.
  The parameter is therefore dropped outright. The surviving 20 parameters keep their
  string IDs and versioned `ParameterID`s unchanged, so VST3/AUv2 automation IDs (hashed
  from the string ID, not the index) are stable. A v1.0.0 session carrying a `sourceSample`
  entry restores harmlessly — the orphan child is inert.
- The Source group's built-in `<select>` combo in the WebView UI, which had nothing left
  to select. Two combos remain (Loop Mode, Pitch Mode).

### Changed
- The plugin now **starts on its one built-in source** with no user action; the recorded
  root (48) is still seeded on a fresh instance so it plays in tune immediately.
- **`Load…` and drag-and-drop are now the only way to change the source**, and are
  otherwise unchanged — WAV / AIFF / FLAC, resampled to the engine rate, 30 s cap. The
  Source group's status line is now seeded at boot with the active built-in's name so a
  fresh instance reads as loaded rather than blank.
- The 7 factory presets are unchanged. All of them were already source-agnostic (none set
  the removed parameter), and the central post-reset root re-seed still runs, so none play
  octave-flat.

## [1.0.0] — 2026-06-26

First release. O-simpleSampler is the sampler sibling to O-simpleFM / O-simpleAdditive /
O-simpleGrain / O-simpleSubtractive — a deliberately simple, pedagogical keyboard
sampler that strips the software sampler down to its spine: **a recording, a region of
it, a root key, a loop, and an envelope**, each with a visible consequence on the
waveform. Built for the MUSC319 wk05 sampling session. Cross-platform WebView UI
(macOS AU + VST3, Windows VST3 via WebView2).

The 21-parameter APVTS contract is frozen for v1.0.

### Added — instrument (Stages 1–3)
- **Polyphonic keyboard sampler** (16-voice) — the active source read through a
  fractional-read varispeed head, region-isolated, anti-aliased, shaped by a per-voice
  amp ADSR + VCA + velocity sensitivity, tuned to the live **Root Key**.
- **Curated built-in source set** (FUNC-02): **piano** (root 48), **cello** (root 69),
  **pizz** (pizzicato strings, root 69), and **hit** (percussive one-shot, neutral root
  60). Selecting a source seeds its recorded-pitch root so it plays in tune. Roots were
  probed via YIN f0 estimation. Embedded as a second `juce_add_binary_data` target with
  a distinct `BinaryData` namespace (UI resources use `UIBinaryData`).
- **Load your own sound** (FUNC-03) — drag-and-drop (macOS WKWebView content-streaming
  bridge) **and** a file-picker fallback; WAV / AIFF / FLAC, resampled to the engine
  rate, capped at 30 s.
- **Region** — Start/End to isolate the useful region; **Loop** (Off / Forward /
  Ping-Pong) with equal-power crossfade + zero-cross marker snapping; **Reverse**.
- **Pitch** — **Repitch** (honest varispeed) ↔ **Stretch** (SOLA pitch/time
  independence), plus Root Key, semitone Tune, and fine cents.
- **Vintage** — SP-1200-style sample-rate decimation + bit-crush (clean at 0).
- **Filter** — resonant TPT low-pass with a live-matching display curve.
- **Interactive waveform editor** — draggable start/end + loop handles (two-way via
  relays), shaded loop band, live playhead, root-key indicator, filter curve, amp-ADSR
  scope. 34/34 controls tooltipped.
- **Concept preset tour** (FUNC-07) — 7 named factory presets, each isolating one
  sampler concept: Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell,
  Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped. Each resets to defaults,
  re-seeds the active source's recorded root (so nothing plays octave-flat), then sets
  only the parameters that isolate its concept; the WebView relays resync every control.

### Changed / hardened (Stage 4 — Polish)
- **RT-safe source-swap handoff.** The audio thread now reads the live source through a
  raw `std::atomic<AudioBuffer*>` (acquire) and never holds a `shared_ptr`, so a swap can
  never free a buffer in the render path. The previous buffer is held one generation
  (`retiredSource`) and freed off the audio thread; the owning shared_ptrs are guarded by
  a `CriticalSection` taken only off the audio thread (replaces the deprecated
  C++20 `std::atomic_load/store(shared_ptr)` helpers).
- **Deferred fresh-instance root seed.** The one-time per-source root seed no longer calls
  `setValueNotifyingHost` inside `prepareToPlay` (discouraged — prepare can run off the
  message thread / during scans); it is deferred to the existing AsyncUpdater
  (guaranteed message thread).
- **Render-harness re-armed.** Dropped `PluginEditor.cpp` from the harness target so the
  offline DSP correctness gate stays buildable now that the editor uses WebView types
  under `JUCE_WEB_BROWSER=0` (`createEditor()` falls back to `GenericAudioProcessorEditor`).

### Validation
- Offline render-harness: **9/9 PASS** (makes-sound, Repitch tuning, Stretch
  pitch-tracks-key, Stretch time-independence, loop-seam continuity fwd + ping-pong,
  region-end declick, Vintage clean-at-zero, anti-alias up-transpose, stress bound).
- `auval` SUCCEEDED — **21** Global Scope Parameters.
- pluginval strictness 5 — exit 0 on **both** VST3 and AU.
- Native-fn WebView bridge: 8 JS ≡ 8 editor ≡ 8 processor, 0 orphans.

### Notes
- **Windows (COMPAT-02):** WebView2 wiring is in place (`NEEDS_WEBVIEW2` +
  static-linking flag, dual `BinaryData`/`UIBinaryData` namespaces). Runtime
  verification on a Windows host/DAW is pending user testing — not a CI gate for v1.0.
