---
title: "O-GranulatorInstrument Stage 2 (DSP) — Phase 2.2 Execution Plan"
created: 2026-11-08
juce_version: "8.0.15"
phase: "2.2"
summary: "Six waves: (W0) pre-flight with the D14 HANDS-ON-2.1 gate, then render hashes captured on the UNMODIFIED 2.1 engine; (W1) out-of-tree parity reference (parent at 82406fce from git archive, n1 precedent, ParityRows.h shared by both drivers); (W2) engine written header-first: ControlGrid ×4 streams + manifest, jitter-aware thinning + (voice, stream) acquire, scale state (engine try-lock publish, processor API, sibling codec), 80 schedulers + (offset, serial, stream) merge + widened spawner, morph-bank renderer with the P14 Hann fast path, coherence-(b) law, 4-stream mixer; (W3) harness gates written, ONE console build, default rows + re-baseline proof, parity null + statistics + scale parity, D16 measured and set, renders re-baselined once; (W4) install, auval, pluginval 10, symbol exclusion, CPU INFO, HANDS-ON-2.2; (W5) SUMMARY, STATUS, path-scoped commit. No tag; version 0.1.0; CMakeLists.txt not edited."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 2 / Phase 2.2 Execution Plan (Four streams, randomisation, windows, scale, quotas + thinning)

## Goal

Full grain-parameter parity with O-Granulator across **4 streams × N voices sharing one pool**. Each counted voice runs one Free-mode scheduler per enabled stream (≤ 80) with telescoping jitter, and onsets merge in (offset, serial, stream) order. Stream N reads slot N. The spawner latches spray, reverse, stereo spread, size / gain / pitch rand (continuous, or `ActiveSet::pick` under a scale with degree masks and the shared custom scale) and the forked `WindowMorphBank` window + skew. The water-fill runs over ≤ 80 equal-weight (voice, stream) demands with jitter-aware integer thinning and (k,s) transition stealing. Per (k,s), the coherence-blended output law feeds a 4-stream mixer with 10 ms enable / mute / solo ramps. Scale state is published lock-free (relaxed atomic masks + the parent's non-blocking try-lock table copy) and round-trips through sibling `<CustomScale>` / `<DegreeMasks>` children. DSP-01 is proven against the parent engine at `82406fce`, built out of tree. `kVoiceGainDb` is set from a measured target.

**Out of scope** (CONTEXT-2.2 § Out of scope): per-grain FX and the CPU verdict (2.3). Feedback, record heads, Freeze, Reset, reverb, the feedback tap (still zero) (2.4). Modulation, Mono/Legato/glide, sustain, MPE/NE, tempo sync, `*_sync` aliases (2.5). Record-in, `<Slots>`, factory sources, telemetry (2.6). Scale / mask UI, native functions, WebView (Stage 3).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT-2.2.md` | D14–D17, the Defaults block, the gate list. **Binding.** D11–D13 (CONTEXT-2.1) still bind. |
| `stages/2-dsp/RESEARCH-2.2.md` | §1–§14 (cited **R§x**), Pitfalls **P1–P25**. Authoritative over ARCHITECTURE where they differ. |
| `stages/2-dsp/{SUMMARY,VERIFICATION,HANDS-ON}-2.1.md`, `PLAN-2.1.md` | What exists: harness 88/0, hook names, row IDs, 2.1 P1 thinning, the D14 gate. |
| `research/ARCHITECTURE.md` | §9–§16, §18, Algorithm §5–§6, the Stage-2 Gate Catalogue. |
| `parameter-spec.md` (LOCKED) + `.planning/params.tsv` | Stream rows (38 per stream; 2.2 reads 26) and ranges. **Not edited.** |
| Parent at the pin: `git show "${PIN}:plugins/O-Granulator/<path>"`, `PIN=82406fce` (brace every variable) | `Source/dsp/Spawner.h:169-311` (latch order, frozen D), `StreamScheduler.h:106-110, 196-232` (u₀, jitter, epoch), `Engine.cpp:269-326` (scale publish), `:358-396` (demand, audibility), `:1477-1512` (P14), `Engine.h:246, 323-366` (`freezeNoMinSpray`, `SpawnRecord`, log 4096), `Source/PluginProcessor.h:115-134, 238-277` + `.cpp:150-165, 373-392, 422-429, 448-612` (scale API + codec), `tests/n1/run.sh` (out-of-tree recipe), `tests/render-harness/engine_gates.h:110-131` (exact-raw setP), `:220` (event-split blocks). **Never read the parent working tree.** |
| This plugin | `Source/dsp/{Engine.h,Engine.cpp,GiSpawner.h,GiRenderer.h,VoiceQuotas.h,VoiceAllocator.h,GiConstants.h}`, `Source/dsp/forked/{ControlGrid,ScaleQuantiser,OutputLaw,WindowMorphBank,EngineConstants}.h`, `FORK-MANIFEST.md`, `Source/PluginProcessor.{h,cpp}`, `tests/render-harness/{main.cpp,engine_gates_21.h}`, `tests/fork-drift-check.sh`. |

---

## Plan-phase decisions (resolve RESEARCH open items)

- **PD1 [user, 2026-11-08] D16 reference = parent at mix 100 %.** An instrument has no dry path, and D16 says "default **stream** parameters". The expected target is ≈ 0 dB, which is +6 dB vs the 2.1 placeholder (`GiConstants.h:54`, −6.0). Implementation: `kVoiceGainDb = kVoiceGainMeasuredDb + kVoiceGainEarTrimDb`. T12 sets the first from GV0 (rounded to 0.01 dB) with a comment recording the target and the reference. HANDS-ON-2.2 sets the second (default 0.0) with a comment recording both numbers. **If |target| > 1.0 dB, STOP:** both chains are unity at N = 1, c = 0 (R§5), so a larger number is a chain mismatch to find, not a gain to absorb.
- **PD2 [user, 2026-11-08] Coherence = R§11.1 option (b).** `Coherence::compute` verbatim (`forked/OutputLaw.h:231-248`), but its pitch argument is the relative velocity `12·|log2(r_kS / v_s)|`, per (k,s): `r_kS` = the voice's latched forward rate on stream s, `v_s = scanRate_s·tapeRate_s/fs`, and `v_s ≤ 0` → `+∞` → `c_pitch = 0` (the reverse share stays in the parent's `(1 − reverseProb)`). Rows CO1/CO2. Hooks: `coherenceOff` (c := 0, 2.1 equivalence), `coherenceVerbatim` (option (a), CO1 control).
- **PD3 [user, 2026-11-08] D17 publish = the parent's non-blocking SpinLock try-lock, ported verbatim and accepted as compliant with "no locks".** The audio side calls `ScopedTryLockType` only at a grid tick and only when `customVersion` moved; a miss retries next tick; the blocking copy runs only in `prepare`. Gated by A2 (custom swap while sounding, 0 allocations) and ST6 (a held lock never blocks a block).
- **PD4 [research flag P16, adopted] Scale state = sibling children `<CustomScale>` / `<DegreeMasks>`, not keys on `<Engine>`.** These are the parent names unchanged, and the Stage 1 strip list already names both (`Source/PluginProcessor.cpp:224-233`). This refines CONTEXT D17's "keys live on the `<Engine>` child"; `parameter-spec.md` (LOCKED) is not edited and SUMMARY records the refinement. Masks `%016x`, read back only as 16 hex digits via `getHexValue64` (P17); restore resets non-parameter state first (P18) and strips before `replaceState` (P19).
- **PD5 [research, adopted] Parity = out-of-tree parent build** (R§1, `82406fce:…/tests/n1/run.sh` precedent): `tests/parity/{run.sh,parent_render.cpp,ParityRows.h}` + the harness `--parity <dir>` mode. `freezeNoMinSpray` on every null row (P1); closed-form `Δ ≡ 0 (mod 2^ℓ)` per row plus a Δ + 1 control (P2); material peak ≤ −40 dBFS (P3); null N ≪ 32 (P4); nothing from the parent enters our CMake graph (P21).
- **PD6 [research + planner choice] Statistical arm: n ≥ 4000 per side per control** (R§3): two-sample KS, one-sample KS vs the analytic law, moments and range; jitter on the i.i.d. displacement plus an even-k χ² (P12). Controls: ×0.5 on our latched samples harness-side (every row) **plus** one engine-side hook (`jitterScale = 0.5` in the scheduler callback), so at least one control runs through the audio path.
- **PD7 [research flag §10.2, adopted] Integer bound at 80 pairs.** `T = max(fs/density, (L_max + 1)/(m − J))` for m ≥ 2; m ≤ 1 keeps `max(fs/density, L_max + 1)` and accepts R3 kills for jittered m = 1 pairs. At J = 0, `m − 0.0 == m` exactly, so every 2.1 period is bitwise unchanged. Hard row: live ≤ cap at **every** tick. "0 steady-state R3 kills" applies only where pairs ≤ cap and (m ≥ 2 or J = 0); at 64 pairs on cap 32, `q < 1` is forced to m = 1 (P10) and the kill rate is INFO.
- **PD8 [research P5/§7, adopted] Window.** The forked bank is used verbatim, with the parent P14 closed-form branch: `windowStep == 0 && exactlyEqual (tiltT, 0.5f)` → the 2.1 Hann statements unchanged (`GiRenderer.h:156-175`). Bank path otherwise. The `forceBankPath` hook is the control.
- **PD9 Stream semantics** (CONTEXT Defaults + R§6): stream N reads slot N. Disabled = no spawn, demand 0, ringing grains fade under the **10 ms enable ramp** (binding, though the parent lets them ring out). Muted = keeps demand and spawning (parent-verified). `anySolo` is over enabled streams.
- **PD10 [research P9/§12, adopted + extended] Re-baseline proof.** W0 hashes 7 jobs × {pinned, default} on the unmodified 2.1 engine; the job stimulus pins every 2.2 control from day one, and `prepareJob()` is the only later diff. Pinned jobs in 2.2 also set `voiceGainDbOverride = −6.0` (planner addition): it isolates the D16 gain change from the honoured-defaults change, which R§12 did not cover. Carried 2.1 rows: `base()` pins + `coherenceOff` in the eg21 `Session` (R§12.2); thresholds unchanged.
- **PD11 Level-flat definition.** Each (density, size) cell with `N_eff ≥ 1` must sit within ±1.5 dB of the d 10 / size 100 cell. Cells with `N_eff < 1` are sparse by design (the law floors at `max(1, ·)`) and are printed as INFO. Density 1 is still covered, by size 2000 (N = 2).
- **PD12 Mixer arithmetic and ramps.** At one enabled stream the per-sample arithmetic must equal 2.1's `sL·(level·aud·voiceGain)·panL`, sign of zero included. The stream sum starts from the first contributing stream's term; a stream whose audibility ramp is at 0 is skipped, never added as `+0.0f` (P6). Ramp lengths stay at 2.1's (level/pan `rampCells20`, audibility `rampCells10` = `kMixerRampMs`): R§11.3 says level/pan are 10 ms, but the tree uses 20 ms (`Engine.cpp:270-276`) and changing it was not asked for, so MX1 builds its same-motion reference harness-side.
- **PD13 Draw keys.** `idx = epoch_kS + onsetIndex`; scheduler `Draw (seed, kSchedStream + s, (serial << 24) | (idx & 0xFFFFFF))`, grain `Draw (seed, kGrainStream + s, …)`. `epoch` grows only on an enable edge while the voice sounds (P14), so s = 0 without a re-enable keeps the exact 2.1 keys (`Engine.cpp:825-826`). Every draw goes through the `Slot` enum (`forked/EngineConstants.h:246-266`), never slot or iteration order.
- **PD14 D14.** T1 checks HANDS-ON-2.1 / VERIFICATION-2.1 Human Verification before any 2.2 file is touched; any unchecked row → STOP and ask. If a 2.1 fix lands after this plan (a commit after `06d7be7b` touching this plugin's `Source/`, `tests/` or `CMakeLists.txt`), the tasks touching the fixed files (map in T1) are re-checked against the new file first.
- **PD15 CPU = INFO only** (R§14). SUMMARY flags it if the no-FX median is > 28 %.
- **PD16 Housekeeping.** Version stays 0.1.0 (I/O unchanged, so the Logic AU I/O cache is unaffected); no tag. `CMakeLists.txt` is **not edited**: every new source is a header, and the parity build lives out of tree. If a new TU proves necessary, it lands in T10 before the build and is recorded as a deviation.

---

## Tasks

**Roles.**
- **[agent]** = dsp-agent writes files (no Bash).
- **[run]** = build/gate runner: commands, builds, validators, commit.
- **BG** = `run_in_background`, log to `$S/<name>.log`. `$S` = the executing session's scratchpad.
- `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`.
- Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

### Wave 0 — pre-flight + 2.1 render hashes

**T1. Pre-flight + D14 gate** [run]
- Files: none. Output to `$S/preflight.txt`.
- Depends on: none.
- Done when (any mismatch → STOP and ask):
  - **D14:** `stages/2-dsp/HANDS-ON-2.1.md` has ✅ in every row, both columns (row 10: Standalone only), Result = PASS, and `VERIFICATION-2.1.md` § Human Verification has every box checked. A ❌ is fixed in 2.1 first, never folded into 2.2.
  - **Late 2.1 fixes:** `git log --oneline 06d7be7b..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` (empty at plan time). If not empty, STOP for a re-check of the mapped tasks: `Engine.{h,cpp}` → T7, T8 · `GiSpawner.h`, `VoiceQuotas.h` → T5 · `GiRenderer.h` → T8 · `VoiceAllocator.h` → T7 · `GiConstants.h`, `forked/ControlGrid.h`, `FORK-MANIFEST.md` → T4, T12 · `PluginProcessor.*` → T6 · `tests/render-harness/*` → T2, T9.
  - **Git state:** `git branch --show-current` = `main`; record `git status --short`. Foreign paths are **never staged**: `modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, GrainScatter `nielsen-dafx26.txt`, any other session's files.
  - **Contracts and pin:** `parameter-spec.md` sha256 (file + body) = STATUS `contract_checksums`; `git cat-file -e 82406fce`; `git diff --quiet 82406fce HEAD -- plugins/O-Granulator/Source/dsp modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump` (empty at plan time).
  - **Baseline:** harness `88 PASS / 0 FAIL` → `$S/h-21.log`; `param-dump` → `$S/pd-21.txt`, byte-identical to `.planning/params.tsv`; `tests/fork-drift-check.sh` 0 drift.

**T2. Render hashes on the unmodified 2.1 engine (P9, PD10)** [agent writes, run executes]
- Files: `tests/render-harness/render_hashes.h` (new), `tests/render-harness/main.cpp` (include + `--dump-render-hashes <file>`), `tests/golden/renders-2.1.sha256` (new).
- Depends on: T1. **No `Source/` file is edited before this task is done.**
- Seven jobs reuse eg21 `Session` / `with()`: **N1** (scripted MIDI, `s1_size_rand` 50 %, block 512) · **V3** (the V2/V3 steal script's grain log; hash only the 2.1 `GrainLogEntry` fields `serial onsetIndex onsetSample frac len startPos rate level`, never `voice`/`poolSlot` or 2.2 fields) · **Q4** (16 voices, noise, cap 64) · **E1** (env set 2, velocity 0.25) · **H1** (4 notes, slot 1 reloaded twice via `loadSlotSync`) · **P2** (2000 ms grain at root + 12) · **CH** (4-note sine chord at defaults).
- Each job is written twice. `<job>-pinned` = stimulus + `pins22()` (s1 spray, jitter, stereo_spread, gain_rand, pitch_rand, reverse_prob, window, window_skew, scale, solo all 0; s2–s4 enable 0). `<job>-default` = stimulus only, so the spec defaults spray 10 / jitter 20 / spread 30 apply.
- `prepareJob (Session&, bool pinned)` is the single extension point and is **empty at T2**; save a copy as `$S/render_hashes.T2.h`.
- Hash: `juce::SHA256` (juce_cryptography is linked into consoles, `scripts/param-dump/ParamDump.cmake:255`) over the float32 L then R bytes (V3: the packed log fields). Lines `<job> <hex>`, sorted.
- Run (BG): `ninja -C build O-GranulatorInstrument-render-harness`, dump twice to `$S`, copy to `tests/golden/renders-2.1.sha256`.
- Done when: 14 lines; the two dumps are byte-identical; **every `-pinned` hash equals its `-default` hash** (2.1 ignores every 2.2 control); harness still 88/0; `git diff --stat -- Source` is empty. **Control:** the N1 job with seed + 1 → a different hash.

### Wave 1 — parity reference (out of tree; runs while W2 is written)

**T3. `tests/parity/`** [agent writes, run executes]
- Files: `tests/parity/run.sh` (`chmod +x`), `tests/parity/parent_render.cpp`, `tests/parity/ParityRows.h` (all new).
- Depends on: T1.
- **`run.sh`** (n1 shape, `82406fce:…/tests/n1/run.sh:28-68`): `set -euo pipefail`; `WORK="${1:-${TMPDIR:-/tmp}/ogi-parity}"` (the runner passes `$S/parity`; never inside the repo); exit 2 if the T1 module diff is non-empty; `rm -rf "$WORK/pin"` then `git archive 82406fce plugins/O-Granulator modules | tar -x -C "$WORK/pin"`; `$WORK/proj/CMakeLists.txt` with `ouaricon_add_processor_console(OuariconGranulator "$WORK/pin/plugins/O-Granulator/Source" "$HERE/parent_render.cpp" parity)`, `cxx_std_20`, `OGRAN_TEST_HOOKS=1`, `modules` symlinked to the archive; configure once, ninja incrementally; run `parent_render "$WORK/out"`, and any self-check failure → non-zero exit.
- **`ParityRows.h`**: plain C++17, no juce / `ogi` / `ogran` types, included by both drivers (one source of truth). It holds the row structs (id, stream, `{param id, raw value}` list, length); the material generators (hash noise ≤ −40 dBFS peak for PN/PS; −30 dBFS RMS, peak ≤ 0.1 for GV); the R§2.3 solver (`T_f ≡ 0 (mod 32)`, `W0`, `A = W0 − margin(ℓ) − pos·S`, our `p0`, F chosen so `Δ = A − p0 ≥ 0`, `Δ ≡ 0 (mod 2^ℓ)`, `Δ + F ≤ W0 − 223`, both spans inside the material); and the SC grid.
- **`parent_render.cpp`**, per row: build the parent processor; set params by exact raw value (setP idiom, counting landed / total); `getEngineForTest().hooks.freezeNoMinSpray = true` on every PN row; feed `x_par[k] = M[k − Δ]`; freeze at `T_f`; enable the row's stream at a tick ≥ `freezeStopTick`; mix 100 %, output 0 dB, grain_limit choice 3 (128); drain `diag.log` after **every** block (P15). Writes:
  - `<row>.f32` (L/R interleaved) and `<row>.grains.tsv` (SpawnRecord `onsetSample writeHead stream dir level delay anchor frac rate len gainL gainR windowStep tiltT pitchCents`)
  - `manifest.tsv` (`row W0 A Δ F T_f T_e landed/total`) and `GV<seed>.f32` ×4
  - `activesets.tsv` + `customtables.tsv`, from the parent leaf + processor plumbing (`setDegreeMask`/`getDegreeMask` canonicalisation, `setCustomScale`, `GranulatorParamIds::scaleChoiceToId`, `ActiveSet::rebuildIfNeeded` + `window`).
- **Rows** (IDs used by T11):

| Group | Rows (s1 unless stated; base = size 100 ms, density 10, window 0, skew 0, pitch 0, position 0.25, reverse 0) |
|---|---|
| size / density | PN01 base · PN02 size 20 · PN03 size 500 · PN04 size 2000 @ d 1 · PN05 d 50 · PN06 d 200 @ size 20 |
| window / skew (steps of 64) | PN07–PN12 steps 11/21/32/43/53/64 (Tukey, B-lerp, trapezoid, percussive) · PN13 step 32 skew +100 · PN14 step 32 skew −100 · PN15 step 0 skew +100 (not the fast path) · PN16 step 64 skew −100 |
| pitch / position | PN17 +7 (ℓ0) · PN18 −12 · PN19 +12 (ℓ1) · PN20 +19 (ℓ1) · PN21 +24 (ℓ2) · PN22 −24 · PN23 position 0.75 |
| reverse / stream | PN24 reverse 100 % · PN25 reverse 100 % @ +12 (ℓ1) · PN26 = PN01 on **s3** (slot 3 path) |
| controls (our side only) | PNc1 PN01 with our note-on at `T_on + 1` · PNc2 PN09 with our window step + 1 · PNc3 PN19 with Δ + 1 |
| statistics | PS1 combined · PS2 gain_rand · PS3 jitter · PS4 scale on (settings in the T9 table) |
| scale / gain | SC grid (T9 SC1) · GV seeds 1–4 (T9 GV0) |

- Done when: rc 0; every PN grain has `rec.anchor == A` and `rec.writeHead == W0` (a self-check of the derivation, never a tuning step); every param landed (n = N); every audio row's reference RMS ≥ −70 dBFS; the parity project's `ninja -t compdb` contains `O-Granulator/Source` (the X1 positive control).

### Wave 2 — engine (agent writes; nothing builds until T10)

Audio-path rules are unchanged: no locks (except PD3's try-lock), allocation, `shared_ptr` or vector growth; fixed arrays only; `ScopedNoDenormals` per block; no listener writes a parameter.

**T4. ControlGrid ×4, constants, manifest** [agent; runner re-hashes]
- Files: `Source/dsp/forked/ControlGrid.h`, `Source/dsp/GiConstants.h`, `Source/dsp/forked/FORK-MANIFEST.md`.
- Depends on: T2.
- **ControlGrid.h** (ADAPTED region `:170-374` only; `:38-165` stays byte-identical): `kGiGridStreams` 1 → 4. `GiParamPointers::Stream` / `GiGridParams::Stream` gain `solo`, `spray`, `jitter`, `pitchRand` (st 0..24), `scale` (int 0..31), `reverseProb`, `window` (0..1 raw), `windowSkew` (−100..100 → /100), `stereoSpread`, `gainRand`, read with the existing `read`/`readPct`/`readInt`/`readBool`, clamped to the `params.tsv` ranges, bound for n = 1..4. The 12 unread stream rows (10 FX, `size_sync`, `density_sync`) stay unbound.
- **GiConstants.h:** `kGiStreams` 1 → 4; `kPairs = kVoiceSlots * kGiStreams` (80); PD1 split `kVoiceGainMeasuredDb = −6.0` (placeholder until T12) + `kVoiceGainEarTrimDb = 0.0` → `kVoiceGainDb`.
- **FORK-MANIFEST.md:** ControlGrid column 7 += "2.2: 4 streams, 10 stream rows" (the runner refreshes column 5 with `shasum -a 256`). ScaleQuantiser `ADAPTED (pending 2.2)` → **`VERBATIM`**, note "GranulatorParamIds named in comments only (:51, :62); the processor static_asserts `GiParamIds::scaleChoiceToId ≡ ogi::scaleChoiceToId`" (R§4.1, P22). WindowMorphBank first-used = 2.2.
- Done when: no VERBATIM leaf is edited; drift check 2 holds for all 13 leaves (run in T10).

**T5. Thinning with jitter + (k,s) acquire** [agent]
- Files: `Source/dsp/VoiceQuotas.h`, `Source/dsp/GiSpawner.h`.
- Depends on: T4.
- **VoiceQuotas.h:** `thinnedPeriod (fs, density, lMax, m, jitter = 0.0)` per PD7. `waterFillN` is untouched; the engine calls `waterFillN<kPairs>` (zeros sort first and subtract 0.0 exactly, so Q0–Q4 stay bitwise, R§10.1).
- **GiSpawner.h:** `acquire (pool, cap, const int (&m)[kVoiceSlots][kGiStreams], stolen[20], spawnK, spawnS, policy)`. Pass 1 fills `live[20][4]` keyed `(vg.voice, vg.g.stream)`; `pk[k][s] = 4·stolen(k) + 2·(live_kS > m_kS) + ((k,s) == spawning)` replaces `:98, :117-122`. Unchanged: lexicographic argmax, killing bonus 8, R3, the 2.1 Q3 killing-steal payment (`:167-178`), D25. Add `countAlivePerPair`; keep `countAlivePerVoice`.

**T6. Scale state — engine side + processor + codec (D17, PD3, PD4)** [agent]
- Files: `Source/dsp/GiScale.h` (new, header-only), `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`.
- Depends on: T4.
- **GiScale.h** (`ogi`): ports the parent `copyCustomTable` / `updateScaleState` / `tableForChoice` (`Engine.cpp:269-326`) minus the 2.5b `absSets`/follow block, and re-declares `PitchSel {const ActiveSet* set; int lo, hi;}` (parent `Spawner.h`, not a leaf). State: `ScaleSource src`, the engine's `PitchCollections::Table custom`, `copiedVersion`, `ActiveSet sets[4]`, `PitchSel sel[4]`. `copyCustomTable (false)` at every tick (non-blocking); `copyCustomTable (true)` only from `Engine::prepare`. `update (gp)` per stream: `masks[scaleMaskIndex (s, choice)]` (relaxed) → `rebuildIfNeeded (choice, mask, pitchRand·100, version, table)` → `window (pitch_s, span)`. Hooks: `customCopies`, `customTryFails`, `activeSetRebuilds`.
- **Processor** (port the parent lines in Inputs; names only changed): `std::array<std::atomic<uint64_t>, 4·32> degreeMasks` (all-on), `juce::SpinLock customLock`, `PitchCollections::Table customTable`, `customText`/`customName`, `std::atomic<uint32_t> customVersion`. `setCustomScale (name, text, badToken)` parses outside the lock, swaps under it, resets that collection's 4 masks to all-on, then `fetch_add (acq_rel)`. `getDegreeMask`/`setDegreeMask` (`canonicalMask` sets every bit ≥ degreeCount), `maskIndex (stream 1-based, coll)`. static_asserts `maskIndex ≡ ogi::scaleMaskIndex` and `GiParamIds::scaleChoiceToId ≡ ogi::scaleChoiceToId` over all 32 choices. `engine->bindScale (ScaleSource {…})` in the ctor after `engine->bind`.
- **Codec (PD4).** `getStateInformation` writes, in order: `<Engine seed stateVersion modDestTableVersion/>`, `<CustomScale name text period version/>` (version `& 0x7fffffff`), `<DegreeMasks>` with one `<Mask stream collection bits/>` per **non-all-on** mask (`juce::String::toHexString ((juce::int64) m).paddedLeft ('0', 16)`), `<Slots/>`. `setStateInformation` reads Engine/CustomScale/DegreeMasks **before** `removeCustomChildren`, then `replaceState`, then `applyNonParameterState (custom, masks, seedVar)`, which calls `resetNonParameterState()` **first** (12-EDO custom, all masks on, version bump; an absent seed keeps the live seed). A mask is applied only if `bits.length() == 16 && bits.containsOnly ("0123456789abcdefABCDEF")` → `getHexValue64()`, never `getLargeIntValue`; `isVoid()` guards only. Update the codec comment (`:205-218`).
- **Hooks (OGI_TEST_HOOKS):** `testNoStateStrip` (ST4 control), `testNoStateReset` (ST3 control), custom name/text/period/version getters, `getCustomLockForTest()` (ST6). No dev-editor UI, no native functions (D17).

**T7. 80 schedulers, merge, quotas wiring, spawner widening** [agent]
- Files: `Source/dsp/VoiceAllocator.h` (`Voice`), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T5, T6.
- **State:** `Voice` gets `sched[4]`, `scan[4]`, `epoch[4]`. Engine gets `freeRun[4]`, `region[4]` (slot s = stream s), `quotaM`/`periodT`/`prevDemand` `[20][4]`, `QuotasN<kPairs>`, `onsetBuf[20][4][kOnsetCap]`, member `merged[kVoiceSlots·kGiStreams·kOnsetCap]` (2640), `PendingOnset.stream`, `GiScaleState scale`, `bindScale`.
- **Schedulers** run for counted voice × enabled stream × loaded slot. Note-on resets every enabled stream at the event sample; an enable edge while a voice sounds does `epoch += getOnsetCount(); reset()` at the tick (P14). Callback `T·(1 + J·(u(k+1) − u(k)))`, `u(0) = ½`, PD13 keys; J = 0 returns T with no draw (`StreamScheduler.h:198-210`); hook `jitterScale` (default 1.0) multiplies J. A quota decrease calls `rescheduleNext` with the jittered gap `T_new·(1 + J(u(n) − u(n−1)))`, n = `getOnsetCount()` (R§8).
- **Merge:** fill pairs in (serial ascending, s ascending), sorting the ≤ 20 counted voices by serial once per segment, then a **stable** insertion sort on offset → (offset, serial, stream). The voice slot is never in the key. Voice ends stay before spawns at a shared offset (`Engine.cpp:783-793`).
- **Quotas:** demand index `k·4 + s`, `streamDemand (density_s, size_s, fs)`; 0 for a non-counted voice, a disabled stream or a missing tape; **muted streams keep demand**. Change detection covers all 4 streams + jitter; each pair gets `thinnedPeriod (…, m_kS, J_s)`.
- **`spawn (o, t)` on stream s** follows the parent latch order (`Spawner.h:169-268`, R§9 table), draws before acquire on PD13 keys:
  - size: the 2.1 code. Pitch: `st = κ·key + pitch_s`, then scale Off `ρ(2u−1)`, or scale on `sel[s].set->pick (draw, lo, hi)/100` when ρ > 0 && hi > lo (P13); clamp ±48; latch `g.pitchCents`.
  - reverse `rp ≥ 1 → −1; rp > 0 → u < rp`. Spray `c + s·W·(u − ½)`; region fit forward `[lo, hi − reach]`, reverse `[lo + reach, hi)` (P20).
  - spread `panGains (σ(2u−1))`; gain `10^(−γ·u·24/20)` (float) × both; window `stepFor (window_s)`, `makeTilt (0.5 + 0.45·skew_s)`; `g.stream = vg.slot = s`, `g.dir`, `g.step = dir·rate`.
- `voiceRate (v, tape, s)` reads `gp.s[s]`; `rateForNote (note, s = 0)` keeps the P1 rows. The grain log gains `stream dir gainL gainR windowStep tiltT pitchCents centre regionW counterIdx`; the 2.1 fields keep their meaning.

**T8. Renderer, law + coherence, mixer, hooks** [agent]
- Files: `Source/dsp/GiRenderer.h`, `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T7.
- **Bus:** `VoiceBus` becomes `float s[kVoiceSlots][kGiStreams][2][kCell]` (20 KB), indexed `(vg.voice, g.stream)`.
- **Window (PD8):** fast path = the 2.1 statements verbatim. Otherwise `WindowMorphBank::read (bank.table (step), Tilt {t, a, b}, phase)`, then the same `gk = w·killGain·fade; bl += l·gk·gainL`. Hook `forceBankPath` turns the fast path off.
- **Reverse:** `step < 0`. Add the wrap `readAbs < wrapLo → += wrapW` beside the forward wrap (`:188`), in both the main read and the crossfade read.
- **Law (PD2)**, per (k,s) at the grid when its inputs change: `n_eff = len̄_s/T_kS`; `c` from `Coherence::compute` (span `W_s·fs/tapeRate_s`; scaleOn / nInWindow / unison from `scale.sel[s]`); `g = outputGainFromPower (streamPower (n_eff, n_eff, win_s, c, spread_s, bank))` → `CellLerp lawGain[k][s]`, pushed every tick. A disabled (k,s) holds the uncapped form. Hooks `coherenceOff`, `coherenceVerbatim`, `lawOff` (g := 1).
- **Mixer (PD12):** per stream `acc_s = Σ_k (env_k·lawGain_kS)·bus[k][s]`, accumulated as `Engine.cpp:927-935`; `aud_s = enable && !mute && (!anySolo || solo)`; `lv_s = level_s·aud_s·voiceGainLin`; term `acc_s·lv_s·pan_s`. Wet safety and `output_gain` unchanged; the bus is cleared after reading. Hooks `mixerRampZero` (MX1 control), `voiceGainDbOverride` (NaN = off; −6.0 in RB1, 0.0 in GV0).
- **Readouts** `[20][4]`: `quotaM quotaQ periodT liveVS lawTarget coherence`. The 2.1 readouts become stream-0 views (`liveVoice` = per-voice total). Feedback tap stays zero; no FX.

### Wave 3 — gates, build, parity, D16, re-baseline

**T9. Harness gates** [agent]
- Files: `tests/render-harness/engine_gates_22.h` (new, `eg22::runAll()`); `tests/render-harness/parity_gates_22.h` (new, `--parity <dir> [--rows <prefix>]`); `engine_gates_21.h` (`base()` adds `pins22()` + s2–s4 enable 0, the `Session` ctor sets `hk().coherenceOff = true`, thresholds untouched); `render_hashes.h` (**`prepareJob` body only**: pinned → `coherenceOff = true; voiceGainDbOverride = −6.0`); `main.cpp` (`--parity`, `--rows`, `--cpu22`, `eg22::runAll()`).
- Depends on: T8, T3 (`ParityRows.h`).
- Each row prints its metric, threshold and margin. A row without a control that can fail is not accepted.

| ID | Row | Stimulus | Pass | Negative control |
|---|---|---|---|---|
| RB1 | Re-baseline proof (P11) | 7 `-pinned` jobs | hash = `renders-2.1.sha256` line, all 7 | `-default` jobs: ≥ 1 differs from 2.1 |
| W0a/W0c | Window 0 ≡ 2.1 | RB1's P2 + N1; law: 1 000 n ∈ [0, 400], `streamPower (n, n, hannSel, 0, 0)` and the engine's window-0 `lawTarget` vs the 2.1 `voiceLawGain` | bitwise | window step 1 → law differs |
| W0b | Fast path under test | P2 job + `forceBankPath` | — | hash ≠ 2.1 (P5) |
| QS1 | Σ live ≤ cap, 80 pairs | 16 notes × 4 streams (4 slots), d 200, size 2000, caps 32/64/128/256 × J 0/20 % | live ≤ cap every tick (all 8); 0 steady-state R3 kills (2.5 s window) at caps 256/128 (both J) and 64 at J 0; INFO kill rate at cap 64 J 20 % and cap 32 | `thinningOff` cap 128 → kills > 0 |
| QS2 | Stream-axis starvation | 1 voice: s1 N 400 fills cap 64, then s2 on (d 10, size 100, N 1) | q = (63, 1) exact; s2 live ≥ 1 within L_max + 1 cell; s1 ≤ 63 after L_max | `periodThinOff + overQuotaOff` → s2 live 0 |
| QS3 | Voice-axis starvation | 2.1 Q2 script with s1–s4 on | B ≥ m_B,s per stream within L_max + 1 cell; A ≤ m_A,s after L_max | as Q2 → B < m_B |
| QS4 | Cap lowered live | 8 notes × 4 streams over-demand, 256 → 32 | alive ≤ 32 within L_max + 5 ms + 1 cell | `thinningOff + d25Off` → > 32 |
| QS5 | Asymmetric fill | demands (400, 1, 1, 1), cap 64 | q = (61, 1, 1, 1) exact | `noSort` → differs |
| QS6 | `<80>` ≡ `<20>` | 10 k random voice vectors, s2–s4 zero | q, m, λ, capped bitwise | one s2 demand 1e-9 → differs |
| QS7 | Mute vs disable | 4 voices, cap 64; s2 muted, then disabled | muted: q_{k,2} unchanged bitwise, s2 spawns > 0, s2 output 0 after 10 ms, unmute RMS within 1 dB of pre-mute in the first post-ramp cell; disabled: q_{k,2} = 0, 0 spawns after the tick | the muted assertions applied to the disabled render → fail |
| LF1 | Level flat (PD11) | stationary noise, 1 voice, cap 32, d {1, 10, 50, 200} × size {100, 500, 2000}, 4 seeds | cells with N_eff ≥ 1 within ±1.5 dB of d 10 / size 100; old-vs-old 0 dB | `lawOff` → spread > 1.5 dB |
| CO1 | Coherence (b), noise | spray 0, scan 0, N 1 vs N 200 | within ±1.5 dB | `coherenceVerbatim` → off by > 1.5 dB |
| CO2 | Coherence (b), coherent | sine, scan 100 %, root, spray 0, Retrigger, N 1 vs N 200 | within ±1.5 dB | `coherenceOff` → off by > 1.5 dB |
| MX1 | Enable / mute / solo clicks (QUAL-01) | 440 Hz on 4 slots; toggle s2 enable, s3 mute, s4 solo at multiples of 32 | per edge, d2 peak ≤ 2.0 × the same-motion reference (untoggled render × the exact 10 ms `ExactRamp` trajectory, computed harness-side); the enable on-edge is compared with the untoggled d2 peak | `mixerRampZero` → > 2.0 |
| V2s | Steal click, 4 streams | tonal source, poly 4, 6 distinct pitches, cap 64 | voiceSteals 2 (serials asserted), grain kills + steals > 0; d2 ratio ≤ 2.0 vs no-steal (poly 16, cap 256) | `stealRampZero` → > 2.0 |
| FS1 | FUNC-01/02 stream arm | 4 slots (220/330/440/550 Hz), roots 48/50/52/55, 4-note chord | 16 (serial, stream) rates = `exp2(st/12)·tapeRate/fs` (1e-12); grain slot = stream; every stream RMS ≥ −60 dB | s3 disabled → 0 stream-2 grains, s3 silent |
| FS2 | Release continues (DSP-02) | 4 streams, release 500 ms, note-off | spawns > 0 per stream in [off + 10, off + 400] ms | release 0 → 0 in that window |
| SC2 | Masks per stream / collection (FUNC-09) | s1, s2 Major, pitch_rand 12; mask (s2, Major) = {0, 4, 7}; s2 pitch +7 | s2 picks ⊂ masked members inside `[max(−span, −2400−100p), min(span, 2400−100p)]`; s1 picks include an unmasked degree; (s2, Minor) unaffected | mask set on s1 instead → s2 assertion fails |
| ST1 | Round trip (D17) | 4 × 32 random canonical masks + custom (19-EDO) | 128 masks, name, text, period exact; version advanced | bit-63 mask through its decimal string + `getLargeIntValue` → clamps (P17) |
| ST2 | Bit 63 | 64-degree custom with only bit 63 set; partial 12-EDO mask | exact | (ST1 control) |
| ST3 | Absent → default | non-default blob, then a blob without CustomScale / DegreeMasks | 12-EDO, all-on, seed kept | `testNoStateReset` → stale values |
| ST4 | Two reopens | save → load → change → save → load | second state wins; live tree has 0 Engine / CustomScale / DegreeMasks children | `testNoStateStrip` → shadowed / child count > 0 |
| ST5 | Hex check | `bits` of 15 / 17 chars, one with `g` | ignored (all-on) | valid 16-hex → applied |
| ST6 | Try-lock never blocks (PD3) | `setCustomScale`, then the harness holds `customLock` for 2 blocks, then releases | held: blocks return, `customTryFails` ≥ 1, engine copy unchanged; released: copied at the next tick | the held phase is the released phase's control |
| A2 | 0 audio-thread allocations | A1 stimulus + 4 streams, all randomisation, window/skew, scale on s2/s3, mask + custom swaps while 6 notes sound, enable/mute/solo, cap 256 → 32 | 0 allocations; `customCopies` > 0; `activeSetRebuilds` > 0 | liveness `malloc(64)` → 1 |
| NB1 | Block-size null (R§13) | 4 streams × 4 slots, 6 notes @ poly 4, spray 30, jitter 50, size_rand 50, spread 70, gain_rand 50, pitch_rand 7 + Major on s3, reverse 40, window 0.6, skew −40, cap 64; s2 enable / s3 mute toggles + a mask swap at multiples of 32 (event-split) | blocks 32/441/512/4096/8192/var bit-identical | seed + 1 → differs |
| V3s | Slot invariance, 4 streams | V3 script with NB1 randomisation | per-(serial, stream) logs identical under slot permutation | seed + 1 → differs |
| N2s | Unread params | all 4 streams on; every ID outside `readSet22()` (148) randomised | bit-identical | `s1_density` and `s4_window` → differ |
| R0/R21 | Regression | 55 Stage 1 + 33 Phase 2.1 rows | 88 PASS, thresholds unchanged | their own |
| PN01–26 | DSP-01 null (`--parity`) | T3 rows; ours: 1 voice, root, kt 100, scan 0, same Δ-shifted material | residual ≤ −90 dB after least-squares gain alignment over n ≥ attack + 2 cells; equal grain counts | PNc1 / PNc2 / PNc3 each > −90 dB |
| PS1 | Stats, combined (`--parity`) | spray 30, size_rand 50, spread 70, pitch_rand 7 Off, reverse 30, position 50 %, region ≥ 2 s, 4 streams × 200 g/s × 20 ms, 5 s per side (n ≥ 4000) | two-sample KS D ≤ 0.060, one-sample ≤ 0.043; \|Δmean\| ≤ 0.134σ; var ratio ∈ [0.887, 1.127]; range inside support + 1e-6 and ≥ 0.98 width; reverse \|Δp\| ≤ 0.061 | ×0.5 on our samples → fails |
| PS2 | gain_rand | 50 %, spread 0; dB ∈ U(−12, 0) | as PS1 | ×0.5 → fails |
| PS3 | jitter | J 50 %, d 200, size 20, uncapped | displacement `e_k` KS (two-sample + analytic); even-k χ², 20 bins ≤ 63.7 | ×0.5 → fails; **engine `jitterScale 0.5` → fails** |
| PS4 | scale-on picks | s1 Major, pitch_rand 12 | member-index χ² (df = members − 1) vs uniform and vs parent | picks limited to the lower half → fails |
| SC1 | Quantiser ≡ parent (`--parity`) | choices 1–31 (31 = a 19-EDO and a 64-degree custom) × masks {all-on, fixed canonical, bit 63 only} × span {0.5, 7, 24} st × pitch {−12, 0, +7} | members + [lo, hi) bitwise vs `activesets.tsv` | our mask bit 0 flipped → differs |
| SC3 | Custom parse | SC1 texts + one bad token | table bitwise vs `customtables.tsv`; badToken equal; a failure keeps the default | — |
| GV0 | D16 measure (`--rows GV`) | 8 s noise, −30 dBFS RMS; parent mix 100 %, s1, defaults, 20 s; ours 1 voice root 48 vel 127, defaults, override 0 dB; 4 seeds; skip 0.5 s | prints target; seed spread ≤ 0.2 dB | — |
| GV1 | D16 verify | GV0 with the shipped `kVoiceGainMeasuredDb` | \|error\| ≤ 0.5 dB; old-vs-old 0 dB | override −6 dB → \|error\| > 0.5 |

**T10. Build + default harness** [run]
- Files: none.
- Depends on: T4–T9.
- Run (BG): `ninja -C build O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump` (consoles only: plugin targets are built AND installed together in T13), then `$H`, then param-dump; the runner refreshes ControlGrid column 5, then `tests/fork-drift-check.sh`.
- Done when: rc 0, no new warnings in plugin sources; harness 0 FAIL (R0 55, R21 33, every eg22 row) with every control firing; param-dump byte-identical to `$S/pd-21.txt`; drift 0 and NC-a / NC-b (re-made in `$S`) fire; `diff $S/render_hashes.T2.h tests/render-harness/render_hashes.h` touches only `prepareJob`; **X1:** `ninja -C build -t compdb` filtered to entries whose output path contains `O-GranulatorInstrument` has 0 matches of `O-Granulator/Source|ogi-parity|82406fce` (control: T3's positive grep).
- On a failure: fix it in the owning task (T1 map) and rebuild. **Never loosen a threshold or re-record a `-pinned` hash.** An RB1 miss means a statement shape changed (P6 / PD8): fix the shape.

**T11. Parity run (DSP-01, FUNC-09)** [run; agent fixes]
- Depends on: T3, T10.
- Run (BG): `tests/parity/run.sh "$S/parity"`, then `$H --parity "$S/parity/out"`. GV1 is excluded here (the constant is still the placeholder).
- Done when every PN / PS / SC1 / SC3 row passes, every control fails, and GV0 has printed its target.
- A PN miss is diagnosed against R§2 (the Δ arithmetic, `freezeNoMinSpray`, the P14 shape). It is **never tuned**: no offset is added until the row nulls.

**T12. D16 set + renders re-baselined once** [agent edits, run executes]
- Files: `Source/dsp/GiConstants.h` (`kVoiceGainMeasuredDb`), `tests/golden/renders-2.2.sha256` (new).
- Depends on: T11.
- PD1: |target| > 1.0 dB → STOP. Otherwise set `kVoiceGainMeasuredDb` = target (0.01 dB) with the comment "measured vs O-Granulator 82406fce, mix 100 %, PLAN-2.2 T12, user 2026-11-08"; rebuild the consoles (BG); `$H --parity "$S/parity/out" --rows GV` → GV1 PASS; default harness still 0 FAIL.
- RB2: `$H --dump-render-hashes "$S/r22.sha256"` → `tests/golden/renders-2.2.sha256`. Every `-pinned` line equals 2.1; the `-default` lines differ → old → new table in SUMMARY, split into "defaults honoured" and "voice gain". An ear trim from HANDS-ON-2.2 later re-runs GV1 + RB2 (the one allowed re-record).

### Wave 4 — install, validators, CPU, hands-on

**T13. Install + validators + CPU INFO** [run]
- Depends on: T12.
- `./scripts/build-and-install.sh O-GranulatorInstrument` (BG): clears the AU cache and sweeps both the `O-GranulatorInstrument` and `-dev` variants; afterwards only `O-GranulatorInstrument-dev.{vst3,component}` are on disk.
- `auval -v aumu OuGI OuDv` PASS + the `ZZZZ` control. pluginval 10: VST3 ×2 + AU, BG, one at a time, `--timeout-ms 600000`, `Main bus num input channels: 2`. Standalone alive after 6 s.
- **X2:** `nm -C` on the installed VST3 + AU binaries and `$H` → 0 `ogran::`; control: the parity binary → > 0.
- **C22** (R§14), `$H --cpu22` alone (no concurrent build or parity run), BG: 48 k / 128; 16 notes × 4 streams × 4 slots, d 200, size 1000, cap 256; assert mean live ≥ 240, no FX; `CLOCK_THREAD_CPUTIME_ID` around `processBlock`, 5 s warm-up + 7 × 10 s, median/min/max; duty witness ≥ 0.9, else "contended — no verdict"; bypass ratio from the same session with all streams disabled; print `INFO C22`.
- Done when: all PASS and the C22 line is recorded. Logic stays closed during validation.

**T14. `HANDS-ON-2.2.md` (human gate)** [agent writes, user runs]
- Files: `stages/2-dsp/HANDS-ON-2.2.md`.
- Depends on: T13.
- Run in the Standalone **and** one DAW (Logic, AU). Rows:
  1. 4 files in 4 slots, s2–s4 enabled, chord → each stream plays its own slot.
  2. Mute / solo / enable per stream: click-free; solo is additive; unmute is instant.
  3. Jitter, spray, stereo spread, reverse and gain rand: each audible per stream.
  4. Window 0 → 1 and skew ±100: audible and click-free.
  5. Built-in scales with pitch rand 12 (Major, a pentatonic, a non-12 collection): pitches snap; Off = continuous.
  6. **Voice gain (D16):** a single note at defaults vs O-Granulator at mix 100 % on the same file → matched loudness; a 6-note chord → no harsh knee. Confirm, or give a trim in dB (recorded per PD1).
  7. Grain Limit 256 → 32 with 4 streams × chord: thins with no clicks or stuck grains.
  8. Polyphony 4, 6+ notes legato with 4 streams: steals are inaudible.
  9. Release with 4 streams: every stream continues through the release.
  10. Session reopen: the `sN_scale` choices restore; slots empty (expected until 2.6).
- Done when: the user marks every row. **Not self-certified.**

### Wave 5 — bookkeeping + commit

**T15. SUMMARY, STATUS, commit** [agent writes, run commits]
- Files: `stages/2-dsp/SUMMARY-2.2.md`, `.planning/STATUS.md`.
- Depends on: T13. T14 may still be pending; STATUS then reads `stage_2_phase_2_2_executed_hands_on_pending`.
- SUMMARY: every row with margin and control; PD1–PD16 with the three user decisions marked; the GV0 target and the constant set; the RB2 old → new table; C22 (flagged if > 28 %); the R3 kill-rate INFO lines; the PD4 refinement of D17; every deviation from this plan with its reason.
- Commit:
  1. `git branch --show-current` (= `main`) and `git status --short` **immediately before**.
  2. `git add` the exact new paths: `Source/dsp/GiScale.h`, `tests/parity/*`, `tests/render-harness/{engine_gates_22,parity_gates_22,render_hashes}.h`, `tests/golden/renders-2.{1,2}.sha256`, `stages/2-dsp/{SUMMARY,HANDS-ON}-2.2.md`; then `git commit -- plugins/O-GranulatorInstrument`. Never `-A`/`-a`; foreign paths never staged.
  3. `git log -1 --stat` shows only our paths. **No tag.**

---

## Dependency graph

```
T1 ─→ T2 ─→ T4 ─┬─→ T5 ─┐
 │              └─→ T6 ─┴─→ T7 ─→ T8 ─→ T9 ─→ T10 ─→ T11 ─→ T12 ─→ T13 ─┬─→ T14 (human)
 │                                      ↑             ↑                  └─→ T15
 └─→ T3 (parity build, parallel with W2)┴─────────────┘   (ParityRows.h → T9; parent renders → T11)
```

T2 must finish before any `Source/` edit, because the hashes have to come from the unmodified engine. T5 and T6 share no file and can be written in either order. T7 and T8 both edit `Engine.{h,cpp}` and run strictly in sequence. CMake is not edited (PD16).

## Execution notes

- **Agent split.** dsp-agent writes T3's sources, T4–T9, T12's constant, T14 and T15's SUMMARY. The runner does every build, gate, validator and the commit. Long commands (ninja, `run.sh`, build-and-install, pluginval, cold auval, `--cpu22`) run BG with logs in `$S` (600 s watchdog).
- **Writing order inside W2:** T4, then T5 / T6, then T7, then T8. Nothing is configured or built between them.
- **Parent reads** always go through `git show "${PIN}:…"` (braced) or the T3 archive, never the working tree.
- **Events in deterministic rows** (NB1, V3s, A2, MX1) go at absolute multiples of 32 with event-split blocks, so the tick that latches them does not depend on block size (R§13).
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument 2-dsp`.

## Success criteria (CONTEXT-2.2 gate)

| Criterion | Proven by |
|---|---|
| DSP-01 null ≤ −90 dB across size, density, window, skew, pitch, position, reverse 0/100; shift control fails | PN01–PN26, PNc1–3 |
| DSP-01 statistics within CI bounds; wrong-distribution controls fail | PS1–PS4 |
| FUNC-09: 31 built-ins + custom ≡ parent; masks per stream and per collection | SC1, SC2, SC3, PS4 |
| D17 state: 4 × 32 + bit 63 + custom round trip, absent → default, two reopens, 0 allocs on swap | ST1–ST6, A2 |
| Level flat ±1.5 dB across density × size with thinning, old-vs-old | LF1, CO1, CO2 |
| PERF-02 quotas: ≤ cap every tick, no starvation on either axis, cap lowered converges, Q0–Q3 unchanged | QS1–QS7, R21 |
| QUAL-01: steal click with verified overflow; enable / mute / solo click-gated | V2s, MX1 |
| Window 0 ≡ 2.1 bitwise; law at window 0 ≡ 2.1 | W0a–W0c, RB1 |
| Block-size null with all randomisation, 4 streams × chord; seed control | NB1, V3s, N2s |
| FUNC-01 stream arm; release continues in every stream | FS1, FS2 |
| D16 measured ±0.5 dB, HANDS-ON confirms | GV0, GV1, T14 row 6 |
| Re-baseline once with old → new; Stage 1 rows + param-dump byte-identical | RB1, RB2, R0, T10 |
| Drift 0, auval, pluginval 10 VST3 + AU; parent code excluded | F1 (T10), T13, X1, X2 |
| CPU INFO recorded | C22 |
| HANDS-ON-2.2 human-verified | T14 |

## Risks

- **R1 Parity build cost.** The first out-of-tree build compiles the archived plugin (minutes). It runs BG and incrementally afterwards, and never builds from the parent working tree.
- **R2 Tuning a null into passing.** A PN miss gets diagnosed, never offset (CONTEXT risk). PNc3 proves the mip-phase term, and the `anchor`/`writeHead` self-check proves the derivation.
- **R3 Coherence differs from the parent by design.** At scan 0 the parent's c = 1 and ours is 0. The law ratio is constant per PN row, so the least-squares gain absorbs it. CO1/CO2 are the behavioural proof.
- **R4 FP statement shape.** Refactoring the renderer or mixer can change contraction or the sign of zero. RB1 catches it; fix the shape, never the golden.
- **R5 R3 kills with jitter at m = 1 and with > cap pairs** (PD7, P10). This is expected and reported as INFO; the hard row is live ≤ cap.
- **R6 The try-lock in a host.** A miss only delays the custom table by one tick. ST6 proves it never blocks.
- **R7 +6 dB voice gain** (PD1). Chords reach the §18 knee sooner. HANDS-ON row 6 confirms or trims.
- **R8 A late 2.1 fix** (D14). T1's file → task map forces a re-check.
- **R9 CPU contention.** C22 runs alone; the witness marks a contended run "no verdict".
- **R10 Concurrent sessions.** Foreign dirty paths exist. Path-scope the commit and re-check branch and status right before it.
