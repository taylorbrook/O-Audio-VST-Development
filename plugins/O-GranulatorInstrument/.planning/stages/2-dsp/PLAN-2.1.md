---
title: "O-GranulatorInstrument Stage 2 (DSP) — Phase 2.1 Execution Plan"
created: 2026-11-07
juce_version: "8.0.15"
phase: "2.1"
summary: "Six waves: (W0) pre-flight, (W1) fork the 13 leaves at 82406fce into Source/dsp/forked/ with FORK-MANIFEST.md and a two-part drift check plus negative controls, (W2) new DSP written header-first (VoiceQuotas waterFillN + integer thinning, Kaiser resampler, MipTape<4>, slot loader + handoff + reaper, MIDI cursor, allocator, amp env, scan/pitch/spawner/renderer/law/mixer, 32-grid Engine) and the processor wiring, (W3) dev-loader editor + harness gates written, then CMake edited and ONE configure/build, harness run with negative controls, (W4) install + pluginval 10 VST3/AU + auval + alloc gate + HANDS-ON-2.1, (W5) SUMMARY, STATUS, path-scoped commit. No tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 2 / Phase 2.1 Execution Plan (Slots, tape, voices, one stream)

## Goal

Notes play. A file loaded into a slot is decoded and resampled off-thread, built into a `MipTape<4>` and swapped in on the audio thread with no lock and no allocation. Up to 16 counted voices (20 slots, WtSynthesiser steal policy, 5 ms steal) each run **one stream (`s1`)**: a Free-mode scheduler thinned to a water-filled integer quota, a scan playhead (3 modes), key-tracked pitch (±48 st), a region-fit spawner, Hann grains from `GrainPool<VoiceGrain,256,32>`, the per-voice output law, the amp env and −6 dB voice headroom. They then pass the wet safety knee and `output_gain`. All of this runs as an absolute 32-sample grid pass engine with MIDI applied at sample offsets. A dev-loader editor (`OGI_DEV_LOADER`) drives the real load API. The feedback return path exists and carries zero.

**Out of scope** (CONTEXT-2.1 § Out of scope): streams 2–4, jitter, window morph, scale, spray/reverse/spread/gain-rand (2.2). Per-grain FX and the CPU gate (2.3). Feedback, record heads, Freeze, Reset, reverb (2.4). Modulation, Mono/Legato/glide, CC64, MPE/NE, tempo (2.5). Record-in, `<Slots>` state, relink, factory sources, telemetry (2.6). WebView (3.1).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT-2.1.md` | D11 fork pin, D12 quotas/thinning, D13 dev-loader editor, the Defaults block, the gate list. **Binding.** |
| `stages/2-dsp/RESEARCH-2.1.md` | §1 quotas (cited **R§1.x**), §2 fork classes and the drift check, §3 MipTape, §4 MIDI, §5 allocator, §6 editor, §7 loader, §8 handoff, §9 seed, §10 alloc gate, Pitfalls **P1–P15**. Authoritative over ARCHITECTURE where they differ. |
| `research/ARCHITECTURE.md` | §2–§12, §18, the Processing Chain (`:273`, `:285`), §A1 (`:520-521`), §A4, the Memory budget. |
| `parameter-spec.md` (LOCKED) | The s1 stream rows, the global rows (root/fine/keyTrack/bend range/polyphony/grain_limit/ADSR/vel_to_amp/output_gain/scan/loop). **Not edited.** |
| Parent at the pin: `git show "${PIN}:plugins/O-Granulator/Source/dsp/<f>"` with `PIN=82406fce` | The 13 leaves. Also reference only, not copied: `Spawner.h:428-540` (acquire), `Engine.h:216`, `StreamScheduler.h`. **Never read the parent working tree** (it holds another session's uncommitted preset work). |
| `modules/audio/grain-engine/…/BandlimitedReader.h` (1.1.0) | `blr::*`, `SincBank`, `latchLevel`, `MipRing<4>` statics (`:253-258`, `:270-290`, `:322-344`, `:418-427`, MipSnapshot `:543`). **Not edited** (G5 module-gap candidate). |
| grain-engine `GrainPool.h` (`:236-250`), `GrainScheduler.h` (`:44-47`, `:84-97`, `:145-152`), `CounterRng.h` | Pool, scheduler, RNG APIs. |
| `plugins/O-simpleWavetable/Source/WtSynthesiser.h:33-115` | Steal policy order (port the policy, not the class). |
| `plugins/O-simpleSampler/Source/PluginProcessor.cpp:749-770` | FileChooser precedent. |
| `plugins/O-Granulator/tests/render-harness/engine_gates.h:65-102,854-870` (`git show 82406fce:`) | `malloc_logger` alloc-gate hook (copied into this harness, renamed). |
| `scripts/param-dump/ParamDump.cmake:150-169` | Consoles take every plugin `.cpp` **except** paths matching `PluginEditor`. This drives P4. |
| This plugin: `Source/PluginProcessor.{h,cpp}`, `tests/render-harness/main.cpp`, `CMakeLists.txt` | Stage 1 shell. 55 harness rows must stay green. |

---

## Plan-phase decisions (resolve RESEARCH open items)

- **P1 [research flag, adopted] Integer-quota thinning.** Thin each counted voice against `m_k = ⌊q_k⌋` and the maximum latched length: `T_k = max(fs/density, (L_max + 1)/m_k)`. With this, `Σ live ≤ Σ m_k ≤ Q` holds exactly, R3 cap kills become a transitions-only path, and the gate asserts **0 R3 kills in steady state** (R§1.3, P2). This refines D12's "thinned to its water-filled quota" and keeps its intent ("never truncated"). The output law uses what thinning produces: `N_eff = len̄/T_k`, called as `streamPower(n_eff, n_eff, hann, …)` (full-window branch). Never pass `(N, q)` with `N > q`. *If the user prefers the literal real-q reading, drop row Q1b only; nothing else changes.*
- **P2 [research flag, adopted] Wire `s1_size_rand` in 2.1.** Without at least one random draw, the "different-seed control differs" row is vacuous (R§9, P10). It is one CounterRng draw per onset, keyed `(serial << 24) | onsetIndex`, and it already feeds `L_max`. It moves out of the "unread params" control set.
- **P3 Layout.** Forked leaves go to `Source/dsp/forked/` with `Source/dsp/forked/FORK-MANIFEST.md` (ARCHITECTURE §A1 `:520-521`). New 2.1 code goes to `Source/dsp/` in namespace `ogi`. The drift script is `tests/fork-drift-check.sh`.
- **P4 The dev editor is named `Source/PluginEditorDevLoader.{h,cpp}`.** The console filter drops any source matching `PluginEditor` (`ParamDump.cmake:156`, VERIFIED), so the harness and param-dump exclude it with no new CMake logic. The `createEditor()` branch is `#if JUCE_WEB_BROWSER && OGI_DEV_LOADER` (consoles build with `JUCE_WEB_BROWSER=0` and never see the define). `OGI_DEV_LOADER=1` is set on the **plugin target only**, by `option(OGI_DEV_LOADER … ON)`.
- **P5 MIDI is iterated in place.** There is no copy, no 512 cap and no re-sort; offsets are clamped to `[0, n−1]` (R§4, P5). Engine-generated events at a sample (the `polyphony`-lowered force-release) apply **before** that sample's host events.
- **P6 The voice axis is indexed by slot (20).** Empty and stolen slots have demand 0, and stolen voices spawn nothing (R§1.1). `waterFillN<20>`. 2.2 changes only N (→ 20×4).
- **P7 Leaf classes per R§2.2.** In 2.1, only `EngineConstants.h` and `ControlGrid.h` are adapted. `ScaleQuantiser.h`, `ModMatrix.h`, `ModSources.h` and `UiTelemetry.h` are copied rename-only now, are included by no TU, and are adapted in their own phases. Their manifest class is `ADAPTED (pending 2.x)`, so the local-fidelity check still covers them byte-for-byte until then.
- **P8 Signal chain in 2.1:** Σ voices → (reverb absent: unity bypass, `reverb_*` unread) → wet safety `1.9953·tanh(x/1.9953)` → `output_gain` → out. The feedback return buffer is allocated in `prepareToPlay` and zero-filled. Nothing writes it.
- **P9 Version stays 0.1.0.** The I/O is unchanged, so the Logic AU I/O cache is unaffected. The version bumps at Stage 2 close.
- **P10 Stage 1 is a regression floor.** All 55 Stage 1 harness rows stay green. The `param-dump` output stays **byte-identical** to the Stage 1 VERIFICATION capture, because no parameter changes. The dest and source goldens are untouched.
- **P11 Goldens re-baseline in 2.2 is expected** (P15): 2.1 ignores the spray, jitter and spread defaults. SUMMARY must say so.

---

## Tasks

**Roles.** **[agent]** = dsp-agent writes files (no Bash). **[run]** = build/gate runner: commands, builds, validators, commit. **BG** = `run_in_background`, log to `$S/<name>.log`. `$S` = the executing session's scratchpad. Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `scripts/`, `build/`, `modules/` or `~`.

### Wave 0 — pre-flight

**T1. Pre-flight** [run]
- Files: none. Output to `$S/preflight.txt`.
- Depends on: none.
- Done when (any mismatch → STOP):
  - `git branch --show-current` = `main`. Record `git status --short`; the foreign paths (`.claude/agent-memory/*`, `modules/registry.yaml`, `.claude/compaction-snapshot.md`, GrainScatter `nielsen-dafx26.txt`, any other session's) are **never staged**.
  - The `parameter-spec.md` sha256 values (file + body) match STATUS `contract_checksums`.
  - `git cat-file -e 82406fce` succeeds. `git diff --stat 82406fce HEAD -- plugins/O-Granulator/Source/dsp` is empty. If it is not, record it: that is upstream drift at fork time, and you must ask the user before proceeding.
  - The Stage 1 baseline (current tree) is captured: harness `55 PASS / 0 FAIL` → `$S/h-stage1.log`, and `param-dump` → `$S/pd-stage1.txt`. This is the P10 regression reference.

### Wave 1 — fork step (D11)

**T2. Copy the 13 leaves + manifest** [run copies, agent writes the manifest]
- Files: `Source/dsp/forked/{GrainFx,WindowMorphBank,OutputLaw,Quotas,ScaleQuantiser,FeedbackChain,TempoTracker,ControlGrid,Grain,EngineConstants,ModSources,ModMatrix,UiTelemetry}.h`, `Source/dsp/forked/FORK-MANIFEST.md`.
- Depends on: T1.
- Steps:
  - `git show "${PIN}:plugins/O-Granulator/Source/dsp/${f}" | sed -E 's/\bogran\b/ogi/g; s/OGRAN_TEST_HOOKS/OGI_TEST_HOOKS/g' > Source/dsp/forked/${f}`. Brace every variable (zsh `:P`/`:h` trap). Use BSD sed word boundaries: verify with `grep -c '\bogran\b'` = 0 after the copy, or use `perl -pe` if `\b` is a no-op on macOS sed.
  - **Adapt now:** `EngineConstants.h` (`kPoolLive 256`, `kPoolFade 32`, `kMaxSemis 48`, `kMaxRate 16`, `kMaxLevel 4`; the mod counts stay at parent values, unused until 2.5, and are flagged in the manifest). `ControlGrid.h` (keep `CellLerp`/`ExactRamp`/`GainPair` `:57-165` verbatim; replace `ParamPointers`/`GridParams` `:169-487` with `GiParamPointers`/`GiGridParams` covering the s1 subset + global rows 2.1 reads).
  - Manifest columns: `leaf | parent path | pin | sha256(pin blob) | sha256(local) | class | local edits | first-used phase`. Mark D43 on `FeedbackChain.h` and D47a on `ModSources.h` as "included at pin".
- Done when: 13 files exist; `grep -rn 'ogran\|OGRAN_' Source/dsp/forked` = 0; no VERBATIM leaf references a changed `EngineConstants` symbol (grep, R§2.2).

**T3. `tests/fork-drift-check.sh`** [agent writes, run executes]
- Files: `tests/fork-drift-check.sh` (bash, `set -euo pipefail`, `--parent-root`, `--local-root`, `--pin` overrides; it reads the leaf list and class from the manifest).
- Depends on: T2.
- Check 1 (upstream drift, all leaves): `sha256(git show PIN:path)` vs parent `HEAD:path` and the parent working-tree file. If either differs: DRIFT plus the diff stat.
- Check 2 (local fidelity, VERBATIM + `ADAPTED (pending)` leaves): renamed pin blob ≡ local file, byte for byte. For ADAPTED-now leaves: the local sha256 equals the manifest's `sha256(local)`, so an unrecorded edit fires.
- Done when: the real run prints `0 drift` and exits 0. **NC-a**: one byte flipped in a scratch copy of the parent tree under `$S` (`--parent-root`) → check 1 fires, non-zero exit. **NC-b**: one byte flipped in a scratch copy of a local VERBATIM leaf (`--local-root`) → check 2 fires. Neither control touches a real tree.

### Wave 2 — engine (agent writes; nothing configures yet)

All new code is header-first in `Source/dsp/`, namespace `ogi`. Audio-path rules: no locks, allocation, `shared_ptr` or `std::vector` growth; fixed arrays only; `ScopedNoDenormals` per block; no listener writes a parameter.

**T4. `VoiceQuotas.h` — `waterFillN<N>` + thinning** [agent]
- Depends on: T2.
- `template<int N> struct QuotasN { double q[N]; int m[N]; double lambda; bool capped; }` and `waterFillN(const double(&)[N], int cap)`: the parent `Quotas.h:53-99` body with `kMaxStreams → N`, ties broken by index, fixed `int idx[N]` (R§1.2).
- `thinnedPeriod(fs, density, Lmax, m)` = `max(fs/density, (Lmax+1)/m)` (P1). `nEff = len̄/T`.
- Recompute only on a change of the voice set, `grain_limit`, density or size, at a grid tick.
- Test hooks (`OGI_TEST_HOOKS`): `testThinningOff` (m = ∞), `testOverQuotaOff`, `testD25Off`.

**T5. `KaiserResampler.h`** [agent]
- Depends on: none (W2 tasks T4–T8 can be written in any order; T9 integrates them).
- Offline, double precision. Half-width = 32 zero-crossings of `fc = 0.95·min(fsIn,fsOut)/2`; a 4096-phase table built once per job with `blr::besselI0`, β = 9; lerp between phase rows; **each output sample is normalised by its weight sum** (DC = 1 exactly). Out-of-range input = 0. Equal rates → bit-exact copy, no filtering (R§7).
- Chunked: an `isCancelled()` callback is polled every 64 k output frames.

**T6. `MipTape.h` — `MipTape<4>`** [agent]
- Depends on: none.
- Level storage = the MipRing convention (child at odd parent index `2j+1`; the delay is applied at read time via `MipRing<4>::levelPos`; **no delay baked into the index**, P7). History before 0 = zeros.
- Level-l size = `((F + MipRing<4>::margin(4)) >> l) + 2·kTaps + 4` (P8). Allocated in `prepare(F)` off-thread. The writer flushes `margin(4)` zero frames past F.
- A private `halfband()` duplicates the grain-engine logic (8 lines, double acc, `0.5·c[0]` + 32 folded odd taps). `writeRun(pos, n, L, R)` writes level 0, then each child index whose parent `2j+1` lies in the new span, reading history from the tape's own level-(l−1) array.
- `readStereo(level, pos, set, l, r)`: positional, MipSnapshot `locateStart` model, start clamped to `[0, size−kTaps]`. Callers use `latchLevel(rate, 4)`, **always passing 4** (P6).
- `sincBank()` is touched once in `prepare`.

**T7. Slot loader + handoff** [agent]
- Files: `Source/dsp/SlotLoader.{h,cpp}`, `Source/dsp/SlotHandoff.h`.
- Depends on: T5, T6.
- Loader (R§7, CONTEXT §5):
  - 1-worker `juce::ThreadPool`. Jobs are keyed (slot, source, generation); a newer job calls `removeJob(old, true, 0)`. `shouldExit()` is polled between decode, resample and build chunks.
  - Pristine kept at its native rate. Trim at `min(30 s·fsIn, 2.88 M session frames ↔ native)` and set `trimmed` from either limit. Mono → L=R at build. Overview = a peak pair per 512 frames.
  - The tape is built through `MipTape::writeRun` sequentially (the 2.4 record head reuses it).
  - `SlotInfo {name, frames, nativeRate, trimmed, lastError, generation}` is guarded by a `CriticalSection` that is **never taken on audio**. `prepareToPlay` publishes from a host thread, so every `SlotInfo` write takes the lock.
- Handoff (CONTEXT §6, R§8):
  - `pending[s].exchange(newTape)`. If the returned old tape was never taken by audio, the message thread deletes it.
  - Audio takes at a grid tick. Live grains of that slot get a 5 ms kill fade; new spawns read the new tape at once.
  - Retire: an old tape goes into the audio-produced SPSC ring (capacity 16) only when **no live or fade grain** holds it, scanning all **288** pool slots (P12). A message-thread reaper `Timer` with an **audio entry counter** (memory: generation reaper freezes when the host idles) frees retired tapes, plus a producer-side sweep on publish.
  - SR change: `prepareToPlay` queues a rebuild from pristine via `callAsync`, deduped by (slot, rate). Until the new tape arrives, the reader uses `tapeRate/fs`. The rebuild arrives with the **20 ms dual-read crossfade** (old and new tape both read, linear blend). Reset uses the same path in 2.4.

**T8. Voice layer + grain path** [agent]
- Files: `Source/dsp/MidiCursor.h`, `VoiceAllocator.h`, `AmpEnv.h`, `ScanPlayhead.h`, `VoiceGrain.h`, `GiSpawner.h`, `GiRenderer.h`, `Engine.{h,cpp}`.
- Depends on: T4, T6, T7.
- `MidiCursor`: a per-piece `MidiBufferIterator` walk (P5).
- `VoiceAllocator`: 20 slots = ≤ 16 counted + 4 steal spares, `uint64` note serial. Port the R§5 policy order exactly: oldest-first by serial; protect `low`/`top` among non-released notes (only `low` if they are the same voice); pick in order (same pitch → released → no key down → unprotected → top → low). When all spares are fading, the quietest spare is ended. A steal is a 5 ms forced release, with the stolen voice's grains rendering under the ramp. Lowering `polyphony` live force-releases the excess at a grid tick. Poly only.
- `AmpEnv`: per-sample exponential segments, 2 ms attack/release floors, times latched at stage entry, `g_v = (1−d) + d·v²`. It fills `float a[20][32]` per piece.
- Pitch: `st = (note − root)·keyTrack + fine + bend·pitch_bend_range`, clamped to ±48. `rate = exp2(st/12)·tapeRate/fs`, `latchLevel(rate, 4)`.
- `ScanPlayhead`: loop region `sort(loop_start, loop_end)`, min width 10 ms, computed in the DSP. Retrigger = per voice (stream axis = 1). Free-run = per stream, shared across voices. Ping-pong = per voice with reflection. 100 % = 1 tape-second per second.
- Scheduler: one `GrainScheduler` per voice slot, Free mode. `reset()` at the note-on offset, so the first onset is exactly at note-on. The interval callback is a pure function of (onset index, grid snapshot). On a quota **decrease**, `rescheduleNext(T_new)` (R§1.3).
- `VoiceGrain {Grain g; int voice; uint64 serial; const SlotTape* tape; int slot;}` + `static_assert(is_trivially_copyable)`. Pool = `GrainPool<VoiceGrain,256,32>`. `Grain.h` stays VERBATIM.
- `GiSpawner`: region fit (no record-head clamp). `size_rand` draw (P2). `acquire` re-derived over the voice key (R§1.4):
  - lexicographic argmax on `(pri, age·invLen)`, ties to the lowest slot
  - `pri = 4·stolenVoice + 2·(live_k > m_k) + (k == spawning)`
  - `progress' = (killing?3:0) + (pri − minPri) + age·invLen`
  - the R3 path: `killStep` 5 ms, applied by `applyPendingKill`
  - D25: one extra over-quota kill per acquire while `alive > cap`
- `GiRenderer`: Hann only. The per-voice law uses `streamPower(n_eff, n_eff, hann, …)` (P1). Mixer: env × `kVoiceGainDb = −6 dB` (provisional, tuned in the 2.2 listening pass), no polyphony normalisation.
- `Engine`: absolute 32-sample grid; each piece `[a,b)` does: grid tick at absolute multiples of 32 (snapshot params via `GiGridParams`, take pending tapes, quotas, polyphony shrink) → engine events → host MIDI at offsets → schedulers → spawn → render → env × gain → mix. Then the wet safety, `output_gain`, and the zeroed feedback return (P8).
- Every random draw keys on the note serial, **never** on the slot.

**T9. Processor wiring** [agent]
- Files: `Source/PluginProcessor.{h,cpp}`.
- Depends on: T7, T8.
- `prepareToPlay` sizes the engine, pool and feedback buffer, and queues SR rebuilds. `processBlock` runs the engine: `ScopedNoDenormals`, sidechain still ignored (Stage 1 R1 rows stay green).
- Public message-thread API (shared with 3.2): `void requestSlotLoad(int slot, const juce::File&)` and `SlotInfo getSlotInfo(int slot) const` (copy under the lock). The `AudioFormatManager` is created in the ctor and does `registerBasicFormats()`.
- `loadSlotSync` (both overloads) is implemented for real: it runs the same job body on the calling thread and publishes through the same `pending` exchange.
- `createEditor()`: `#if JUCE_WEB_BROWSER && OGI_DEV_LOADER` → `new DevLoaderEditor(*this)`; otherwise `GenericAudioProcessorEditor` (P4).
- Test hooks: latched-rate readout, per-serial grain log (onset sample, size, start pos), R3-kill counter, live count per tick, `pending` depth, retire and reap counters, and the T4 switches.

### Wave 3 — editor, gates, CMake, build

**T10. `Source/PluginEditorDevLoader.{h,cpp}` (D13)** [agent]
- Depends on: T9.
- Wraps `GenericAudioProcessorEditor`. Above it: 4 "Load slot N" buttons and 4 status lines. One `std::unique_ptr<juce::FileChooser>`, re-made per click, plus `int pendingSlot`.
- `launchAsync(openMode|canSelectFiles, [safe = SafePointer(this), slot] { if (!safe) return; … proc.requestSlotLoad(slot, f); })`. An empty result early-outs.
- A 10 Hz `Timer` reads `getSlotInfo`. The processor never calls the editor.
- Header comment: "Removed in Phase 3.1 together with `OGI_DEV_LOADER` and the `createEditor` branch."

**T11. Harness gates** [agent]
- Files: `tests/render-harness/engine_gates_21.h` (included from `main.cpp`), plus synthetic-source helpers that write WAVs into a temp dir under `juce::File::getSpecialLocation(tempDirectory)` and delete them at exit. No checked-in audio.
- Depends on: T9.
- Rows (each with its negative control; a row without a control that can fail is not accepted):

| ID | Row | Stimulus | Pass | Negative control |
|---|---|---|---|---|
| F1 | Fork drift | (runner executes T3) | 0 drift | NC-a, NC-b |
| Q0 | `waterFillN<4> ≡ waterFill` | 10 k random demand vectors, random caps | bitwise q/λ/capped | perturbed tie order → differs |
| Q1a | Σ live ≤ cap | 16 notes, d 200, size 2000 ms, cap 32/64/128/256 | non-killing live ≤ cap at every tick | — (Q1b is the control) |
| Q1b | 0 steady-state R3 kills | as Q1a, after the transition window | R3 kills = 0 | `testThinningOff` → kills > 0 |
| Q2 | No starvation | A alone 3 s (fills cap 64), then B | B ≥ m_B within L_max + 1 cell; A ≤ m_A after L_max | `testOverQuotaOff` → B < m_B at the same tick |
| Q3 | Cap lowered live | 8 notes over-demand, cap 256 → 32 | alive ≤ 32 within L_max + 5 ms | `testD25Off` → alive > 32 |
| Q4 | Level under thinning | stationary noise, 1/4/16 voices, over-demand | per-voice RMS within ±0.5 dB of the law | old-vs-old (same render twice) = 0 dB |
| T1 | MipTape ≡ MipRing | stereo noise, `writeRun` in random runs 1…4096 vs `MipRing<4>::push` | bitwise per level, settled indices | 1-ulp perturbed halfband tap → fails |
| T2 | readStereo ≡ MipSnapshot | 1 k random (level, pos, set) | bitwise | same perturbation → fails |
| K1 | Resampler DC | constant 0.5, 44.1→96 k and 96→44.1 k | `|out−0.5| ≤ 1e-6` interior | unnormalised weights → fails |
| K2 | Pass/stop | sweep tones | ripple ≤ 0.01 dB to 0.9·min/2; stopband ≥ 90 dB | β = 2 → stopband fails |
| K3 | Equal rates | 48→48 k | bit-exact copy | — |
| L1 | Trim | 40 s @ 44.1 k; 20 s @ 192 k | trimmed at 30 s; 192 k trimmed at 15 s (2.88 M cap) | 10 s file → not trimmed |
| L2 | Mono upmix | mono file | L ≡ R | stereo file → L ≠ R |
| P1 | Pitch (rate hook) | root, root ± 12, keyTrack 0 × 3 keys | latched rate = `exp2(st/12)·tapeRate/fs` to 1e-12; keyTrack 0 constant | keyTrack 100 → differs across keys |
| P2 | Pitch (audio) FUNC-03 | 440 Hz sine, one long grain (2000 ms, d 0.5), autocorr over the central 50 % | ±1 ¢ | fine +5 ¢ → detected ≥ 4 ¢ off |
| P3 | SR mismatch DSP-07 | 96 k session, 44.1 k source; +48 st 4 kHz → 32 kHz bin; +24 st 15 kHz → 36 kHz bin | alias ≤ −60 dB vs reference | positive row: 2.5 kHz @ +48 → 40 kHz **present** |
| P4 | ±48 clamp | key root+60 | pitch = +48; level 4 latched | — |
| S1 | Scan FUNC-04 | scan 100 % / 0 %, 3 modes | 100 % = 1 s/s over the region; 0 % holds; Retrigger restarts at note-on; Free-run does not; Ping-pong reflects | Retrigger vs Free-run second note → start pos differs |
| S2 | Loop region | loop_start > loop_end; width < 10 ms | sorted; width = 10 ms; no param writes | — |
| V1 | 4-note chord FUNC-01 | 4 distinct pitches | 4 clouds at 4 rates (hook) | — |
| V2 | Steal click | tonal source, polyphony 4, 6 **distinct** pitches (P9) | overflow verified (steal count = 2, stolen serials asserted); click metric ≤ threshold | no-steal control (polyphony 16) is the baseline; the 0 ms-ramp hook → click > threshold |
| V3 | Steal invariance | same script, slot assignment forced different | per-serial grain logs identical | seed change → logs differ |
| V4 | Polyphony lowered live | 12 → 4 | excess released over 5 ms; no click | — |
| M1 | MIDI ties | same-offset on→off(60); off→on(60) with 60 sounding; 600 events/block | no stuck note; retrigger; all applied, 0 stuck | — |
| E1 | ADSR DSP-02 | A/D/S/R at 4 settings; vel 0.25/1.0 × `vel_to_amp` 0/50/100 | stage times within one cell; level = `g_v` ±0.05 dB; click-free | — |
| H1 | Load while sounding FUNC-02 | 4 notes held, load slot 1 twice | no click, no dropout (RMS gap), kill fades 5 ms | — |
| H2 | Host idle | 100 `requestSlotLoad`, 0 `processBlock`, message loop pumped | `pending[s]` ≤ 1; every superseded tape freed | reaper without the entry counter → leak observed |
| H3 | Retire vs fade spare | swap while grains sit in fade slots | tape not retired until all 288 slots are clear | 256-slot scan hook → early retire detected |
| H4 | SR change mid-note | 48 → 96 k with a note held | correct pitch throughout (rate hook) + 20 ms crossfade, no click | — |
| H5 | prepare/setState stress | 200 interleaved calls, loads in flight | no crash, no leak (leak detector), SlotInfo consistent | — |
| N1 | Block-size null | scripted MIDI, `s1_size_rand` 50 %, blocks 32/441/512/4096/variable + large-block probe | bit-identical | different seed → differs |
| N2 | Unread params | touch every unread param (s2–s4, FX, feedback, reverb, mod) | bit-identical to defaults | touch `s1_density` → differs |
| A1 | Audio-thread allocations | note on/off; 20-note chord @ poly 8; swap mid-note; SR change mid-note; cap 256→32 | 0 allocations (`malloc_logger`, audio-thread scoped) | liveness probe (`malloc(64)` armed → 1) |
| R0 | Stage 1 regression | existing 55 rows | 55 PASS | — |

- The click metric and threshold use the existing suite click-gate recipe (second-difference peak vs the no-steal control, memory index `index_dsp_gate_design`). State the threshold in the row and print the margin.

**T12. CMake + configure + build + harness** [agent edits CMake, run executes]
- Files: `CMakeLists.txt`.
- Depends on: T2, T3, T10, T11.
- Edits:
  - Add every new `Source/dsp/*.cpp` and `Source/PluginEditorDevLoader.cpp` to `target_sources`.
  - Add `option(OGI_DEV_LOADER "Dev loader editor until Phase 3.1" ON)` → `target_compile_definitions(OuariconGranulatorInstrument PRIVATE OGI_DEV_LOADER=1)`, plugin target only.
  - Add `Source/dsp` and `Source/dsp/forked` to the include paths.
  - No change to the console definitions beyond the existing `OGI_TEST_HOOKS=1`.
- Run (BG): `cmake -S . -B build` → `ninja OuariconGranulatorInstrument_{VST3,AU,Standalone} O-GranulatorInstrument-{render-harness,param-dump}`.
- Done when:
  - the build has rc 0 and no new warnings in the new TUs
  - the harness passes every row in T11 with 0 FAIL, and every negative control fires
  - `param-dump` is byte-identical to `$S/pd-stage1.txt` (P10)
  - `grep -rn 'PluginEditorDevLoader\|DevLoaderEditor'` in the console compile commands = 0 (exclusion proven)
  - `tests/fork-drift-check.sh` is green, and NC-a and NC-b fire
- On a configure failure: move `CMakeLists.txt` aside to its Stage 1 version and STOP, so the shared configure stays healthy for other sessions.

### Wave 4 — install + validation + hands-on

**T13. Install + validators** [run]
- Depends on: T12.
- `./scripts/build-and-install.sh O-GranulatorInstrument` (BG; the dual-variant sweep is automatic).
- `auval -v aumu OuGI OuDv` PASS, plus the `ZZZZ` negative control. Never `auval -a`.
- pluginval strictness 10, VST3 ×2 + AU (BG, one at a time, `--timeout-ms 600000`). The editor tests run against `DevLoaderEditor`. Record `Main bus num input channels: 2`.
- Standalone alive after 6 s.
- Done when: all PASS. A failure attributable to the editor → fix in T10 and rerun T12/T13.

**T14. `HANDS-ON-2.1.md` (human gate)** [agent writes the checklist, user runs]
- Files: `stages/2-dsp/HANDS-ON-2.1.md`.
- Depends on: T13.
- Rows (in the Standalone **and** one DAW):
  - load each of the 4 slots via the buttons; the status line shows name/frames/rate/trimmed
  - play a chord: 4 pitched clouds
  - play past polyphony: steals are inaudible
  - ADSR sweep audible
  - scan modes audible
  - load while holding notes: no click
  - load a > 30 s file: trimmed flag
  - change the session rate mid-note (Standalone audio settings): pitch holds
  - **UAF row:** open the chooser, close the plugin window, then pick a file → no crash and no load
  - reopen the session → slots empty (expected until 2.6)
- Done when: the user marks every row; failures are recorded with notes. This gate is **not** self-certified.

### Wave 5 — bookkeeping + commit

**T15. SUMMARY, STATUS, commit** [agent writes, run commits]
- Files: `stages/2-dsp/SUMMARY-2.1.md`, `.planning/STATUS.md`.
- Depends on: T13 (T14 may still be pending; STATUS then says `stage_2_phase_2_1_executed_hands_on_pending`).
- SUMMARY includes:
  - row results with margins
  - P1/P2 adopted, as research flags
  - "goldens re-baseline in 2.2 expected" (P11)
  - `kVoiceGainDb −6 dB` provisional
  - fork manifest and drift result
  - any deviation from this plan, with its reason
- Commit:
  1. Re-run `git branch --show-current` (= `main`) and `git status --short` **immediately before**.
  2. Every 2.1 file is new: `git add` the exact new paths under `plugins/O-GranulatorInstrument/`, then `git commit -- plugins/O-GranulatorInstrument`.
  3. Never `-A` or `-a`. The `modules/registry.yaml` foreign hunk is not ours and is not staged.
  4. Afterwards, `git log -1 --stat` shows only our paths.
  5. **No tag.**

---

## Dependency graph

```
T1 ─→ T2 ─┬─→ T3 ──────────────────────────────────────────┐
          └─→ T4 ─┐                                        │
     T5 ─→ T7 ────┼─→ T8 ─→ T9 ─┬─→ T10 ─┐                 │
     T6 ─┴────────┘             └─→ T11 ─┴─→ T12 (CMake, configure, build, gates) ─→ T13 ─→ T14 (human)
                                                                                      └──→ T15
```

T5 and T6 are independent of the fork and can be written first. CMake lands last (T12), after every file it names exists. A broken plugin CMake breaks the shared configure for every session.

## Execution notes

- **Agent split.** dsp-agent writes T2's manifest, T3–T11, T14's checklist and T15's SUMMARY. The runner does the copies, every build, gate and validator, and the commit. Long commands (configure, ninja, build-and-install, pluginval, cold auval) run BG with logs in `$S`.
- **Writing order inside W2:** pure headers first (T4, T5, T6), then T7, T8, T9. Do not configure between them.
- **Parent reads** always go through `git show 82406fce:…`, never the working tree.
- **AU / Logic.** The I/O is unchanged from 0.1.0. Clear the AU cache via build-and-install. Logic stays closed while the validators run.
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument 2-dsp`.

## Success criteria (CONTEXT-2.1 gate)

| Criterion | Proven by |
|---|---|
| 4-note chord = 4 pitched clouds; steals click-free on a tonal source with verified overflow vs a no-steal control (FUNC-01) | V1, V2, V3, V4 |
| Root + keyTrack 100 % = file pitch ±1 ¢; keyTrack 0 fixed (FUNC-03) | P1, P2 |
| 44.1 k file in a 96 k session at the correct pitch; +24/+48 st alias ≥ 60 dB down (DSP-07) | P1 (tapeRate/fs), P3, P4 |
| Scan 100 % real time, 0 % holds, Retrigger vs Free-run (FUNC-04) | S1, S2 |
| Load while sounding: no click/dropout, 0 audio allocations; 30 s+ trimmed (FUNC-02, PERF-01/03) | H1, A1, L1 |
| MipTape `writeRun` ≡ MipRing bitwise | T1, T2 |
| Host idle ≤ 1 pending/slot; prepare/setState stress; SR change mid-note | H2, H3, H4, H5 |
| Block-size null bit-identical + different-seed control | N1, N2 |
| ADSR audible, click-free; velocity per `vel_to_amp` (DSP-02) | E1 + HANDS-ON |
| D12: Σ live ≤ cap; no starvation; lowered cap converges with a no-converge control | Q0–Q4 |
| D11: manifest at pin `82406fce` with sha256; 0 drift; negative controls fire | T2, T3, F1 |
| D13: dev loader works in Standalone + DAW; pluginval 10 VST3 + AU; auval | T13, T14 |
| Stage 1 regression floor | R0, param-dump byte-identical (T12) |

## Risks

- **R1 Integer thinning wastes < 1 grain per voice** (≤ 16 of 256). Accepted for exactness (P1). Q4 proves the level law still holds.
- **R2 The MipTape tail.** The level arrays must include `margin(4)`. Undersizing reads past the end only in the last ~1 k frames at high pitch. T1 covers settled indices; add a +48 st read in the last 1 k frames to P4.
- **R3 Tape retire races.** A tape held by a fade spare that gets retired is a UAF. H3 with its 256-slot control is the gate; ASan is optional (memory: ASan on macOS 26 caveats, `index_build_notes`).
- **R4 Reaper freeze when the host idles the audio thread.** The entry counter is mandatory, and the H2 control proves it matters.
- **R5 The async chooser UAF is invisible to pluginval.** It is covered by construction (dtor nulls the callback + SafePointer) and by the HANDS-ON UAF row.
- **R6 pluginval runtime** with 261 params + Generic editor: BG, one run at a time, 600 s timeout.
- **R7 Shared configure.** CMake is edited last; on failure, revert to the Stage 1 file and STOP.
- **R8 Steal tests that replay a sounding pitch go vacuous** (P9). V2 uses distinct pitches and asserts the stolen serials.
- **R9 The 2.2 golden shift** looks like a regression unless SUMMARY flags it (P11).
- **R10 Concurrent sessions.** Foreign dirty paths exist now (T1). Path-scope the commit and re-check right before it.
