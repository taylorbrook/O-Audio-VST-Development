# O-SimpleReverb Notes

(Formerly OuariconSimpleReverb - renamed v1.5.0)

## Status
- **Current Status:** 📦 Installed
- **Version:** 2.0.0
- **Type:** Audio Effect (Reverb)
- **Complexity:** 4.2 (Complex)

## Lifecycle Timeline

- **2026-01-13:** Creative brief created - lightweight reverb for instrument chains
- **2026-01-13 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 4.2, Phase-based implementation)
- **2026-01-13 (v1.0.0):** Initial release - All 3 stages complete, plugin installed
- **2026-01-13 (v1.0.1):** Bug fixes - Fixed knob interactivity, reverb type switching, title update
- **2026-01-13 (v1.1.0):** Type-specific DSP - Each reverb type now has distinct sonic character through dedicated processing chains (pre-delay, early reflections, all-pass dispersion, modulation, shimmer, type-specific EQ)
- **2026-01-13 (v1.2.0):** UI overhaul - Botanical seed knob design, VU meter, character display
- **2026-01-13 (v1.2.1):** VU meter fix - Connected meter to audio output, dial alignment
- **2026-01-13 (v1.3.0):** LP filter & VU improvements - Added 20-400Hz lowpass filter with toggle, VU meter now -90°/+90° with dB indicators and green-to-red color gradient, flora background 2x larger
- **2026-01-13 (v1.3.1):** Filter & Decay fixes - Changed LP filter to Low Cut (highpass), toggle is now clickable button, Decay is now 0.5x-2.0x multiplier
- **2026-01-13 (v1.3.2):** UI polish - Decay centered at 1.0x, Hz indicators on Low Cut, removed footer
- **2026-01-24 (v1.5.0):** Renamed from OuariconSimpleReverb to O-SimpleReverb
- **2026-02-07..15 (v1.5.1–v1.5.5):** VU meter accuracy/size polish, version footer
- **2026-07-07 (v1.5.6):** Code-review fixes (CR-01..04, WR-01..05) - FileChooser SafePointer UAF guards, corrected DECAY skew (0.6309), RT-safe ArrayCoefficients filter updates, pre-allocated work buffers, preset reset-to-defaults + version-stamped factory sentinel (preset-manager v1.0.3), CHARACTER readout aligned to DSP, smoothed wet/dry gains, mono/stereo bus constraint
- **2026-09-27 (v1.10.0):** UI pass (review 260924-nho R4/R5) - palette tokens (0 hex outside :root), naturalist AA text colours, 0.40 --paper wash over paper.jpg, 9px floor, bundled EB Garamond; real-ground probe 416/552 -> 3/503 below AA. No DSP change
- **2026-09-27 (v1.11.0):** Full review - factory TYPE recall fixed (k/6 table: 12/24 presets - Spring/Plate/Ambient - played the type below), Spring all-pass sign fixed (was +20.5 dB DC comb cascade), click-free CHARACTER (SVF warm + shelf, smoothed; 2154x -> 1.03x) + LOW CUT enable, rate-sized delay capacities, VU peak hold, dblclick reset + wheel/toggle host gestures, stuck-drag release; tests/render-check added
- **2026-09-30 (v1.12.0):** Fresh audit - click-free LOW CUT on/off (HF burst -1.1 -> -59.5 dB) and freq (-23.5 -> -54.6), CHARACTER Bright zipper + +0.5 step (-46 -> -67 dB), TYPE duck-and-swap + EQ crossfade (5.4x -> 1.48x), per-type level trim at the reverb input (spread 7.7 -> 0.07 dB), 270-degree knob arcs, one host gesture per param, knob/switch keyboard + ARIA (R7), time-based VU, failed-load display, host names "Low Cut Freq/On" (IN-02), scoped WebView2 folder (IN-03); render-check 16/16, v1.11.0 fails 7
- **2026-09-30 (v1.13.1):** Info-tier review sweep — IN-01: removed the unused savePreset/deletePreset/isFactoryPreset WebView natives; untracked the gitignored modules/preset-manager.js copy
- **2026-09-30 (v1.13.0):** Real pitch flutter (swept delay: Spring 6c@4.5Hz, Hall 3c@0.15Hz, Ambient 4c@0.4Hz; L/R share phase — the mono-summing reverb turned an offset into a flanger), real octave-up Plate shimmer (two-grain shifter, was 1.5 kHz ring mod), DECAY headroom map above 1.0x (Ambient tail 1.25/1.6/2.0x: 6.01/6.11/6.11 -> 4.27/4.95/6.28 s); render-check 23/23
- **2026-09-30 (v1.14.0):** Factory bank review - 24 -> 48 presets (8/type incl. a WET 100/DRY 0 Send each; dark/bright CHARACTER voicings), insert presets level-capped (+0.4..+7.5 -> +0.2..+4.7 dB; Infinite Drone/Ethereal/Cloud Nine WET+DRY scaled), "Dub Echo" -> "Dub Spring" with stale-factory-file sweep, table authored in engineering units + convertTo0to1; render-check 32/32
- **2026-10-01 (v2.0.0):** Reverb engine rewrite - `juce::dsp::Reverb` replaced by three engines in `Source/dsp/`: a 16-line Hadamard FDN with stereo early reflections at the output (Booth/Room/Hall/Ambient, each on its own delay set), Dattorro's plate tank with the octave shimmer inside the loop (Plate), three dispersive springs (Spring). DECAY is a multiple of a per-type decay time in seconds (0.40 / 1.1 / 3.0 / 2.5 / 2.5 / 7.0 s), held within 10 % across SIZE; SIZE scales lengths only and glides; a TYPE change lets the old tail ring out in a second slot; level law at the slot input (SIZE flat within 0.6 dB, DECAY 1.9..2.6 dB across the knob); mono bus level-matched to stereo; 48 presets re-voiced (names kept); tip.type/decay/size rewritten in en/fr/zh-Hans. IDs, ranges and state format unchanged - every session sounds different. render-check 126 gates + 30 mutants; v1.14.0 fails the new gates out of tree

## Known Issues

None known.

## Additional Notes

### Concept
Lightweight, CPU-efficient reverb designed to add subtle color and realism to instrument chains. Removes the "in-a-box" feel without dominating the mix.

### Parameters (8 total)
1. **Type** (Dropdown): Booth, Room, Hall, Spring, Plate, Ambient
2. **Character** (Knob): Warm ← → Bright
3. **Low Cut** (Knob): High-pass filter cutoff (20-400Hz), cuts bass from reverb
4. **Wet** (Knob): Reverb signal level
5. **Dry** (Knob): Original signal level
6. **Decay** (Knob): Tail length, as a multiple (0.5x-2.0x) of the type's own decay time
7. **Size** (Knob): Scale of the space (delay lengths, early-reflection spacing); does not move the tail time
8. **Low Cut On** (Toggle): ON/OFF button below Low Cut knob

### Design
- Aesthetic: Ouaricon Naturalist (botanical theme)
- Priority: sound quality over CPU (v2.0.0 milestone decision; the original brief said the reverse)
- Future: Module version for embedding in other VSTs

### Sound Character
- Subtle, musical, transparent
- Natural early reflections
- Smooth decay
- Not a special effect reverb - utility focused

### DSP Architecture (v2.0.0)

Three reverb engines, header-only and per-sample, in `Source/dsp/`. `PluginProcessor` hosts them in
two slots and keeps everything after the reverb. The v1.x path (one `juce::dsp::Reverb` behind a
per-type pre-chain) is gone; `backups/O-SimpleReverb/v1.14.0/` holds it.

**Signal path, per slot:** input gain (TYPE duck x type trim x level law) -> pre-delay -> engine
(+ early reflections, FDN types only) -> mono fold if the bus is mono -> type EQ. The two slots are
summed, then CHARACTER -> LOW CUT -> wet/dry -> VU peak.

| File | What it is |
|---|---|
| `dsp/ReverbPrimitives.h` | Ring delay (integer and 3rd-order Lagrange reads), allpass, one-poles, RT60 <-> gain, the mid-band shelf solve, the two-pole length glide |
| `dsp/FdnEngine.h` | 16 lines, Hadamard feedback, geometric delay ladder per type, two-band absorption per line computed from the line's current length, odd lines modulated, four input allpasses a side |
| `dsp/EarlyReflections.h` | Stereo tap line, different times L and R, mixed to the slot OUTPUT (not into the tank) |
| `dsp/PlateEngine.h` | Dattorro's figure-8 tank scaled from 29761 Hz, 14 output taps; the octave-up shifter (`ModulationFx.h`) sits on both cross-feeds |
| `dsp/SpringEngine.h` | Three springs: delay + 72 / 80 / 88 stretched allpasses + 6th-order low-pass + 100 Hz high-pass, inverting reflection |

**Per type** (`typePresets` in `PluginProcessor.cpp`):

| Type | Engine | Decay at 1.0x | SIZE scale 0 / 50 / 100 | Pre-delay | Early reflections | EQ |
|------|--------|---------------|-------------------------|-----------|-------------------|-----|
| Booth | FDN, 2.9-13.7 ms lines | 0.40 s | x0.5 / x1 / x2 | 3 ms | 6.9 ms span | HP 150 Hz |
| Room | FDN, 8.3-37.9 ms | 1.1 s | x0.5 / x1 / x2 | 15 ms | 23 ms | none |
| Hall | FDN, 21.7-83.1 ms | 3.0 s | x0.5 / x1 / x2 | 50 ms | 46 ms | HS -2 dB @ 3 kHz |
| Spring | 3 springs, 33 / 37 / 41 ms echoes | 2.5 s | x0.75 / x1 / x1.33 | none | none | Peak +4 dB @ 800 Hz |
| Plate | Dattorro tank | 2.5 s | x0.40 / x0.57 / x0.80 | 8 ms | none | HS +3 dB @ 5 kHz |
| Ambient | FDN, 30.7-121.3 ms | 7.0 s | x0.5 / x1 / x2 | 35 ms | 57.5 ms | HS -3 dB @ 2.5 kHz |

**Rules the code keeps:**
- **DECAY** multiplies the type's decay time; each engine turns seconds into loop gains from its
  current lengths, so the tail time holds as SIZE moves (gated within 10 %).
- **SIZE** scales lengths only. Lengths glide through two poles (125 ms each): a ringing tail bends
  in pitch and does not click.
- **TYPE change:** the playing slot's input ducks and the slot rings out under its own type, size,
  decay and EQ; the other slot starts the new type. A third change while both sound fades the older
  slot's output over 10 ms and reuses it.
- **Level law** (slot input): SIZE is compensated fully per engine, DECAY by half
  (`kSizeLevelDbPerOctave`, `kDecayLevelDbPerOctave`). `wetTrimDb` / `monoTrimDb` level-match the
  types at defaults, stereo and mono.
- **Real time:** capacities are set in `prepareToPlay` from the running rate; a host block larger
  than the prepared one is processed in chunks; a non-finite slot output clears that slot and the
  next chunk recovers. No allocation in `processBlock` (gated).
- **Determinism:** modulation phases are fixed; no `Random`, no clock.

**Voicing constants set by choice, kept by the listening pass (2026-10-01, no change):**
early-reflection levels and spans, pre-delays, type EQs (all carried from v1.14.0), the plate's
shimmer share (`kShimmerPerLoopSecond`), damping and SIZE range, the spring table
(`SpringEngine::kSpring`), and the DECAY half of the level law.

**CPU** (render-check `--baseline`, one core, 48 / 96 kHz, one slot; a ring-out runs two): FDN types
about 0.6 % / 1.3 %, Plate 0.3 % / 0.8 %, Spring 1.2 % / 2.3 %. v1.14.0 was 0.3-0.4 % / 0.7-0.9 %.

**Tests:** `tests/render-check` (build with `-DOUARICON_BUILD_TESTS=ON`; `cmake-build-tests/` is the
directory used for this milestone). Modes: gates, `--mutants`, `--levels [raw]`, `--baseline`.

### Implementation Plan (original v1.0.0 build, historical)
**Strategy:** Phase-based implementation (Complex plugin, score 4.2)

**DSP Phases:**
1. Phase 3.1: Core Processing (Room reverb + dry/wet)
2. Phase 3.2: Type Switching (6 types)
3. Phase 3.3: Character Control (warm/bright filter)

**GUI Phases:**
1. Phase 4.1: Layout and Basic Controls (Ouaricon Naturalist aesthetic)
2. Phase 4.2: Parameter Binding (6 parameters)

### References
- v2.0.0 engine rewrite: `plugins/O-SimpleReverb/improvements/reverb-engine-rewrite.md` (brief), `.planning/improvements/reverb-engine-rewrite/` (context, research, plan, baseline, summary)
- Creative brief: `plugins/O-SimpleReverb/.ideas/creative-brief.md`
- DSP architecture: `plugins/O-SimpleReverb/.ideas/architecture.md`
- Implementation plan: `plugins/O-SimpleReverb/.ideas/plan.md`
- Reference plugins: FlutterVerb, DriveVerb, LushVerb

### Next Steps
Plugin complete and installed.
