# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.1 — CONTEXT

**Date:** 2026-11-07
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:** `parameter-spec.md` (LOCKED, 261 params), `BRIEF.md`, `REQUIREMENTS.md`, `research/ARCHITECTURE.md` (§2–§12, §18, §A1, §A4), `ROADMAP.md` § Phase 2.1, `stages/1-foundation/VERIFICATION.md`

## Goal (ROADMAP Phase 2.1)

Build the load-bearing skeleton:
- loader → MipTape → audio-owned swap
- the voice allocator plus the amp env
- **one stream per voice (`s1` only)**
- the absolute 32-grid pass engine, with MIDI applied at sample offsets
- the pool

There are no per-grain FX and no feedback in 2.1. The feedback return path is present and carries zero.

## Decisions (locked this session)

### D11 — Fork pin: O-Granulator `82406fce` (HEAD, 1.1.0), not the 3.3-closing `00e462b8`
- The parent shipped 1.0.0 (`9866e37a`) and 1.1.0 (`82406fce`) after 3.3 closed. Since `00e462b8`, Stage 4 changed two fork leaves:
  - `FeedbackChain.h`: D43, the AGC follower runs on `d·|x|`, so the 0.7 ceiling tracks saturation. `stepAgc` gains a `d` argument. Test hook `testAgcLegacy`.
  - `ModSources.h`: D47a, the R96 predicate. A non-finite sample or one above `kAnalysisTapMax` contributes 0. Test hook `testPinOff`.
- `dsp/` is byte-identical from `9866e37a` to `82406fce` (1.1.0 changed only presets), and `Source/dsp/` has no uncommitted parent changes. The pin therefore takes the shipped, DAW-verified leaves, and the manifest starts with **zero drift**.
- `FORK-MANIFEST.md` records, per file:
  - source path
  - pin `82406fce`
  - sha256 at copy
  - local modifications, at minimum the `ogran` → `ogi` namespace rename and the `OGRAN_TEST_HOOKS` → `OGI_TEST_HOOKS` macro rename
- It also notes that D43 and D47a are already included, so Stage 4's "fork drift review" does not re-flag them.
- **Fork the full leaf list from ARCHITECTURE §A1 step 1 at 2.1 start**, even the leaves that are first used in 2.2–2.5. This keeps one pin and one manifest. The leaves are:
  - `GrainFx.h`, `WindowMorphBank.h`, `OutputLaw.h`, `Quotas.h`, `ScaleQuantiser.h`, `FeedbackChain.h`, `TempoTracker.h`, `ControlGrid.h`, `Grain.h`
  - the `EngineConstants.h` subset, the `ModSources.h` LFO core, the `ModMatrix.h` slot logic, `UiTelemetry.h`
- Leaves that 2.1 does not use are copied but not included by any translation unit, so they are not compiled into the plugin. The drift check (parent sha256 vs manifest) still covers them.
- Ship the drift-check script in 2.1 (`tests/fork-drift-check.sh` or equivalent). Every Stage 2 verify runs it.

### D12 — Grain budget in 2.1: water-fill + onset thinning on the voice axis
- Fork `Quotas.h` now. 2.1 runs the steady-state law from ARCHITECTURE §11 / §A4 over **≤ 16 demands**: (sounding voice) × `s1`, with the stream axis = 1.
- **Steady state:** each voice's onset clock is thinned to its water-filled quota, so grains are never truncated under sustained over-demand.
- **Transitions:** these are a new note shrinking the other voices' quotas, `grain_limit` lowered live, or a voice steal. They use `obtain(progress)` with the §11 priority (`4·stolenVoice + 2·overQuota + spawning + age·invLen`). The stream term is constant in 2.1. Victims move to the 32 fade spares (5 ms).
- Phase 2.2 only **widens** the same code to 80 demands (4 streams). It does not replace it, so the 2.1 gates must not need a re-baseline in 2.2 beyond the expected stream-axis rows.
- Pull these gates forward into 2.1, on the voice axis:
  - total live grains never exceed `grain_limit`
  - one dense voice cannot starve the others
  - `grain_limit` lowered live converges, and a no-converge negative control differs
- The output law per voice uses `N_eff = min(N_k, q_k)` with full windows (§15). The coherence blend and μ₂ for the window morph arrive with the morph bank in 2.2. 2.1 uses Hann only.

### D13 — Audition path: dev-loader editor behind `OGI_DEV_LOADER`
- New CMake option `OGI_DEV_LOADER`, default **ON** until Phase 3.1. Phase 3.1 removes the option, the class and the `createEditor` branch together, and its gate checks that no `OGI_DEV_LOADER` reference remains.
- `createEditor()` returns a `DevLoaderEditor` that wraps `juce::GenericAudioProcessorEditor`. Above the generic editor it adds:
  - 4 "Load slot N" buttons, each opening an async `FileChooser`
  - one status line per slot (name, frames, native rate, trimmed flag, last error)
- The editor is pure C++ with no WebView and no `Source/ui/` files, so repo check-i18n rule [8] stays inert (Stage 1 D8 is unchanged).
- A button calls the **same message-thread load API** that 3.2's drop and file browser will call (`requestSlotLoad (slot, File)` or similar → loader job → `pending[s].exchange`). The dev editor must not have a private load path. It exists to exercise the real one.
- Slot paths are **not** persisted until 2.6, so a reopened session comes back with empty slots. This is expected and documented in HANDS-ON-2.1.
- The render harness and param-dump consoles exclude `DevLoaderEditor` (the same TU exclusion and `#if` guard pattern as `PluginEditor*`).
- pluginval runs with the dev editor in place. Its editor tests must pass, and the async `FileChooser` must be safe to destroy if the editor closes while the chooser is open (SafePointer / weak reference).
- Each Stage 2 phase closes with a short HANDS-ON-2.x check run through this editor in the Standalone and in a DAW.

## Defaults (no decision needed; restated from ARCHITECTURE as binding for plan)

- **Test material:** the harness synthesises its own source files into a temp dir and loads them through `loadSlotSync`. No checked-in audio.
  - sines at known pitch, for the ±1 ¢ autocorrelation probe
  - a 44.1 k file, for the 96 k session test
  - a long file, for the 30 s trim
  - stationary noise
  - a mono file, for the L=R upmix
  - Factory sources are Stage 4 / 2.6.
- **Slot loader (§5):**
  - 1-worker `ThreadPool`, with jobs keyed (slot, source, generation). A newer job cancels the older one.
  - pristine kept at its native rate
  - 30 s / 2.88 M-frame cap with a `trimmed` flag
  - offline double-precision Kaiser-windowed sinc: 32 zero-crossings per side, cutoff 0.95·min/2, β ≈ 9. Not `WindowedSincInterpolator`, because of its 0.990 DC gain.
  - mono upmixed L=R at tape build
  - overview = a peak pair per 512 frames
  - `SlotInfo` under an off-audio `CriticalSection`
- **SR change:** `prepareToPlay` queues a rebuild from the pristine copy via `callAsync`, deduped by (slot, rate). Until the new tape arrives, the reader uses `tapeRate/fs`.
- **MipTape<4> (§7):**
  - `writeRun` with MipRing group-delay compensation
  - positional `readStereo` with double positions and mirrored edges
  - The loader builds the tape through the same sequential writer that the record head will use in 2.4.
- **Handoff (§6):**
  - publish: `pending[s].exchange`. If the old tape was never seen by audio, the message thread deletes it.
  - take: at a grid tick
  - retire: only via the audio-produced SPSC ring (capacity 16) + a reaper timer + a producer-side sweep
  - new file: the stream's live grains get a 5 ms kill fade, and new spawns read the new tape at once
  - The 20 ms dual-read crossfade is for Reset / SR rebuild. 2.1 ships it for the **SR rebuild** path. Reset arrives in 2.4.
- **Allocator (§3):**
  - 20 slots = ≤ 16 counted voices + 4 steal spares, using the `WtSynthesiser.h` policy: oldest-first, protect the lowest and highest held notes, prefer released voices. When every spare is still fading, the quietest spare is ended.
  - 5 ms forced release on steal, with the stolen voice's grains rendering under the ramp
  - note serial `uint64`. Every random draw keys on the serial, never on the slot.
  - Lowering `polyphony` live force-releases the excess voices over 5 ms.
  - Mono/Legato, glide and sustain are **2.5**. 2.1 runs `voice_mode` = Poly only. Sustain pedal (CC64) is also 2.5.
- **Amp env (§4):**
  - custom per-sample exponential segments, with 2 ms attack and release floors
  - times latched at stage entry
  - velocity gain `g_v = (1 − d) + d·v²`
  - `aₖ[n]` filled into `float[20][32]` per piece
- **Pitch:**
  - inputs: root / fine / keyTrack / channel bend × `pitch_bend_range` (non-MPE)
  - final per-grain pitch clamped to ±48 st (rate ≤ 16, `latchLevel ≤ 4`)
  - read rate × `tapeRate/fs`
  - A key above root + 48 holds that pitch.
- **Scan (§8):**
  - all 3 modes, with loop region `sort(loop_start, loop_end)` and a minimum width of 10 ms, in the DSP with no parameter writes
  - Retrigger: per (voice, stream). Free-run: per stream, shared. Ping-pong: per voice with reflection.
  - scan 100 % = 1 tape-second per second
- **Scheduler:** Free mode only. The first onset lands exactly at the note-on sample. Tempo sync is 2.5.
- **Spawner:** region fit. The record-head clamp is 2.4.
- **Pool:** `GrainPool<Grain, 256, 32>` from grain-engine 1.1.0.
- **Renderer:** Hann window only.
- **Mixer:** per-voice env × `kVoiceGainDb` = **−6 dB, provisional**. It is tuned at the 2.2 listening pass. The polyphony sum is not normalised.
- **Wet safety knee (§18):** in, followed by `output_gain`.
- **Parameters 2.1 does not read:**
  - `s2`–`s4`, every FX row, feedback, reverb and all modulation rows
  - These keep their defaults and must not affect the output: the block-null and parameter-sweep controls include a "touch an unread param" row.
- **Thread rules:** no locks, allocation or `shared_ptr` on the audio thread. No listener writes another parameter. `ScopedNoDenormals` per block.

## Test criteria (Phase 2.1 gate — ROADMAP + D12 additions)

- [ ] A 4-note chord gives 4 independently pitched clouds. Notes beyond polyphony steal with no click above the gate threshold, on a tonal source with verified overflow, against a no-steal control (FUNC-01).
- [ ] With root note + keyTrack 100 %, the output equals the file pitch ±1 ¢ (autocorrelation probe). With keyTrack 0, the pitch is fixed across keys (FUNC-03).
- [ ] A 44.1 k file in a 96 k session plays at the correct pitch. At +24 / +48 st, aliasing is ≥ 60 dB down (DSP-07).
- [ ] Scan 100 % traverses the region in real time and 0 % holds still. Retrigger restarts at note-on and Free-run does not (FUNC-04).
- [ ] Loading while notes sound gives no click and no dropout, with 0 audio-thread allocations (malloc_logger, thread-scoped) (FUNC-02, PERF-01/03). A 30 s+ file loads trimmed.
- [ ] MipTape `writeRun` ≡ MipRing, bitwise on the settled indices.
- [ ] Host idle: 100 publishes with no processBlock leave ≤ 1 pending tape per slot. `prepareToPlay` / `setState` stress is clean, and an SR change mid-note rebuilds with correct pitch throughout.
- [ ] Block-size null with a scripted MIDI sequence (32/441/512/4096/variable, large-block probe included) is bit-identical. A different-seed control differs.
- [ ] ADSR stages are audible and click-free. Velocity scales the level per `vel_to_amp` (DSP-02).
- [ ] **D12:** total live grains ≤ `grain_limit` at every grid tick. A dense voice cannot starve the others. Lowering the cap live converges, and the no-converge negative control differs.
- [ ] **D11:** `FORK-MANIFEST.md` lists every leaf at pin `82406fce` with sha256. The drift-check script reports 0 drift, and a negative control (one byte altered in a scratch copy of the parent file) fires.
- [ ] **D13:** the dev-loader editor loads each slot in the Standalone and in one DAW. pluginval 10 VST3 + AU still pass with it, and auval passes. HANDS-ON-2.1 is human-verified.

## Constraints and risks to flag

- **Parent working tree:** O-Granulator has uncommitted 1.1.x preset work (`FactoryPresets.h`, `index.html`, tests), but none in `Source/dsp/`. Fork from the **committed blob** at `82406fce` (`git show 82406fce:path`), never from the working tree. Brace the zsh variable: `${SHA}:path` (memory `pattern_zsh_var_colon_modifier_eats_git_revpath`).
- **Shared checkout:** commits are path-scoped to `plugins/O-GranulatorInstrument`. The 5 foreign `used_by` lines in `modules/registry.yaml` still belong to the O-Granulator owner.
- **Random content vs slot order:** every draw keys on the note serial, and the block-null and steal gates must prove it. A steal changes which slot a note lands in, but must not change its grains.
- **Generation reaper trap:** the reaper is audio-produced by construction, and the host-idle gate is the proof.
- **SR rebuild race:** `prepareToPlay` publishes from a host thread. `SlotInfo` must stay under its lock (memory `pattern_texture_forge_swap_needs_lock_if_prepare_publishes`).
- **Dev editor lifetime:** an async `FileChooser` callback after editor teardown is a use-after-free risk. pluginval's editor open/close test runs repeatedly.

## Open questions for research

1. Where do 2.1's `Quotas.h` demand inputs come from with one stream? Is it the parent's density × length overlap estimate per voice, unchanged? Confirm that the forked water-fill takes an arbitrary demand count with no parent assumption of exactly 4.
2. grain-engine `MipRing` internals: which `BandlimitedReader.h` symbols can MipTape reuse directly, and which must be duplicated? (G5 is a module-gap candidate; do not edit grain-engine in 2.1.)
3. Fixed-capacity MIDI event list (512 per block): confirm the sort order (note-off before note-on at the same offset), and how overflow is counted with no telemetry surface yet.
4. Dev-loader async `FileChooser` lifetime pattern: is there a suite precedent (O-simpleSampler / O-TextureForge pre-WebView)?
5. Kaiser sinc implementation: is there an existing suite resampler to reuse, or is it written fresh? Gate it on DC gain (exactly 1 ± 1e-6) and on passband / stopband.

## Out of scope for 2.1

- Streams 2–4, jitter, window morph, scale quantiser, spray / reverse / spread / gain randomisation (2.2)
- Per-grain FX, CPU budget gate (2.3)
- Feedback, record heads, Freeze, Reset, reverb (2.4)
- Modulation, Mono/Legato/glide, sustain, MPE, NE, tempo (2.5)
- Record-in, `<Slots>` state / relink, factory sources, telemetry (2.6)
- WebView editor (3.1)

## Next Phase

Ready for: research (`/plugin-research O-GranulatorInstrument 2-dsp`), scoped to Phase 2.1.
