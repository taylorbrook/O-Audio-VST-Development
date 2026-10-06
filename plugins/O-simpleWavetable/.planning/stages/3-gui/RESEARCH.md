# Stage 3: GUI - Research

**Plugin:** O-simpleWavetable · **Researched:** 2026-10-06 · **Domain:** JUCE 8.0.15 WebView editor, message-thread visualization (exact cycle render + FFT), lesson presets, import UX, i18n gates
**Confidence:** HIGH for the codebase findings (every Source/ claim read this session), MEDIUM for the payload-size and timing estimates (computed, not measured in a build)

<user_constraints>
## User Constraints (from CONTEXT.md, verbatim)

### Locked Decisions

**Requirements Confirmed**

- **Phase 3.1, Layout and Bindings:** `mockups/v1-ui.html` → `Source/ui/public/index.html`, `v1-i18n.js` → `js/i18n.js`, JUCE 8 frontend + `check_native_interop.js` + `img/insects.png` from O-simpleAdditive, EB Garamond embedded from `modules/ui/eb-garamond`, `webview-drop-streaming` via `ouaricon_add_module`. Editor from the `v1-PluginEditor.{h,cpp}` templates (adapt, don't paste): member order 21 relays → `webView` → 21 attachments; `.withOptionsFrom()` for all 21 (13 slider, 6 combo, 2 toggle). Rate ↔ Division swap on `lfo_sync`; `bit_depth` / `lfo_div` stepped knobs bind through `getComboBoxState`. Fixed 1120 × 780, not resizable.
- **Phase 3.2, Visualization Panels (UI-01..03, PERF-03):** 30 Hz editor timer → `buildCycleView` (exact 2048-sample cycle with the voice's shared read code → quantizer → 2048 real FFT → bins 1..32, dB re heard max, −60 floor → 256-point point-sampled cycle) → hash-gated `cycleUpdate`; `bankUpdate` (N × 128 level-0 thumbnails) on bank change / import publish / state restore / `uiReady`. JS: stacked bank panel with highlight + marker (fractional for Interp On, jumping for Off), cycle panel showing staircases, harmonics 1–32 bars incl. band-limit ceiling and rust aliasing bars.
- **Phase 3.3, Import UX, Tooltips, i18n (UI-04..06):** Import button (FileChooser via native function, `complete()` before `launchAsync`, SafePointer completion) + WebView drop (`importDroppedAudio`, `Base64::convertFromBase64`, 96 MB cap); filename + frame count; empty-Imported state (dashed stack, prompt, amber button, silence); `importStatus` errors localized (`tooShort`, `unreadable`, `tooLarge`). Plain-language tooltips on every control; the Band-limiting, Formant and Import tips name the aliasing, formant-shift and loop-buzz lessons.
- **Lesson buttons (new decision):** `applyFactoryPreset(lessonId)` is wired **live in Stage 3** with the five mockup recipes (checklist §Lesson preset ids). Reset to defaults first, never set `output_level`; `aliasDemo` also jumps the on-screen keyboard to octave 6 (page side). FUNC-08 stays Stage 4: Stage 4 refines the recipes per ARCHITECTURE §A9 and adds the preset-manager bank.
- **On-screen keyboard:** 2 octaves + C, A–K / W–U, Z/X octave, via existing `handleUiMidi` → `MidiMessageCollector`; window blur releases held notes.
- **Native functions** (all registered, or the control is silently dead): `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `uiReady`, `importAudio`, `importDroppedAudio`, `uiMidi`, `applyFactoryPreset`.

**Constraints Identified**

- **Imported bank reads go only through `getImportedBankSnapshot()`** (ARCHITECTURE Amendment 12). Never load the raw `importedForAudio` atomic on the message thread: the worker and `setStateInformation` also publish, so a raw load can race a retire.
- **Processor API still to add:** `applyFactoryPreset`, `getBankThumbnails(BankThumbs&)`, `buildCycleView(CycleView&)`. Already present from Stage 2: `handleUiMidi`, `uiLanguage`, `importFromFile` / `importFromMemory`, `getImportStatus` / `getImportStatusVersion`, `getBankDisplayGeneration`, `getImportedBankSnapshot`. The import status is polled by version on the timer (D-E), not pushed from the worker.
- **Cycle render must share the voice's read code** (`WtRead.h`, `BitQuantizer.h`), so the bars equal an offline FFT of the heard cycle by construction. The UI-03 gate compares the `cycleUpdate` JSON payload against an offline FFT of a rendered cycle.
- **No audio-thread cost:** the audio thread publishes display atomics only (already in place from Stage 2). Rendering, FFT and JSON build happen on the message thread. The 30 Hz timer must not allocate per tick beyond the JSON payload. Pre-size the FFT and cycle buffers.
- `emitEventIfBrowserIsVisible` drops events while hidden, so the page re-sends `uiReady` on `visibilitychange` → visible, and C++ re-sends `bankUpdate` + `importStatus` + a forced `cycleUpdate` (project memory: completions dropped when hidden; one-shot state push stale on preset load). A lesson preset or host preset load must also force a `bankUpdate` / `cycleUpdate`.
- The bank `<select>` stays English (its entries are the automation names).
- i18n layout pins (`.title-block` 384 px, `.import-btn` 124 px, `.bank-readout` 300 px, `.tour-buttons` 458 px, etc.) must survive integration. check-ui-labels [7] gates them.
- **UI gates before the Stage 3 commit, on the real tree** (the finalization fixture is not the build): `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint` (zh-Hans at `reviewed: 'mt'`), `check-ui-labels`, `boot-all-uis --strict-tips`. Exit 77 = Playwright not resolved = nothing verified, not a pass.
- `Source/ui/public/modules/webview-drop-streaming.js` is gitignored and written at configure time. Do not commit it. A `git archive` snapshot lacks it (project memory).
- Windows: `withUserDataFolder()` always, static WebView2 linking. The Windows build itself is a Stage 4 item (COMPAT-02).
- Build/install via `./scripts/build-and-install.sh O-simpleWavetable`. The Standalone stays stale under that script (project memory), so rebuild it explicitly for the hands-on. Commits are path-scoped with the shared-checkout temp-index discipline.

**Approach Decisions**

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Execution cadence | **3.1 + 3.2 → build/install → user visual checkpoint → 3.3** | The visual panels are the teaching core (UI-01..03). Check marker motion, the 3-bit staircase and the alias bars by eye before adding import UX, tooltips and i18n gates on top. Mirrors the Stage 2 listening checkpoint. |
| Lesson buttons | **Wire `applyFactoryPreset` live now** with the mockup recipes | The row is in the finalized mockup, and the buttons are how a class demos each concept. Stage 4 refines the recipes and adds the FUNC-08 preset bank. |
| Hands-on venue | **Standalone + one DAW (Logic or Live)** | Standalone covers the file drop and the keyboard. The DAW pass covers automation → UI, hide/re-show (`uiReady` re-send) and save/reopen with Imported (deferred from Stage 2 VERIFICATION). |
| Layout, style, controls, event contract, i18n | **As mockup v1 + integration checklist** | Finalized 2026-10-06 and pre-verified on a fixture tree (check-i18n 16/16, fr/zh lint clean, check-ui-labels 8 states, boot-all-uis clean). Not re-litigated. |
| Live output scope / spectrum | **Omitted** | ARCHITECTURE §Visualization: one truthful cycle picture; not in the brief. |

### Claude's Discretion (CONTEXT "Open Questions (for research)", verbatim)

- Exact `CycleView` / `BankThumbs` struct shapes and the hash inputs for the `cycleUpdate` gate (pos, frame, level, bank gen, interp, bit_depth, bandlimit, sounding, note). Is a float hash of the 256-point cycle cheaper than hashing the inputs?
- Which lead-voice display atomics Stage 2 already publishes vs what `buildCycleView` still needs (mip level read, latched frame, effective Position, `f0` / `nyquistH` for the band-limit ceiling marker).
- JSON payload cost at 30 Hz: `juce::var` / `DynamicObject` building vs a pre-formatted string. Does it stay within PERF-03 with the bank panel at N = 256?
- The reference editor to adapt from for the timer, import chooser and drop path: O-simpleAdditive vs O-simpleFM (the latest in-flight UI work).
- Harness for UI-03: a scratch driver that calls `buildCycleView` and compares against an offline FFT of a `WtVoice` render, vs. a Playwright capture of the live payload.
- How `applyFactoryPreset` resets to defaults without a host-visible preset load. Check the reset-defaults-first pattern and the gesture begin/end per param.

### Deferred Ideas (OUT OF SCOPE; quoted from the locked text above)

- "FUNC-08 stays Stage 4: Stage 4 refines the recipes per ARCHITECTURE §A9 and adds the preset-manager bank."
- "The Windows build itself is a Stage 4 item (COMPAT-02)."
- "Live output scope / spectrum | **Omitted**"
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description (REQUIREMENTS.md) | Research support |
|----|-------------------------------|------------------|
| UI-01 | Bank panel: 3D/stacked view of all frames, current frame highlighted, Position marker | Q2 (display atomics), Q3 (thumbnails already in `WavetableBank::thumbs`), Q4 (bankUpdate off the 30 Hz path), P2 (bank-index watch) |
| UI-02 | Current-cycle panel: the interpolated, bit-quantized cycle being read now | Q3 (`CycleRenderer` on `wt::readSample` + `BitQuantizer`), 256-pt point sampling |
| UI-03 | Harmonics panel: live bar graph of harmonics 1–32 of the cycle being heard (incl. band-limit level) | Q3 (FFT, dB re heard max), Q6 (G-VIZ gates, negative controls), `kmax`/`nyquistH` fields |
| UI-04 | Import button showing source filename and frame count | Q5 (natives), P8 (filename from `cachedBlob`, not `ImportStatus`) |
| UI-05 | Plain-language tooltips on every control, localized per suite convention | §UI gates (`boot-all-uis --strict-tips`, `check-i18n`, fr/zh lint) |
| UI-06 | Projector-readable single page, consistent with O-simpleFM / O-simpleAdditive | Mockup v1 locked; `check-ui-labels` 8 states; hands-on |
| PERF-03 | Visual panels update smoothly (≥ 30 fps) without stalling the audio thread | Q2 (relaxed stores only), Q4 (payload sizes), G-VIZ-ALLOC + timing log |
</phase_requirements>

## Summary

Stage 2 left almost every seam Stage 3 needs. `WtRead.h` already exposes the single reference read (`wt::readSample`, proved bit-exact against the voice by dsp-check G-READ, 0 ulp). `BitQuantizer` is a self-contained struct. `WavetableBank` already stores N × 128 level-0 thumbnails, filled by `MipmapBuilder` for built-in and imported banks alike. The processor already publishes `dispPos/dispLevel/dispFrame/dispSounding/dispLfo/dispMenv/dispAmp`, plus `bankDisplayGen`, `importStatusVersion` and `getImportedBankSnapshot()`. The mockup's `v1-PluginEditor.{h,cpp}` template was written against these exact names and compiles against the current processor except for three missing methods (`applyFactoryPreset`, `getBankThumbnails`, `buildCycleView`) and their structs.

Missing pieces: three display atomics (`dispNote`, `dispHz`, `displayFs`) plus a `midiNote` member on `WtVoice`, so the page can name the note and place the band-limit / Nyquist ceiling. Next, a message-thread `CycleRenderer` (FFT order 11, pre-sized arrays) in a non-editor TU so the offline harness can drive it. Then a JSON payload builder in a header, so the UI-03 gate compares the exact wire values. Finally, five template fixes: rounding, a quiet hash at idle, a bank-index watch, the filename source, and `frame = −1` for Interp On.

UI-03 is gated offline. A new `viz-check` console target drives the processor at fs = 2048·f, so one period is exactly 2048 samples and the voice reads table points at t = 0. The payload's `cycle` and `harmonics` must then equal the rendered output and an independent double-precision DFT of it to within the rounding quanta. Negative controls prove the gate can fail. No Playwright is needed for UI-03. Playwright is needed only for the two layout/tip gates.

**Primary recommendation:** adapt `mockups/v1-PluginEditor.{h,cpp}` as the editor base. Put the renderer and payload in `Source/CycleView.{h,cpp}` + `Source/VizPayload.h`, which the console helper compiles automatically. Hash the *rounded payload*, not the inputs. Register all 8 natives in 3.1 so nothing is dead at the visual checkpoint.

## Project Constraints (from CLAUDE.md)

- **Install:** `./scripts/build-and-install.sh O-simpleWavetable` (cache clear + `-dev` / unsuffixed sweep). Local builds are `-dev` branded (CMakeLists.txt:13).
- **AU check:** targeted `auval -v aumu OSiW OuDv`, never `auval -a` (memory: SIGABRT). Test in a DAW after install.
- **Commits:** trunk `main`. Path-scoped only (`git commit -- plugins/O-simpleWavetable`), never `-A` / `-a`. Re-check branch + `git status --short` right before each commit; temp-index discipline. Other sessions' edits (O-simpleFM, O-Gain, O-Reed, O-Bassoon, O-Freeze, `PLUGINS.md`) must not be touched.
- **Handoff:** after each phase, `/clear` + the exact next command, then STOP.
- **Never tag** unless `/publish`. Presets never set output gain.

## Architectural Responsibility Map

- **Lead-voice display state:** audio thread, relaxed stores once per block, publish only.
- **Exact cycle render + FFT:** message thread (`CycleRenderer`, processor-owned). It is a deterministic function of the bank and params (ARCHITECTURE Decision 5).
- **Imported bank for the UI:** message thread, via `getImportedBankSnapshot()` only (Amendment 12).
- **JSON payload + change hash:** message thread (`VizPayload.h`), shared with viz-check so the gate tests the wire.
- **Emission / handshake:** the editor timer + the `uiReady` native; the page calls `uiReady` on load and on `visibilitychange`.
- **Lesson recipes:** processor (`applyFactoryPreset`, message thread, host-visible gestures); the page does the octave jump.
- **Import decode / publish:** the existing worker (D-E / D-M).
- **Drawing:** browser canvas (locked mockup).

## Standard Stack

No new external packages. JUCE 8.0.15 (`.github/juce-version.txt`, local `/Users/taylorbrook/JUCE`) plus in-repo modules and scripts:

- **Bindings:** `juce::WebBrowserComponent` + Web*Relay + 3-arg attachments (template).
- **Harmonics:** `juce::dsp::FFT` order 11 via `performFrequencyOnlyForwardTransform (data, true)`. The array must be 4096 floats `[VERIFIED: juce_FFT.h:111-112 "The size of the array passed in must be 2 * getSize()"]`.
- **Wire format:** `JSON::toString (var, true)` inside `emitEvent`, doubles at max 15 dp `[VERIFIED: juce_WebBrowserComponent.cpp:423; juce_JSON.h:179-181 "int maximumDecimalPlaces = 15"]`.
- **Drop decode:** `juce::Base64::convertFromBase64` (Amendment 7).
- **Page-side drop encoder:** `modules/core/webview-drop-streaming` (`arrayBufferToBase64`, js line 409).
- **Font:** `modules/ui/eb-garamond` (css + 3 woff2).
- **JUCE frontend:** `js/juce/{index.js, check_native_interop.js}` from O-simpleAdditive (byte-identical to O-simpleFM's, `cmp` this session).
- **UI gates:** `scripts/{check-i18n, i18n-fr-lint, i18n-zh-lint, check-ui-labels, boot-all-uis}.js`.
- **Offline harness:** `ouaricon_add_processor_console` (`scripts/param-dump/ParamDump.cmake:67`).

## Package Legitimacy Audit

None. This phase installs no external packages: JUCE modules, in-repo `modules/`, and existing repo scripts only. Playwright is a pre-existing gate dependency, resolved by the gate scripts themselves (exit 77 if absent). It is not installed by this phase.

---

## Open Questions: Answers

### Q1. `CycleView` / `BankThumbs` shapes and the cycleUpdate change detection

**Recommended shapes** (`Source/CycleView.h`, juce_core + juce_dsp only, no BinaryData, no editor include):

```cpp
struct CycleView                                     // filled in place; no heap
{
    static constexpr int kPoints = 256, kHarmonics = 32;
    std::array<float, kPoints>    cycle {}, preQ {};          // preQ valid only when quantized
    std::array<float, kHarmonics> harmonicsDb {}, harmonicsRawDb {};   // dB re heard max, [-60, 0]
    float pos = 0.0f;      // effective position (lead voice) or knob when silent
    int   frame = -1;      // latched frame (Interp Off), -1 when Interp On
    int   level = 0;       // mip level read (0 when silent)
    int   kmax = 1023;     // WavetableBank::kmax (level)
    float nyquistH = 0.0f; // (fs/2)/f0; 0 when silent
    int   note = -1;       // lead MIDI note; -1 when silent
    float f0 = 0.0f;       // Hz incl. bend; 0 when silent
    float lfo = 0.0f, menv = 0.0f, amp = 0.0f;
    bool  sounding = false, quantized = false, empty = false;   // empty = Imported with no bank
};

struct BankThumbs                                    // NOT on the 30 Hz path
{
    int bank = 0; bool imported = false; int numFrames = 0;
    juce::String filename;                            // Imported only, from cachedBlob (see P8)
    std::vector<float> points;                        // numFrames * 128; editor member, reserve(256*128) once
};
```

`kPoints * 8 == WavetableBank::kTableSize`, so every 8th table sample is point-sampled. Add `static_assert`s on `kTableSize == 2048` `[VERIFIED: WavetableBank.h:53-59 "kTableSize = 2048", "kThumbSize = 128", "kThumbStep = kTableSize / kThumbSize", "kMaxFrames = 256"]` and on `kmax` `[VERIFIED: WavetableBank.h:62 "return level <= 0 ? 1023 : (1024 >> level);"]`.

**Change detection: hash the rounded OUTPUT payload, not the inputs** (as the template does, `v1-PluginEditor.cpp:497-507`, plus fixes).
1. **Cost is not the issue:** a full build is 2 × 2048 `readSample` + 2 FFTs, roughly tens of µs `[ASSUMED]`, logged by G-VIZ-TIME.
2. **An input list is fragile.** Inputs that change the picture but are missing from the CONTEXT list: ABA on the Imported pointer (a new bank at a freed address); the sample rate (level, `nyquistH`); pitch bend (same note, new `f0`); `numFrames` (frame clamp); `lfo_depth` (the LFO lamp). The output hash is correct by construction.
3. **Hash the rounded doubles** that go into the `var`, so sub-LSB noise cannot defeat the gate.
4. **Template bug, emits at 30 Hz forever at idle:** `dispLfo` is stored every block even at depth 0 `[VERIFIED: PluginProcessor.cpp:451-453 "Rendered even at depth 0 (display)", 474]`, and the template hashes raw `lfo` (:503). Fix: payload `lfo = lfo_depth > 0 ? dispLfo : 0`. `menv`/`amp` are already 0 when silent (cpp:582-586). The page only reads `lfo` when depth > 0 `[VERIFIED: v1-ui.html:2173 "const on = depth && depth.getNormalisedValue() > 0 && cycleData && Number.isFinite(cycleData.lfo);"]`. Result: idle → zero events.

### Q2. Display atomics: present vs needed

**Already published** (audio thread, once per block, `std::memory_order_relaxed`) `[VERIFIED: PluginProcessor.h:400-406; PluginProcessor.cpp:560-588]`:

| Atomic | Source | Note |
|--------|--------|------|
| `dispPos` (float) | `lead->getLastPos()` = `effPos` after the block (WtVoice.h:431-434) | Smoothed with Interp On, raw with Off. Includes LFO + env (DSP-05). **Not cleared when silent** (keeps the last value). |
| `dispLevel` (int) | `lead->getLastLevel()` = `cfg.level` incl. D-K hysteresis (WtVoice.h:435) | Not cleared when silent |
| `dispFrame` (int) | `lead->getLastFrame()` (WtVoice.h:436: `cfg.interp ? wt::latchFrame (eff, nF) : latchedFrame`) | **With Interp On this is a rounded frame, not −1.** The renderer must map it to −1 from the `interp` param. |
| `dispSounding` (bool) | lead found | false when no amp env is active |
| `dispLfo` (float −1..1) | `lfo.lastValue(...)` every block | Global; runs at depth 0 |
| `dispMenv`, `dispAmp` (float) | lead voice | 0 when silent |
| `bankDisplayGen` (uint32, relaxed) | `processBlock` on a bank-index change (cpp:393-398) + every `publishImportedBank` (cpp:784) | **The bank-param half is audio-thread only** (P2) |

**Needed, add these.** All are relaxed stores in `updateDisplayFromLeadVoice()` / `prepareToPlay()`. That adds no RT cost beyond 2 stores per block, with no allocation, lock or branch change:

| New | Written | Value | Why |
|-----|---------|-------|-----|
| `std::atomic<int> dispNote { -1 }` | `updateDisplayFromLeadVoice`; −1 in the no-lead branch and in `prepareToPlay` | `lead->getLastNote()` | The page prints `noteName(view.note)` and gates "held" on `note !== null` `[VERIFIED: v1-ui.html:1686, 1986, 2156]` |
| `std::atomic<float> dispHz { 0 }` | same | `(float) lead->getCurrentHz()` (bend included, WtVoice.h:498, 622) | `f0`, `nyquistH` |
| `std::atomic<double> displayFs { 44100 }` | `prepareToPlay` | `sampleRate` | `nyquistH = 0.5·fs/f0`. Reading `getSampleRate()` (a plain double) from the message thread is a data race; an atomic avoids it. |

`WtVoice` has no MIDI note member. `noteHz` is set in `startNote` (WtVoice.h:270), `noteOnDirect` (:454) and `setPitchNote` (:484). The Mono path bypasses `SynthesiserVoice::startNote`, so `getCurrentlyPlayingNote()` is wrong in Mono. Add `int midiNote = -1;` assigned at those three sites, plus `int getLastNote() const noexcept`. This is a display member only, so dsp goldens are unaffected; re-run dsp-check G-BLOCK / G-MONO to prove it.

**What buildCycleView uses when silent** (ARCHITECTURE §Visualization Data Path: "the UI uses the knob Position with level 0"):
- `pos = currentKnob()`, `level = 0`.
- `frame = interp ? −1 : wt::latchFrame (knob, nF)`.
- `note = −1`, `f0 = nyquistH = 0`.

This matches the page, which uses the knob when silent `[VERIFIED: v1-ui.html:1660 "const eff = sounding && typeof cycleData.pos === 'number' ? clamp01(cycleData.pos) : knobPos;"]` and `Math.round(eff * (N - 1))` for Off (:1672). The rounding agrees with `wt::latchFrame`'s `lround`.

**Tearing:** the separate relaxed loads can mix blocks n and n+1. This is harmless. `readSample` clamps the frame to `nF − 1` and the level to `[0, 10]` (WtRead.h:106, 119), so a frame from a 256-frame bank read against a 32-frame bank is safe.

### Q3. How buildCycleView reproduces the heard cycle

**Placement.**
- `class CycleRenderer` lives in `Source/CycleView.{h,cpp}`. It owns `juce::dsp::FFT fft { 11 }`, `std::array<float, 4096> work`, and `std::array<float, 2048> heard, pre, raw`.
- It is a processor member (`vizRenderer`), constructed in the processor ctor (message thread; the FFT allocates there once).
- `processor.buildCycleView (CycleView&)` is **message-thread only, non-reentrant**. Document this; don't assert it, because the console harness calls it from main.
- Why not the editor: the console helper compiles every plugin `.cpp` except `PluginEditor*` `[VERIFIED: ParamDump.cmake:151-159 "if(_pd_src MATCHES \"PluginEditor\") continue()"]`. So a renderer in `CycleView.cpp` is automatically in `viz-check` (and in the four existing drivers, harmlessly). Never include `BinaryData.h` outside `PluginEditor.cpp`, because console targets don't link it.

**Algorithm (shares the voice's code exactly):**

```cpp
// processor, message thread
void OSimpleWavetableAudioProcessor::buildCycleView (CycleView& v)
{
    const int bankIdx = choiceIndex (pBank->load(), kNumBanks);
    std::shared_ptr<const WavetableBank> hold;              // keeps Imported alive for THIS call only
    const WavetableBank* b = bankIdx < BuiltInBanks::kCount ? builtIns->get (bankIdx)
                                                            : (hold = getImportedBankSnapshot()).get();
    const bool interp   = finiteOr (pInterp->load(), 1.0f) >= 0.5f;          // same reads as processBlock:404-406
    const int  bitIdx   = choiceIndex (pBitDepth->load(), kNumBitDepthChoices);
    const bool sounding = dispSounding.load (std::memory_order_relaxed);
    const int  nF       = b != nullptr ? b->numFrames : 0;
    const float pos     = sounding ? dispPos.load (relaxed) : currentKnob();
    const int  level    = sounding ? dispLevel.load (relaxed) : 0;
    const int  latched  = sounding ? dispFrame.load (relaxed) : wt::latchFrame (pos, nF);
    vizRenderer.render (b, level, interp, pos, latched, bitIdx, v);   // fills cycle/preQ/harmonics*
    v.frame = interp ? -1 : juce::jlimit (0, juce::jmax (0, nF - 1), latched);
    // + level, kmax, note, f0, nyquistH (from dispHz/displayFs), lfo/menv/amp, sounding, empty = (b == nullptr)
}

// CycleRenderer::render, no allocation
BitQuantizer q;  q.setChoiceIndex (bitIdx);  v.quantized = q.bits > 0;
for (int j = 0; j < 2048; ++j)
{
    const float s = (b != nullptr) ? wt::readSample (*b, level, (double) j / 2048.0, interp, pos, latched) : 0.0f;
    pre[j] = s;  heard[j] = q.apply (s);                     // same order as WtVoice.h:398
}
for (int i = 0; i < 256; ++i) { v.cycle[i] = heard[i * 8]; v.preQ[i] = pre[i * 8]; }   // POINT sampling
// FFT: copy heard -> work[0..2047], zero work[2048..4095], fft.performFrequencyOnlyForwardTransform (work.data(), true)
// ref = max |X_k| over k = 1..1023 (the heard cycle's strongest harmonic, DC excluded)
// harmonicsDb[k-1] = ref > 1e-12 ? max (-60, 20*log10 (|X_k| / ref)) : -60      for k = 1..32
// harmonicsRawDb: same with level 0 (raw frame, same quantizer), dB re the SAME ref, clamped [-60, 0];
//                 when level == 0 copy harmonicsDb (no second render)
```

- **Phase j/2048 gives `idx = j`, so `t = 0`:** the cycle is the frame-lerped (On) or latched (Off) table samples, then quantized. The quantizer is memoryless, so bins 1..32 are the harmonics of the continuous heard cycle. Folded quantization harmonics ≥ 2016 are negligible. No window: the cycle is exactly periodic (row 17).
- **"dB re heard max" = max over bins 1..1023**, not 1..32. A frame whose strongest partial is above h32 then shows honestly lower bars. `[ASSUMED, discretion]`
- **`harmonicsRaw`** feeds the dashed ghost bars for `k > kmax` (band-limit On, note held) `[VERIFIED: v1-ui.html:1989-1999]`. It is the level-0 spectrum at the same position, on the heard scale, clamped to ≤ 0 dB (at level 10, kmax = 1, and raw h2 can exceed h1).
- **Quantizer:** `[VERIFIED: BitQuantizer.h:52 "bits = idx <= 0 ? 0 : 17 - juce::jlimit (1, 14, idx);"]`. Read `bit_depth` as `processBlock` does (cpp:406).

**Bank lifetime.**
- **Built-ins:** they live in the processor's `SharedResourcePointer<BuiltInBanks>` (PluginProcessor.h:186-188, 304).
- **Imported:** `getImportedBankSnapshot()` copies `importedOwner` under `bankStateLock` (cpp:856-860). The local `hold` keeps the bank alive through a concurrent publish + sweep. If `hold` is the last reference, the ≤ 23 MB bank is freed on the message thread, which is acceptable. Never cache the snapshot across ticks. Never touch `importedForAudio` (Amendment 12).

### Q4. JSON payload cost at 30 Hz

**Wire mechanics** `[VERIFIED: juce_WebBrowserComponent.cpp:414-428, 607-611]`: `if (isVisible())` → `JSON::toString (object, true)` (15 dp) → two `replace` passes → `evaluateJavascript(...emitByBackend...)`. `serialiseDouble` (juce_String.cpp:2287-2329) trims trailing zeros. An un-rounded float costs about 17–18 bytes; a value pre-rounded to d dp costs d + 2–3.

**Estimated sizes** (computed this session by a Python emulation of `serialiseDouble` on a Sine→Saw cycle, scratch `jsonsize.py`; MEDIUM):

| Event | Un-rounded | Rounded (cycle 4 dp, dB 2 dp, thumbs 3 dp) | Rate |
|-------|-----------|----------------------------------------------|------|
| `cycleUpdate` (cycle + preQ + 2 × 32 dB + scalars) | ≈ 10.8 KB | **≈ 5.0 KB** (≈ 150 KB/s at 30 Hz) | 30 Hz while moving; 0 at idle |
| `bankUpdate` N = 32 (built-in) | ≈ 75 KB | **≈ 26 KB** | event-driven |
| `bankUpdate` N = 256 (imported max) | ≈ 600 KB | **≈ 209 KB** | event-driven |

**Recommendation:**
- Keep `juce::var` / `DynamicObject`: the page stores the object directly (v1-ui.html:2254-2257). A pre-formatted String would arrive JSON-quoted and force a change to the locked page.
- **Round in the var builder:** 4 dp for cycle / preQ / pos / f0 / nyquistH, 3 dp for thumbs, 2 dp for dB. 4 dp is below one pixel on the ~150 px cycle panel.
- Per-tick allocation is then only the `var` tree (allowed). `CycleView` is an editor member, and the renderer scratch is pre-sized.

**Keeping bankUpdate off the 30 Hz path.** Emit it only when `getBankDisplayGeneration()` changes, **or the editor-seen APVTS bank index** changes (P2), or on `uiReady`. The lesson-preset bank change is covered by the index watch. Order is **bankUpdate, then importStatus**: the page's `onBankUpdate` clears `importError` `[VERIFIED: v1-ui.html:2250 "importError = null;"]`.

**Visibility gate, corrected scope.** Events drop only when the `WebBrowserComponent`'s **own** visible flag is false, not when the editor or window is hidden (memory `critical_webview_completion_gated_on_isvisible`, corrected 2026-08-25). The editor never hides the view, so the real losses are events emitted **before the page registers its listeners** (async load; the timer starts in the ctor) and in the ctor gap. `uiReady` (v1-ui.html:2273-2286) is therefore load-bearing. The `visibilitychange` re-send is harmless belt-and-braces; keep both. `evaluateJavascript` has no documented size limit `[ASSUMED]`.

PERF-03: 33 ms per tick. The estimate is ~0.1 ms render + ~1 ms var + JSON `[ASSUMED]`, and the audio thread gains 2 relaxed stores per block. G-VIZ-TIME logs the real numbers; there is no wall-clock verdict (memory).

### Q5. Reference editor to adapt

**Use `plugins/O-simpleWavetable/.planning/mockups/v1-PluginEditor.{h,cpp}` as the base.** It already calls this processor's names: `getAPVTS`, `uiLanguage`, `languageCode/Index`, `handleUiMidi`, `importFromFile`, `importFromMemory(name, MemoryBlock&&)`, `ImportStatus::State {idle,busy,done,error}`, `getImportStatusVersion`, `getBankDisplayGeneration`. All of them exist (PluginProcessor.h:159-217). It also already has complete-before-`launchAsync`, a SafePointer bail without `complete`, `Base64::convertFromBase64` with a cap, `uiReady`, `withKeepPageLoadedWhenBrowserIsHidden`, and `withUserDataFolder` always. Cross-check:

| Concern | Reference | Notes |
|---------|-----------|-------|
| Resource provider: EB Garamond branches, `insects.png`, language natives, `uiMidi`, timer | `plugins/O-simpleAdditive/Source/PluginEditor.cpp` (committed `0b219f4f`) | The branches are identical to the template's. **Do not copy** its resizable / `editorScale` block (Stage 3 is fixed 1120 × 780, locked). **Do not copy** its `applyFactoryPreset` reset loop (it resets `output_level`, P9). |
| Single-payload base64 drop | `plugins/O-Prism/Source/PluginEditor.cpp:557-590` (`importUserWavetableData`) | Precedent for `MemoryOutputStream` + `convertFromBase64`. Its chooser completes *inside* the callback; the template's complete-first form is better. |
| FileChooser + SafePointer | `plugins/O-Prism/Source/PluginEditor.cpp:524-553` | `if (safeThis == nullptr) return;` without `complete` |
| `webview-drop-streaming` via CMake | `plugins/O-simpleGrain/CMakeLists.txt:63-67` | Already present in our CMakeLists.txt:52 |
| **O-simpleFM: do not use** | — | It has uncommitted edits from another session (`git status`: `M plugins/O-simpleFM/Source/PluginEditor.cpp`), so it is a moving target. It is also preset-manager-heavy and resizable. |

No sibling has `uiReady` (grep: 0 hits in `plugins/*/Source/PluginEditor.cpp`). The template is the only source for that contract.

**Template edits required** (adapt, don't paste):
- (a) Round values and hash the rounded payload, gating `lfo` on depth (Q1).
- (b) Bank-index watch in `timerCallback` (P2).
- (c) Have `uiReady` refresh `lastBankGeneration` / `lastBankIndex` after emitting, so the next tick does not resend.
- (d) Move `toVarArray` + the hash into `Source/VizPayload.h`, so viz-check tests the same code.
- (e) Leave `setSize (1120, 780)` as the numeric literal (gates parse it).
- (f) Keep the exact `url == "/js/i18n.js"` branch form (check-i18n [8]).

### Q6. UI-03 gate harness

**Recommended: a new console target `viz-check`** (`tests/viz-check/main.cpp`, added in the `OUARICON_BUILD_TESTS` block with `OSIW_TEST_HOOKS=1`).
It drives the **processor**: display atomics → `buildCycleView` → `VizPayload` → `JSON::toString` → `JSON::parse`, which is the real wire. Reuse dsp-check's `Rig` / `setParam` / `armAlloc` (`tests/dsp-check/main.cpp:234-327`). **Not Playwright:** a capture proves only that *some* payload arrived, not that the bars equal the heard cycle.

Gates:

- **G-VIZ-EXACT**
  - *Stimulus:* fs = **901120** (= 440·2048), note **69**. Then `inc = 2^-11` exactly, phase accumulates exactly, and every output sample is a table point. Amp A 0.001 / S 1, out 0 dB, LFO depth 0. Take 2048 samples a whole number of periods after note-on (phase resets at note-on, WtVoice.h:280).
  - *Matrix:* 5 banks × pos {0, 0.37, 1} × interp {On, Off} × bit_depth idx {0, 9 ("8"), 14 ("3")}.
  - *Assert:*
    - (1) `cycle[j]·g == y[8j]`, where g is the least-squares gain (env · vel² · 0.5 · out).
    - (2) `harmonics` match an **independent double-precision direct DFT** of y (max over 1..1023; not juce FFT).
    - (3) `level == 1` (x = 1 → ceil(log2) = 0 → `kMinBandLimitedLevel`), `frame` = −1 or latched, `pos` = knob, `note` 69, `f0` 440.
  - *Tolerance:* cycle ≤ 2e-4 abs; dB ≤ 0.02 for bars > −59.5.
- **G-VIZ-CEIL**
  - *Stimulus:* fs 48000, Sine→Saw pos 1 (frame 32 = h1..h32), note 96 (2093 Hz): x = 89.3 → level 7, kmax 8, nyquistH 11.47. Render 1 s, Blackman-Harris window, double-precision Goertzel at k·f0.
  - *Assert:*
    - On: `level` 7, `kmax` 8, k > 8 at −60, k ≤ 8 within ±0.5 dB, `nyquistH` within 1e-2.
    - Off: `level` 0, `kmax` 1023, k ≤ 11 within ±0.5 dB.
- **G-VIZ-SILENT**
  - *Stimulus:* no note, knob 0.6, Interp Off, Sine→Square.
  - *Assert:* `sounding` false, `note` −1, `level` 0, `frame` = lround(0.6·31) = 19. Two builds give the same hash, with the free LFO running at depth 0 (idle quiet).
- **G-VIZ-IMPORTED**
  - *Stimulus:* `publishImportedBankForTesting` (PluginProcessor.h:285) with 3 frames, bank 5; then publish nullptr.
  - *Assert:* thumbnails bit-exact against `thumbs`, `numFrames` 3, cached filename. When empty: `empty`, cycle all 0, harmonics all −60, `numFrames` 0.
- **G-VIZ-ALLOC:** the dsp-check alloc stimulus, with `buildCycleView` + `getBankThumbnails` looping on a second thread. Pass = 0 audio-thread allocations (liveness counted).
- **G-VIZ-TIME** (log only, no wall-clock verdict): 1000 × (build + var + `JSON::toString`), plus N = 256 bankUpdate × 20. Print µs and bytes.
- **G-LESSON:** randomise all 21 params (`output_level` −17.3), apply each id. Assert:
  - `output_level` is unchanged;
  - every other param = default, except the recipe overrides (1e-6);
  - an unknown id returns false and changes nothing;
  - every recipe id matched a real param.

**Negative controls** (each must FAIL its comparison by more than 10× tolerance, or the gate is vacuous; memory `pattern_gate_stimulus_below_threshold_is_vacuous`):
- **N1 quantizer:** render at idx 14 (3 bits), set `bit_depth` to 0 *after* the render, and build. The payload is then the unquantized cycle. G-VIZ-EXACT must fail (3-bit error ≈ 0.06 FS ≫ 2e-4).
- **N2 level:** in G-VIZ-CEIL On, compare `harmonicsRaw` (level 0) against the render for k = 9..32. It must exceed −60 by ≥ 20 dB where the render is at the floor. This uses the payload's own ghost field, so no hook is needed.
- **N3 position:** compare against a render at pos + 1/31 (one frame later) in Sine→Saw Interp Off. Some bar must differ by ≥ 3 dB (frame k adds harmonic k + 1).
- **N4 liveness:** `harmonics[0] == 0 dB` and `max|cycle| > 0.5` in every non-empty case.

### Q7. `applyFactoryPreset(lessonId)`

- **Thread:** the message thread. Natives arrive via `didReceiveScriptMessage` (juce_WebBrowserComponent_mac.mm:481-494); main-thread delivery is `[ASSUMED]`, and every sibling relies on it.
- **No host preset load:** these are plain `setValueNotifyingHost` edits. No program change, no `setStateInformation`, and Imported data is untouched.
- **Gestures:** begin/set/end per parameter (O-simpleAdditive; Logic touch/latch drops un-gestured edits). O-simpleSubtractive omits them; don't copy that.
- **Apply once, target first:** this beats reset-then-apply (no transient bank switch, i.e. no extra crossfade, and half the host edits). Parameters already at target are skipped.

```cpp
bool OSimpleWavetableAudioProcessor::applyFactoryPreset (const juce::String& id)   // message thread
{
    namespace ids = OSimpleWavetable::ParamIDs;
    struct Set { const char* id; float real; };
    std::vector<Set> r;      // NOT std::initializer_list: assigning a braced list to one dangles
    if      (id == "steppedSmooth") r = { {ids::bank,0}, {ids::position,0.5f}, {ids::interp,0}, {ids::lfoSync,0},
                                          {ids::lfoShape,1}, {ids::lfoRate,0.18f}, {ids::lfoDepth,1.0f} };
    else if (id == "aliasDemo")     r = { {ids::bank,0}, {ids::position,1.0f}, {ids::bandlimit,0} };
    else if (id == "driveSweep")    r = { {ids::bank,4}, {ids::position,0.0f}, {ids::envAmount,1.0f},
                                          {ids::menvAttack,0.9f}, {ids::menvDecay,1.6f}, {ids::menvSustain,0.25f}, {ids::menvRelease,0.8f} };
    else if (id == "vowelPad")      r = { {ids::bank,3}, {ids::position,0.5f}, {ids::lfoSync,0}, {ids::lfoShape,0},
                                          {ids::lfoRate,0.12f}, {ids::lfoDepth,0.9f}, {ids::ampAttack,0.6f}, {ids::ampRelease,1.4f} };
    else if (id == "ppg8bit")       r = { {ids::bank,1}, {ids::position,0.6f}, {ids::interp,0}, {ids::bitDepth,9},
                                          {ids::lfoSync,0}, {ids::lfoShape,4}, {ids::lfoRate,3.0f}, {ids::lfoDepth,0.4f} };
    else return false;                                                    // unknown id: no change

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            if (rp->getParameterID() == ids::outputLevel) continue;      // user-owned: never reset, never set
            float target = rp->getDefaultValue();                         // normalised default = "reset first"
            for (const auto& s : r) if (rp->getParameterID() == s.id) target = rp->convertTo0to1 (s.real);
            if (std::abs (rp->getValue() - target) < 1.0e-6f) continue;
            rp->beginChangeGesture(); rp->setValueNotifyingHost (target); rp->endChangeGesture();
        }
    return true;
}
```

**Verified values:**
- Recipes: `[VERIFIED: v1-integration-checklist.md:67-71]`.
- Ids match the page: `[VERIFIED: v1-ui.html:1029-1033 data-preset="steppedSmooth" | "aliasDemo" | "driveSweep" | "vowelPad" | "ppg8bit"]`.
- Choice indices `[VERIFIED: PluginProcessor.cpp:129-160]`:
  - Bank `"Sine \xE2\x86\x92 Saw"`=0, `"Sine \xE2\x86\x92 Square"`=1, `"Pulse Width"`=2, `"Formant"`=3, `"Drive"`=4, `"Imported"`=5.
  - Bit Depth `{ "Full", "16", "15", "14", "13", "12", "11", "10", "9", "8", ... "3" }` → "8" = index 9.
  - LFO Sync `{ "Free", "Tempo" }`.
  - LFO Shape `{ "Sine", "Triangle", "Saw", "Square", "S&H" }` → Triangle 1, Sine 0, S&H 4.
- All recipe values are inside their ranges (menv times 0.001–10, amp times 0.001–5, `lfo_rate` 0.01–20). `getParameterID()` on `RangedAudioParameter` is `[ASSUMED: JUCE 8 API]`.

**After apply:**
- The native sets `forceCycleEmit = true`.
- The bank reaches the page via the index watch (P2), and the controls via the relays.
- The page jumps the octave (`LESSON_OCTAVE = { aliasDemo: 6 }`, v1-ui.html:1155).

**Edge cases:**
- A lesson clicked during a busy import is overridden by the D-M auto-select. Acceptable.
- ARCHITECTURE §A9's Alias Demo uses Drive, while the checklist uses Sine→Saw. The checklist is locked for Stage 3; Stage 4 refines.

---

## Native Functions + Events Contract (final)

Natives (page → C++). All 8 are registered in 3.1:

- `getParameterDefaults` and `getUiLanguage` / `setUiLanguage`: template as-is (`languageIndex` allow-list, PluginProcessor.h:172-173).
- `uiReady` → `true`: emit bankUpdate → importStatus, set `forceCycleEmit`, refresh the counters.
- `importAudio` → `true` **before** `launchAsync` (SafePointer; on null, `return` without `complete`).
- `importDroppedAudio [name, base64]` → `importFromMemory` bool: length cap `96 MB / 3 * 4 + 4`, then `convertFromBase64`; the processor re-caps at `kMaxMemoryBytes` (WavetableImporter.h:89) and sanitises the name (cpp:888).
- `uiMidi [note, isOn, vel?]`: `handleUiMidi` clamps (cpp:597-599).
- `applyFactoryPreset [id]` → bool (Q7).

**Name-diff check** (must print nothing; memory `pattern_webview_native_fn_bridge_gap`):
```bash
cd plugins/O-simpleWavetable && comm -23 \
  <(grep -o "getNativeFunction('[A-Za-z]*')" Source/ui/public/index.html | sed "s/.*('\(.*\)')/\1/" | sort -u) \
  <(grep -o 'withNativeFunction ("[A-Za-z]*"' Source/PluginEditor.cpp | sed 's/.*("\(.*\)"/\1/' | sort -u)
```

| Event (C++ → page) | When | Payload |
|--------------------|------|---------|
| `bankUpdate` | gen change ∨ bank-index change ∨ `uiReady` | `{ bank, imported, numFrames, filename, frames: [[128] × N] }` (thumbs 3 dp) |
| `cycleUpdate` | each 30 Hz tick if the rounded-payload hash changed, or forced (`uiReady`, bank change, lesson) | `{ cycle[256], harmonics[32], harmonicsRaw[32], preQ[256]? (only quantized), pos, frame, level, kmax, nyquistH, note, f0, lfo, menv, amp, sounding }` |
| `importStatus` | `getImportStatusVersion()` change ∨ `uiReady` | `{ state, filename, frames, error }`. Error codes from Source: `"tooShort" \| "unreadable" \| "tooLarge" \| "unsupported"` `[VERIFIED: PluginProcessor.h:202]`. The page localizes the first three; anything else falls to generic. `"cancelled"` is never surfaced (cpp:933-934). |

## CMake Changes (merge `v1-CMakeLists.txt`)

Already present, verify only `[VERIFIED: CMakeLists.txt:15-21, 52, 54-73, 80-86]`:
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE` (same as all four simple siblings).
- `ouaricon_add_module(O-simpleWavetable webview-drop-streaming)`.
- `juce::juce_gui_extra` and `juce::juce_dsp` linked.
- `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.

Add:
1. `target_sources`: `Source/CycleView.cpp`, `Source/CycleView.h`, `Source/VizPayload.h`.
2. `juce_add_binary_data(O-simpleWavetable_UIResources SOURCES …)`, exactly the v1 list:
   - `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`, `modules/webview-drop-streaming.js`, `img/insects.png`
   - `${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css` + the 3 woff2

   Use the default `BinaryData` namespace; there is no second binary-data target. Declare it **before** the link line.
3. `target_link_libraries(O-simpleWavetable PRIVATE O-simpleWavetable_UIResources)`.
4. In the tests block:
   ```cmake
   ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/viz-check/main.cpp viz-check)
   target_compile_definitions(O-simpleWavetable-viz-check PRIVATE OSIW_TEST_HOOKS=1)
   ```

BinaryData symbols strip hyphens: `webviewdropstreaming_js`, `ebgaramond_css`, `EBGaramondRegular_woff2`, `index_js` (JUCE frontend), `index_html`, `i18n_js`, `insects_png`, `check_native_interop_js`. Two files share the name `index.*` but different extensions (`index_html` vs `index_js`), so there is no collision. Same as O-simpleAdditive.

## Recommended File Layout

```
Source/
├── CycleView.h / .cpp      # CycleView, BankThumbs, CycleRenderer (juce_dsp; no editor, no BinaryData)
├── VizPayload.h            # cycleToVar(view, &hash), bankToVar(thumbs), importToVar(status); rounding; FNV
├── PluginEditor.h / .cpp   # from v1 template (+ Q5 edits)
├── PluginProcessor.*       # + dispNote/dispHz/displayFs, vizRenderer, buildCycleView, getBankThumbnails, applyFactoryPreset
├── WtVoice.h               # + midiNote / getLastNote()
└── ui/public/
    ├── index.html          # ← mockups/v1-ui.html (verbatim)
    ├── js/i18n.js          # ← mockups/v1-i18n.js (verbatim)
    ├── js/juce/{index.js, check_native_interop.js}   # ← O-simpleAdditive
    ├── img/insects.png     # ← O-simpleAdditive
    └── modules/webview-drop-streaming.js             # configure-generated, GITIGNORED (.gitignore:26)
tests/
├── i18n-states.json        # ← mockups/v1-i18n-states.json
└── viz-check/main.cpp      # G-VIZ-*, G-LESSON
```

`getBankThumbnails (BankThumbs& t)` (message thread):
- Read the bank index.
- For a built-in, copy `bank.thumbs`.
- For Imported, in **one** `bankStateLock` scope, copy `importedOwner` and `cachedBlob.filename` / `numFrames`, then copy `thumbs` outside the lock.
- `thumbs` is already point-sampled every 16 from level 0 `[VERIFIED: MipmapBuilder.cpp:89-93 "th[t] = dst[t * WavetableBank::kThumbStep];"]` for built-ins (BankFactory.cpp:245 `allocate`) and imports (`buildFromLevel0` → `bank->allocate`, MipmapBuilder.cpp:135).

## Don't Hand-Roll

- **Cycle read:** use `wt::readSample` (WtRead.h:99-131), never a second interpolation. G-READ proves it is the voice's read to 0 ulp.
- **Quantizer:** use `BitQuantizer::apply` (mid-rise), never a JS or editor copy. Additive's mid-tread differs.
- **Thumbnails:** use `WavetableBank::thumbs` (prebuilt).
- **FFT:** use `juce::dsp::FFT` in the plugin. The *gate* uses an independent direct DFT on purpose.
- **Base64:** `Base64::convertFromBase64`, never `MemoryBlock::fromBase64Encoding`.
- **Drop encoder:** the module's `arrayBufferToBase64`.
- **Language persistence:** already done (`uiLanguage`, cpp:629-652).

## Common Pitfalls

1. **P1 Idle hash churn:** `dispLfo` updates every block at depth 0 (cpp:451-453, 474). Gate `lfo` on depth and hash the rounded payload. *Sign:* cycleUpdate traffic with no note held.
2. **P2 Bank-param changes are counted only on the audio thread** (`bankDisplayGen`, cpp:393-398). A host that idles an instrument (Logic non-selected track, Standalone with no device) leaves the stack stale. The editor must also watch the APVTS bank index.
3. **P3 `dispFrame` with Interp On is a rounded frame**, not −1 (WtVoice.h:436). Map it from the `interp` param.
4. **P4 Stale atomics after release:** `dispPos/Level/Frame` keep their last values. Branch on `dispSounding` and use knob / level 0.
5. **P5 Mono has no `currentlyPlayingNote`** (`noteOnDirect`/`setPitchNote` bypass `startNote`). Use the new `WtVoice::midiNote` at all three sites; a legato move must update the note name.
6. **P6 Imported only via the snapshot**, never `importedForAudio` on the message thread; never hold the `shared_ptr` across ticks.
7. **P7 BinaryData in a console TU:** console targets compile every non-`PluginEditor` `.cpp` without UIResources. Include `BinaryData.h` only in `PluginEditor.cpp`. Keep the editor include under `#if JUCE_WEB_BROWSER` at the bottom of PluginProcessor.cpp (cpp:609-621).
8. **P8 Wrong filename:** a failed import overwrites `importStatus.filename` but not the bank (cpp:937 vs 957-963). bankUpdate takes `cachedBlob.filename` under the same lock as the snapshot.
9. **P9 The sibling reset loop resets `output_level`** (O-simpleAdditive `applyFactoryPreset`). Skip it; G-LESSON asserts it (memory `feedback_presets_never_set_output_gain`).
10. **P10 Event order:** bankUpdate before importStatus (the page clears `importError` on bankUpdate, v1-ui.html:2250).
11. **P11 An unregistered native hangs silently** (Release: `jassertfalse; return;`, no completion). Register all 8 in 3.1 and run the `comm -23` diff. boot-all-uis' stub invents values and cannot see it.
12. **P12 FileChooser after editor close:** complete first; the callback captures `SafePointer` + `chooser`; on null, return without touching the bridge. Hands-on: close the editor with the dialog open.
13. **P13 A large drop stalls the message thread:** 96 MB becomes 128 MB of base64, JSON-parsed by the script handler (mac.mm:494) and decoded on the message thread. Acceptable for a rare action (the page shows busy first).
14. **P14 Stale Standalone:** `build-and-install.sh` builds VST3 + AU only. Run `ninja -C build O-simpleWavetable_Standalone` and `strings … | grep -c cycleUpdate` before any visual check.
15. **P15 Gitignored drop module** (`.gitignore:26`): never commit it. A `git archive` UI snapshot must copy it in.
16. **P16 Canvas fonts:** keep the page's `document.fonts.load("11px 'EB Garamond'").then(scheduleRender)` (v1-ui.html:2714-2715) and `SERIF_CANVAS` (:1146) when copying. Canvas text is invisible to the label gates.
17. **P17 Exit 77 ≠ pass** (check-ui-labels.js:496, boot-all-uis.js:127). Cached browsers are chromium / headless-shell 1228/1234; 1243 is absent.
18. **P18 CI turns main red:** `ui-static-gates.yml` runs check-i18n + fr/zh lint on every push. Run them locally before the commit that adds `index.html` / `i18n.js`.
19. **P19 Shared checkout:** path-scoped temp-index commits; `PLUGINS.md` is foreign-staged right now (`MM`). Never stage it; re-check HEAD after the commit.

## Phase / Task Breakdown (locked cadence)

**Plan A, Phase 3.1 Layout & Bindings** (wave 1)
1. Copy the UI files (index.html, i18n.js verbatim; JUCE frontend + insects.png from O-simpleAdditive; `tests/i18n-states.json`). CMake: UIResources + link + new `target_sources` stubs.
2. Editor from the template (Q5 edits), with **all 8 natives registered**, including importAudio / importDroppedAudio (the processor API exists). The viz emit functions are wired but get data in Plan B.
3. `applyFactoryPreset` (Q7) + `viz-check` skeleton with G-LESSON.
4. Build the plugin + the out-of-repo Debug `viz-check`, then run the `comm -23` native diff. Quick boot check: `node scripts/boot-all-uis.js --plugin O-simpleWavetable` (0 DEAD, 0 404).

**Plan B, Phase 3.2 Visualization** (wave 2; depends on A)
5. `WtVoice::midiNote` + `dispNote` / `dispHz` / `displayFs`. Re-run dsp-check (G-READ, G-MONO, G-BLOCK, `--alloc-check`) and mod-check / import-check unchanged.
6. `CycleView.{h,cpp}` renderer + `buildCycleView` + `getBankThumbnails`.
7. `VizPayload.h` (rounding, hash, var builders), then editor timer wiring (bank-index watch, order, idle-quiet hash).
8. `viz-check`: G-VIZ-EXACT / CEIL / SILENT / IMPORTED / ALLOC + negative controls N1–N4 + G-VIZ-TIME log.
9. `./scripts/build-and-install.sh O-simpleWavetable`, then `ninja -C build O-simpleWavetable_Standalone`, then `auval -v aumu OSiW OuDv` and pluginval VST3/AU strictness 10.

**CHECKPOINT: user visual** in the Standalone:
- Marker follows LFO / env.
- Interp On fractional vs Off jumping.
- 3-bit staircase.
- A high note with Band-limit On shows the green ceiling + ghosts; Off shows the rust alias bars.
- Lesson buttons.

STOP for sign-off.

**Plan C, Phase 3.3 Import UX, Tooltips, i18n** (wave 3; after sign-off)
10. Import UX states: busy, done, error tooShort / unreadable / tooLarge, folder / type refusals, empty Imported. Add G-VIZ-IMPORTED if it was not done in B.
11. Run the 5 UI gates on the real tree. Fix only real findings: the mockup is locked, so a fix that changes layout or copy needs the pins re-measured.
12. Hands-on: checklist §9 in the Standalone (real file drop, keyboard, Reduce Motion, language switch). DAW pass in Logic or Live: automation → UI, hide/re-show, and **save/reopen with Imported** (the Stage 2 deferred item, VERIFICATION.md:131).
13. Path-scoped commit(s); handoff `/plugin-verify O-simpleWavetable`.

## Validation Architecture

### Test framework
| Property | Value |
|----------|-------|
| Framework | Offline console drivers (`ouaricon_add_processor_console`, `OSIW_TEST_HOOKS=1`) + node UI gates (+ Playwright for 2) |
| Build | Out-of-repo Debug, per Stage 2 RESEARCH:191 (`cmake -S . -B "$SCRATCH/build-oswt" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOUARICON_BUILD_TESTS=ON "-DSKIP_PLUGINS=$SKIP"`). Never set these in the shared `build/` (its cache has `OUARICON_BUILD_TESTS:BOOL=OFF`). Run long builds in the background (600 s watchdog memory). |
| Quick run | `"$SCRATCH/build-oswt/plugins/O-simpleWavetable/O-simpleWavetable-viz-check_artefacts/Debug/O-simpleWavetable-viz-check"`; require exit 0 and `grep -c 'JUCE Assertion failure'` = 0 (path pattern as Stage 2) |
| Full suite | bank-check, dsp-check (+ `--alloc-check`), mod-check, import-check, state-check, viz-check + the 5 UI gates |

### Requirement → gate map
| Req | Behaviour | Gate | Command |
|-----|-----------|------|---------|
| UI-01 | stack + highlight + marker follow the effective position | G-VIZ-EXACT / SILENT fields (`pos`, `frame`, `sounding`); thumbs exact (G-VIZ-IMPORTED); visual checkpoint | viz-check; Standalone |
| UI-02 | quantized cycle, staircases | G-VIZ-EXACT `cycle` vs output (3-bit row) + N1 | viz-check |
| UI-03 | bars = offline FFT of the heard cycle; band-limit ceiling | G-VIZ-EXACT dB, G-VIZ-CEIL, N2/N3/N4 | viz-check |
| UI-04 | filename + frame count; error states | G-VIZ-IMPORTED filename source; check-ui-labels states 4–6 (`tests/i18n-states.json`); hands-on drop | viz-check; `node scripts/check-ui-labels.js --plugin O-simpleWavetable` |
| UI-05 | tooltips on every control, localized | `node scripts/boot-all-uis.js --plugin O-simpleWavetable --strict-tips` (0 DEAD; exit 2 = dead binding); `node scripts/check-i18n.js --plugin O-simpleWavetable` (exit = failed-assertion count); `node scripts/i18n-fr-lint.js --plugin O-simpleWavetable`; `node scripts/i18n-zh-lint.js --plugin O-simpleWavetable` (exit 2 on a finding) | repo root |
| UI-06 | single page, projector-readable, pins hold | `check-ui-labels` [7] pins across 8 states; hands-on | repo root |
| PERF-03 | no audio-thread stall; smooth | G-VIZ-ALLOC (0 allocs with concurrent viz); dsp-check `--alloc-check`; G-VIZ-TIME log; idle-quiet (G-VIZ-SILENT); visual | viz-check |
| (lessons) | ids + recipes + output untouched | G-LESSON; `comm` diff of `data-preset` ids vs C++ ids | viz-check |
| (bridge) | every page native is registered | `comm -23` name diff (above) | shell |
| (regression) | Stage 2 goldens unchanged by the new atomics / `midiNote` | all 5 Stage 2 drivers ALL PASS; pluginval VST3/AU 10; `auval -v aumu OSiW OuDv` | — |

### Sampling rate
- Per task: build + run viz-check (and dsp-check after task 5).
- Per plan: all 6 drivers + boot-all-uis.
- Phase gate (before the Stage 3 commit): full suite + all 5 UI gates on the real tree + install + auval + pluginval + hands-on.

### Wave 0 gaps
- [ ] `tests/viz-check/main.cpp` + its CMake target (covers UI-01..03, PERF-03, lessons)
- [ ] `tests/i18n-states.json` (copied from the mockup; needed by check-ui-labels)
- [ ] Out-of-repo Debug tree `$SCRATCH/build-oswt` re-configured (new sources)

## Security Domain

V5 input validation is the only ASVS category that applies (no auth, sessions or crypto). Every WebView argument is untrusted:
- `uiMidi` clamps (cpp:597-603).
- `setUiLanguage` uses an allow-list.
- `applyFactoryPreset` accepts a 5-id allow-list (unknown → false, no change).
- `importDroppedAudio` gets a base64 length cap → standard decoder → processor 96 MB cap + `sanitiseName` (cpp:888).
- Chooser paths go through `importFromFile`'s existence check (cpp:865).

Threats:
- **Oversized drop (DoS):** capped before decode.
- **Filename injection (Tampering):** the name is sanitised, rendered as text, and `emitEvent` escapes `'` / `\` (cpp:424).
- **Async-chooser UAF:** SafePointer bail.

## Environment Availability

| Dependency | Available | Version / note |
|------------|-----------|----------------|
| JUCE | ✓ `/Users/taylorbrook/JUCE` | 8.0.15 |
| CMake / Ninja / Node | ✓ | 4.2.1 / 1.13.2 / v24.19.0 |
| Playwright browsers | ✓ partial | chromium + headless-shell 1228/1234 (`~/Library/Caches/ms-playwright`). On exit 77: `npx playwright install chromium`. |
| Shared `build/` Release artefacts (AU/VST3/Standalone) | ✓ | `OUARICON_BUILD_TESTS:BOOL=OFF` there; harness builds go out-of-repo |
| DAW (Logic / Live) | not probed | DAW items wait for the user |

## Contradictions Found (mockup scaffolding vs Source/); none blocking

1. Missing processor API: `applyFactoryPreset`, `getBankThumbnails`/`BankThumbs`, `buildCycleView`/`CycleView` (template v1-PluginEditor.cpp:52-64).
2. No `note` / `f0` / `fs` display source for the template's fields: needs 3 atomics + `WtVoice::midiNote` (Q2).
3. `frame` must be −1 for Interp On, but `dispFrame` holds a rounded frame there (WtVoice.h:436).
4. The template hashes the free-running `lfo`, so it emits at 30 Hz even at idle (Q1).
5. CONTEXT's "drops events while hidden" is broader than JUCE does (own visible flag only). The re-send is harmless; keep it.
6. The template comment says the worker publishes via `callAsync`; Source uses an AsyncUpdater (D-M, cpp:967-986). Comment only.
7. ROADMAP says "22 params", the code has 21 (cpp:163). This is the documented slip.
8. Alias Demo bank: ARCHITECTURE §A9 says Drive, the checklist says Sine→Saw. The checklist is locked for Stage 3.

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|-------|---------|---------------|
| A1 | The full `buildCycleView` costs tens of µs and var + JSON for 5 KB ~1 ms | Q1/Q4 | Low: G-VIZ-TIME logs reality; at 33 ms per tick even 10× is fine |
| A2 | No practical size limit on `evaluateJavascript` for ~200 KB | Q4 | Low: O-Prism moves MBs the other way; fallback is a flat `frames` array or 2 dp thumbs |
| A3 | WebKit delivers script messages on the main thread, so natives run on the message thread | Q7 | Low: every sibling relies on it |
| A4 | "dB re heard max" = max over bins 1..1023 (not 1..32) | Q3 | Low, a discretion call: a one-line change if the user prefers re max(1..32) |
| A5 | `RangedAudioParameter::getParameterID()` is available (JUCE 8) | Q7 | Low: compile-time; the alternative is iterating `ParamIDs::all` with `getParameter(id)` |
| A6 | JSON size estimates (Python emulation of `serialiseDouble`) | Q4 | Low: G-VIZ-TIME prints the real byte counts |

## Sources

- **Primary (HIGH, read this session):**
  - O-simpleWavetable `Source/` (PluginProcessor.*, WtVoice.h, WtRead.h, BitQuantizer.h, WavetableBank.h, MipmapBuilder.cpp, WavetableImporter.*), `CMakeLists.txt`.
  - `.planning/` CONTEXT, REQUIREMENTS, ROADMAP, ARCHITECTURE (row 17, §12, §Visualization, A9, Amendments 1–13), and stage 2 SUMMARY / VERIFICATION.
  - All `mockups/v1-*` files.
  - JUCE 8.0.15: `juce_WebBrowserComponent.cpp:414-428, 607-611`, `juce_JSON.{h,cpp}`, `juce_String.cpp:2287-2329`, `juce_FFT.h:80-115`, `juce_WebBrowserComponent_mac.mm:481-494`.
  - `scripts/param-dump/ParamDump.cmake:67-205`; `tests/dsp-check/main.cpp`; gate script usage blocks; `ui-static-gates.yml`.
  - Sibling editors / processors: O-simpleAdditive, O-Prism (515-590), O-simpleSubtractive (250-330).
- **Project memory (HIGH):**
  - WebView: completion-gated-on-isVisible (corrected scope), native-fn bridge gap, launchAsync SafePointer, one-shot push stale.
  - Presets: reset-to-defaults, never-set-output-gain, AsyncUpdater guard (already satisfied: `pendingAutoSelect` is cleared under the lock in every restore path, cpp:679/708/748).
  - Build / gates: Standalone stale, git-archive module copies, canvas font repaint, resource-provider bare paths, WebView2 user-data folder, UI-gates index, zh-Hans rollout.
- **Secondary (MEDIUM):** payload-size estimates via the scratch `jsonsize.py` (emulation of `serialiseDouble`, not a JUCE build).

## Metadata

- **Standard stack:** HIGH. No new dependencies; JUCE source read locally.
- **Architecture:** HIGH. Every seam was read in Source/; the template was diffed against the processor API.
- **Pitfalls:** HIGH. Each is tied to a file:line or a project-memory entry.
- **Payload / perf numbers:** MEDIUM (estimates; G-VIZ-TIME measures).
- **Research date:** 2026-10-06. **Valid until:** Stage 3 close, or any change to `WtVoice.h` / `PluginProcessor.*` display code.
