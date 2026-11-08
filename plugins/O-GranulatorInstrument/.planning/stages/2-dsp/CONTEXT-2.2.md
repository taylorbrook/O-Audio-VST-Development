# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.2 — CONTEXT

**Date:** 2026-11-08
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:**
- `parameter-spec.md` (LOCKED, 261 params; §Streams, `<Engine>` keys `customScale.*` / `degreeMasks[4][32]`)
- `REQUIREMENTS.md` (DSP-01, FUNC-09, PERF-02, QUAL-01 and the FUNC-01 stream arm)
- `research/ARCHITECTURE.md` §9–§13, §15–§16, Algorithm §5–§6, A4, and the Stage-2 Gate Catalogue
- `ROADMAP.md` § Phase 2.2
- `stages/2-dsp/CONTEXT-2.1.md` (D11–D13 still binding), `SUMMARY-2.1.md`, `VERIFICATION-2.1.md`

## Goal (ROADMAP Phase 2.2)

Reach full grain-parameter parity with O-Granulator: 4 streams × N voices sharing one pool.

- 80 voice×stream schedulers with jitter, and a deterministic (offset, serial, stream) onset merge
- the forked `WindowMorphBank` (window + skew, μ₁/μ₂)
- `ScaleQuantiser`: interval mode, degree masks, custom scale
- spray, reverse, stereo spread, size rand and gain rand
- water-fill widened from ≤ 16 to ≤ 80 demands, with onset thinning and transition stealing (stream term live)
- coherence-blended output law with full-window `N_eff`
- stream enable / mute / solo ramps, and per-(k,s) level and pan

Still excluded: per-grain FX (2.3), feedback / Freeze / Reset / reverb (2.4), modulation, voice modes, MPE / NE and tempo sync (2.5), Record-in, `<Slots>` and factory sources (2.6).

## Decisions (locked this session)

### D14 — HANDS-ON-2.1 gates 2.2 execute, not 2.2 discuss / research / plan
- Phase 2.2 discuss, research and plan go ahead while HANDS-ON-2.1 (13 human rows) is pending.
- **`/plugin-execute O-GranulatorInstrument 2-dsp` for 2.2 must not start until HANDS-ON-2.1 is all ✅.**
  - The execute precondition reads `VERIFICATION-2.1.md` → Human Verification. If any row is unchecked, it stops and asks.
  - A ❌ row is fixed in 2.1 first (re-run 2.1 execute → verify). It is not folded silently into 2.2.
  - If a 2.1 fix lands after the 2.2 plan, the planner re-checks any 2.2 task that touches the fixed file.

### D15 — DSP-01 parity: derandomised sample null + statistical match for the random params
- **Null arm:** 1 voice, root key, keyTrack 100 %, scan 0, feedback 0, FX off, one stream enabled. Every random control is at its deterministic value:
  - `spray` 0, `jitter` 0, `size_rand` 0, `gain_rand` 0, `stereo_spread` 0, `pitch_rand` 0
  - `reverse_prob` at **0 % and at 100 %** (two rows; both are exact by spec)
  - The tolerance is **−90 dB** residual after gain alignment, against the O-Granulator engine at pin `82406fce` rendering the same material as a frozen buffer.
  - The rows sweep `size`, `density`, `window` and `window_skew` (including the Tukey, trapezoid and percussive regions and skew ±100 %), `pitch` offsets and `position`.
  - Negative control: a deliberate one-sample onset shift (or window index +1) on our side must fail the null.
- **Statistical arm** (the draw keys differ by design, since ours are `(serial << 24) | onsetIndex` and the parent's are its own stream counters):
  - Each random control (spray, jitter, size rand, gain rand, spread, pitch rand, reverse 0 < p < 1) is compared with the parent over a long render.
  - Compare the moments of the latched per-grain value distribution (mean, variance, min/max range) using the **latched grain fields from a test hook**, not the audio. Also compare the onset-interval histogram for jitter.
  - The tolerance is set by research from the sample count (a CI bound, not a fixed %). Each row has a control with a wrong distribution (for example jitter scaled ×0.5) that must fail.
- The draw design is **not** bent to match the parent's counters. Serial keying stays (it carries the 2.1 steal and slot-permutation invariance).

### D16 — `kVoiceGainDb`: objective target, then an ear check
- The plan sets `kVoiceGainDb` from a measured target, and HANDS-ON-2.2 has one row where the user confirms or trims it.
- **Target rule:** O-GranulatorInstrument and O-Granulator each render the same stationary-noise file as a frozen buffer, with default stream parameters and the same set of enabled streams.
  - O-GranulatorInstrument plays 1 held voice at the root, velocity 127 with `vel_to_amp` at its default (or with the velocity law made neutral).
  - `kVoiceGainDb` is chosen so the long-term RMS of the two renders matches within **±0.5 dB**.
  - Research confirms which streams the parent's default patch enables, and whether the parent output path has a fixed trim that also needs matching.
- Chords then sum without normalisation (ARCHITECTURE §16), and the §18 knee bounds the peak.
- **HANDS-ON-2.2 row:** play a single note and a 6-note chord at default settings, then confirm or trim by ear. If the user trims, the new value goes into `GiConstants.h` with a comment that records both the measured target and the by-ear offset.

### D17 — Scale state: engine API + `<Engine>` save / restore in 2.2
- Port the parent's scale state verbatim:
  - `customScale.name / .text / .period / .version`: one shared scale, used by any `sN_scale` = 31. `text` is the source of truth and is re-parsed on load.
  - `degreeMasks[4][32]`: per stream **and** per collection, as uint64 saved as **16-hex-digit strings**, never `getLargeIntValue`. An absent mask means all degrees on.
- Changes reach the audio thread the way the parent does it: a lock-free publish of the parsed scale and the masks, never a parse on the audio thread. 0 audio-thread allocations, gated with the A1-style malloc logger, including a custom-scale swap while notes sound.
- The keys live on the `<Engine>` child of the state root, alongside `seed` (int64), `stateVersion` and `modDestTableVersion`, using the parent key names unchanged.
- **Preset-apply caveat:** custom state is reset to defaults FIRST on apply (memory `pattern_preset_apply_needs_reset_to_defaults`). 2.2 has no preset manager yet, but the restore path must already treat an absent key as "set to the default", not "leave as is".
- Watch for the stale `<CustomState>` child that shadows a second reopen (memory `critical_preset_manager_stale_customstate_child`). The `setStateInformation` path replaces the `<Engine>` child; it does not append one.
- **Verification is harness-only:** quantiser output vs the parent leaf at the pin, mask application, custom-scale parse, a save → load round-trip of all 4 × 32 masks plus the custom scale, a high-bit mask (bit 63 set) that survives the round-trip, and an absent-key-resets-to-default row.
- **No dev-editor UI** for masks or custom scale. HANDS-ON-2.2 covers the 31 built-in scales through `sN_scale` in the generic editor. The native functions (`getScaleState`, `setDegreeMask`, `setCustomScale`, `parseScaleText`, `loadScala`, `saveScala`) come with the WebView in Stage 3.

## Defaults (no decision needed; binding for plan)

- **Stream → slot mapping:** stream N reads slot N. `s1_enable` is On and `s2`–`s4_enable` are Off (spec). Enabling s2–s4 makes slots 2–4 audible, which closes the FUNC-02 "slots 2–4 audible" arm.
- **Default-patch output changes in 2.2.** The defaults are jitter 20 %, spray 10 % and stereo spread 30 %. 2.1 ignored them and 2.2 honours them, so the goldens re-baseline (P11) is expected, not a regression.
  - Rebaseline once, in a dedicated task, and record the old → new hashes.
  - Every 2.1 gate row that depends on a deterministic render pins its random controls to 0 explicitly. The test does not rely on the default values.
- **Schedulers (§9):** run only for (sounding voice) × (enabled stream). The first onset is exactly at the note-on sample.
  - Draw keys: `Draw (seed, kSchedStream + s, (serial << 24) | onsetIndex)`.
  - Telescoping jitter as in the parent `StreamScheduler`.
  - **Free mode only.** Tempo grid lock and the power-of-two thinning round-up are 2.5.
- **Onset merge:** spawns happen in (offset, serial, stream) order inside a 32-cell. This is the tie-break the block null depends on.
- **Spawner (§10):**
  - Fixed CounterRng slots: the parent's `Slot` enum, append-only, on `kGrainStream + s` with counter `(serial << 24) | onsetIndex`. Draw before obtain.
  - Pitch: `Q(pitchRand·(2u − 1))` in interval mode counted from unison. The key term is never quantised, and the ±48 clamp comes after it.
  - Length: `size·(1 + sizeRand·(2u − 1)·0.5)`, clamped to [5, 2000] ms.
  - Spray within the region fit. Reverse is `u < reverseProb`, so 0 % and 100 % are exact.
  - Window step, skew, pan (stream pan + spread), and gain rand are latched as in the parent (GR-ARCH §8–§9).
  - The `s1_size_rand` seed control added in 2.1 stays valid.
- **Window morph (§13):** forked `WindowMorphBank` verbatim: 65 × 2048 tables, per-half μ₁/μ₂, latched at spawn. This replaces 2.1's Hann-only renderer path. At `window` 0 it must render bit-identically to Hann, which is a regression row against 2.1.
- **Quotas (D12 widened):**
  - The same `waterFillN` code runs over ≤ 80 demands `N_kS = density·len/fs`, each (sounding voice, enabled stream) weighted equally.
  - Steal priority: `4·stolenVoice + 2·overQuota(k,s) + (k,s == spawning) + age·invLen`. The stream term is now live.
  - The 2.1 Q-rows must pass unchanged with a single stream. The new rows add the stream axis.
  - A disabled or muted-but-running stream: **disabled** spawns nothing and leaves the water-fill. **Muted** keeps spawning (spec: "unmuting is instant"), so it keeps its quota.
- **Output law (§15):** `g_out = 1/sqrt(max(1, N_eff·μ₂(m,skew)/μ₂_Hann))` with the parent coherence blend, where `N_eff = min(N_kS, q_kS)` uses full windows. Gains are computed per (k,s) at the grid and ramped over one cell. 2.1's Hann-only law must equal this law at `window` 0 (regression row).
- **Mixer (§16):** per (k,s): `aₖ[n] · g_out · level · pan · bus`. Enable, mute and solo ramp over 10 ms per stream. Solo is additive (any solo means non-soloed streams go to 0) with no listener writes. Buses are `float[20][4][2][32]`.
- **Feedback tap:** still zero in 2.2. The §16 tap formula is not wired until 2.4.
- **CPU:** **informational only in 2.2.** Record one figure at 16 voices × 4 streams saturating 256 with no FX, as a ratio against bypass with the machine-speed witness, and print it as INFO. The PERF-02 budget verdict stays in 2.3 (with FX). If the no-FX figure is already > 28 % (the ARCHITECTURE grain-only projection), flag it in SUMMARY-2.2 so the 2.3 plan starts with the ARCHITECTURE levers.
- **Parameters 2.2 still does not read:** every FX row, feedback, freeze, reset, reverb, all modulation rows, `voice_mode`/glide, MPE/NE, the `*_sync` aliases, and Record-in. The "touch an unread param" null row (N2) carries over with this reduced set.
- **Thread rules, unchanged:** no locks, allocation or `shared_ptr` on the audio thread; no listener writes another parameter; `ScopedNoDenormals` per block.
- **Fork discipline:** `WindowMorphBank.h`, `ScaleQuantiser.h`, `OutputLaw.h` and `Quotas.h` are already forked at `82406fce` (D11). 2.2 includes them. Any local modification is recorded in `FORK-MANIFEST.md`, and `fork-drift-check.sh` stays at 0 drift on check 1.

## Test criteria (Phase 2.2 gate — ROADMAP + D15–D17)

- [ ] **DSP-01 null (D15):** derandomised rows null at ≤ −90 dB vs the parent at `82406fce`, across size, density, window, skew, pitch, position and reverse 0/100. The one-sample-shift control fails.
- [ ] **DSP-01 statistics (D15):** spray, jitter, size rand, gain rand, spread, pitch rand and reverse 0 < p < 1 latched distributions match the parent within the CI bound. The wrong-distribution controls fail.
- [ ] **FUNC-09:** quantiser output for all 31 built-in collections + the custom scale is identical to the parent leaf. Degree masks apply per stream and per collection.
- [ ] **D17 state:** a save → load round-trip restores 4 × 32 masks (including bit 63) and the custom scale exactly. An absent key resets to the default. A second reopen is not shadowed by a stale child. A scale or mask swap while notes sound gives 0 audio-thread allocations.
- [ ] Level flat within ±1.5 dB across density 1 → 200 and across size with thinning engaged (stationary noise, 4-render average, old-vs-old row).
- [ ] **PERF-02 quotas:** total live grains ≤ `grain_limit` at every grid tick with 16 voices × 4 streams. A dense (voice, stream) cannot starve the others on either axis. Lowering the cap live converges, and the no-converge control differs. The 2.1 Q0–Q3 rows still pass unchanged.
- [ ] **QUAL-01:** steal click gate on a tonal source with verified overflow, voice and grain steals, vs a no-steal control. Enable / mute / solo toggles are click-gated against same-motion controls.
- [ ] Window 0 ≡ 2.1 Hann render (bitwise), and the morph-bank law at window 0 ≡ the 2.1 law.
- [ ] Block-size null (32/441/512/4096/var + large-block probe) is bit-identical with **all 2.2 randomisation on** and 4 streams × chord. The different-seed control differs.
- [ ] FUNC-01 stream arm: a 4-note chord across 4 enabled streams (4 slots) gives independently pitched clouds per stream. Release continues grains in every stream (DSP-02 re-check).
- [ ] **D16:** `kVoiceGainDb` is set to the measured target (±0.5 dB vs the parent default render). HANDS-ON-2.2 confirms or trims it.
- [ ] Goldens re-baselined once (P11), with the old → new hashes recorded. The Stage 1 rows and param-dump stay byte-identical.
- [ ] Fork drift check 0, auval PASS, pluginval 10 VST3 + AU SUCCESS (2 ch in).
- [ ] CPU INFO line recorded (no verdict).
- [ ] HANDS-ON-2.2 is human-verified. It covers 4 streams on 4 slots, mute/solo/enable, jitter/spray/spread/reverse/gain rand by ear, window morph + skew, the built-in scales with pitch rand, the voice-gain row, and Grain Limit with 4 streams × chord.

## Constraints and risks to flag

- **Parity reference:** the parent's `Spawner.h`, `StreamScheduler.h` and `Engine.cpp` are **not** forked leaves. The null needs the parent engine at the pin to render inside a test target.
  - Build it from `git show 82406fce:<path>` blobs into a scratch or test-only tree. Brace the zsh variable as `${SHA}:path`.
  - Never build it from the parent working tree, which has uncommitted preset work.
  - It must not be linked into the plugin target, and the console-exclusion check covers it.
- **Frozen-material equivalence:** the parent reads a live MipRing behind a write head, and we read a positional MipTape. For the null, the parent buffer is filled with the material and frozen. `position` maps to "distance behind head" vs "start in region". The mapping and any 1-sample offset must be derived in research, not tuned until it nulls.
- **Determinism at 80 schedulers:** the merge tie-break and the per-(k,s) draw keys are the block-null risk. Every new random draw goes through the serial-keyed counters, never through slot or stream iteration order.
- **Re-baseline masking:** the goldens shift for a known reason (the defaults are now honoured). The re-baseline task must prove the shift comes **only** from the honoured defaults: with them pinned to 0, the old goldens must still match.
- **Shared checkout:** commits are path-scoped to `plugins/O-GranulatorInstrument`. Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`, O-GrainScatter) are not touched.

## Open questions for research

1. **Parent-at-pin test build:** what is the minimal set of parent files at `82406fce` needed to render one frozen stream headlessly? Can the parent harness's own frozen-buffer path be reused, and how does it stay out of the plugin target?
2. **Position / frozen mapping:** what exact transform maps our `sN_position` + region onto the parent's distance behind the head on a frozen ring, including any write-head offset or mip group-delay sample?
3. **Statistical arm bounds:** how many grains per row give a CI tight enough to catch a ×0.5 distribution error with no flakes? What harness wall-time does that cost?
4. **Parent scale-state publish:** how does the parent hand parsed scales and masks to the audio thread (atomic pointer swap, triple buffer, versioned copy)? Can it port as-is under the `ogi` namespace without touching the forked leaves?
5. **D16 target:** which streams does the parent's default patch enable, and is there a fixed output trim in the parent path? Does velocity 127 at the default `vel_to_amp` give unity, or must the velocity law be neutralised for the measurement?
6. **Quotas with muted streams:** confirm the parent keeps a muted stream's demand in the water-fill. If it does not, decide which behaviour matches "unmuting is instant".
7. **Window-0 bitwise row:** does the forked `WindowMorphBank` at m = 0, skew 0 reproduce the 2.1 Hann table bit-for-bit? If it does not (a different table build), the row becomes ≤ −120 dB, and the reason is documented.

## Out of scope for 2.2

- Per-grain FX and the CPU budget verdict (2.3)
- Feedback, record heads, Freeze, Reset, reverb, the feedback tap (2.4)
- Modulation, Mono/Legato/glide, sustain, MPE, NE, tempo sync and `*_sync` aliases (2.5)
- Record-in, `<Slots>` state / relink, factory sources, telemetry (2.6)
- Scale / mask UI and native functions, WebView editor (Stage 3)

## Next Phase

Ready for: research (`/plugin-research O-GranulatorInstrument 2-dsp`), scoped to Phase 2.2. Execute is gated on HANDS-ON-2.1 (D14).
