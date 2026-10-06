# Stage 2 (DSP) — SUMMARY

**Plugin:** O-simpleWavetable · **Stage:** 2 of 4 · **Phases:** 2.1 Bank Engine · 2.2 Voice + Oscillator · 2.3 Modulation → Position · 2.4 Bank Switch / Import / Persistence
**Executed:** 2026-10-05 (Part 1, commit `9d1911ac`) and 2026-10-06 (Part 2) · **Version:** 0.1.0 (D-O)
**Result:** every planned gate is green across 5 offline drivers (96 PASS lines, 0 FAIL, 0 JUCE assertions), plus the host checks (auval, pluginval VST3/AU strictness 10, pedalboard on the installed VST3).

---

## Files

| Path | Part | Purpose |
|------|------|---------|
| `Source/WavetableBank.h` | 1 (+2) | Flat `[level][frame][2049]` bank, thumbs, test-only live/reap counters |
| `Source/MipmapBuilder.{h,cpp}` | 1 | Strict 11-level FFT mips, DC + Nyquist zeroed |
| `Source/BankFactory.{h,cpp}`, `Source/BuiltInBanks.{h,cpp}` | 1 | 5 × 32-frame built-ins, process-wide `SharedResourcePointer` |
| `Source/WtRead.h`, `Source/BitQuantizer.h`, `Source/MonoStack.h` | 1 | Band-limited linear read, mid-rise quantizer, last-note stack |
| `Source/WtSynthesiser.h` | 1 | Alloc-free port of JUCE `findVoiceToSteal` (8.0.15 mallocs per steal) |
| `Source/WtVoice.h` | 1 → 2 | Voice: phase, level + D-K hysteresis, latched/interp frames, amp + mod env, 2 ms position smoother, frozen-cycle crossfader |
| `Source/PositionLfo.h` | 2 | Global per-sample LFO: free / PPQ-locked tempo, splitmix64 S&H (D-J) |
| `Source/PositionSmoother.h` | 2 | `kPoles = 1`, τ = 2 ms (D-N: the one-pole shipped) |
| `Source/WavetableImporter.{h,cpp}` | 2 | `decode`, `buildImportedBank` (one builder), `sanitiseName`, flac16 / pcm16gz codecs |
| `Source/PluginProcessor.{h,cpp}` | 1 → 2 | Engine, chunk loop, output stage, modulation buffers, REG-01 + D-C reaper, import API (D-E), `IMPORTED_BANK` persistence |
| `CMakeLists.txt` | 1, 2 | `target_sources`; console targets bank-check, dsp-check, mod-check, import-check (all `OSIW_TEST_HOOKS=1`) |
| `tests/bank-check`, `tests/dsp-check` | 1 | 2.1 / 2.2 gates (dsp-check G-READ reference updated in 2.3 for the smoother) |
| `tests/mod-check` | 2 | 2.3 gates (18) |
| `tests/import-check` | 2 | 2.4 gates (34) |
| `tests/state-check` | 1, 2 | P6′ flip; P9 valid two-reopen, P10 stale child |
| `.planning/research/ARCHITECTURE.md`, `parameter-spec.md`, `REQUIREMENTS.md` | 2 | Dated amendment notes (Task 21); checksums refreshed in STATUS |

## Decisions as applied

| ID | Applied |
|----|---------|
| D-A | `kMinBandLimitedLevel = 1`. Worst sweep −74.9 dB at every rate. |
| D-B | Narrow pulses (16/24/32) are judged on the equal-RMS metric: −67.0 dB (≤ −60). **NAMED EXCEPTION** is printed in the dsp-check log and noted in REQUIREMENTS QUAL-02. |
| D-C | `audioHeldBank` is published before the exit increment and the sweep never frees it; seq_cst reaper atomics. Proved by the graveyard negative control (below). |
| D-D / D-I | 20 ms smoothers on the position knob, `lfo_depth` and `env_amount`, seeded in `prepareToPlay`. |
| D-E | Import API `importFromFile`, `importFromMemory`, `getImportStatus`, `getImportStatusVersion`, `getBankDisplayGeneration` (+ `getImportedBankSnapshot` for Stage 3 thumbnails). |
| D-F | QUAL-02 runs on Drive 32, Pulse frame 1 and a voice-level Saw-1023 arm. |
| D-G | QUAL-03 has two tiers: exactness on every trigger, and the ratio tier on bank/import swaps. |
| D-H | One `BlockContext`, indexed at absolute in-buffer positions. |
| D-J | S&H = splitmix64(seed, cycleIndex). |
| D-K | Downward-only level hysteresis of a quarter-tone. The G-Q2 re-run after 2.4 is unchanged (−114.3 / −74.9 dB). |
| D-L | `setMinimumRenderingSubdivisionSize (1, true)`. G-BLOCKSIZE: 64/512/4096/random renders are bit-identical. |
| D-M | Auto-select Imported through `AsyncUpdater` (the harness calls `handleUpdateNowIfNeeded`). |
| **D-N** | **One-pole shipped.** G-CLICK-SQ 1.414 and G-CLICK-SH 1.144 (≤ 1.5). The two-pole probe would give 0.971 / 1.011. Fallback not taken. |
| D-O | VERSION stays 0.1.0. |

## Deviations

1. **Crossfade law is raised-cosine, not linear** (orchestrator fix, Task 20 route; ARCHITECTURE amendment 11).
   - **What changed:** `w = 0.5 − 0.5·cos(π·k/len)`. The 5 ms length, equal-gain sum and fold rule are unchanged.
   - **Why:** the linear ramp's slope corners add a second-difference kick of |A−B|/len at both ends of the fade. On smooth low notes the ratio tier failed:
     - A1 Sine→Saw ↔ Formant: 1.54 / 1.73
     - Imported → empty: 14.1
     - 16 held notes: 1.68
   - **After:** 0.996 / 0.996 / 1.31 / 0.88.
   - **Not loosened:** thresholds unchanged. The exactness tier, measured against the new law, stays ≤ 6e-8.
2. **The 2.4 harness was written by the orchestrator.** dsp-agent dispatch C wrote all of the 2.4 source and the state-check P9/P10 probes, then stalled twice (stream watchdog) before writing `tests/import-check`. The orchestrator wrote that driver against RESEARCH §7.1–7.5.
3. **Extra test hooks (test builds only):** `testPolesOverride`, `setKnobRampOffForTesting` (2.3), and `publishImportedBankForTesting` (2.4, which is the worker's publish path without the worker).
4. dsp-agent dispatch B appended two entries to the shared `.claude/agent-memory/dsp-agent.md`. That file also holds another session's uncommitted edits, so it is **not** in this stage's commits.

## Measured gate values (Debug, out-of-repo build, 2026-10-06)

**bank-check (13/13)**
- G-PEAK: 0.000003 dB
- G-DC: −181.8 dB
- G-BL: −137.6 dB above kmax
- G-DRIVE: frame 1 h3 −45.80 dB
- G-NEG: fails as designed (−34.0 dB with Nyquist kept)
- buildMillis: 24.7 ms

**dsp-check (20/20, plus `--alloc-check` 0 allocs)**

| Gate | Value | Limit | Margin |
|------|-------|-------|--------|
| G-Q2-C8 (Drive 32 / Pulse 1 / Saw-1023 × 44.1/48/96k) | −114.3 dB | ≤ −100 | 14.3 dB |
| G-Q2-SWEEP (MIDI 21..108, same set) | −74.9 dB | ≤ −70 | 4.9 dB |
| G-Q2-PULSE (D-B named exception, equal-RMS) | −67.0 dB | ≤ −60 | 7.0 dB |
| G-DSP02 (analyzer liveness) | Off −20.8 / On −114.4 dB | Off > −40 | — |
| G-PITCH | 0.0009 c | ≤ 1 | — |
| G-VEL | −11.905 dB | −11.90 ± 0.05 | — |

Also passing: G-DSP01, G-DSP03, G-FULL, G-READ (0 ulp), G-POLY, G-MONO, G-RETRIG, G-NOTEOFF, G-OUT, G-IMPEMPTY, G-BLOCK, G-FINITE.

**mod-check (18/18)**
- LFO shapes vs analytic: ≤ 3e-8
- Free 0.5 Hz period: 96000 samples
- Tempo phase error: 1.3e-15 over 2381 random blocks × 16 divisions × 2 tempos
- Loop re-lock: exact
- Global phase: bit-identical
- S&H determinism: byte-identical across runs and a re-prepare; 74 unique job seeds
- DSP-05: clamp holds; zipper 1.003 (negative control fires)
- FUNC-06: sweep, lifetime and re-push all pass
- Click gates: SQ 1.414, S&H 1.144
- 0 audio-thread allocations

**import-check (34/34)**
- **QUAL-03 exactness:** bank, empty→X, X→Y, X→empty, bandlimit, interp, level, fold. max |y − ideal| ≤ 5.96e-8; every hard-switch negative control ≥ 0.157.
- **QUAL-03 ratio:** A1/A2 × both directions × 8 phases, import swap, Imported→empty, 16 held, fold. Worst 1.31 (≤ 1.5), liveness ≥ 0.26, negative controls ≥ 56.
- **G-ALLOC-24:** 0 audio-thread allocations over 200 bank switches + 40 import publishes + 10 restore-publishes from a second thread (liveness counted 1).
- **Reaper:**
  - Quiescent: A freed at once.
  - Held: X survives the idle sweep and is freed after 1 block.
  - **D-C:** without the amendment, X is reaped and deref-after-reap = 1; with it, 0.
  - +2 rule: kept at exits e and e+1, freed at e+2, all in flight.
  - Soak: 3163 blocks, 50 imports, deref 0, max held 2 (≤ 3), 41 reaps; at the end retired = 0 and live banks = 6 (5 + 1).
- **FUNC-03:**
  - Frame counts: 441000@44.1k → 215, 600000 → 256, 2048 → 1, 2·2048+100 → 2; auto-select works.
  - Rejections: tooShort, unreadable and tooLarge (97 MB), each leaving the bank untouched.
- **COMPAT-03:**
  - WAV/AIFF/FLAC × 44.1/48/96: identical int16 9/9, bit-identical banks 9/9.
  - 24-bit WAV gives the same PCM.
  - Float NaN/inf is scrubbed (== the zero-substituted file).
  - Stereo mean: Δ 0 LSB. 6-channel mean: Δ 1 LSB, against 28938 LSB if only the first 2 channels were averaged.
- **FUNC-04:**
  - memcmp of all 11 levels × 5 frames (450780 bytes) is equal; filename and frame count match; the data string is re-saved verbatim.
  - Negative control: one flipped int16 fails both the builder compare and the restore compare.
  - pcm16gz restores bit-identically and is re-saved verbatim.
  - Absent child → empty Imported, exact silence (liveness peak 0.342).

**state-check (11/11):** P0–P8 unchanged, plus P9 (valid two-reopen: 1 child per save, identical data, bit-identical bank) and P10 (the reopen restores the **second** file).

**Static checks**
- 0 non-ASCII bytes
- 0 `PLUGIN_VERSION`
- 0 `toBase64Encoding`
- No `isVoiceActive` definition
- Every `.cpp` is in `target_sources`
- `OSIW_TEST_HOOKS` appears only inside `#if` blocks and on test CMake lines
- No locks or allocation in `processBlock` / `renderBlock` / `renderMono` or the voice render path (the alloc gates are the proof)

## Host validation (Release, installed `-dev` bundles)

- **Build:** `build-and-install.sh` with VST3 + AU, plus the Standalone. 0 warnings from plugin sources.
- **auval:** `auval -v aumu OSiW OuDv` — AU VALIDATION SUCCEEDED.
- **pluginval strictness 10:** VST3 SUCCESS and AU SUCCESS. The state tests now exercise the real `setStateInformation`.
- **pedalboard** (installed VST3, params set by `p.parameters[name].raw_value`):

| Check | Result |
|-------|--------|
| Output | finite |
| C4 pitch | −0.22 c (estimator resolution) |
| Imported, nothing loaded | exact 0 |
| C7 Saw-32 band-limit Off / On | −21.3 / −91.9 dB (the On value is the Blackman-Harris floor) |
| LFO depth 1 vs 0, spectral-centroid max/min | 4.20 vs 1.000 |
| **IMPORTED_BANK through the real VST3 `raw_state`** | injected pcm16gz child → bank = Imported, plays (peak 0.229), re-saved as 1 child with verbatim data; the same state without the child is silent |

- **Harness gotcha** (for future verify runs): pedalboard's `getattr(plugin, name).raw_value = v` is silently dropped. Write through `plugin.parameters[name]`. The VST3 state is outer `VC2!` XML → `<IComponent>` holding JUCE `toBase64Encoding` (`size.chars`) → inner `VC2!` plugin XML + a JUCEPrivateData tail.

## Task 11 listening checkpoint

Signed off by Taylor on 2026-10-05 (resumed with `/plugin-execute O-simpleWavetable 2-dsp`; no tone or feel notes).

## Memory and other notes

- About 23 MB steady state per instance with a full 256-frame import; about 75 MB transient during an import (accepted, RESEARCH §8).
- Task 23 (DAW smoke) is a non-blocking human check for the verify phase: Square/S&H LFO play, bank switches while notes are held, octave bends, save/reopen with Imported. With no UI yet, an import has to come from session state.
