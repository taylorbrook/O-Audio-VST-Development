---
title: "O-GranulatorInstrument Stage 1 (Foundation) — Execution Summary (SUMMARY-1)"
date: 2026-11-07
stage: 1
version: 0.1.0
result: COMPLETE — every PLAN.md gate green; D9 contingency did NOT fire
---

# O-GranulatorInstrument — Stage 1 Summary

O-GranulatorInstrument 0.1.0 builds as VST3 + AU + Standalone (`aumu OuGI OuDv`). All 261 parameters are registered in spec order (176 Float / 70 Choice / 15 Bool). The 126-entry dest table and the 18-entry source table are literal. The shell is a silent instrument with a stereo `Sidechain` input (VST3 `kAux`). The `<Engine>` + `<Slots>` state scaffold and the render harness are in place. pluginval 10 passes on VST3 and AU with the sidechain both active and inactive, and auval passes.

Gate bypass: the 0→1 run-gate build check always blocks, because CMakeLists.txt does not exist before Stage 1. It was bypassed with `--force --skip-review` and logged to `.planning/gate-bypasses.log`.

## Files

| File | Lines |
|---|---|
| `CMakeLists.txt` | 215 |
| `Source/GiParamIds.h` | 500 |
| `Source/PluginProcessor.h` / `.cpp` | 130 / 269 |
| `Source/PluginEditor.cpp` (placeholder; no `PluginEditor.h`, D8) | 12 |
| `tests/golden/extract_spec.py` + `params.tsv` / `mod-dests.tsv` / `mod-sources.tsv` | 376 |
| `tests/render-harness/main.cpp` | 1008 |
| `.planning/params.tsv` (param-dump) | 261 rows |

Repo root: `PLUGINS.md` (row 76 only) and `modules/registry.yaml` (P6).

## Per-wave results

### W0 — T1 pre-flight: PASS
- **#0 at start:** whole `b6bc70e3…ca751b`, body `c5553e0c…543191`. Both match STATUS.
- OuGI unused (0). JUCE 8.0.15.
- `JUCE-NE-PATCH` markers: 1 in each of the two files.
- `OUARICON_BUILD_TESTS:BOOL=ON`. The plugin folder held only `.planning`.
- i18n baseline `ALL CHECKS PASS — 47 localized plugin(s)`. Branch `main`.

### W1 — T2 goldens: PASS
- `extract_spec.py`: `OK: 261 params (176 Float / 70 Choice / 15 Bool), 126 dests, 18 sources`, exit 0.
- 0 CR bytes in all three TSVs. Data rows 261 / 126 / 18.
- **Negative control.** A scratch spec with dest idx 2↔3 swapped and `glide` typed Choice → exit 1, 7 failures named, nothing written:
  - the glide type mismatch
  - 2 Mod-dest column disagreements
  - counts 175 / 71 / 15
  - 2 dest host-name mismatches
  - formula mismatch at [2, 3]

### W1/W2 — T3–T5: written by foundation-shell-agent (see Deviations)

### W3 — T6 configure + build: PASS
- **#13a:** `juce_add_plugin (` count = 1. `resolve-target.sh cmake-target` → `OuariconGranulatorInstrument`.
- **#2 configure:** rc 0. No `OGI_GATE_SC_DEFAULT_OFF` warning. NE line:
  `-- [note-expression] JUCE-NE-PATCH markers verified in /Users/taylorbrook/JUCE`
- **#3 build:** VST3, AU, Standalone, render-harness and param-dump all build, rc 0. **0** `O-GranulatorInstrument/…warning:` lines.

### W3 — T7 harness + static gates: PASS
- **#4 harness: `55 PASS, 0 FAIL`**, rc 0. All 26 row IDs are present: G1–G6, D1–D5, SRC1, B1, B3, M1, R1, R1v, R1n, S1a, S1b, S2–S5, H1, H2.
  - Worst G4 engineering-default error: 5.1e-8 of span.
  - S1a: 0 values differ, seed `0x80000001`.
  - S1b reads back `-1148986563833708817`.
- **Golden negative control:** a perturbed `golden-bad/` gave rc 8 with `47 PASS, 8 FAIL`. The failing rows were G1 ×2, G4 ×2 (amp_release norm + eng), G5 (grain_limit), D1, D3 (0/16) and SRC1, which covers all six required rows. Rerun with no flag: `55 PASS, 0 FAIL`.
- **#5 param-dump:** 261 rows. The ID diff and the host-name diff against the spec Full ID list are both empty (261 vs 261 lines, so neither diff is vacuous).
- **#13 greps:** all clean, with one note on the listener/notify/adder grep. Its first run hit `Source/ui/public/modules/reverb-unit.js:177`, a comment in the configure-copied module JS. That file is gitignored (`.gitignore:27 plugins/*/Source/ui/public/modules/*`) and is not plugin source. The rerun excluding that dir is clean.
  - The other greps:
    - WebView headers: clean.
    - Lambda ranges: clean.
    - `·` through the ASCII ctor: clean.
    - `setLatencySamples`: once, at `:105`, inside `prepareToPlay` (`:100`).
    - Neither `PluginEditor.h` nor `i18n.js` exists.
  - **Stage 3 gate note:** from 3.1 on, scope #13 to exclude `Source/ui/public/modules/`.
- **#11 i18n:** `ALL CHECKS PASS — 47 localized plugin(s)` (unchanged). `--plugin O-GranulatorInstrument` → rc 0, "0 plugins localized".
- **#12 (informational):** boot-all-uis exit 0, 0 rendered elements (no UI root until 3.1).

### W4 — T8 install + COMPAT-01 (shipping, sidechain active): PASS
- **#6:** build-and-install rc 0. Installed: exactly `O-GranulatorInstrument-dev.vst3` + `O-GranulatorInstrument-dev.component`.
- **#7:** pluginval 10 VST3 ×3, each `SUCCESS` with rc 0. Each log shows:
  ```
  Main bus num input channels: 2
  Main bus num output channels: 2
  ```
  "Enabling all buses", "Disabling non-main busses" and "Restoring default layout" all ran.
- **#8:** pluginval 10 AU in-process `SUCCESS`. It printed the expected `!!! WARNING: Current program is -1... Is this correct?`.
- **#9:** `auval -v aumu OuGI OuDv` → `AU VALIDATION SUCCEEDED.` Negative control `aumu ZZZZ OuDv` → rc 2. Blocks verbatim (now FROZEN):
  ```
  Input Scope Bus Configuration:
   Default Bus Count:1
      Bus Name: Sidechain
      Default Format: AudioStreamBasicDescription:  2 ch,  44100 Hz, Float32, deinterleaved
      Has Channel Layouts: 0x640001 0x650002 0x660002 0x670002 0x6A0002 0x930000
      Default Layout:
  	Tag=0x650002, Num Chan Descs=0

  Output Scope Bus Configuration:
   Default Bus Count:1
      Bus Name: Output
      Default Format: AudioStreamBasicDescription:  2 ch,  44100 Hz, Float32, deinterleaved
      Has Channel Layouts: 0x650002 0x660002 0x670002 0x6A0002
      Default Layout:
  	Tag=0x650002, Num Chan Descs=0

  Reported Channel Capabilities (explicit):
        [1, 2]  [2, 2]
  ```
  `[1, 2] [2, 2]` matches the INFERRED prediction.
- **#10 Standalone:** `ALIVE` after 6 s.

### W5 — T9 sidechain-inactive arm: PASS
- **Variant:** configure with `-DOGI_GATE_SC_DEFAULT_OFF=ON`. The `[O-GranulatorInstrument] OGI_GATE_SC_DEFAULT_OFF=ON: gate variant, do not ship` warning appeared and the cache read `:BOOL=ON`. Then build-and-install, rc 0.
  - pluginval VST3 ×2: `SUCCESS`, each with **`Main bus num input channels: 0`**.
  - pluginval AU: `SUCCESS`.
  - auval: SUCCEEDED, capabilities `[1, 2] [2, 2]`.
  - Logic itself was not running; only its QuickLook and thumbnail extensions were.
- **Restore:** configure with `-DOGI_GATE_SC_DEFAULT_OFF=OFF`: cache `:BOOL=OFF`, 0 warning lines. Then build-and-install, rc 0.
  - pluginval VST3: `SUCCESS` with `Main bus num input channels: 2`.
  - Warm auval: SUCCEEDED. The capabilities block and the Input Scope block are both byte-identical to T8.
  - Standalone rebuilt from the restored config.
- **The installed binaries are the shipping build.**

### W6 — T10
- **#0 at end:** whole `b6bc70e3…ca751b`, body `c5553e0c…543191`, unchanged. Cache `OGI_GATE_SC_DEFAULT_OFF:BOOL=OFF`.
- `PLUGINS.md` row 76 → `🚧 Stage 1 | 0.1.0`.

## P3 — kAux evidence
- **Automated:** harness H2 PASS, `getVST3ClientExtensions() != nullptr && ! getPluginHasMainInput()`.
- **Code path:** `~/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp:3108-3113`. For the input bus at index 0, the VST3 wrapper returns `extensions->getPluginHasMainInput() ? Vst::kMain : Vst::kAux`.
- The Live/Reaper check is deferred to the Stage 4 DAW pass.

## P6 — registry resolution
- `bash scripts/regen-registry-used-by.sh` added this plugin's 7 `used_by` rows: webview-drop-streaming, grain-engine, pitch-collections, reverb-unit, preset-manager, eb-garamond and note-expression. It also bumped the header to 1.3.2 / 2026-11-07.
- Then a **manual bump**: the note-expression entry `version: 1.1.1` → `1.1.2`, matching `module.yaml`.
- **Foreign hunk excluded:** regen also rewrote O-Granulator's `used_by` version 0.1.0 → 1.0.0 in 5 module entries. That is O-Granulator's own release bump, committed by another session (HEAD `658161fd`) without a registry regen. The committed blob keeps O-Granulator at 0.1.0. The working tree retains those 5 lines, uncommitted, for the O-Granulator owner to commit.

## Deviations (from foundation-shell-agent; all gates green with them)
1. `Def` range fields are named `lo` / `hi`, because of the no-`end`-identifier rule. Every `Def` is built via `floatDef` / `choiceDef` / `boolDef`.
2. `GiParamIds.h` also carries the parent's `scaleChoiceToId` and its static_asserts, `boolDefaultFor()` and `sourceClassToken()`. It includes `PitchCollections.h`.
3. `processBlock` also calls `midiMessages.clear()`.
4. `setStateInformation` removes the root `uiLanguage` attribute before `replaceState`, so the live tree never carries it.
5. **R1v** checks two things: per block, that the buffer is not flagged clear; per case, the RMS over the whole render on every channel. A per-block RMS on 1-sample blocks would be random.
6. **R1n** runs on the first unprocessed block of every case, plus a fixed control: all +0 passes, one −0.0f fails.
7. `mpe_enable` is set On in every R1 case, because the script always carries MPE member notes.
8. S1a / S3 / S5 write "settled" normalised values, where convertFrom → convertTo returns exactly n. The bitwise compare therefore tests the codec, not APVTS float rounding.
9. Golden formats:
   - dest class `-` for None
   - source classes `none` / `per-voice` / `global` / `lfo-mode`
   - the interval column is empty on Choice and Bool rows
10. The harness exit code is the FAIL count capped at 255. An unknown argument exits 255.
11. `createPluginFilter()` is declared once before its definition (`-Wmissing-prototypes` in the consoles).
12. Orchestrator: the #13 notify/listener grep is rescoped to exclude the gitignored `Source/ui/public/modules/` (see T7). The #12 first attempt failed on a missing `timeout` binary; it was rerun without one (exit 0).

The D9 contingency did **not** fire, and `OGI_GATE_NO_SIDECHAIN` was not added.

## Notes for Stage 2
- **R1 stays a permanent harness row (QUAL-02).** Sidechain-only input must keep producing bitwise-zero output once DSP lands.
- **Skewed defaults are not bit-exact through APVTS** (worst 5.1e-8 of span). Never derive unity gain from `get()` on a skewed row.
- **The SC channel count comes from `getSidechainChannelCount()`** (the negotiated bus), never from the buffer width.
- **An unrouted AU sidechain arrives as SILENCE, not as a disabled bus.** Record-in must decide on signal presence (D6).
- `loadSlotSync` overloads are stubs (`"loadSlotSync: stub until 2.1"`), slot base 0…3 (P5).
- The `CustomScale` / `DegreeMasks` codec lands in 2.2 (P2). Those tags are already in the strip list.
- AU I/O is frozen (blocks above). Any change needs a VERSION bump before Logic sees it.
