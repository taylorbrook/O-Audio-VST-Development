---
status: issues_found
plugin: O-MicrotonalSampler
version_reviewed: 1.23.0
depth: deep
date: 2026-06-30
reviewers: 5 (parallel gsd-code-reviewer, domain-split)
files_reviewed: 15
findings:
  critical: 3
  warning: 18
  info: 22
  total: 43
---

# O-MicrotonalSampler — Full-Instrument Code Review (v1.23.0)

Deep review of the whole instrument, split across 5 parallel domain reviewers.
Per-domain detail in the sibling `REVIEW-*.md` files. This is the consolidated,
severity-ranked digest.

**Headline:** DSP real-time discipline in `processBlock` and the WebView bridge are
solid — every documented project gotcha (native-fn registration, JUCE base64 decoder,
resource-provider bare-path matching, `Juce` ES-module namespace, XSS) was traced and
verified handled. The real risk cluster is **state serialization** and **loader-thread
lifecycle**, plus a stale-state audio-thread use-after-free in the CC-crossfade voice.

---

## 🔴 CRITICAL (3) — fix before next release

### C1 — Stale CC state → audio-thread use-after-free / stuck voice
`MicrotonalSamplerVoice.cpp:434-443` and `:473-482` (domain: DSP/voice)
The two `startNote` failure early-returns clear the velocity-path pointers but **not**
`ccDynamicsActive` / `dynLayerCount`, and do not call `adsr.reset()`. `renderNextBlock`
dispatches on `ccDynamicsActive` (line 743) *before* the `variantLow`/`adsr` guard, so a
failed note-start after a CC-mode note leaves the voice rendering from `dynLayers[k]`
pointers into a `SampleMap` whose last ref was just released (common right after a
ReplaceAll). `renderCcCrossfade`'s own guard doesn't fire because `adsr` was never reset.
**Fix:** add `ccDynamicsActive = false; dynLayerCount = 0;` to both failure blocks (match `stopNote`).

### C2 — Embedded-audio state round-trip silently drops `SampleCell.technique`
`PluginProcessor.cpp:2634-2635` (write) / `:2675-2676` (read) (domain: processor/state)
Serialization persists only `midi` + `layer`. `technique` is part of the cell key
(`SampleMap.h:137`), so any embedded folder on a non-zero technique reloads with **every
cell collapsed onto technique 0** — silent sample-map corruption and possible collisions
with real `ord` cells on project reopen. `kickNextReplayOp` never re-derives it.
**Fix:** add a `tech` attribute in both write and read paths.

### C3 — `stopThread(500)` force-kills the loader mid-decode → leak + UI hang
`SampleLoader.cpp` (load entry points) (domain: loading/parsing)
`processOneFile` has no cancellation checkpoint inside its read/resample, so the 500 ms
timeout escalates to `pthread_cancel` (macOS, confirmed JUCE `Threads_mac.mm:172`). On the
large libraries this plugin targets (3–5 s for 250 MB per project memory), a second load or
project-close mid-decode leaks the reader, half-mutates state, abandons I/O locks, and never
fires the completion/failure callback → **UI hangs**.
**Fix:** add a `threadShouldExit()` checkpoint inside the per-file loop; lengthen/negotiate shutdown.

---

## 🟠 WARNINGS (18)

### State & persistence (processor)
- **W1** `captureStateValueTree` reads `techniqueNames` + `TuningEngine` state **without `persistenceLock`** — off-message-thread `getStateInformation` (Reaper, the codebase's own HG-08 case) can tear-read/crash during a concurrent rename/tuning edit. Lock is applied to `loadOpHistory` but not its siblings. `PluginProcessor.cpp`.
- **W2** Preset load bleed-through: `restoreStateValueTree` resets trims but not `techniqueNames` or CC/PC tables → a partial `.omspreset` leaves stale values from the previous preset.
- **W3** Fresh-instance default `ks_enabled=true` (range 0-9) silently **absorbs MIDI notes 0-9 as keyswitches**; the adjacent comment claims the opposite back-compat default.
- **W4** Large embedded-library WAV re-encode runs on the host's `getStateInformation` thread (save-latency hazard; correctness OK).

### Loader & parsing
- **W5** Untrusted-header allocation with no cap; `bad_alloc` swallowed by JUCE's `catch(...)` in Release → entire folder load aborts with zero feedback (no per-file try).
- **W6** `int64→int` truncation of `lengthInSamples` mis-sizes very long files.
- **W7** `ceil`-based resample length → ~1-sample heap over-read of interpolator input on odd-length SR conversion (44.1→48 kHz, the common case).
- **W8** RR split-form (`take_60`) double-consumes a bare-integer note token, fabricating an RR index.

### DSP / voice
- **W9** Loop crossfade clicks: 8-entry equal-power LUT built at `i/8` caps at 7/8, never reaching full incoming weight at the wrap point → per-cycle click on sustained looped samples. Drive with continuous phase reaching 1.0.
- **W10** `prevMap` destruction at `startNote` return can free a whole `SampleMap` + buffers on the RT thread (reload boundary) — needs a message-thread reaper. (Related to C1.)
- **W11** `hostSR==0` → `+Inf` playRate → non-terminating `wrapLoopPosition` while-loop (hard lockup). Low probability, guard the divisor.

### Editor / WebView C++ bridge
- **W12** *(most serious in this domain — conditional UAF)* ~9 `chooser->launchAsync` completions capture raw `this` and call back after possible editor teardown; `chooser` is kept alive by shared_ptr but `this`/WebView are not. **Fix:** `juce::Component::SafePointer`. `PluginEditor.cpp`.
- **W13** Silent data loss: `saveScalaFile` (1599), `saveKBMFile` (1631), `exportTuningHTML` (1888) discard the `replaceWithText` bool and always report success. `saveCurrentPreset` (1756) does it right — inconsistent.

### WebView frontend (JS/CSS)
- **W14** ADSR knob readouts wrong: `KNOB_FORMATS` says attack/decay/release are `0.001–5.0 s` linear, but the real `NormalisableRange` is `(0.0, 10.0, 0.001, 0.5f)` (max 10 s, skew 0.5). Full knob shows `5.00 s`, real value is `10.0 s` — off by up to 2×. Audio unaffected; label misleads.
- **W15** `subscribeTechniqueStateUpdates` / `subscribeTriggerStateUpdates` dereference `.backend` without the guard every other subscriber uses → latent boot-time `TypeError`.
- **W16** CC/PC trigger tables `innerHTML=''`-rebuild on every state echo with no `activeElement` guard → in-progress edits and focus clobbered. (The v1.23 trim panel already solved this pattern — reuse it.)
- **W17** Knob double-click "reset" snaps to normalised 0.5 instead of the parameter default (moves attack to ~2.5 s).
- **W18** Knob wheel edits not wrapped in `sliderDragStarted/Ended` → some hosts won't record them as automation.

---

## 🟡 INFO (22) — polish / latent

**DSP:** equal-power +3 dB bump on correlated layers; RR counter advanced for skipped degenerate layers; stale "squared CC gain" comment vs. shipped dB-linear path.
**Loader:** non-recursive folder enumeration silently drops nested samples; mixed explicit-RR/no-token groups suppress the ambiguity modal; single-letter pre-note dynamics false positives (`F-C3`→forte); stale `lowest/highestNote` on single-variant replace; one dead clamp.
**Processor:** non-lock-free `atomic_load(shared_ptr)` in audio path (accepted C++17 constraint); double MIDI-buffer walk; docstring/default-vocabulary mismatch; KS clamp saturating two notes to one technique.
**Editor:** 3 dead registered native fns (`getEmbeddedTuningCategories`, `getSkippedFiles`, `resetTechniqueNames`); `reportCellLayout` never resets `folderZoneRect` on omitted payload; `filesDropped` cell-hit path loads without `existsAsFile()`.
**Frontend:** stale comments (octave labels, one-shot marker); modal-Esc/loop-editor-Esc bubble collision; dead `window.confirm` fallbacks in WKWebView; `vel`-vs-`layer` naming trap.

---

## Verified clean (traced, not assumed)
- `processBlock` RT-safety: cached param pointers, no-alloc MIDI scan, deferred CC11 host notify, denormals guarded, output buffer cleared.
- WebView bridge: all 41 native-fn calls use the `Juce` ES-module namespace and are registered in `buildNativeFunctionRegistry()` — **no bridge gaps**.
- Base64 drop-streaming uses `juce::Base64::convertFromBase64` (not the JUCE-format decoder).
- Resource provider matches bare paths by equality (also closes path traversal); no hard-coded WebView scheme; Windows `withUserDataFolder()` set.
- Free-while-audio-reads contract: message thread publishes via `atomic_store(shared_ptr)`, voice snapshots via `atomic_load` at note-on.
- XSS: filenames/technique names/backend strings go through `textContent`/`.value`; `innerHTML` writes use developer constants only.
- `TriggerMapping.h` band math; CC-crossfade bracket selection; RandomNoRepeat; cubic-interp bounds clamp; setter index args (processor clamps all).
