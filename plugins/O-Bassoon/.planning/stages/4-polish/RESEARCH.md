# Stage 4: Polish — Research

**Generated:** 2026-05-01 from `/plugin-research O-Bassoon 4-polish`
**Inputs:** `.planning/stages/4-polish/CONTEXT.md` (8 user-confirmed approach decisions + 10 derived + 10 OQs + 10 risks)
**Schema:** family-canonical RESEARCH.md (matches Stage 2 / Stage 3 precedent)
**Cycle scope:** Single Stage 4 polish pass — pluginval-10 cross-platform + Dorico parity + Logic-AU smoke + DSP-06 MPE + 4 factory presets + CHANGELOG + PLUGINS.md update.

---

## §1 Open Question Resolutions

### OQ1 — Dorico Playback Template ingestion path  *(R1 mitigation)*

**Resolution:** D11 was directionally right but used the wrong file extension. The canonical artefact is **`.doricolib`**, not `.dorico_pt`. The note-expression module v1.1.0 ships exactly this file.

**File path (in-repo source of truth):**
```
modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
```

**Production install path (target the user already has from prior cohort plugins, OR copy-stage manually for O-Bassoon Stage 4):**
- macOS: `~/Library/Application Support/Ouaricon/Microtonal Suite/`
- Windows: `%APPDATA%\Ouaricon\Microtonal Suite\`

**End-user import flow (one-time per machine, persistent across Dorico restarts/upgrades):**
1. Open Dorico → `Library → Library Manager → Import…`
2. Navigate to either the in-repo path OR the install path; select `Ouaricon-VST3-NoteExpression.doricolib`
3. Confirm import. Expression map "Ouaricon VST3 Note Expression" appears under `Library → Expression Maps`.

**Per-project assignment:**
1. `Play → Endpoints → Add Plug-in` → load `O-Bassoon-dev`
2. `Play → Endpoints → Expression Map` dropdown → select "Ouaricon VST3 Note Expression"

**Verification (memorized in module README + spike-findings v002):**
- Quarter-sharp accidental on C4 → measured pitch ≈ **269.29 Hz** (vs standard C4 = 261.63 Hz; +50¢)
- If pitch lands at standard C4 → expression map not assigned, OR import not yet performed

**Underlying mechanics (cited from note-expression/README.md:210-216):**
> Dorico's default and "Auto" microtonality settings route microtones to VST2 detune or pitch bend for non-Steinberg VST3 plugins — neither reaches a JUCE-based VST3 plugin. The shipped expression map sets `microtonalPlaybackMethod=kVST3NoteExpression`, the load-bearing Dorico setting that routes microtones as VST3 Note Expression events.

**Stage 4 implication for O-Bassoon (no installer per D4 "internal install only"):** the user must manually import the `.doricolib` from one of two source paths:
- in-repo path (works always)
- shared-suite path under `~/Library/Application Support/Ouaricon/Microtonal Suite/` (only present if a prior Ouaricon plugin was installed via PKG/EXE installer; not guaranteed for O-Bassoon at v1.0 since `/package` and `/publish` are out-of-scope per CONTEXT D4)

**Resolved CONTEXT discrepancy:** Update CONTEXT D11 + R1 references from `.dorico_pt` to `.doricolib` at plan-phase doc-fix.

---

### OQ2 — Dorico microtonal test score authorship  *(R1 mitigation)*

**Resolution:** O-Lyrica's spike-findings does **NOT** ship a canonical `.dorico` file. We author one fresh from the spike-002 README recipe. Recipe is fully documented at `.claude/skills/spike-findings-VST-development/sources/002-quarter-sharp-end-to-end/README.md`.

**Setup steps (lifted verbatim from spike-002):**
1. `File → New from Template → Solo → Piano` (any pitched template; Piano is convenient because clef matches O-Bassoon C1–C6 range)
2. `Play mode` → swap Piano track's VST to **Ouaricon Audio → O-Bassoon-dev**
3. Back in `Write mode`, select bar
4. `Library → Tonality Systems…` → add preset **24-EDO Equal Temperament** (or "24-EDO" preset that includes ¼♯/¼♭ accidentals)
5. Right panel Key Signatures → set 24-EDO key sig (C major 24-EDO is fine)
6. Per-project: Library Manager already imported `.doricolib` (per OQ1); assign "Ouaricon VST3 Note Expression" to the O-Bassoon channel

**Test note set (Stage 4 expanded vs spike-002):**

Author 8 quarter notes back-to-back on a single C4 staff line in 4/4. Two bars total, one note per beat. Three categories:

| Beat | Accidental | Expected pitch (Hz @ A=440) | Expected NE (cents) | Notes |
|------|-----------|------------------------------|---------------------|-------|
| 1.1  | none (12-TET reference) | 261.63 (C4)              | 0¢                  | baseline |
| 1.2  | ¼♯ (quarter-sharp)      | 269.29                   | +50¢ (Dorico sends `pitch=61, NE=-50¢` per spike-002 Surprise 1) | quarter-tone up |
| 1.3  | ½♯ (semi/half-sharp)    | 277.18                   | +100¢ (= C#4)       | sanity check (Dorico will probably send plain `pitch=61`) |
| 1.4  | ¾♯ (three-quarter-sharp) | 285.31                  | +150¢               | between C#4 and D4 |
| 2.1  | none (12-TET reference) | 261.63 (C4)              | 0¢                  | baseline (loop check — same pitch) |
| 2.2  | ¼♭ (quarter-flat)       | 254.18                   | -50¢ (Dorico sends `pitch=59, NE=+50¢` per spike-002 Surprise 1) | quarter-tone down |
| 2.3  | ½♭ (semi/half-flat)     | 246.94 (= B3)            | -100¢               | sanity check |
| 2.4  | ¾♭ (three-quarter-flat) | 239.91                   | -150¢               | between B3 and Bb3 |

**Pass criterion (Method A — fastest):** load a tuner plugin in Dorico's effect chain (e.g., GTune, Voxengo Tuner, MTuner). Solo O-Bassoon track, play each beat, read cents offset. Tolerance: **±5¢** per spike-002 verdict bar. Treat as PASS if every beat lands within ±5¢ of the Expected column.

**A/B parity test against O-Lyrica baseline (per CONTEXT D12):**
- Save the .dorico file with O-Bassoon as the channel VST → save copy with O-Lyrica as the channel VST
- Play both; verify identical audible pitches (same quarter-tone offsets)
- Pitch should track audibly identical between the two plugins; this proves the note-expression module's `applyPendingTuning` composition is ABI-stable across consumers (per project memory `project_o_lyrica_spike_reference.md`)

**Persist artifact:** save the authored .dorico file at `plugins/O-Bassoon/.planning/stages/4-polish/test-score-microtonal.dorico` (commit at execute-phase). Recipe is reproducible from RESEARCH.md if file is lost.

---

### OQ3 — Logic Pro MPE setup  *(R5 mitigation)*

**Resolution:** Logic Pro's MPE support is **stock-instrument-centric** — MIDI Mono mode is exposed only on Logic's own AU instruments (Alchemy, EFM1, ES2, Quick Sampler, Retro Synth, Sampler, Sculpture, Vintage Clav). For **third-party AU plugins like O-Bassoon-dev, Logic does NOT expose a host-side "make this plugin MPE" setting** — the plugin itself must implement MPE-aware MIDI handling, AND a per-channel MIDI source must reach the plugin instance.

**Critical fact:** O-Bassoon already implements MPE-compatible per-voice pitch-bend routing. Stage 0 D3 + Phase 2.4 OQ#7-rev-4 lock this:
> `juce::Synthesiser::handlePitchWheel` iterates voices and calls `pitchWheelMoved` only when `voice->isPlayingChannel(midiChannel)` is true. So a plain `juce::Synthesiser` (NOT `MPESynthesiser`) routes per-channel pitch-bend to per-voice — provided each voice is started on a distinct MIDI channel.

**Verified at code level (BassoonVoice.cpp):**
- L54-55, L127-128: pitch-wheel-to-semitones conversion (`((value - 8192) / 8192) × PITCH_BEND_RANGE_SEMITONES`)
- L124-138: `pitchWheelMoved` per-voice override; recomputes `modeBank.setFundamental(fBent)` only if `currentFrequencyBase > 0`
- L70: startNote applies `fBent = currentFrequencyBase × pow(2, pb/12)` so first sample is already pitch-bent

**Logic-side test setup paths (3 options, ranked by friction):**

**Path A — Pre-authored MIDI file (recommended for Stage 4; no hardware required):**
1. Author a Standard MIDI File (Type 1) externally with notes on channels 2/3/4/5 + per-channel pitch-bend events. Tools: Reaper (free trial), MuseScore, or `mido`/`pretty_midi` Python script. Spike-style: a 50-line Python script emitting 4 simultaneous noteOn events on channels 2/3/4/5 with respective `pitchwheel` events at +1 semi / 0 / +2 semi / -1 semi.
2. In Logic: New project → Software Instrument track → load `O-Bassoon-dev` AU. Drag the .mid file onto the track. Logic imports each MIDI channel as a separate region OR (if all channels are merged into one track at import) replays the channel data verbatim.
3. Insert `MTuner` (or any tuner) as the next plugin in the channel strip; play the region.
4. Verify each note's pitch lands at the expected per-channel-PB-shifted value.

**Path B — Logic 11 MPE-keyboard input (no hardware, but requires Logic 11):**
- Logic Pro 11.0+ exposes an MPE input mode for the typing-keyboard / Touch Bar that auto-rotates new notes through channels 2-16. Per-channel modulation is mapped to mouse-Y on the typing keyboard. Adequate for a quick "are voices isolated by channel" check; fiddly for precise pitch-bend values.

**Path C — Hardware MPE controller (Linnstrument / Seaboard / Osmose):**
- Most expressive; not required for Stage 4 since fallback Path A covers DSP-06 MPE-half closure with deterministic input.

**Decision: use Path A.** Author a deterministic MIDI test file once, commit it at `plugins/O-Bassoon/.planning/stages/4-polish/test-mpe-pitchbend.mid`, replay in any future Logic regression test.

**Sources cited:**
- Apple support: [Use MPE with software instruments in Logic Pro for Mac](https://support.apple.com/guide/logicpro/use-mpe-with-software-instruments-lgcp8f599497/mac)
- Apple support: [Pitch bend events in Logic Pro for Mac](https://support.apple.com/guide/logicpro/pitch-bend-events-lgcp2158a4c2/mac)
- Pitch Innovations: [MPE Explained — Complete Guide 2025](https://pitchinnovations.com/blog/mpe-explained-what-is-mpe-and-why-you-should-care/)

---

### OQ4 — Windows VST3 build environment  *(R2 mitigation)*

**Resolution:** Build infrastructure already in place. `scripts/build-and-install.ps1` exists at repo root and is fully wired for O-Bassoon — same 7-phase pipeline used for O-AnalogEQ, O-Wind, O-MicrotonalSampler (all clear pluginval-10 on Windows per CONTEXT R2).

**Confirmed assets:**

| Asset | Path | Status |
|-------|------|--------|
| Build script | `scripts/build-and-install.ps1` | EXISTS — verified header lines 1-45 + phase functions L107-431 |
| O-Bassoon CMakeLists.txt webview flags | `plugins/O-Bassoon/CMakeLists.txt:113` | `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` ✅ present |
| O-Bassoon CMakeLists.txt NEEDS_WEBVIEW2 | `plugins/O-Bassoon/CMakeLists.txt:20` | `NEEDS_WEBVIEW2 TRUE` ✅ present |
| Editor user-data folder (Win sandbox guard) | `plugins/O-Bassoon/Source/PluginEditor.cpp` | Confirm at execute-phase per project memory critical pattern; should follow O-MicrotonalSampler pattern `withWinWebView2Options(Options::WinWebView2{}.withUserDataFolder(File::getSpecialLocation(File::tempDirectory).getChildFile("OBassoon_WebView")))` |

**Build invocation (per CLAUDE.md Windows protocol + script -Reconfigure flag if needed):**
```powershell
# Direct cmake (manual)
cmake --build build --config Release --target O-Bassoon_VST3 --parallel

# OR via script (recommended — also installs + clears DAW caches)
.\scripts\build-and-install.ps1 O-Bassoon
.\scripts\build-and-install.ps1 O-Bassoon -Reconfigure   # if first build OR cmake config dirty
```

**Build pre-flight (ps1 phase 1 verifies):** CMake, Ninja, Visual Studio 2022 (vswhere), JUCE at `C:\JUCE` or `$env:JUCE_DIR`. Not verified at research-phase since this is a macOS host; user/operator confirms when hand-running on Windows.

**pluginval-10 invocation (per CONTEXT D14):**
```powershell
pluginval.exe --strictness-level 10 --validate "$env:COMMONPROGRAMFILES\VST3\O-Bassoon-dev.vst3"
```
- Family precedent (O-AnalogEQ + O-Wind + O-MicrotonalSampler): all clear strictness-10 on Windows.
- pluginval download: <https://github.com/Tracktion/pluginval/releases> (Windows .zip → extract `pluginval.exe` to PATH or known location)

**No Inno Setup needed at v1.0** — D4 specifies internal install only, no PKG/EXE installer (Inno Setup applies only when `/package` is run; out of scope here).

**WebView2 runtime:** static-linked per `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, NO `WebView2Loader.dll` redistribution required. Project memory critical pattern confirms this is the load-bearing flag.

**Risk delta:** R2 medium-medium → **low-low** after this resolution (script + flags already correct).

---

### OQ5 — Preset binary format + commit hygiene  *(R6 mitigation)*

**Resolution:** Use the family-canonical `OuariconPresetManager` infrastructure (header-only, located at `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`). Factory presets are defined as **compile-time C++ structs** — NO `.aupreset` / `.fxp` / `.vstpreset` binaries ever land in git.

**Family precedent (3 production examples):**

| Plugin | Presets | Approach | Reference |
|--------|---------|----------|-----------|
| O-Bowed v1.3.0 | ~10 (Violin / Cello / Viola / Double Bass / Erhu / Sarangi / Nyckelharpa + 3 sound-design) | Inline in `PluginProcessor::initializeFactoryPresets()` | `PluginProcessor.cpp:456-587` |
| O-Wind v1.16.0 | factory presets | Same: `presetManager(parameters, "O-Wind")` + `initializeFactoryPresets()` | `PluginProcessor.cpp:405,442` |
| O-Prism v1.16.1 | 96 across 9 categories | External `Source/FactoryPresets.{h,cpp}` (bigger preset set warrants separate file) | `Source/FactoryPresets.h:14-22` |

**Mechanism:**
- `OuariconPresetManager` writes presets as **JSON files** at first run to `~/Library/Application Support/{pluginName}/Presets/Factory/` (and `User/` for user-saved).
- The C++ definition (`vector<FactoryPresetDef>`) is written to disk by `presetManager.initializeFactoryPresets(factoryPresets)` only if the Factory dir is empty.
- DAW state save/load (`getStateInformation` / `setStateInformation`) is then a thin XML round-trip via `presetManager.getStateAsXml()` / `presetManager.setStateFromXml()`.
- **Optional custom-state hook** (`setCustomStateCallbacks`) wraps APVTS extras like the tuning state — O-Bassoon currently has only APVTS state (10 params), so custom-state is **NOT required** at v1.0; can be added in v1.1 if Tuning panel state needs preset-level persistence.

**Wiring delta for O-Bassoon (Stage 4 plan-phase will ratify):**

```cmake
# CMakeLists.txt — add include path (NOT ouaricon_add_module — header-only module)
target_include_directories(O-Bassoon
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp   # NEW
)
```

```cpp
// PluginProcessor.h — add member + helper signature
#include "OuariconPresetManager.h"

class OBassoonAudioProcessor : public juce::AudioProcessor {
    // ... existing members
    OuariconPresetManager presetManager;     // NEW
    void initializeFactoryPresets();         // NEW
};
```

```cpp
// PluginProcessor.cpp — constructor delegate-init + factory dispatch
OBassoonAudioProcessor::OBassoonAudioProcessor()
    : AudioProcessor(...)
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Bassoon")    // NEW init
{
    // ... existing voice setup
    initializeFactoryPresets();                  // NEW dispatch
}

// REPLACE existing getStateInformation/setStateInformation:
void OBassoonAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OBassoonAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}
```

**Factory preset definitions (4 presets per CONTEXT D4 / ROADMAP §Stage4):**

All values stored as normalized 0.0–1.0 (param.convertTo0to1 / range mapping). Param ranges from `createParameterLayout()`:
- vibrato_rate ∈ [0,10] Hz
- vibrato_depth ∈ [0,100] cents
- vibrato_onset ∈ [0,2000] ms
- breath ∈ [0,1] (already normalized)
- tone ∈ [0,1] (already normalized)
- attack_character ∈ [0,1] (already normalized)
- attack_time ∈ [0,2000] ms
- release_time ∈ [0,3000] ms
- voice_count ∈ [1,16] int
- output_gain ∈ [-24,+6] dB

**Starting values (execute-phase tunes by ear; these are sane defaults):**

```cpp
// Source/FactoryPresets.cpp — separate file for cleanliness (O-Prism precedent)
std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
    {
        "Long Drone",
        {{"vibrato_rate",     0.45f},   // 4.5 Hz
         {"vibrato_depth",    0.12f},   // 12 cents
         {"vibrato_onset",    0.40f},   // 800 ms
         {"breath",           0.50f},
         {"tone",             0.40f},
         {"attack_character", 0.00f},   // soft
         {"attack_time",      0.60f},   // 1200 ms
         {"release_time",     0.60f},   // 1800 ms
         {"voice_count",      0.20f},   // 4 voices ((4-1)/15)
         {"output_gain",      0.70f}},  // -3 dB ((-3-(-24))/30)
        juce::var()
    },
    {
        "Microtonal Pad",
        {{"vibrato_rate",     0.40f},   // 4 Hz
         {"vibrato_depth",    0.25f},   // 25 cents
         {"vibrato_onset",    0.75f},   // 1500 ms (fade-in)
         {"breath",           0.65f},
         {"tone",             0.50f},
         {"attack_character", 0.00f},
         {"attack_time",      0.75f},   // 1500 ms
         {"release_time",     0.7333f}, // 2200 ms
         {"voice_count",      0.4667f}, // 8 voices (7/15)
         {"output_gain",      0.7333f}},// -2 dB
        juce::var()
    },
    {
        "Tongued Long Tone",
        {{"vibrato_rate",     0.55f},   // 5.5 Hz
         {"vibrato_depth",    0.18f},   // 18 cents
         {"vibrato_onset",    0.15f},   // 300 ms (fast)
         {"breath",           0.75f},
         {"tone",             0.55f},
         {"attack_character", 1.00f},   // tongued
         {"attack_time",      0.04f},   // 80 ms
         {"release_time",     0.40f},   // 1200 ms
         {"voice_count",      0.20f},   // 4 voices
         {"output_gain",      0.7667f}},// -1 dB
        juce::var()
    },
    {
        "Bright Bassoon",
        {{"vibrato_rate",     0.60f},   // 6 Hz
         {"vibrato_depth",    0.10f},   // 10 cents (lighter)
         {"vibrato_onset",    0.20f},   // 400 ms
         {"breath",           0.70f},
         {"tone",             1.00f},   // max bright
         {"attack_character", 0.20f},   // soft-ish
         {"attack_time",      0.10f},   // 200 ms
         {"release_time",     0.2667f}, // 800 ms
         {"voice_count",      0.20f},   // 4 voices
         {"output_gain",      0.80f}},  // 0 dB
        juce::var()
    }
};
```

**Round-trip verification path (Gate 6 item 8):**
1. In Logic, recall each preset via DAW preset menu (the standard Logic AU preset menu shows whatever the host sees from `setStateInformation`).
2. Save current state to `.aupreset` → reload → confirm parameter values match.
3. **For DAW-host portability** (FXP for VST3 in Reaper, etc.): not required at v1.0. The OuariconPresetManager JSON path is the canonical "preset" surface — DAW presets are a secondary concern handled by JUCE's standard `getStateInformation` path.

**Commit hygiene:**
- `Source/FactoryPresets.cpp` is the canonical preset payload (commits cleanly to git, ~70 lines).
- The JSON files at `~/Library/Application Support/O-Bassoon/Presets/Factory/*.json` are **generated at first run**, NOT committed — they are user-data. (.gitignore already excludes user-home paths trivially.)
- No `.aupreset` / `.fxp` / `.vstpreset` binaries ever land in git.

**Risk delta:** R6 medium-medium → **low-low** (family-canonical infra eliminates cross-DAW portability question).

---

### OQ6 — CHANGELOG location + schema  *(R9 mitigation)*

**Resolution:** Per-plugin location at `plugins/O-Bassoon/CHANGELOG.md`. Schema = Keep-a-Changelog flavor as established by O-MicrotonalSampler v1.11.0, O-Wind v1.16.0, O-Prism v1.16.1 (10+ existing per-plugin CHANGELOGs in repo).

**Format template (lifted from O-MicrotonalSampler v1.11.0):**

```markdown
# O-Bassoon Changelog

## [1.0.0] - 2026-05-01

### Added — Modal-Synthesis Bassoon for Microtonal Long Tones

**O-Bassoon** is a 16-voice modal-synthesis bassoon synthesizer designed for
sustained microtonal long tones. The voice combines a bassoon-tuned 16-mode
biquad bank, continuous filtered-noise excitation, and a dual-shape onset
exciter (soft pad ↔ tongued articulation morph).

**DSP architecture (Phase 2.1–2.4):**
- 16-mode parallel biquad bank, frequency-adaptive partial table tuned to
  bassoon resonance pattern (Phase 2.2 spectral shaping).
- Continuous filtered-noise excitation (1-pole LP @ 2 kHz, breath-scaled,
  per-voice deterministic seed) replaces struck-modal-only sustain (Phase 2.3).
- Dual-shape onset exciter — 30 ms LP-filtered soft pad shape vs 7.5 ms
  exp-decay × white-noise tongued shape, morphed by `attack_character`
  param (Phase 2.4 — DSP-05 v1.1 candidate, see Known Limitations).
- Per-voice sine-LFO vibrato (rate 0–10 Hz, depth 0–100 cents, onset 0–2 s)
  with multiplicative pitch-bend compose + block-rate recompute (Phase 2.3).
- ADSR amplitude envelope tuned for long-tone use (attack 0–2 s, release
  0–3 s) via `juce::ADSR` (Phase 2.3).
- 16-voice polyphony with active-cap, release-tail-first voice stealing
  (`BassoonSynthesiser` subclass override of `findFreeVoice` + base
  `findVoiceToSteal`) (Phase 2.4 — FUNC-02, FUNC-05).

**Microtonal pipeline (Phase 2.4 + Stage 4):**
- VST3 Note Expression for Dorico microtonal scores via shared
  `note-expression` v1.1.0 module (`kTuningTypeID` per-note tuning deltas).
- MPE per-channel pitch-bend routing for hosts/controllers that emit
  channel-rotated notes (Logic Pro MPE mode, Linnstrument, Seaboard).
- Headless `TuningEngine` v2.1.0 wired at v1.0 (12-TET default + 12 embedded
  tunings + Scala .scl import via Tuning panel).
- Composition order: `TuningEngine.getFrequency` → `applyPendingTuning` (NE
  delta) → pitch-bend multiplier → mode bank fundamental.

**Expression surface (10 APVTS parameters):**
- Vibrato (3): rate, depth, onset
- Expression (3): breath, tone, attack_character
- Envelope (2): attack_time, release_time
- Voicing (1): voice_count (1–16)
- Output (1): output_gain (-24..+6 dB)
- + CC2 (MIDI breath controller) → continuous breath drive
- + CC1 (mod wheel) → reserved for future vibrato remapping

**WebView UI (Stage 3 — UI-01, UI-02):**
- 900×600 Ouaricon-botanical aesthetic (paper + sage-green palette, fern
  overlay, Garamond serif).
- 4 sections: Vibrato 3 / Expression 3 / Envelope 2 / Voicing+Output 2.
- 3 tabs: Sound (default) / Tuning (shared `tuning-panel.{js,css}`) / About.
- 30 Hz live feedback: active-voice dots + breath/CC2 meter + pulsing
  vibrato-active dot.

**Factory presets (4):**
- "Long Drone" — low breath, slow attack/release, moderate vibrato
- "Microtonal Pad" — 8 voices, slow attack, vibrato fade-in
- "Tongued Long Tone" — `attack_character=1`, fast attack, mod wheel vibrato
- "Bright Bassoon" — `tone=1`, soft attack, lighter vibrato

**Validation:**
- pluginval --strictness 10: macOS AU + VST3 + Windows VST3 ✅
- auval `aumu OBsn OuDv` ✅
- Logic-AU 60s long-tone (numpy.isfinite + RMS drift <0.5 dB + CPU drift
  <2%) ✅ (Phase 2.4) + post-Stage-3 regression-confirmed ✅
- 8-voice polyphony CPU <25% ✅
- Dorico microtonal parity (24-EDO ¼♯/¼♭/¾♯/¾♭/½♯/½♭ test score) ✅
  audibly identical to O-Lyrica baseline.
- DSP-06 MPE-half: 4-voice channel-rotated pitch-bend test (CH 2/3/4/5,
  ±2 semis) ✅ per-voice tracking confirmed.

**Files Added:**
- `Source/PluginProcessor.{h,cpp}` (10 APVTS params + voice manager + NE drain)
- `Source/PluginEditor.{h,cpp}` (WebView UI)
- `Source/BassoonSound.h`, `BassoonVoice.{h,cpp}`, `BassoonSynthesiser.{h,cpp}`
- `Source/ModeBank.{h,cpp}`, `Exciter.{h,cpp}`, `Vibrato.{h,cpp}`, `NoiseExciter.{h,cpp}`
- `Source/FactoryPresets.{h,cpp}` (4 factory presets)
- `Resources/ui/index.html` + JS + fern.png
- `CMakeLists.txt` (juce_add_plugin + binary data + WebView2 static linking)

**Modules consumed:**
- `note-expression` v1.1.0 — VST3 Note Expression for Dorico
- `scala-tuning-engine` v2.1.0 — 12 embedded tunings + .scl import + Tuning panel
- `persistence/preset-manager` (header-only) — JSON-based preset persistence

### Known Limitations

- **Attack character morph is subtle** (DSP-05 v1.1 candidate). The 30 ms
  LP-filtered soft pad ↔ 7.5 ms tongued shape morph is audibly perceptible
  but does not produce a dramatic textural shift across the full 0–1
  range. v1.1 architectural pivot path: NoiseExciter onset gate ramp 0→1
  over the first ~30 ms so the dual-shape Exciter dominates the audible
  onset character.
- **No preset browser UI** at v1.0. Presets recall via DAW preset menu
  (Logic AU dropdown) only. Inline preset browser deferred to v1.1.
- **No PKG/EXE installer** at v1.0. Internal install only via
  `/install-plugin O-Bassoon` to `~/Library/Audio/Plug-Ins/{VST3,Components}/`.
  Public distribution deferred to v1.0.1 / v1.1 (`/package` + `/publish`).
- **Bitwig MPE not verified.** Logic Pro MPE substitutes per Stage 4 D5;
  Bitwig MPE host verification deferred (Logic-MPE is production-quality
  Apple-shipped equivalent).

### Phase Trail (commit shas)

- Phase 2.1 (first audio): `d1b3370`
- Phase 2.2 (spectral tuning + tone): `baac74f`
- Phase 2.3 (expression — vibrato + breath + ADSR + output_gain): `0a64b77`
- Phase 2.4 (polyphony + NE/MPE + attack-character — Gate 4 PARTIAL): `dcc442c`
- Stage 1 (foundation — silent shell + APVTS + NE wiring): `b24dc0c`
- Stage 3 (GUI — WebView UI + Tuning tab + push channels): _(pending land
  before Stage 4 commit per Process Invariant 1)_
- Stage 4 (polish + v1.0.0 release): _(this commit)_
```

**Schema rules (locked at research-phase):**
- Top H1: `# O-Bassoon Changelog`
- Each release H2: `## [X.Y.Z] - YYYY-MM-DD`
- Sections: `### Added` (new feature) / `### Changed` (modify existing) / `### Fixed` (bug) / `### Notes` (housekeeping) / `### Improved` (enhancement) / `### Known Limitations` (carry-forward to next version)
- Phase trail at end with commit shas (matches O-MicrotonalSampler tradition; useful for git archaeology)

**Risk delta:** R9 low-medium → **low-low** (template fully drafted).

---

### OQ7 — PLUGINS.md row schema  *(R9 mitigation)*

**Resolution:** Schema fully documented in PLUGINS.md§Entry Template (L88-90):

```markdown
| [PluginName] | [Emoji] [State] | [X.Y.Z or -] | [Type or -] | YYYY-MM-DD |
```

**State vocabulary (Legend L4-10):**
- 💡 Ideated — Creative brief exists, no implementation
- 💡 Ideated (Draft Params) — Creative brief + draft parameters
- 🚧 Stage N — In development (specific stage number)
- ✅ Working — Completed Stage 6, not installed
- 📦 Installed — Deployed to system folders
- 🐛 Has Issues — Known problems (combines with other states)
- 🗑️ Archived — Deprecated

**Current row (PLUGINS.md L58):**
```
| O-Bassoon | 🚧 Stage 0 | - | Synth (Physical Model Bassoon) | 2026-04-27 |
```

**Stage 4 update (Gate 6 PASS post-`/install-plugin O-Bassoon`):**
```
| O-Bassoon | 📦 Installed | 1.0.0 | Synth (Physical Model Bassoon) | 2026-05-01 |
```

**Type field:** ROADMAP describes O-Bassoon as "modal synthesis bassoon" — but every other Ouaricon synth uses "Physical Model" or "Physical Modeling" prefix even when the algorithm is technically modal. Match the existing draft row's "Synth (Physical Model Bassoon)" — this also tracks family precedent (O-Bowed = "Physical Model Bowed String", O-Wind = "Physical Model Flute", O-Reed = "Physical Modeling Reed Wind"). _Do NOT_ change "Physical Model" to "Modal Synthesis" at v1.0; cohort-uniform naming wins.

**NOTES.md — NOT required at v1.0.** Audited recent v1.0 pitched plugins: O-Wind, O-Bowed, O-Reed, O-Lyrica all ship WITHOUT a NOTES.md sibling (only legacy O-* plugins from earlier cohorts have it). The PLUGINS.md "for detailed plugin info, see NOTES.md" pointer is a v1.0.1+ candidate; out of scope per CONTEXT D4.

**Risk delta:** R9 low-medium → **low-low** (one-line edit, schema fully resolved).

---

### OQ8 — Logic-AU 60s sustain success criterion  *(R4 mitigation)*

**Resolution:** Subjective "no glitches" Logic-AU audition. **NOT** the strict numpy battery used at Phase 2.4 verify-phase.

**Rationale:**
- QUAL-02 (60-second long-tone stability) is **already complete** at Phase 2.4 via the strict bar (numpy.isfinite + RMS drift <0.5 dB + CPU drift <2% — REQUIREMENTS.md L66 "complete; stage-2"). Stage 4 is a regression re-confirmation, not a fresh acceptance test.
- Stage 3 verify (`.planning/stages/3-gui/VERIFICATION.md`) used T17 Gate 5 "8-item Logic-AU full incl. 60s long-tone + push-channel audition" — subjective bar; PENDING USER (carries forward as Stage 4 Gate 6 item #2).
- The Stage 3 atomic commit only changes UI binaries + push-channel atomics + 3 header-inline accessors. None touch the audio render path (Phase 2.4 modeBank / NoiseExciter / Vibrato / ADSR / output_gain). Risk of audible regression is **low-low** (R4 in CONTEXT).
- O-Bassoon does NOT have a render-harness / tests/ directory (audited 2026-05-01). Reproducing the Phase 2.4 strict numpy bar would require building one, which is out-of-scope for Stage 4 polish.
- CONTEXT OQ8 explicitly invites "subjective may be appropriate" for Stage 4 because it's end-user-focused.

**Pass criterion (Stage 4 Gate 6 item #2 = T17 carry-forward):**
- Hold a single C3 note for 60 seconds in Logic-AU at `~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component`.
- Listen for: clicks, pops, pitch drift, amplitude drift, NaN-driven silence, denormal-driven CPU spikes, vibrato-onset glitches, breath envelope flicker.
- PASS if none audible. (Same effective bar as Phase 2.4 user gate — which was reported PASS by user.)

**Optional enhanced bar (NOT required, available if regression suspected):**
- Render 60-second C3 sustain via Logic Bounce → save as `phase4-60s-c3-regression.wav` → run `python3 -c 'import scipy.io.wavfile as w; r = w.read("...")[1]; assert numpy.all(numpy.isfinite(r))'` for the explicit no-NaN/no-Inf check.
- Skip unless subjective audition shows anything questionable.

**Decision: subjective audition by default; numpy bar only if subjective flags an issue.**

---

### OQ9 — DSP-06 MPE Logic verification protocol  *(R5 mitigation)*

**Resolution:** Path-A from OQ3 (pre-authored MIDI file) is the canonical Stage 4 protocol. 4-voice channel-rotated MPE smoke test.

**Test recipe:**

**Setup:**
1. Author a SMF Type-1 MIDI file with 4 simultaneous notes (each lasting 4 beats / 2000 ms at 120 BPM):
   - Channel 2: noteOn pitch=60 (C3), velocity=80, **PB +8192** (= +1 semitone at default 2-semi range)
   - Channel 3: noteOn pitch=64 (E3), velocity=80, **PB +0** (= no bend)
   - Channel 4: noteOn pitch=67 (G3), velocity=80, **PB +16383** (= +2 semitones, full positive)
   - Channel 5: noteOn pitch=71 (B3), velocity=80, **PB -8192** (= -1 semitone)
   - All 4 noteOff at beat 5 / 2500 ms

2. Save as `plugins/O-Bassoon/.planning/stages/4-polish/test-mpe-pitchbend.mid`. Commit at execute-phase.

3. Authoring tool (recommended Python script for repeatability):
```python
# scripts/author-mpe-test-midi.py (reference, not committed unless reused)
import mido
mid = mido.MidiFile(type=1)
trk = mido.MidiTrack(); mid.tracks.append(trk)
trk.append(mido.MetaMessage('set_tempo', tempo=mido.bpm2tempo(120), time=0))

# Bend events first (so synth has bend ready when noteOn fires)
notes = [(2,60,8192-0+1024), (3,64,0), (4,67,8191), (5,71,-8192)]
for ch, pitch, bend in notes:
    trk.append(mido.Message('pitchwheel', channel=ch-1, pitch=bend, time=0))

# Simultaneous noteOn (delta=0 between them)
for i, (ch, pitch, _) in enumerate(notes):
    trk.append(mido.Message('note_on', channel=ch-1, note=pitch, velocity=80, time=0))

# Hold 2000 ms (= 960 ticks at default 480 ppq + 120 BPM)
# noteOff cluster
for i, (ch, pitch, _) in enumerate(notes):
    trk.append(mido.Message('note_off', channel=ch-1, note=pitch,
                            velocity=64, time=(960 if i == 0 else 0)))

mid.save('test-mpe-pitchbend.mid')
```

**Test execution in Logic:**
1. New Logic project → Software Instrument track → load `O-Bassoon-dev` AU
2. **Critical: enable per-track multi-channel MIDI input.** Default Logic strips/remaps incoming MIDI to channel 1; this would defeat the test. Open Track Inspector → MIDI Channel = "All" or remove any "Translate to Channel 1" plugin in the track input chain.
3. Drag `test-mpe-pitchbend.mid` onto the track. Logic creates a region.
4. Insert tuner plugin (MTuner or any chromatic tuner) on the channel strip after O-Bassoon.
5. Play the region.

**Expected pitches (all ±5¢):**
- C3 + 1 semi = C#3 ≈ 138.59 Hz
- E3 + 0 semi = E3 ≈ 164.81 Hz
- G3 + 2 semi = A3 ≈ 220.00 Hz
- B3 - 1 semi = Bb3 ≈ 233.08 Hz

**Pass criterion:** all 4 voices play simultaneously at distinct, per-channel-PB-offset pitches matching the table within ±5¢. Each voice's pitch is independent — bending one channel does NOT affect others.

**Underlying mechanic check (no code-level test required at Stage 4 — already verified at Phase 2.4):**
- `juce::Synthesiser::handlePitchWheel(int midiChannel, int wheelValue)` iterates `voices[]` and calls `voice->pitchWheelMoved(wheelValue)` only when `voice->isPlayingChannel(midiChannel) == true` (per Stage 0 D3 lock + Phase 2.4 OQ#7-rev-4).
- Each voice's `currentlyPlayingChannel` is set at `startNote` time from the noteOn event's MIDI channel.
- Therefore: noteOn ch=2 → voice 0 binds to ch=2 → CH2 PB only mutates voice 0's `pitchBendSemitones` → voice 0's `modeBank.setFundamental(fBent)`.

**Fallback if Path A fails (low risk per CONTEXT R5):** Skip to Logic 11 typing-keyboard MPE input mode (Path B). Final fallback: trust Phase 2.4 static-check coverage + accept partial DSP-06 MPE-half (CONTEXT R5 final-fallback).

---

### OQ10 — Atomic commit scope (Stage 3 + Stage 4 fold-in)  *(R7 mitigation)*

**Resolution:** Two strict commits, separated by an explicit hard gate.

**Working-tree audit (2026-05-01):**

```
Modified:
  plugins/O-Bassoon/.planning/BRIEF.md
  plugins/O-Bassoon/.planning/REQUIREMENTS.md
  plugins/O-Bassoon/.planning/ROADMAP.md
  plugins/O-Bassoon/.planning/STATUS.md
  plugins/O-Bassoon/CMakeLists.txt
  plugins/O-Bassoon/Source/BassoonVoice.h
  plugins/O-Bassoon/Source/PluginEditor.cpp
  plugins/O-Bassoon/Source/PluginEditor.h
  plugins/O-Bassoon/Source/PluginProcessor.cpp
  plugins/O-Bassoon/Source/PluginProcessor.h
  plugins/O-Bassoon/Source/Vibrato.h

Untracked:
  plugins/O-Bassoon/.planning/stages/3-gui/
  plugins/O-Bassoon/.planning/stages/4-polish/
  plugins/O-Bassoon/Resources/
```

**Commit 1 — Stage 3 (lands FIRST per CONTEXT D8 process invariant; user trigger required):**

Subject: `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`

Files (11 modified + 3 untracked groups):
| Path | Change | Stage 3 rationale |
|------|--------|-------------------|
| `plugins/O-Bassoon/CMakeLists.txt` | mod | `juce_add_binary_data` block + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `NEEDS_WEBVIEW2 TRUE` |
| `plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` | mod | WebView UI rewrite (Stage 3 execute-phase deliverable) |
| `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` | mod | Push-channel atomics (Phase 3.2 — `currentActiveVoiceCount`, `currentEffectiveBreath`, `currentVibratoEnvelope`) |
| `plugins/O-Bassoon/Source/BassoonVoice.h` | mod | header-inline `getEffectiveBreath()`, `getVibratoEnvelope()` accessors |
| `plugins/O-Bassoon/Source/Vibrato.h` | mod | `getEnvelope()` accessor |
| `plugins/O-Bassoon/Resources/` | new | `ui/index.html`, `ui/js/juce/*.js`, `ui/img/fern.png` |
| `plugins/O-Bassoon/.planning/stages/3-gui/` | new | CONTEXT, RESEARCH, PLAN, SUMMARY, VERIFICATION (5 docs) |
| `plugins/O-Bassoon/.planning/BRIEF.md` | mod | non-goals row backfill (T0 doc fix per Stage 3 plan) |
| `plugins/O-Bassoon/.planning/ROADMAP.md` | mod | D8 amendment (2-phase split locked) |
| `plugins/O-Bassoon/.planning/REQUIREMENTS.md` | mod | UI-01 / UI-02 acceptance criteria → complete |
| `plugins/O-Bassoon/.planning/STATUS.md` | mod (partial — Stage 3 deltas only; Stage 4 deltas land at Commit 2) | Stage 3 closure context |

**STATUS.md split point:** the current STATUS.md contains BOTH Stage 3 closure context AND Stage 4 discuss-phase context. Plan-phase Task 1 must split: lift Stage-4 deltas into a temporary patch, commit Stage 3 deltas, restore Stage-4 deltas to working tree for Commit 2. Mechanically: `git checkout HEAD -- STATUS.md`, manually re-apply only Stage 3 lines, commit, then `git apply` saved Stage-4 deltas. Fragile — alternative: hand-edit STATUS.md in two passes.

**Commit 2 — Stage 4 (after Gate 6 PASS):**

Subject: `feat(O-Bassoon): Stage 4 polish + v1.0.0 release - COMPAT-01/02 + DSP-06 PASS`

Files:
| Path | Change | Stage 4 rationale |
|------|--------|-------------------|
| `plugins/O-Bassoon/CMakeLists.txt` | mod | (i) **fix `PLUGIN_VERSION "1.0.0"` → `VERSION "1.0.0"`** per O-MicrotonalSampler v1.11.0 lesson — `PLUGIN_VERSION` is silently dropped by `juce_add_plugin`, causing `JucePlugin_VersionString` to fall back to `PROJECT_VERSION = 1.0.0`. Coincidentally harmless at v1.0.0 but a footgun for v1.0.1 onward. (ii) Add `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp` to `target_include_directories`. (iii) Add `Source/FactoryPresets.cpp` to `target_sources` if separate file used. |
| `plugins/O-Bassoon/Source/FactoryPresets.{h,cpp}` | new | 4 factory preset definitions (per OQ5) |
| `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` | mod | Wire `OuariconPresetManager`: include header, member, constructor delegate-init, `initializeFactoryPresets()` dispatch, replace existing `getStateInformation`/`setStateInformation` (PluginProcessor.cpp:318-333) with delegate-to-presetManager pattern (per OQ5). |
| `plugins/O-Bassoon/CHANGELOG.md` | new | v1.0.0 entry (full template per OQ6) |
| `plugins/O-Bassoon/.planning/REQUIREMENTS.md` | mod | COMPAT-01 partial → complete; COMPAT-02 pending → complete; DSP-06 partial → complete |
| `plugins/O-Bassoon/.planning/STATUS.md` | mod | Stage 4 closure: `status: stage_4_complete` + `phase: shipped_v1_0_0` + carry forward Stage 3 closure context |
| `plugins/O-Bassoon/.planning/stages/4-polish/` | new at execute-phase (research + plan are present at research-phase end) | CONTEXT, RESEARCH, PLAN, SUMMARY, VERIFICATION (5 docs) |
| `plugins/O-Bassoon/.planning/stages/4-polish/test-score-microtonal.dorico` | new | OQ2 authored Dorico test score |
| `plugins/O-Bassoon/.planning/stages/4-polish/test-mpe-pitchbend.mid` | new | OQ9 authored MPE MIDI test file |
| `PLUGINS.md` (repo root) | mod | O-Bassoon row update (🚧 Stage 0 → 📦 Installed v1.0.0) per OQ7 |

**No file overlap risk between Commit 1 and Commit 2:**
- `CMakeLists.txt` — Stage 3 commit ratifies the `juce_add_binary_data` block and WebView2 flags; Stage 4 ratifies the preset-manager include + `VERSION` keyword fix. Both are local edits to non-overlapping line ranges.
- `Source/PluginProcessor.{h,cpp}` — Stage 3 ratifies the push-channel atomics (member + load); Stage 4 ratifies the preset-manager wiring (constructor init-list, getStateInformation override). Non-overlapping.
- `STATUS.md` — overlap requires the 2-pass edit per audit above.
- `REQUIREMENTS.md` — Stage 3 ratifies UI-01/UI-02; Stage 4 ratifies COMPAT-01/02/DSP-06. Different sections; non-overlapping.

**Working-tree clean check (Stage 4 plan-phase Task 1 hard-gate):**
After Stage 3 commit lands and Stage 4 deltas are restored, the working tree must show ONLY the files in the Commit-2 table. Run `git status plugins/O-Bassoon/ PLUGINS.md` and verify no stray files. PLAN-rev-1 task #1 codifies this gate.

**Risk delta:** R7 low-low → **resolved** with explicit 2-commit choreography + STATUS.md 2-pass edit recipe.

---

## §2 JUCE / Module API Touchpoints

### `OuariconPresetManager` (header-only, modules/persistence/preset-manager/cpp/OuariconPresetManager.h)

**Public surface used by Stage 4:**

| Method | Signature (verbatim) | Stage 4 use |
|--------|----------------------|-------------|
| ctor | `OuariconPresetManager(juce::AudioProcessorValueTreeState& apvts, const juce::String& pluginName)` | constructor delegate-init; `pluginName="O-Bassoon"` |
| `initializeFactoryPresets` | `void initializeFactoryPresets(const std::vector<FactoryPresetDef>&)` | called once from `OBassoonAudioProcessor::initializeFactoryPresets()` if `getFactoryPresetsDirectory()` is empty |
| `getFactoryPresetsDirectory` | `juce::File getFactoryPresetsDirectory()` | check before re-writing factory presets (idempotency) |
| `getStateAsXml` | `std::unique_ptr<juce::XmlElement> getStateAsXml()` | replace plain `parameters.copyState().createXml()` |
| `setStateFromXml` | `bool setStateFromXml(juce::XmlElement* xml)` | replace plain `parameters.replaceState(juce::ValueTree::fromXml(*xml))` |
| `setCustomStateCallbacks` (optional) | `void setCustomStateCallbacks(CustomSaveCallback, CustomLoadCallback)` | NOT used at v1.0 — APVTS-only state. Available for v1.1 if Tuning panel state needs preset-level persistence. |
| `FactoryPresetDef` struct | `{ String name; std::map<String,float> params; juce::var customState }` | 4-element vector built in `Source/FactoryPresets.cpp` |

**Storage location (OS):**
- macOS: `~/Library/Application Support/O-Bassoon/Presets/{Factory,User}/*.json`
- Windows: `%APPDATA%\O-Bassoon\Presets\{Factory,User}\*.json`

(JUCE's `File::getSpecialLocation(File::userApplicationDataDirectory)` resolves correctly on each platform; managed inside `OuariconPresetManager`.)

### `juce::Synthesiser::handlePitchWheel` (already used; no changes at Stage 4)

JUCE 8.0.4 source-line citation (verified at Phase 2.4 RESEARCH-rev-4):
- `juce_Synthesiser.h:577` — `virtual void handlePitchWheel (int midiChannel, int wheelValue);`
- `juce_Synthesiser.cpp:509-523` — iterates `voices[]`, calls `voice->pitchWheelMoved(wheelValue)` only when `voice->isPlayingChannel(midiChannel) == true`

This is the load-bearing API for DSP-06 MPE-half closure (OQ9). No code change needed; verification only.

### `juce::AudioProcessorValueTreeState::copyState/replaceState` (replaced by preset-manager wrapper)

Current `PluginProcessor.cpp:318-333` calls `parameters.copyState() → createXml() → copyXmlToBinary` and inverse. Stage 4 replaces both with `presetManager.getStateAsXml()` / `presetManager.setStateFromXml()` — same semantics, plus factory-preset hooks.

### `juce_add_plugin` `VERSION` keyword (CMake)

Citation: O-MicrotonalSampler v1.11.0 CHANGELOG (lines 19-28):
> Plugin `CMakeLists.txt` used `PLUGIN_VERSION "x.y.z"`, which is **not** a recognized `juce_add_plugin` keyword — JUCE silently dropped it and fell back to `PROJECT_VERSION` from the root `project(JUCEPlugins VERSION 1.0.0)` declaration. The About tab's `getPluginVersion` native function returns `JucePlugin_VersionString`, which was therefore stuck at `"1.0.0"` for every shipped version (v1.0.0–v1.10.0). Renamed the arg to the correct `VERSION "1.11.0"` so future bumps wire through to the About pill and the bundle plist (`CFBundleShortVersionString`) automatically.

O-Bassoon CMakeLists.txt:14 currently has `PLUGIN_VERSION "1.0.0"` — same bug. Coincidentally harmless because root project version is also 1.0.0, but **must fix at Stage 4** to prevent v1.0.1 / v1.1 silent regression.

Family precedent for correct usage: O-Lyrica `VERSION "2.3.0"`, O-MicrotonalSampler `VERSION "1.11.0"`. Family bug-still-present: O-Bowed `PLUGIN_VERSION "1.3.0"`, O-Wind `PLUGIN_VERSION "1.16.0"` (out of scope to fix here; can flag as broader cohort issue in v1.0.1 sweep).

---

## §3 Risk Updates

| # | Risk | Pre-research | Post-research | Mitigation locked |
|---|------|--------------|---------------|-------------------|
| R1 | Dorico Playback Template ingestion silent-fail | High/Medium | **Low/Low** | OQ1 — `.doricolib` from `note-expression` v1.1.0 module, Library Manager Import flow; verification = quarter-sharp C4 → 269.29 Hz |
| R2 | Windows VST3 build environment friction | Medium/Medium | **Low/Low** | OQ4 — `build-and-install.ps1` + WebView2 static linking flag both already in place; family precedent (3 plugins) clear strict-10 |
| R3 | pluginval-10 Win regression vs macOS | Low/Low | **Low/Low** (unchanged) | WebView2 user-data folder pattern verified at memory; static linking already set |
| R4 | Logic-AU 60s sustain regression after Stage 3 | Low/Low | **Low/Low** (unchanged) | OQ8 — subjective audition bar; Stage 3 didn't touch audio render path |
| R5 | DSP-06 Logic-MPE setup uncertainty | Medium/Medium | **Low/Low** | OQ3 + OQ9 — Path A (pre-authored MIDI file) deterministic; no hardware required |
| R6 | Preset binary cross-DAW portability | Medium/Medium | **Low/Low** | OQ5 — OuariconPresetManager + JSON + APVTS state; family-canonical pattern |
| R7 | Stage 3 commit fold-in scope creep | Low/Low | **Low/Low** | OQ10 — explicit 2-commit choreography + non-overlapping file/line audit |
| R8 | Bitwig MPE drop blocks family-canonical DSP-06 | Low/Low | **Low/Low** (unchanged) | D5 chose Logic-MPE substitute; production-quality |
| R9 | CHANGELOG / PLUGINS.md schema drift | Low/Medium | **Low/Low** | OQ6 + OQ7 — both schemas fully drafted at research-phase |
| R10 | rev-3 iteration ceiling burn on Win build / Dorico parity | Medium/Medium | **Medium/Low** | Per-task independent budget; preset-tuning is the most likely iteration consumer (audition bar requires ear) |
| **R11 (NEW)** | **STATUS.md 2-pass edit fragility (OQ10 sub-risk)** | — | **Low/Low** | Plan-phase Task 1 will codify the 2-pass edit recipe; alternative is to manually craft Commit 1's STATUS.md content as a reset-checkout patch |

**Net risk profile:** all 10 (now 11) risks at Low/Low except R10 (Medium-Low). Stage 4 polish is well-scoped and de-risked by family precedent.

---

## §4 Implementation Skeletons

### §4.1 `Source/FactoryPresets.h` (new)

```cpp
/*
  ==============================================================================
    FactoryPresets.h
    O-Bassoon - 4 factory preset definitions for v1.0.0
  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the 4-preset factory vector for O-Bassoon v1.0.0.
        Values are stored as normalized [0,1] per OuariconPresetManager
        convention. Tuning state is not preset-persisted at v1.0
        (custom-state callback unused). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
```

### §4.2 `Source/FactoryPresets.cpp` (new)

```cpp
/*
  ==============================================================================
    FactoryPresets.cpp - O-Bassoon factory preset definitions
  ==============================================================================
*/
#include "FactoryPresets.h"

std::vector<OuariconPresetManager::FactoryPresetDef>
FactoryPresets::build (juce::AudioProcessorValueTreeState& /*apvts*/)
{
    return {
        { "Long Drone",
          {{"vibrato_rate", 0.45f}, {"vibrato_depth", 0.12f}, {"vibrato_onset", 0.40f},
           {"breath", 0.50f}, {"tone", 0.40f}, {"attack_character", 0.00f},
           {"attack_time", 0.60f}, {"release_time", 0.60f},
           {"voice_count", 0.20f}, {"output_gain", 0.70f}},
          juce::var() },
        { "Microtonal Pad",
          {{"vibrato_rate", 0.40f}, {"vibrato_depth", 0.25f}, {"vibrato_onset", 0.75f},
           {"breath", 0.65f}, {"tone", 0.50f}, {"attack_character", 0.00f},
           {"attack_time", 0.75f}, {"release_time", 0.7333f},
           {"voice_count", 0.4667f}, {"output_gain", 0.7333f}},
          juce::var() },
        { "Tongued Long Tone",
          {{"vibrato_rate", 0.55f}, {"vibrato_depth", 0.18f}, {"vibrato_onset", 0.15f},
           {"breath", 0.75f}, {"tone", 0.55f}, {"attack_character", 1.00f},
           {"attack_time", 0.04f}, {"release_time", 0.40f},
           {"voice_count", 0.20f}, {"output_gain", 0.7667f}},
          juce::var() },
        { "Bright Bassoon",
          {{"vibrato_rate", 0.60f}, {"vibrato_depth", 0.10f}, {"vibrato_onset", 0.20f},
           {"breath", 0.70f}, {"tone", 1.00f}, {"attack_character", 0.20f},
           {"attack_time", 0.10f}, {"release_time", 0.2667f},
           {"voice_count", 0.20f}, {"output_gain", 0.80f}},
          juce::var() }
    };
}
```

### §4.3 `Source/PluginProcessor.h` deltas

```diff
 #include "BassoonSynthesiser.h"
+#include "OuariconPresetManager.h"
 #include "NoteExpression.h"

 class OBassoonAudioProcessor : public juce::AudioProcessor {
 public:
     // ... existing public surface
+    OuariconPresetManager& getPresetManager() { return presetManager; }

 private:
     juce::AudioProcessorValueTreeState parameters;
     // ... existing private members
+    OuariconPresetManager presetManager;
+    void initializeFactoryPresets();
 };
```

### §4.4 `Source/PluginProcessor.cpp` deltas

```diff
 OBassoonAudioProcessor::OBassoonAudioProcessor()
     : AudioProcessor(...)
     , parameters(*this, nullptr, "Parameters", createParameterLayout())
+    , presetManager(parameters, "O-Bassoon")
 {
     for (int i = 0; i < 16; ++i) { ... }
+    initializeFactoryPresets();
 }

+void OBassoonAudioProcessor::initializeFactoryPresets()
+{
+    auto factoryDir = presetManager.getFactoryPresetsDirectory();
+    if (factoryDir.isDirectory()
+        && factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
+        return;
+
+    presetManager.initializeFactoryPresets(FactoryPresets::build(parameters));
+}

 void OBassoonAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
-    auto state = parameters.copyState();
-    std::unique_ptr<juce::XmlElement> xml(state.createXml());
+    auto xml = presetManager.getStateAsXml();
     if (xml != nullptr)
         copyXmlToBinary(*xml, destData);
 }

 void OBassoonAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
     std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
-    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
-        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
+    if (xmlState != nullptr)
+        presetManager.setStateFromXml(xmlState.get());
 }
```

### §4.5 `CMakeLists.txt` deltas

```diff
 juce_add_plugin(O-Bassoon
     COMPANY_NAME "${OUARICON_COMPANY_NAME}"
     PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
     PLUGIN_CODE OBsn
     FORMATS VST3 AU Standalone
     PRODUCT_NAME "O-Bassoon${OUARICON_DEV_SUFFIX}"
-    PLUGIN_VERSION "1.0.0"
+    # NOTE: must be `VERSION` (not `PLUGIN_VERSION`). The latter is silently
+    # ignored by juce_add_plugin and the build falls back to PROJECT_VERSION
+    # (=1.0.0 from the root CMakeLists), so the About tab and bundle plist
+    # would always show v1.0.0. Keep this token as `VERSION` to wire bumps
+    # through to JucePlugin_VersionString → getPluginVersion → About tab.
+    VERSION "1.0.0"
     IS_SYNTH TRUE
     ...
 )

 target_sources(O-Bassoon
     PRIVATE
         Source/PluginProcessor.cpp
         Source/PluginEditor.cpp
+        Source/FactoryPresets.cpp
         Source/BassoonSound.h
         ...
 )

 target_include_directories(O-Bassoon
     PRIVATE
         Source
         ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
+        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
 )
```

---

## §5 Verify-Phase Static-Check Battery

For Stage 4 verify-phase Gate 6, family-precedent grep gates plus Stage-4-specific items:

| # | Check | Command | Expected |
|---|-------|---------|----------|
| 1 | RT-safety in render path | `grep -n "new \|malloc\|delete \|free " plugins/O-Bassoon/Source/PluginProcessor.cpp` | only construction-time editor/processor factories (3 hits or fewer per Phase 2.4 baseline) |
| 2 | NE drain ordering preserved | `grep -B1 -A2 "vst3Extensions.drainAndUpdate\|renderNextBlock" plugins/O-Bassoon/Source/PluginProcessor.cpp` | drain at processBlock prologue BEFORE renderNextBlock |
| 3 | Preset manager wired | `grep -n "presetManager\|OuariconPresetManager" plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` | header include + member + ctor init + init dispatch + getStateAsXml + setStateFromXml |
| 4 | FactoryPresets included | `grep -n "FactoryPresets::build\|#include \"FactoryPresets.h\"" plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` | exactly 1 include + 1 build() call |
| 5 | VERSION keyword fix | `grep -n "VERSION\\|PLUGIN_VERSION" plugins/O-Bassoon/CMakeLists.txt` | `VERSION "1.0.0"` present; `PLUGIN_VERSION "1.0.0"` zero matches |
| 6 | preset-manager include path | `grep -n "preset-manager/cpp" plugins/O-Bassoon/CMakeLists.txt` | exactly 1 hit in target_include_directories |
| 7 | FactoryPresets.cpp in target_sources | `grep -n "FactoryPresets.cpp" plugins/O-Bassoon/CMakeLists.txt` | exactly 1 hit |
| 8 | WebView2 static linking still set | `grep -n "JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING" plugins/O-Bassoon/CMakeLists.txt` | 1 hit; value=1 |
| 9 | NEEDS_WEBVIEW2 still set | `grep -n "NEEDS_WEBVIEW2" plugins/O-Bassoon/CMakeLists.txt` | 1 hit; value=TRUE |
| 10 | Stage 3 push-channel atomics still present | `grep -n "currentActiveVoiceCount\|currentEffectiveBreath\|currentVibratoEnvelope" plugins/O-Bassoon/Source/PluginProcessor.h` | 3 hits (member declarations) |
| 11 | Stage 2 ordering invariants still present | `grep -n "tone-dispatch\|expression dispatch\|drainAndUpdate\|renderNextBlock\|applyGainRamp" plugins/O-Bassoon/Source/PluginProcessor.cpp \| head -20` | tone → expression → drain → render → output_gain order preserved |
| 12 | CHANGELOG.md exists with v1.0.0 entry | `grep -n "## \[1.0.0\] - 2026-05-01" plugins/O-Bassoon/CHANGELOG.md` | exactly 1 hit |
| 13 | PLUGINS.md row updated | `grep -n "O-Bassoon.*Installed.*1.0.0" PLUGINS.md` | exactly 1 hit (replaces "🚧 Stage 0") |
| 14 | REQUIREMENTS.md COMPAT-01/02/DSP-06 complete | `grep -n "COMPAT-01\\|COMPAT-02\\|DSP-06" plugins/O-Bassoon/.planning/REQUIREMENTS.md \| head -10` | all three rows show "complete" |
| 15 | auval | `auval -v aumu OBsn OuDv` | AU VALIDATION SUCCEEDED |
| 16 | pluginval-10 macOS AU | `pluginval --strictness-level 10 --validate-in-process ~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component` | exit 0 |
| 17 | pluginval-10 macOS VST3 | `pluginval --strictness-level 10 --validate-in-process ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` | exit 0 |
| 18 | pluginval-10 Windows VST3 (run on Win box) | `pluginval.exe --strictness-level 10 --validate "$env:COMMONPROGRAMFILES\VST3\O-Bassoon-dev.vst3"` | exit 0 |

**Manual Gate 6 items (10 total — D9 in CONTEXT):**

| # | Manual item | Bar | Carry-source |
|---|-------------|-----|--------------|
| 1 | Logic-AU T9 smoke (8-item Phase 3.1 subset) | subjective "no glitches in Logic" | Stage 3 verify carry-forward |
| 2 | Logic-AU T17 Gate 5 (8-item full incl. 60s long-tone + push-channel audition) | subjective per OQ8 | Stage 3 verify carry-forward |
| 3 | pluginval-10 macOS AU+VST3 | exit 0 (also static-check #16, #17) | re-confirm post Stage-3 commit |
| 4 | pluginval-10 Windows VST3 | exit 0 (also static-check #18) | NEW — first Win build of O-Bassoon |
| 5 | Dorico Playback Template ingestion | "Ouaricon VST3 Note Expression" appears under Library → Expression Maps after Library Manager Import | OQ1 |
| 6 | Dorico microtonal score parity vs O-Lyrica | 8-beat 24-EDO test score — every beat ±5¢ matches expected; A/B identical to O-Lyrica | OQ2 |
| 7 | DSP-06 MPE-half via Logic-MPE | 4-channel MPE MIDI test — all 4 voices play at expected per-channel-PB-shifted pitches ±5¢ | OQ9 |
| 8 | 4-preset recall round-trip | All 4 factory presets recall correctly in Logic AU dropdown; save/load preserves params | OQ5 |
| 9 | CHANGELOG.md v1.0.0 entry written | Static-check #12 + readability check | OQ6 |
| 10 | Windows install verification | O-Bassoon-dev appears in FL Studio OR Reaper OR Ableton-Win plugin scanner; `auval`-equivalent (e.g., DAW load + 1 noteOn) | OQ4 |

---

## §6 Sources

**In-repo references:**
- `plugins/O-Bassoon/.planning/stages/4-polish/CONTEXT.md` (8 user-confirmed approach decisions + 10 OQs + 10 risks)
- `plugins/O-Bassoon/.planning/REQUIREMENTS.md` (UI-01/UI-02 → complete; COMPAT-01/02 + DSP-06 pending → complete at verify-phase)
- `plugins/O-Bassoon/.planning/ROADMAP.md` L309-322 (Stage 4 deliverable scope + 4 preset names + test criteria)
- `plugins/O-Bassoon/CMakeLists.txt` (current state — VERSION keyword fix needed)
- `plugins/O-Bassoon/Source/PluginProcessor.cpp:318-333` (current getStateInformation/setStateInformation — to be replaced)
- `plugins/O-Bassoon/Source/BassoonVoice.cpp:50-138` (per-voice pitchWheelMoved verified)
- `modules/tuning/note-expression/README.md` L155-216 (Dorico ingestion mechanics)
- `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` (canonical .doricolib file)
- `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` L1-552 (header-only API surface)
- `plugins/O-Bowed/Source/PluginProcessor.cpp:241,257,442-453,456-587` (preset-manager + factory-preset family precedent)
- `plugins/O-MicrotonalSampler/CMakeLists.txt` (correct VERSION keyword example + bug context comment)
- `plugins/O-MicrotonalSampler/CHANGELOG.md` v1.11.0 (Keep-a-Changelog template; CMake VERSION fix history)
- `plugins/O-Wind/CHANGELOG.md` v1.16.0 (Note Expression integration changelog template — closest analogue to O-Bassoon)
- `PLUGINS.md` L88-90 (Entry Template — schema authoritative)
- `.claude/skills/spike-findings-VST-development/sources/002-quarter-sharp-end-to-end/README.md` (Dorico microtonal test recipe — Method A/B/C, expected pitches, surprises)
- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` L78-105 (Dorico pitch representation surprise; NEC ignored by Dorico 6)
- `scripts/build-and-install.ps1` (Windows 7-phase build pipeline)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_distribution_mechanism.md` (5-day-old memory; .doricolib path validated against current code at research-phase)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/project_o_lyrica_spike_reference.md` (5-day-old memory; O-Lyrica is canonical PASS state for note-expression module)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/MEMORY.md` (WebView2 static-linking critical pattern; resource-provider bare-path equality regression sentinel)

**External references:**
- Apple Support — [Use MPE with software instruments in Logic Pro for Mac](https://support.apple.com/guide/logicpro/use-mpe-with-software-instruments-lgcp8f599497/mac)
- Apple Support — [Pitch bend events in Logic Pro for Mac](https://support.apple.com/guide/logicpro/pitch-bend-events-lgcp2158a4c2/mac)
- Pitch Innovations — [MPE Explained — Complete Guide 2025](https://pitchinnovations.com/blog/mpe-explained-what-is-mpe-and-why-you-should-care/)
- Tracktion / pluginval — <https://github.com/Tracktion/pluginval/releases>
- Steinberg / Dorico Library Manager — referenced via note-expression/README.md "Manual Import Steps" L172-187

---

## §7 Hand-off to Plan-Phase

**Research-phase deliverables locked:**
- 10 OQs resolved (10/10) with concrete recipes, file paths, code skeletons
- 11 risks categorized; 10 of 11 at Low/Low; R10 at Medium/Low
- Stage 3 + Stage 4 commit choreography fully audited (file table per commit)
- 5 implementation skeletons drafted (FactoryPresets.{h,cpp}, PluginProcessor.{h,cpp} deltas, CMakeLists.txt deltas)
- 18 static-check + 10 manual Gate 6 items locked

**Open at plan-phase (no NEW questions; pure planning):**
- Task ordering (research recommends: STATUS.md split → Stage 3 commit gate → preset wiring → factory presets → CHANGELOG → Dorico/Logic test → Win build → install → REQUIREMENTS+STATUS+PLUGINS update → atomic Stage 4 commit)
- iteration-budget allocation across the 4 independent task groups (Win build / Dorico parity / Logic smoke / presets); rev-3 per-task per CONTEXT D6 + R10
- decide whether to author the Python MIDI script as a committed `scripts/author-mpe-test-midi.py` or one-shot at execute-phase (recommend: one-shot — the .mid output is what's committed, not the generator)

**Process invariants carried forward:**
- IV1: Stage 3 atomic commit MUST land on `main` BEFORE Stage 4 execute-phase begins (CONTEXT D8 + Process Invariants §1).
- IV2: Single Stage 4 atomic commit on Gate 6 PASS (no mid-stage commits).
- IV3: rev-3 inline iteration ceiling, per-task independent budget.
- IV4: No DSP-05 v1.1 work in Stage 4 (out-of-scope register).
- IV5: v1.0.0 ships at Stage 4 atomic commit landing on `main`.

**Ready for:** `/plugin-plan O-Bassoon 4-polish`

---
*Generated 2026-05-01 from `/plugin-research O-Bassoon 4-polish`*
*Schema: family-canonical RESEARCH.md (matches Phase 2.1–2.4 + Stage 3 precedent)*
