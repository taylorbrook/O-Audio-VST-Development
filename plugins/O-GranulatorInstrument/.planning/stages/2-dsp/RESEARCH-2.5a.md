# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.5a — RESEARCH

**Researched:** 2026-11-08
**Scope:** Phase 2.5a only (D26): mod sources (2 × Mod Env, 4 × LFO global / per-voice, velocity, key, note random, mod wheel, non-MPE pressure / slide / bend, 4 macros), the 16-slot matrix with per-voice evaluation, the newest-voice rule, the forked TempoTracker + Tempo divisions + Tempo-mode thinning, the D27 proof arms and the D28 CPU verdict.
**Confidence:** HIGH on code facts (current tree, the pin's blobs). MEDIUM on the CPU projection (§11) and the per-pair mixer arithmetic (§3.4), which are derived, not run. Every claim is tagged **VERIFIED** (read this session: `path:line`, or `82406fce:<parent path>:line` from `git show` blobs, never the parent working tree) or **INFERRED** (reasoning, not run).
**Inputs read:**
- `CONTEXT-2.5a.md` (D26–D28, Defaults, criteria, 7 open questions); `RESEARCH-2.4.md` (format); `SUMMARY-2.4.md`, `VERIFICATION-2.4.md`; `research/ARCHITECTURE.md` §2, §4, §9–§11, §16, §19–§21, Algorithm §1, CPU risk, A5; `ROADMAP.md` § 2.5a; `REQUIREMENTS.md` DSP-06 / PERF-01/02 / QUAL-01; `tests/golden/{params,mod-dests,mod-sources}.tsv`
- Plugin: `Source/dsp/{Engine.h,Engine.cpp,VoiceAllocator.h,VoiceQuotas.h,AmpEnv.h,GiFxLatch.h,GiScale.h,ScanPlayhead.h,GiConstants.h}`, `forked/{ControlGrid.h,ModMatrix.h,ModSources.h,TempoTracker.h,EngineConstants.h,OutputLaw.h,FORK-MANIFEST.md}`, `Source/GiParamIds.h`, `Source/PluginProcessor.cpp`; `tests/parity/{parent_render.cpp,ParityRows.h}`; `tests/render-harness/{main.cpp,parity_gates_24.h,engine_gates_24.h,engine_gates_24b.h}` (+ grep of eg21–eg23)
- Parent blobs at `82406fce` (`plugins/O-Granulator/`): `Source/dsp/{Engine.h,Engine.cpp,ControlGrid.h,StreamScheduler.h,EngineConstants.h}`, `tests/render-harness/engine_gates_26b.h`, `.planning/stages/2-dsp/SUMMARY-2.5a.md`, `SUMMARY-2.6.md`
- Memory: `index_dsp_module_bugs`, `pattern_global_lfo_stateless_block_state`, `pattern_sh_lfo_clock_seeded_random_breaks_harness_determinism`, `pattern_click_kappa_masked_by_level_change`, `pattern_metric_window_vs_modulation_period`, `pattern_float_and_double_linear_ramp_arrive_a_grid_apart`, `pattern_gate_inherits_the_review_findings_scope`, `pattern_gate_stimulus_below_threshold_is_vacuous`, `pattern_run_flag_past_dirty_check_latches`, `index_dsp_gate_design`

## User Constraints (CONTEXT-2.5a; binding)
- **D26:** 2.5a = sources + per-voice ControlGrid / ModMatrix + newest-voice rule + forked TempoTracker / Tempo divisions / Tempo thinning + matrix CPU verdict. Pressure / Slide / Bend run in their non-MPE form; 2.5b reroutes them per member channel without moving any 2.5a golden made with `mpe_enable` off.
- **D27:** parity arm vs `82406fce` (fresh out-of-tree build) at **−90 dB** after gain alignment, gain band per row, a failing negative control per row family: global LFO 1–4 (every shape, Free + Tempo, phase, S&H / smooth), macro 1–4 (20 ms ramp), slot ramps on depth edits, shadow crossfade (source swap, dest swap, sign flip). Parent-only sources (Env, Pitch, Onset) excluded. Law arm: per-voice independence (+ collapse control), newest-voice handover (step ≤ the 20 ms ramp bound, held when silent, un-ramped control), Mod Env golden (retrigger from current, stage-entry latch, 2 ms floors), per-voice LFO phase reset + `(serial, cycle)` S&H + allocation-order invariance, source ranges, neutral controllers add bitwise 0, destination clamp with 16 stacked routes. Route-edit click gates (all MX3 kinds) with local κ vs same-motion controls.
- **D28:** C25w + all 16 slots live (per-voice LFOs + both Mod Envs → per-voice destinations, 16 voices) ≤ **40 %** (C23/C25 method + calibration witness; C25w re-run in the same session). Typical proxy + 4 routes ≤ 12 %. D21 lever ladder; the `grain_limit` 256 → 128 fallback asks first.
- **Defaults:** TempoTracker byte-identical; onsets within 1 sample at 44.1 / 48 / 96 k across tempo changes and loop jumps; Tempo thinning drops only power-of-two onsets; Free-mode control; LFO Tempo = ppq phase. All per-voice draws key on the note serial; block-size null with every source live + different-seed control. `ControlGrid.h` / `ModMatrix.h` / `ModSources.h` / `TempoTracker.h` stay at drift 0/13; new logic in Gi files. The engine never writes APVTS; UI values are `std::atomic<float>`. Allocation gate across route edits, mode switches, tempo changes, steals. True bypass: unrouted → renders-2.4 byte-identical; harness 179/0, `--parity` 133/0 + GV 2/0 carried; param-dump identical. `lfoN_mode` switch uses a 20 ms ramp. HANDS-ON-2.5a written and deferred. Standalone rebuilt explicitly.

## Project Constraints
- Trunk on `main`. Commits are path-scoped: `git commit -- plugins/O-GranulatorInstrument`. Re-check `git branch --show-current && git status --short` right before each commit. Foreign dirty paths at research time (**VERIFIED**): `.claude/agent-memory/{dsp,gui}-agent.md`, `modules/registry.yaml`, untracked `.claude/compaction-snapshot.md`, `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`. Never sweep them in. Never tag.
- Build with `./scripts/build-and-install.sh O-GranulatorInstrument`. Long commands in the background (600 s watchdog). The parity reference is built from `git archive 82406fce` (`tests/parity/run.sh`); the pin is never edited.

---

## Summary (what the plan must absorb)

1. **Nothing of 2.5a exists yet.** No translation unit includes `ModMatrix.h`, `ModSources.h` or `TempoTracker.h` (grep, **VERIFIED**). `GiGridParams` has no LFO / env / macro / mod / sync fields (`forked/ControlGrid.h:339-404`). `Engine::process` takes no transport (`Engine.h:394`) and the processor never reads the playhead (`PluginProcessor.cpp:209`). `applyEvent` parses only note on/off, pitch bend into one global `bend`, CC120 / CC123 (`Engine.cpp:988-1018`). The only per-voice source data today: `velocity` and `serial` (`Engine.cpp:1059, 1062`). **VERIFIED.**
2. **Q3: the parent `ModMatrix` cannot be wrapped; port it.** `ModMatrix.h` is hard-wired to the parent table: `static_assert (… LfoRate1 + 3 == kModDests − 1)` with `kModDests` 113 (`forked/ModMatrix.h:75`, `EngineConstants.h:161`), parent dest numbering (`:59-65`, s1 level = 5, stride 24), and `GridParams::Stream` maps (`:91-147`) for a type our ControlGrid replaced. The instrument table is 126 / 18 (`GiParamIds.h:57-58`). **[correction]** to ARCH §20 "forked ModMatrix.h slot/ramp/shadow logic": write `GiModMatrix.h` as a line-for-line port of `ModMatrix::tick` / `apply` / `offsetAt` / `writeTelemetry` sized 126 × 18. The leaf stays byte-identical and uncompiled; the PL / PM / PE parity rows prove the port. **VERIFIED** (leaf) / **INFERRED** (plan).
3. **The parent `Lfo` class CAN be used verbatim.** Its only foreign dependency is `GridParams::Lfo {shape, rateHz, rateSync, phaseDeg}` (`ModSources.h:106`; parent `ControlGrid.h:326-331`). Add `struct Lfo` to `GiGridParams` plus `using GridParams = GiGridParams;` in our ADAPTED `ControlGrid.h` (re-hash). `ModSources.h` then compiles unchanged, `TempoTracker.h` too. A verbatim LFO makes the global-LFO nulls bitwise in their source values. **INFERRED** (compile not run) **[decision for plan]**.
4. **Per-voice evaluation is sparse and tick-only (Q2).** Global routes sum once (port of the parent). Per-voice-source routes evaluate per non-free voice into dense-stored, sparsely-touched `offV[k][2][126]` (prev / cur), with per-voice residuals for route changes. Spawn-latched destinations re-evaluate at the spawn sample into a local `Stream` copy, as the parent does (`82406fce:…/Engine.cpp:1155-1182`), so `planFx` (`Engine.cpp:1514`) and every draw slot are untouched. Ramped per-voice state exists only for `level` / `pan` (per-pair `CellLerp`s). §3.
5. **Destination classes (Q5): 112 V, 9 G, 4 G/V** (`tests/golden/mod-dests.tsv`). Many V destinations are also **tick consumers**: density / size / size_rand / jitter feed quotas and the period; pitch / pitch_rand / reverse / spray / window / skew / spread / scan_rate / loop feed the per-pair law and the scale window. Those need per-pair views at the tick, not just at spawn. §4.
6. **Newest-voice rule (Q4):** one `from` value per per-voice source and one shared 20 ms `w` ramp. A handover snapshots the current blended value into `from` and restarts `w` (ramp from current, never stack). The newest = the max serial among `State::active` voices (gated or releasing). Spares are `State::stolen`, so they are never chosen (`VoiceAllocator.h:54-76`). Held when no voice sounds. §5.
7. **`lfoN_mode` switch:** a per-LFO crossfade `xf_N` (20 ms ExactRamp) between the global value and each voice's own LFO. While `xf_N ≠ 0` or mode = Per-voice, LFO N's routes run on the per-voice path. The blend makes the class flip value-continuous with no residual. §4.2.
8. **Tempo (Q6):** the parent's power-of-two is the matrix **density multiplier** `2^round(4·off)` clamped to [0.5, 200] g/s (`82406fce:…/Engine.cpp:1013-1033`). The parent has **no** quota thinning in Tempo. ARCH §9 conflates the two **[correction]**. Ours composes them: `div_kS = divBeats · 2^(−e_kS) · 2^(j_kS)`, where `j_kS` = the smallest j ≥ 0 with `I·2^j ≥ thinnedPeriod(…)`. Feeding `div_kS` into a port of the parent's absolute-time grid clock (`82406fce:…/StreamScheduler.h:139-188`) keeps every onset on the bar-anchored grid. §7.
9. **Parity (Q1): no parent hooks are needed.** Route params are ordinary APVTS params on both sides, mapped by **choice name** (index differs: parent s1 level = 5, ours 2; Macro 1 = 8 vs 14). The playhead is `AudioProcessor::setPlayHead` (public). The parent already has a `traceDest` per-tick trajectory hook (`82406fce:…/Engine.h:259-263`). Phase alignment comes free from the 2.4 pre-roll-to-T_e precedent, provided our prepare mirrors the parent's LFO / macro / matrix init (`82406fce:…/Engine.cpp:92-101`). The clean stream destination is **`s1_level`** (linear in the output, same dB-domain apply). Avoid `position` / `pitch` (the parity anchor maps pos·S vs pos·F and needs Δ ≡ 0 mod 2^ℓ). §8.
10. **2.5b seam:** per-voice `pressure`, `slide`, `bendSrc` fields, written by 2.5a's channel-wide events (to every matching voice) and copied from channel state at note-on. 2.5b changes only the routing branch under `mpe_enable`. `voiceSemis` keeps the global `bend` (`Engine.cpp:227`) so renders-2.4 stay byte-identical. §6.
11. **Three carried rows break unless re-scoped.** N2 (`engine_gates_21.h:2109`), N2s (`engine_gates_22.h:1471`) and FN3 (`engine_gates_23.h:359`) randomise the 99 unread params ("sync, mod, voice mode, MPE, record") and assert `touched == 99 && bound == 162`. In 2.5a, 89 of them become read (sync 1 + sN size/density sync 8 + menv 8 + lfo 20 + macro 4 + mod 48). Add `readSet25a()` (≈ 10 unread left: voice mode / glide / MPE / record). Re-scope the three rows with a reason (the P16 precedent). **VERIFIED** (rows) / **INFERRED** (count).
12. **CPU projection: C25m ≈ 29–32 % vs 40 %.** The dominant term is **quotas / law recomputed every tick** once per-voice LFOs drive law-key destinations (+1–2.5 points); the sources and the matrix add < 1 point. The parent's global matrix was below its noise floor (C5 16.26 % vs C4 re-run 17.52 %, `82406fce:…/SUMMARY-2.5a.md`). §11.
13. **The route-edit click control must be proven live.** The parent's MF8n `noDepthRamp` read only 2.09× on s1_level (`82406fce:…/SUMMARY-2.6.md:63`), barely over 2.0. Use local κ (the 2.4 S6 adoption) and also carry the parent's MX3 **trajectory** arm (per-cell |Δv01| ≤ 1.05 × the analytic bound, `82406fce:…/engine_gates_26b.h:443-480`). That arm is deterministic and has a 13.9–29.8× control margin. **VERIFIED** (parent figures) / **INFERRED** (adoption).

---

## 1. Exact current state (task item 1) — **VERIFIED**

| piece | today | 2.5a |
|---|---|---|
| `ControlGrid.h` (ADAPTED) | 2.1 globals + s1; 2.2 4 streams + 10 rows; 2.3 10 FX rows; 2.4 9 globals (FORK-MANIFEST:26). `ExactRamp::setCell` already in (`:115-121`) | + `syncMode`, `sN.sizeSync/densitySync`, `lfo[4]{shape,rateHz,rateSync,phaseDeg,mode}`, `menv[2]{a,d,s,r}`, `macro[4]`, `mod[16]{src,dest,depth}` + pointers; `using GridParams = GiGridParams`; re-hash cols 5/7 |
| `ModSources.h`, `ModMatrix.h` | "ADAPTED (pending 2.5)", rename only, unincluded (FORK-MANIFEST:29-30) | ModSources: **VERBATIM**, included (the Lfo class). ModMatrix: **VERBATIM, reference only** (ported). A manifest reclass, not a leaf edit → drift 0 **[decision for plan]** |
| `TempoTracker.h` | VERBATIM, unincluded (FORK-MANIFEST:25) | included; `update()` once per host block |
| `EngineConstants.h` | `kModSources 12`, `kModDests 113` "unused until 2.5" (FORK-MANIFEST:28) | **stay at parent values permanently**; `ModSources.h:78` / `ModMatrix.h:75` depend on them. Instrument sizes = `GiParamIds::kModDests 126 / kNumModSources 18` **[correction]** |
| sources | `v.velocity`, `v.serial`, global `bend` (`Engine.h:517`) | see §6 |
| state | `<Engine … modDestTableVersion="1"/>` already written (`PluginProcessor.cpp:340`) | no new keys; 2.5a params already in the 261 (params.tsv rows 14–21, 37/40…, 189–260) |

## 2. Wrapping the parent leaves without a leaf edit (Open Q3)

### 2.1 What is usable — **VERIFIED**
- `Lfo` (`ModSources.h:82-226`): `prepare` (`:88-100`) is plain field resets (noexcept, no allocation), so it is callable at note-on. `tick` (`:106-180`): Free = accumulator advanced by `32·rateHz/fs` from the previous cell's rate (R50.5). Tempo = Direct `frac((ppq − barOrigin)/divBeats)` on the held anchor (`:148-155`); a re-lock re-seeds even in Accum. `shape` (`:182-202`): `h(c) = Draw(seed, kLfoStream + index, c)`.
- `TempoTracker` (VERBATIM): `TransportSnapshot` POD, `update(s, absBlockStart)`, `anchor()`, `ppqAt()`, `getRelockEpoch()` (`TempoTracker.h:47-139`).
- `ExactRamp` / `CellLerp` / `setCell` / `settledAt` (`ControlGrid.h:61-131`).

### 2.2 What is not — **VERIFIED** → **[new piece]** `GiModMatrix.h`
- See Summary 2. Port these bodies verbatim in logic:
  - slot compare → residual `r.reset(r.current() + lastContrib)`, target 0 over `rampCells`, depth restarts at 0 (`ModMatrix.h:197-213`);
  - depth `setTarget` + `tick` (`:219-223`);
  - sum before clamp (`:225-240`);
  - non-finite → 0; canonical +0, so a `−0` sum counts as unrouted (`:242-249`);
  - `apply = convertFrom0to1(clamp(convertTo0to1(clamp(raw)) + off, 0, 1))` (`:277-282`);
  - `offsetAt` lerp (`:260-264`);
  - telemetry (`:290-308`).
- Hooks `noResidual`, `noDepthRamp`, `fullRoundTrip` (`:311-313`) keep their parent names, so the parity controls read the same.
- Ranges come from the processor's `AudioParameterFloat::getNormalisableRange()` on the message thread (parent deviation 10, `82406fce:…/SUMMARY-2.5a.md`). Copying a `NormalisableRange` copies `std::function` members, so it happens once, never on the audio thread.

### 2.3 Per-voice LFO through the verbatim class — **INFERRED**
- **Free:** `tick(index, p, tempo = false, …)` with the voice's own `rateHz` / `rateOffset`. `prepare(fs, rampCells, phaseDeg)` at note-on gives the D27 phase reset.
- **S&H keyed `(serial, cycle)`:** pass a derived seed `seedV = mix(seed ^ kVoiceLfoStream ^ mix(serial))`.
  - `kVoiceLfoStream` is a new `GiConstants.h` constant, disjoint from `0x0A1F / 0x0A5C / 0x0A6E` (`EngineConstants.h:240-242`).
  - The cycle counts from note-on.
- **Tempo (per-voice) [decision for plan]:** the class can't do "reset at note-on" with `tempo = true` (the first tick re-locks to the bar, `ModSources.h:127-155`).
  - Recommend: call it with `tempo = false` and `rateHz = fs·pps/divBeats·2^(4·off)`. That is a tempo-rate free run from the note-on phase.
  - The global LFO keeps the parent hybrid (bar-locked).

## 3. Per-voice data layout and tick placement (Open Q2, task item 3)

### 3.1 Layout — **INFERRED** **[new piece]**
- **Global (`GiModMatrix`):**
  - `Slot slots[16]{src, dest, ExactRamp depth, lastContrib}`;
  - `ExactRamp residual[126]` + `residualLive[126]`;
  - `offPrev / offCur[126]`;
  - `ranges[126]`, `rawPtr[126]`;
  - a `touched` list (≤ 16 dests + live residuals) so only routed dests are rolled.
- **Per voice slot (`GiVoiceMod`, ×20, reset in `startVoice`):**
  - `src[18]` (cell values);
  - `Lfo lfo[4]`, `lfoRateHz[4]`, `lfoRateOff[4]` (previous cell);
  - `GiModEnv env[2]`;
  - `offV[2][126]` (prev / cur), `lastContribV[16]`;
  - `resV[126]{value, step, remaining}` + a live list;
  - `pvTouched[≤32]`.
  - ≈ 3.5 KB per voice → 70 KB total. The Engine is heap-held. Fixed arrays, no allocation.
- **Per pair (k, s):**
  - `bool pairRouted[k][s]`, `Stream view[k][s]` (built at the tick only when routed);
  - `CellLerp lvlL/lvlR[k][s]` (level × pan gains, ramped dests only);
  - `PitchSel sel[k][s]` (only when pitch / pitch_rand / scale window are per-voice routed).
- **Route path per slot per tick:**
  - G source → any dest: global `offCur[d] += depth·src`.
  - Per-voice source → G dest: global `offCur[d] += depth·mono(src)` (§5).
  - Per-voice source → V dest: `offV[k][cur][d] += depth·src_k` for every non-free k.
  - Pair offset = `offG(at) + offV_k(at)`, summed **before** the single clamp (D27). Pair views always start from `rawBase` (the APVTS raw), never from a `gp` field the global pass already overwrote (double application, P3).

### 3.2 Tick order (gridTick, `Engine.cpp:553-681`) — **INFERRED**
1. `gp.latch`; Tempo effective values (§7) from the anchor `update()` set at block start.
2. **tickSources:**
   - global LFO ×4 (verbatim `Lfo`, R50.5 rates);
   - macros (20 ms ExactRamp, parent `Engine.cpp:925-930`);
   - mod wheel;
   - for every non-free voice: Mod Env levels (state advanced per segment, §6), per-voice LFOs (Per-voice mode or `xf ≠ 0`), MIDI sources;
   - then newest-voice mono values (§5);
   - `xf_N` ramps.
3. **matrixTick:**
   - the global pass (port);
   - the per-voice pass;
   - global-only routed tick destinations overwrite `gp` fields (the parent pattern, `82406fce:…/Engine.cpp:983-996`; `gp` is re-latched every tick);
   - per-pair views for `pairRouted` pairs;
   - LFO rates for the next cell;
   - telemetry atomics.
4. Then the 2.4 order, unchanged in sequence:
   - enable edges;
   - `outGainRamp` + substitute (parent R47, `82406fce:…/Engine.cpp:698-713`);
   - `setStreamRamps` + level / pan substitutes (`:742-755`) or the pair mixer mode (§3.4);
   - modulated `writeReverbAtomics`;
   - chain controls + feedback (λ) via `setCell` substitutes;
   - `updateHeads` → `takeTapes` → … → `updateRegion` (global routes) → `enforcePolyphony` → `updateScale` (+ per-pair windows) → `updateQuotas` (per-pair density / lMax / jitter / law key) → `updateScan` (per-voice scan rate) → `writeHeads`.
- **Versus the 2.3 FX latch:** it is untouched. `spawn` builds `local` before any draw and passes `&local` where `&gp.s[s]` was (`Engine.cpp:1371`, `planFx` `:1514`). Draws use fixed slots, so a modulated `sizeRand > 0` predicate changes no other draw's value (`Engine.cpp:1376-1380` comment).
- **Versus the 2.4 piece loop:**
  - `writeHeads` still runs at the tick (feedback modulation enters λ through the substituted `fbRamp`);
  - segments as today;
  - `mixPiece` gains the pair mode (§3.4);
  - `runChains` unchanged.

### 3.3 Per-pair tick consumers — **VERIFIED** (today's per-stream reads) / **INFERRED** (change)
- `updateQuotas` reads `gp.s[s].density / sizeMs / sizeRand` per stream (`Engine.cpp:884-895`) and `jit[s]` (`:559`, used in `runSegment:1219`).
- `pairCoherence` / `pairLaw` / `pairFbGain` read `gp.s[s]` (`:349-427`).
- `fillLawKey` is per stream.
- All of these switch to `view(k, s)` = `pairRouted ? view[k][s] : gp.s[s]`. The `changed` dirty-check then fires every tick while a per-voice LFO drives a law-key destination. Expected, and costed in §11.
- **Run-flag trap:** derive `pairRouted` / `pairMode` from `routed || routedPrev` in **both** exits of the tick, before the quota dirty-check (memory `pattern_run_flag_past_dirty_check_latches`).

### 3.4 Per-voice level / pan in the mixer — **INFERRED** **[correction]** to ARCH §16
- **Today:** level and pan are stream ramps applied after the k-sum, `y += acc_s·lv·aud·vgain·pan` (`Engine.cpp:1945-1953`).
- **A per-voice level / pan route is not separable** (normalised-space clamp). For stream s with any pair routed (or routed in the previous cell):
  - the k-loop multiplies `bl·gL_kS(c)`, `br·gR_kS(c)`, with `g = level_kS·pan_kS` per-pair `CellLerp`;
  - the y statement uses `aud·vgain` only.
- The tap uses per-pair `min(level_kS, 1)` inside its own statements (P11 separation kept).
- Switching back after the last route's residual lands is a rounding-level step (multiplication order), not a level step. Unrouted streams keep the 2.4 statements, so true bypass holds.

## 4. Destination classes (Open Q5, task item 4) — **VERIFIED** (`mod-dests.tsv`, `GiParamIds.h:445-478`) / **INFERRED** (consumers)

| dest (index) | class | consumer / application |
|---|---|---|
| `output_gain` (1) | G | ramped: `outGainRamp` substitute (gain → dB → apply → gain) |
| `sN_level`, `sN_pan` | V | ramped: stream substitute when only global; pair mixer mode when per-voice (§3.4) |
| `sN_density`, `sN_size`, `sN_size_rand`, `sN_jitter` | V | tick (per-pair demand, lMax, period, jitter, law) + spawn (size, size_rand); Tempo: R45 rules (§7) |
| `sN_pitch`, `sN_pitch_rand`, `sN_key_track` | V | tick (scale window, coherence `voiceRate`) + spawn (`voiceSemis`, `Engine.cpp:220-230`) |
| `sN_reverse_prob`, `sN_spray`, `sN_window`, `sN_window_skew`, `sN_stereo_spread` | V | tick (law: `Coherence::compute` / `streamPower`) + spawn |
| `sN_scan_rate` | V | tick (`v.scan[s].reanchor`, already per voice, `Engine.cpp:971-983`) + coherence |
| `sN_loop_start`, `sN_loop_end` | V | tick. **[decision for plan]**: global routes move `region[s]` (reads + record head, as today `:829-844`). Per-voice routes move only that voice's **read** region at spawn. The head keeps the global view. The clamp's `circular` test already falls back to absolute distance when the regions differ (`Engine.cpp:1467-1468`) |
| `sN_position`, `sN_gain_rand`, filter ×3, drive, crush ×2, ring ×3 | V | spawn only (local copy → `planFx`) |
| `feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion` (114–118) | G | ramped / tick: λ (`fbRamp` setCell), chain knob / diff / sat substitutes, cutoff log-ramps (parent `:764-794`) |
| `reverb_mix`, `reverb_size`, `reverb_damp` (119–121) | G | **tick** (atomics), not cell-lerped **[correction]** to ARCH §20. Parent R54: mix ≤ 0.001 clears the tank, so an LFO crossing 0 restarts a cold tank (INFO) |
| `lfoN_rate` (122–125) | G/V | G when LFO N is Global (per-voice sources via newest voice); V when Per-voice (each voice's LFO rate) |

### 4.1 Global-only vs per-voice routing of a V destination — **INFERRED**
A V dest routed only by global sources = the parent's path (one value for all voices, `gp` overwrite + spawn `offsetAt`). Per-pair views exist only when a per-voice-source route targets that stream.

### 4.2 `lfoN_mode` flip — **INFERRED** **[decision for plan]**
- Per-voice LFO N ticks only when the mode is Per-voice or `xf_N ≠ 0`.
- **Global → Per-voice:** at that tick, every non-free voice `prepare`s its LFO N at `lfoN_phase` (deterministic, at a tick).
- Source value: `src_k = (1 − xf)·g_N + xf·pv_{N,k}`.
- **Class flips** (LFO N as a source, and the class of `lfoN_rate`) follow the path rule. At `xf = 0` the per-voice path yields `depth·g` exactly, the global value, so returning to the global path is a rounding-level step at most.
- A switch mid-ramp retargets from the current value (`ExactRamp::setTarget`, `ControlGrid.h:96-103`): never stacked.

## 5. Newest-voice rule with steals and ramps (Open Q4, task item 5) — **INFERRED**
- **Newest:** `argmax serial` over `isCounted()` voices, evaluated **at the tick**. Note-on visibility therefore lags ≤ 1 cell, which is block-size invariant.
  - A steal moves the victim to `State::stolen` and the new note takes a free slot (`Engine.cpp:1021-1050`, `1119-1144`). The stealer is the newest; a spare can never be the handover source.
  - When the newest ends (`endVoice`), the next-highest serial takes over through the same ramp.
- **Value per per-voice source `p`:** `mono_p = from_p + (src_p(newest) − from_p)·w`, with `w` an ExactRamp 0 → 1 over `rampCells20`, shared by all p.
  - **On a handover:** `from_p := mono_p` (current, including a ramp in flight), `w.reset(0); w.setTarget(1)`. A second handover inside 20 ms restarts from the blended value: no stacking, no overshoot beyond the convex hull of the values.
  - **No counted voice:** `from_p := mono_p`, `w := 1` with the target frozen to `from_p` (held).
  - **Initial value:** 0 at prepare, and the first note ramps from 0 **[decision for plan]** (no special case; the alternative is a snap on the first note ever).
- **Per-sample bound** for a ramped G dest (e.g. `output_gain`):
  - per cell: `|Δv01| ≤ Σ|depth|·(|Δsrc_newest| + |src_new − from|/rampCells)`;
  - the dest lerps inside the cell, so per sample = per cell / 32.
- **Hook `newestNoRamp`** (control): `w` snaps to 1 → |Δ| per cell = |src_new − from|, 30× the bound at 48 k.
- **Telemetry:** `newestSerial`, `handovers`, `newestWasStolen` (must stay 0).

## 6. Sources, ranges, MIDI, determinism, 2.5b seam (task items 6, 8)

### 6.1 Sources — **INFERRED** unless noted

| src | class | value | key / state |
|---|---|---|---|
| LFO 1–4 | lfo-mode | bipolar, verbatim `Lfo` | global: `Draw(seed, kLfoStream+n, cycle)` (**VERIFIED** `ModSources.h:184-188`); per-voice: `seedV(serial)` (§2.3) |
| Mod Env 1–2 | V | 0..1 | `GiModEnv`: AmpEnv math (RC 1.2 overshoot attack, −60 dB decay / release, 2 ms floor, times latched at stage entry, **VERIFIED** `AmpEnv.h:27-41, 181-200`), **no** voice end. `retrigger()` from the current level (AmpEnv's `noteOn` resets to 0, `:66-71`; used by 2.5b mono, unit-gated now). Advanced per segment beside `v.env.process` (`Engine.cpp:1182-1195`), sampled at the tick |
| Velocity | V | `v.velocity` 0..1 (**VERIFIED** `Engine.cpp:1059`) | — |
| Key | V | `clamp((note − 60)/64, −1, 1)` | — |
| Note Random | V | `Draw(seed, kNoteRandStream, serial).nextFloat(0)` | new constant **[new piece]** |
| Mod Wheel | G | `(cc1·128 + cc33)/16383`; a CC1 write clears the LSB | — |
| Pressure | V | 0..1 | channel AT → every voice; poly AT (note) → voices on that note; last write wins (ARCH §2). Note-on copies the last channel AT **[decision for plan]** |
| Slide | V | `(cc74 − 64)/64` | CC74 any channel → every voice; note-on copies the channel value |
| Bend | V | the existing −1..1 normalisation (`Engine.cpp:1007-1011`) | per-voice copy; `voiceSemis` keeps the global `bend` |
| Macro 1–4 | G | ExactRamp 20 ms of `macroN/100` | prepare: `reset(gp.macro)` (parent `:95`) |

- CC121 (reset all controllers): wheel 0, pressure 0, slide 64, bend 0 **[decision for plan]** (standard; ARCH silent).
- **Neutral:** `depth·(+0)` gives ±0; `+0 + −0 = +0`; the canonical-+0 rule makes the dest unrouted → raw path, bitwise (**VERIFIED** `ModMatrix.h:245-248`).
- **2.5b seam:** the fields live in `Voice`. 2.5b only adds an `mpe_enable` branch that writes member-channel events to the voice whose channel matches. The off-branch is the 2.5a code, so the 2.5a goldens cannot move.

### 6.2 Determinism — **INFERRED** from **VERIFIED** parts
- Every source is sampled at the tick.
- MIDI controller events are applied at their sample (`process` splits segments, `Engine.cpp:526-541`) and are seen at the next tick.
- Mod Envs advance per sample across the same segment splits as the amp env.
- Per-voice random keys on the serial; S&H on `(serial, cycle)`; voice-slot order never enters (the V3 `slotPerm` hook exists, `Engine.h` hooks).
- Block-size null **BN1**: every source live, 16 routes incl. edits, Tempo with a bpm step + loop jump, at 32 / 441 / 512 / 4096 / var → 0 diffs; **BN1n**: seed + 1 differs.

## 7. Tempo (Open Q6, task item 7)

### 7.1 Wiring — **VERIFIED** (parent) / **INFERRED** (ours)
- **Processor:** read `getPlayHead()->getPosition()` once per block into `TransportSnapshot` (`TempoTracker.h:47-56`). `Engine::process(…, transport = {})`, then `tracker.update(s, absPos)` before the first piece (parent `Engine.cpp` process head).
- **Tempo effective values per pair** (parent P50, `82406fce:…/Engine.cpp:1013-1033`):
  - `e = clamp(floor(4·offD + 0.5), ceil(log2(0.5/d0)), floor(log2(200/d0)))`, `density = d0·2^e`, `tempoDiv = divBeats·2^(−e)`;
  - `size = clamp(divBeats·60000/bpm·2^(4·offS), 5, 2000)`.
  - The spawn evaluates the size at its sample (parent `:1170-1175`).
- **Clock:** **[new piece]** `GiTempoClock` per (k, s) = the parent `processTempo` (`StreamScheduler.h:139-188`).
  - Absolute-time grid point `t = A.abs + (barOrigin + slot·div − A.ppq)/A.pps`, emitted at `ceil(t)` with `frac = smp − t`. So |onset − grid| < 1 sample by construction; the parent TS1 measured 0 (`SUMMARY-2.5a`).
  - The slot is re-derived only at a re-lock, enable, mode switch or div change (parent deviation 3).
  - Jitter draws use our key `(serial << 24) | (epoch + tIndex)` on `kSchedStream + s`, not the parent's `ep + tIndex`.
  - Free ↔ Tempo folds the epoch (the P14 pattern, `Engine.cpp:559-572`).
- **First onset at note-on [decision for plan]:**
  - (a) parent R41: the next grid slot ≥ note-on. Up to one division of silence (2 s at density sync 1 bar, 120 bpm).
  - (b) **recommended:** one burst onset at the note-on sample, on the reserved burst counter `2^63 | n` (`EngineConstants.h:231`), then the grid. TS rows exclude the burst by its counter.

### 7.2 Where thinning meets quotas — **INFERRED** **[correction]** to ARCH §9
- **Demand:** `N_kS` = effective density × effective len (Tempo values, parent TS "quotas from effective density").
- **Water-fill unchanged** (`Engine.cpp:925`). Free: `T = thinnedPeriod(…)` (`VoiceQuotas.h:155-163`).
- **Tempo:** `I = tempoDiv_kS/pps`, `j = min{ j ≥ 0 : I·2^j ≥ T }` (integer loop, no log), `div_kS = tempoDiv_kS·2^j`, `periodT = I·2^j`; nEff for the law uses that period.
- A div change re-derives `slot = ceil((ppq − barOrigin)/div − 1e-9)` → the next coarse grid point. All voices share the bar-anchored lattice; thinning only drops onsets.
- `rescheduleNext` is not used in Tempo.

### 7.3 Rows
- **TS1:** onsets vs the analytic grid (mock playhead v2, port of the parent's harness mock; no parent edit), |Δ| ≤ 1:
  - 44.1 / 48 / 96 k;
  - div {1/16, 1/8 T, 1/4 D, 1 bar};
  - 120 / 133 / 97.3 bpm;
  - a bpm step and a loop jump;
  - boundary slots excluded and counted (parent deviation 6).
- **TS1n:** the parent `ceilPerTick` form drops slots (133 bpm, 1/16 T: at 120 bpm the ε-ceil is exact, so that stimulus is vacuous, parent deviation 4).
- **TS2:** cap 32, 16 voices → every onset on the base lattice, and each per-pair interval a power-of-two multiple of I. **TS2n:** hook `tempoContinuousThin` (period = T) → off-grid onsets.
- **TS3 (Free control):** same density in Free → fraction of onsets within 1 sample of the grid < 10 % (must not lock).
- **TS4:** global LFO Tempo value == `shape(frac((ppq − barOrigin)/div) + φ)` bitwise at every tick, across the loop re-lock.
- **TS5:** R45 density steps + size scale.
- **TT1:** grid ppq bitwise at 32 / 441 / 512 / 4096 / var (proves our `update()` call site).

## 8. Parity arm (Open Q1, task item 2)

### 8.1 Hooks — **VERIFIED**
- Parent `TestHooks` already has `traceDest` (per-tick ramped value / per-spawn latched value, `Engine.h:259-263`; written at `Engine.cpp:749, 1189-1196`), `forceLfoMode`, `noResidual`, `noDepthRamp`, `perBlockOrigin`, `ceilPerTick` (`Engine.h:263-284`).
- The trace ring holds 65 536 entries (`EngineConstants.h:163`): drain per block, as the spawn log is drained (`parent_render.cpp:230-262`).
- **Nothing needs a parent edit.** Only `parent_render.cpp` (route / event / transport columns), `ParityRows.h` (a `Route{slot, srcName, destId, depth}`, timed `Event{at, id, value}`, `Transport{bpm, ppq0, playing, steps, loop}`) and our `parity_gates_25a.h`.
- **Name mapping:** each side maps names to its own choice index via `AudioParameterChoice::choices.indexOf`. The host names match ("S1 · Level", "Macro 1", "LFO 1 · Rate", params.tsv both sides).

### 8.2 Alignment — **VERIFIED** precedent / **INFERRED** design
- Our side: silent pre-roll to T_e, note-on at T_e, stream enable at T_e (2.4 PB precedent, `SUMMARY-2.4.md:69-70`; `parity_gates_24.h:60-83`).
- Both engines then tick the same number of cells from prepare. If our prepare mirrors the parent's (`lfos.prepare(fs, rampCellsFor(fs, kModRampMs), phase)`, `lfoRateHz = rate`, `macroRamp.reset`, `matrix.prepare` snapping slots to the latched routes, `82406fce:…/Engine.cpp:92-101`), the LFO accumulators are **bitwise** equal at T_e.
- A different prepare init shifts the phase for the whole render: P4.
- **Free LFO phase** depends on time since prepare, on both sides, so the alignment is structural.
- **Tempo rows:** the same mock transport function on both sides (ParityRows has the math, each driver wraps it in a `juce::AudioPlayHead`); the same block sizes → the same anchors (R39 held anchor).

### 8.3 Rows (≈ 34; route on `s1_level` unless noted, base −12 dB, depth ±50, peak < kKneeT) — **INFERRED**

| id | stimulus | gate |
|---|---|---|
| PL0 | PN01 geometry, 16 slots at None / 0 + all sources busy | null ≤ −130 dB; sets g_PL; + **PL0t**: s1_level trace ≤ 1e-7 abs per tick (precondition) |
| PL1–6 | LFO1 × 6 shapes, Free 3.7 Hz | ≤ −90 dB, \|g/g_PL − 1\| ≤ 1e-4 |
| PL7–9 | LFO2 / 3 / 4 (sine, saw, S&H) at depth −50 | same |
| PL10 | LFO1 phase 90 → 359 → 1 edits (shortest arc) | same |
| PL11–13 | Tempo: sine 1/4 @ 120; S&H 1/8 T @ 133; saw 1 bar @ 97.3 with a bpm step + loop jump | same |
| PL14 | LFO2 → `lfo1_rate` + LFO1 → s1_level (R50.5) | same |
| PL15 | LFO1 → `output_gain` (G dest) | same |
| PM1–4 | Macro 1–4: 0 → 70 → 20 steps | same |
| PE1–6 | depth +60 → −60; 0 → 60; source swap LFO1 → LFO2; dest swap s1_level → output_gain; source → None; 4 stacked slots to the clamp edge | same |
| PLc1–3 | ours-only: `lfo1_phase` +1°; seed + 1 (S&H row); mock bpm +0.1 | each must FAIL the −90 dB gate |
| PMc1 | hook `macroRampZero` | FAIL |
| PEc1–3 | `noDepthRamp`; `noResidual` (source swap); depth −59 | FAIL |

- **Optional latched family [decision for plan]:** LFO → `s1_size` ±10 % (len is geometry-neutral) proves the R48 `offsetAt` spawn latch in parity. Position and pitch are excluded (Summary 9).
- **Per row:** parent ≈ 1–2 s + an 8.3 s silent pre-roll on our side. Run in the background.

## 9. Law arm, click gates and remaining rows (task item 11) — **INFERRED**

| id | stimulus | pass | control |
|---|---|---|---|
| PV1 | 3 held voices (48 / 60 / 72; vel 0.3 / 0.6 / 0.9); velocity → s1_size, key → s1_filter_cutoff, note random → s1_pan, per-voice LFO1 (phase 0 / 120 / 240 via staggered onsets) → s1_spray | each logged spawn value == `apply(raw, offV_k(at))` ≤ 1 ulp; spread across voices > 0 | **PV1n** `pvSharedSource` (all voices read voice 0) → spread 0 → row fails |
| VL1 | per-voice LFO saw → s1_level, 2 notes 0.31 s apart | each pair trace starts at `lfo1_phase` on its first tick | `pvLfoNoReset` → fails |
| VL2 | per-voice S&H + note random, `setSlotOffset` 0 vs 7 | renders bitwise | slot-keyed seed hook → differs |
| VL3 | 64 notes with steals, S&H | per-serial values == a voice-free recompute | — |
| ME1 | menv1 A 50 / D 200 / S 40 % / R 300 ms → trace | ≤ 1e-6 vs the closed form `1.2(1 − c^n)`, `S + (1 − S)c^n`, `L_r c^n` | — |
| ME2 | A = R = 0 | 2 ms floors (n = 96 at 48 k) | — |
| ME3 | decay edited mid-decay | segment unchanged until the next stage (bitwise) | `envLiveTimes` hook → differs |
| ME4 | unit: `retrigger()` at level 0.5 | continues from 0.5 | — |
| RG1 | unit table: vel / key 0, 60, 124, 127 / slide 0, 64, 127 / bend ±, 0 / pressure / wheel 14-bit / macro | exact values (§6.1) | — |
| NZ1 | routes at ±100 from wheel 0, pressure 0, slide 64, bend 0, key 60, macro 0 | bitwise == unrouted | macro 0.1 % → differs |
| CP1 | 16 × Macro1 (= 1) +100 → s1_level; then −100 | +6 dB exactly / gain 0; 0 NaN / wrap | — |
| NV1 | vel 0.2 then 0.9 → output_gain | per cell \|Δ\| ≤ 1.05 × bound + 1e-6 | **NV1n** `newestNoRamp` ≥ 10× |
| NV2 | all voices end | value held, bitwise constant 2 s | — |
| NV3 | poly 1, 8 notes at 50 ms (each steals the newest) | bound holds; within the hull; `newestWasStolen == 0` | — |
| NV4 | second handover at 10 ms into the ramp | bound holds, no overshoot | `newestStack` → overshoot |
| RTa–g | MX3 trajectory: 7 kinds (add, src→None, dest→None, depth→0, +60→−60, dest swap, src swap) × {global LFO, per-voice LFO, Mod Env} on 16 dests | per-cell \|Δv01\| ≤ 1.05 × the parent bound (`engine_gates_26b.h:446-480`), 0 cross-talk | **RTn** `noResidual` / `noDepthRamp` > 2× |
| REa–g | the same 7 kinds, audio local κ vs held references at pre- **and** post-edit states (parent deviation 6) | ≤ 2.0× | **REn** `noDepthRamp` at the LFO peak > 2.0× (assert it fires; P7) |
| LM1 | `lfo1_mode` G ↔ PV with 4 voices, LFO → s1_level | trajectory bound + κ ≤ 2.0× | **LM1n** `modeSwitchSnap` |
| SD1 / SD2 | each of 17 sources → s1_level ±60 (direction + moves); per-voice sources also → `reverb_mix` (mono) | 17 / 17 | — |
| DR1 | Macro1 +50 → each of 125 dests | its consumer moves (trace / spawn log / atomics), others constant | the class gate (P10) |
| MD1 | MIDI script: CC1 / 33, CC74, channel AT, poly AT, bend, CC121 | source values exact at the next tick | — |
| BN1 / BN1n | §6.2 | 0 diffs / differs | — |
| FA5 | route add / remove / swap, mode switches with voices, bpm step + loop, steals with 16 routes, CC storms, macro sweeps | 0 allocations; each event's counter > 0 | — |
| TB3 / TB4 | §10 | bitwise | — |
| TS1–5, TS1n, TS2n, TT1 | §7.3 | — | — |

## 10. True bypass, readset, telemetry (task item 9) — **INFERRED** from **VERIFIED** parts
- **Unrouted = 16 slots at None or depth 0:** `offCur ≡ +0`, no pair is routed, the spawn uses `&gp.s[s]`, and the mixer, ramps, quotas and law follow the 2.4 statements.
  - Sources ticking (LFOs, envs, macros, the tracker) touch no audio.
  - **TB3:** renders-2.4, and renders-2.2 / FR1 under the 2.4 legacy hooks, byte-identical with the sources busy (macros moving, LFO1 Per-voice, CC / AT / bend events in the MIDI).
  - **TB4:** neutral live routes (NZ1 set) on the renders-2.4 recipes → identical.
- **No new params or `<Engine>` keys:** the 2.5a IDs are in the locked 261 (params.tsv). Param-dump identical.
- **Readset:** N2 / N2s / FN3 → `readSet25a`, with a reason (Summary 11).
- **UI:** engine-owned `std::atomic<float>`:
  - `destTel[126]`: G from the global pass; V from the newest voice (`v01` routed, `base01` when the raw moved, the parent rule `ModMatrix.h:290-308`);
  - `srcTel[17]`;
  - `lfoPhaseTel[4]`.
- No APVTS write. Static gate: no `setValueNotifyingHost | beginChangeGesture | setValue(` in `Source/dsp`.

## 11. CPU (Open Q7, D28, task item 10) — **INFERRED** from **VERIFIED** inputs

| term | estimate | basis |
|---|---|---|
| C25w (2.4 verify) | 26.59 % | `VERIFICATION-2.4.md:56, 99` (margin 13.4 points; C23w@2.4 25.13 %) |
| global matrix (16 slots, 126 residual scan) + global LFO / macros | +0.1 % | parent C5 16.26 vs C4 17.52 (noise) |
| per-voice sources: 4 LFOs × 20 (sin / hash per cell) + 2 Mod Envs per sample × 20 | +0.3–0.5 % | ≈ 160 ns + 64 double FMA per voice per cell |
| per-voice pass: 16 routes × 20 voices + touched-list roll + views (≈ 170 B copy + ≤ 16 range converts per routed pair) | +0.3–0.6 % | 1500 ticks/s |
| `updateQuotas` changed **every tick**: water-fill 80 + 80 × (`pairLaw` + `pairFbGain` + coherence) | +1.0–2.5 % | ~100–200 ns per pair (`OutputLaw.h:137-170` O(1), `voiceRate` exp2 / log2) |
| pair mixer mode (2 lerps + 2 mul per pair-sample, 64 pairs) | +0.3–0.6 % | 6 M ops/s |
| spawn re-evaluation (~2.5 k spawns/s × ≤ 4 fields) | ≤ 0.1 % | — |
| **C25m** | **≈ 29–32 %** | vs **40 %** |

- **C25m recipe [decision for plan]:**
  - C25w + LFO1–4 Per-voice (1.3 / 2.1 / 3.7 / 5.3 Hz, ±30 %) + Mod Env 1–2 (A 5 / D 300 / S 50 %);
  - 16 routes covering every path: s1–s4 density (quota every tick), s1–s4 level + 2 pan (pair mixer), 2 pitch + 2 filter_cutoff (spawn + coherence), s1 spray (law), s2 scan_rate.
  - Density depth ±30 % keeps demand ≫ cap (saturated).
- **Witnesses:**
  - routed per-voice dests ≥ 16 every tick;
  - quota recomputes ≥ 0.9 × ticks;
  - pair mode active on 4 streams;
  - live ≥ 240, duty ≥ 0.9, calibration vs `cpu-calib-2.3.txt`;
  - C25w re-run in the same session (the delta is measured).
- **C25tm:** C25t (5.51 %) + 4 routes (global LFO → s1_position, velocity → s1_level, Mod Env 1 → s1_size, Macro 1 → s1_filter_cutoff) ≤ 12 %. Projected ≈ 6–6.5 %.
- **Levers (D21 order):**
  - **L0 (new, bitwise-neutral):** per-pair dirty bits, so `pairLaw` / `pairFbGain` recompute only for pairs whose view changed (a pure function: outputs are identical).
  - Then the ARCH ladder: Hermite level-0, 4-lane SIMD, FX skip.
  - The `grain_limit` 128 fallback **asks first**.

## Pitfalls checklist (PLAN must-haves)

| # | pitfall | must-have |
|---|---|---|
| P1 | compiling `ModMatrix.h` for 126 dests (static_assert, parent numbering) or bumping `kModDests` | port to `GiModMatrix.h`; EngineConstants untouched; drift 0/13 |
| P2 | parity route indices differ per side (s1 level 5 vs 2, Macro1 8 vs 14) | map by choice **name** on each side; a landed-index self-check in the manifest |
| P3 | per-voice view built from a `gp` field the global pass already overwrote (double offset) | views from `rawBase` with `offG + offV`, one clamp |
| P4 | prepare init differs from the parent (LFO rate / phase / macro / slot snap) → constant phase error in every PL row | mirror `82406fce:…/Engine.cpp:92-101`; PL0t trace precondition |
| P5 | per-voice Tempo LFO through `tempo = true` re-locks to the bar (no note-on reset) | `tempo = false` with a tempo-derived rate (§2.3) |
| P6 | N2 / N2s / FN3 randomise mod params → FAIL | `readSet25a`, re-scoped with a reason |
| P7 | route-edit κ control vacuous (parent MF8n 2.09×) | local κ, edit at the LFO peak, RT trajectory arm, assert `ctrl FIRED` |
| P8 | `pairRouted` / pair-mode flag derived after the quota dirty-check skip → latches | derive in both exits, with `routedPrev` (memory) |
| P9 | spare voice chosen as newest; ramps stacking | newest from `isCounted()` only; `from := current` on every handover; NV3 / NV4 |
| P10 | gating 1 dest per path (the review-scope trap) | DR1 over all 125 dests, SD1 over all 17 sources |
| P11 | Tempo thinning applied as a continuous period → off-grid onsets | `div·2^j` (§7.2); TS2 + TS2n |
| P12 | slot re-derived per tick (R40) → dropped slots | re-derive only on re-lock / enable / mode / div change; TS1n at 133 bpm |
| P13 | CPU row with routes idle (envs static after decay, LFOs Global) | witnesses: ≥ 16 routed per-voice dests, quota recomputes ≥ 0.9 × ticks |
| P14 | `-0` from a negative depth × neutral source counted as routed | canonical +0 (`ModMatrix.h:247-248`); NZ1 |
| P15 | `NormalisableRange` copied on the audio thread (`std::function`) | ranges set once on the message thread; FA5 |
| P16 | `voiceSemis` switched to a per-voice bend in 2.5a → renders-2.4 move | keep the global `bend` until 2.5b (`Engine.cpp:227`) |
| P17 | per-voice loop region also moving the record head | head on the global view (§4) |
| P18 | Standalone stale | build `OuariconGranulatorInstrument_Standalone` explicitly |

## Open items for the planner
1. **Decisions (technical; ask only if the plan wants the alternative):**
   - `using GridParams = GiGridParams` (Summary 3);
   - FORK-MANIFEST reclass (§1);
   - per-voice Tempo LFO = note-on reset at tempo rate (§2.3);
   - per-voice loop region read-only (§4);
   - lfo-mode restart on Global → Per-voice (§4.2);
   - first held mono value 0 (§5);
   - pressure / slide initialised from channel state, CC121 honoured (§6.1);
   - Tempo note-on burst (b) vs R41 grid-only (a) (§7.1) — **musical, worth one question**;
   - optional PL size family (§8.3);
   - the C25m recipe (§11).
2. **Freeze thresholds:** −90 dB + 1e-4 band (PL / PM / PE), PL0 ≤ −130 dB, PL0t ≤ 1e-7; κ 2.0×; trajectory 1.05× + 1e-6; TS ≤ 1 sample; TS3 < 10 %; ME ≤ 1e-6.
3. **Expected counts:**
   - harness ≈ 179 + PV 2 + VL 3 + ME 4 + RG 1 + NZ 1 + CP 1 + NV 5 + RT 8 + RE 8 + LM 2 + SD 2 + DR 1 + MD 1 + TS 7 + TT 1 + BN 2 + FA5 1 + TB 2 ≈ **230**;
   - parity ≈ 133 + PL0 / PL0t 2 + PL 15 + PM 4 + PE 6 + controls 7 ≈ **167** (+ GV 2);
   - `--cpu25a`: C25w re-run + C25m + C25tm (2 verdict rows + 1 carried).
4. **Commit order:**
   - (1) leaves + `GiGridParams` + re-hash, TB3 green (no behaviour);
   - (2) sources + global matrix + PL / PM / PE parity;
   - (3) per-voice pass + newest voice + law rows;
   - (4) tempo;
   - (5) CPU.
   - TB3 is re-run at every step, so any hash move is attributable.
5. **HANDS-ON-2.5a items** (deferred post-Stage-3): LFO feel per shape; per-voice spread on chords; tempo lock in a DAW loop; macro sweeps; the reverb_mix LFO cold-tank restarts (R54).
