# Stage 2 — DSP · Phase 2.5b (Mono / Legato / glide, sustain, MPE lower zone, VST3 note expression) — Verification

## Verification Date

2026-11-09

I re-ran every automated check below myself in the verify session, against `c7efbee7`. The plugin tree was clean beforehand.
- I touched `Engine.cpp` and `PluginProcessor.cpp` and rebuilt the harness and param-dump (rc 0, 0 warnings).
- I re-extracted and re-built the parity reference from the pin into a fresh work dir (`tests/parity/run.sh`, scratchpad, never in the repo).
- I reinstalled with `build-and-install.sh` and relinked the Standalone explicitly.
- `--cpu25a` and `--cpu25b` ran alone, after pluginval finished. Logic was not running.

No automated result here is copied from SUMMARY-2.5b.md.

HANDS-ON-2.1 (13 rows), 2.2 (10), 2.3 (9), 2.4 (11), 2.5a (12) and 2.5b (11) are human gates. On 2026-11-08 the user deferred all of them until after Stage 3 (D14 override). They are **not** self-certified here.

There is no parent equivalent for 2.5b (O-Granulator is an effect). The proof is law gates, same-motion click gates, the block-size null, the allocation gate and true bypass. The parity arm is carried, not extended.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.5b / ROADMAP Phase 2.5b)

1. **D29 Mono.** An overlapping note mints a new serial. Envs retrigger from the current level, scan restarts, LFO phase resets, Note Random and velocity update.
2. **D29 Legato.** Same serial; only the pitch moves (FUNC-05: Legato does not retrigger env or scan).
3. **D30 fallback.** Releasing the top key glides back to the held key in both modes, with no retrigger.
4. **Glide.** Linear in semitones at the spawn samples; constant time; Poly glide-from-last; glide 0 is an exact jump.
5. **Sustain (CC64).** Note-off deferred to pedal-up; the steal policy treats a sustained voice as "no key down"; the Mono stack excludes sustained keys; CC121 / CC123 / CC120 behave.
6. **D32 mode switch.** Newest voice kept, others 5 ms release, no stuck note, no click beyond the control.
7. **MPE (D31, DSP-06 MPE arm).** Member bend / pressure / slide move only their own note; RPN 0 sets the member range (default 48); master bend adds; master AT / CC74 and poly AT are ignored.
8. **NE (D5).** NE −50 ¢ on C#4 renders a quarter-sharp C4 (±0.01 ¢); NE and MPE bend add; live, keyed by noteId.
9. **True bypass.** `mpe_enable` off, Poly, glide 0, no CC64 → every 2.5a golden unchanged; param-dump identical; drift 0 / 13.
10. **QUAL-01 / PERF-03 / PERF-01.** Click gates vs same-motion controls; block-size null with the full script + seed control; 0 allocations across the MIDI / MPE / NE script.
11. **CPU regression.** C25m ≤ 40 %, C25tm ≤ 12 %.
12. **Validators.** auval and pluginval 10 (VST3 + AU) pass; parent code stays out of our binaries.

### Deliverables (SUMMARY-2.5b.md)

- **C1 `e528c48a`:** `ControlGrid.h` binds `voice_mode` / `glide` / `mpe_enable` (ADAPTED, re-hashed); `Voice` gains `keyDown` / `noteId` / `neSemis` / `mpeSemis` / glide state; `AmpEnv::retrigger`; `GiNoteStack.h` (cap 128); appended pitch terms + `pvSemis` in the law key; CC64 sustain with release-with-tail re-strike (PD9, user); `noteOnMono` / `noteOffMono` / `retriggerVoice` / `moveCloudPitch`; D32 `modeSwitch`; `GiMpe.h` (lower zone, `juce::MidiRPNDetector`); the stamped NE path through `GiVst3Extensions::onVst3RawEvent` + `processBlockBypassed`.
- **C2 `c7efbee7`:** `engine_gates_25c.h` (33) + `engine_gates_25d.h` (36), `--cpu25b`, `HANDS-ON-2.5b.md` (11 rows, deferred), test-only probes under `OGI_TEST_HOOKS`.

Code read in verify:
- `Engine.cpp:2444-2528` `noteOnMono` / `noteOffMono`. A held cloud takes `moveCloudPitch` in Legato and `retriggerVoice` in Mono. No held key means a same-slot retrigger, or a fresh voice when there is no cloud. The fallback moves the pitch only; a sustained last key clears `keyDown` and holds the gate.
- `Engine.cpp:2551-2610` `retriggerVoice` never calls `startVoice` in the shipping path (the VM5n control is the only caller under a hook). It mints a serial, calls `env.retrigger (…, kStealMs)` and restarts the schedulers / scan / Tempo clock per stream.
- `Engine.cpp:2697-2731` `modeSwitch` keeps the max-serial counted voice, rebuilds the stack from physical keys oldest-first, force-releases the rest at `kStealMs`.
- `GiNoteStack.h`: fixed 128-entry array, shift-based remove, drop-oldest counted — no allocation.
- `PluginProcessor.h:56-68` / `.cpp:253-292`: a fixed 64-entry raw-event copy, converted into a fixed engine array, cleared after every block and in `processBlockBypassed`. `git diff --quiet 98836951 HEAD -- modules/tuning/note-expression scripts/juce-patches` → unchanged.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Mono | ✅ Achieved | **VM1** serial 1 → 2, \|Δ amp\| = \|Δ menv\| = 1.31e-4 (≤ 1e-3), σ 0, LFO 1 == phase, NR redrawn, vel 0.8976. **VM1n** ampResetZero 0.4998 (fires). **VM2** gain linear to 0.9 within 3.0e-8 over 272 samples; **VM2n** gainSnap 0.449. **VM4** 4 / 4. **VM5** lawGain / gFb bitwise across the retrigger; **VM5n** retrigViaStartVoice snaps 0.153 → 0.354. **VM6** 0.0837 ≤ 1.05 B (B 0.119); **VM6n** 1.324. |
| 2. Legato | ✅ Achieved | **VM3** serial / env / gain / NR / velocity / LFO / scan **bitwise** == a single-note render; note 64, Key 0.0625. **VM3n** legatoAsMono differs. |
| 3. Fallback | ✅ Achieved | **FK1** Mono + Legato: serial kept, note 60, stage unchanged. **FK1n** fallbackRetrig mints a serial. **FK2** 3 / 3. **FK3** off-first = retrigger, on-first = legato, 32 vs 4096 0 diffs. |
| 4. Glide | ✅ Achieved | **GL1** worst 0 st over 196 grains (≤ 1e-9); **GL1n** 24 st. **GL2** N 14400.003 for 1 st and 24 st. **GL3** retarget continuous (0 st). **GL4** Poly second note from 60 → 67. **GL5** glide 0: state untouched, exact. **GL6** lawTarget == fresh pairLaw over 3099 ticks (852 changes); **GL6n** pvKeyOff 1.309. |
| 5. Sustain | ✅ Achieved | **SU1** release at the pedal-up sample. **SU2** victim = the sustained voice. **SU3** PD9 re-strike: old voice releasing (stage 4), new slot fresh, ch 2 untouched. **SU4** 3 / 3. **SU5** 3 / 3 (CC121 / CC123 / CC120). **SU6** polyphony 16 → 4 + pedal-up + Poly → Mono in one tick: 0 left after 2 s, 0 non-finite. |
| 6. D32 switch | ✅ Achieved | **VS1** 16 → 1 counted (kept serial 16 = max), 15 forced, stack 16. **VS2** Mono → Poly 0 diffs before the next note. **VS3** Mono ↔ Legato 0 diffs. **VS4** detached cloud 3 / 3. **CK5d** κ 1.000× (control 1279×). |
| 7. MPE | ✅ Achieved | **MP1** ch 3 worst 0 st over 98 grains, ch 2 / 4 grain logs bitwise; **MP1n** mpeBroadcast moves them. **MP2** 7 / 7 (48 → 12 → 12.5; NRPN / null / ch 1 ignored). **MP3** key 49.500122070, err 0. **MP4** master AT / CC74 / poly AT ignored with MPE on, move with MPE off. **MP5** initial values exact. **MP6–MP8** 3 / 3 each. **MR1** 5 / 5. **MR2** ch 2 0 → 0.5, ch 3 bitwise. **MR3** 0.236 → 0.787. **MR4** slot offset 0 vs 7: 0 diffs. |
| 8. NE | ✅ Achieved | **NE1** C#4 + NE −50 ¢ → worst \|cents − 50\| **0.000000** over 48 grains (≤ 0.01 ¢); **NE1c** no NE / noteId 99 → 50.000 ¢ away. **NE2** mid-block change at 441 / 4096: 0.000 ¢ before and after; **NE2n** neModuleTable 70 ¢ off, collision 80 / 30 ¢. **NE3** two noteIds on one pitch: 0.000 ¢ each. **NE4** Legato + NE: start 59.70, end 64.25, fallback 59.70. **NE5** 64 applied / 6 dropped / 0 allocs / 0 replayed after bypass. MP3 covers NE + MPE bend adding. |
| 9. True bypass | ✅ Achieved | `--dump-render-hashes` **byte-identical** to `renders-2.4.sha256`; `--dump-render-hashes-legacy` **byte-identical** to `renders-2.2.sha256`, its 7 `-pinned` lines == `renders-2.1.sha256`. **TB5** 14 / 14 + legacy 14 / 14; **TB6** 14 / 14 with RPN / NRPN traffic, sources 2 / 2. Every 2.5a row (231) still passes in the 300. param-dump `cmp`-identical. Drift **0 / 13**; NC-a / NC-b fire. |
| 10. Clicks / block size / alloc | ✅ Achieved | Click table below. **BS1** 441 / 512 / 4096 / var vs 32: 0 diffs; **BS1n** seed + 1 → 78 998. **AL1** 0 allocations over 104 blocks (132 retriggers, 4 switches, 6 deferred, 2 RPN, 180 member writes, NE 64 / 6 dropped, 130 held keys = stack overflow). |
| 11. CPU | ✅ Achieved | **C25m 27.62 %** ≤ 40; **C25tm 6.56 %** ≤ 12. C25mpe 13.11 % INFO. See Automated Checks. |
| 12. Validators | ✅ Achieved | See Automated Checks. |

**Click gates (local κ vs same-motion held pre / post, thr ≤ 2.0×), re-run:**

| Row | κ ratio | Hard control |
|---|---|---|
| CK5a Mono retrigger | 1.285 | ampResetZero 337.0× |
| CK5b Legato glide | 0.673 | none constructible (bound only → HANDS-ON-2.5b row 10) |
| CK5c D30 fallback | 1.012 | none constructible (bound only → row 10) |
| CK5d D32 switch | 1.000 | modeSwitchHard 1279.0× |
| CK5e pedal-up | 1.028 | pedalUpHard 511.1× |
| CK5f member bend sweep | 0.585 | none constructible (bound only → row 10) |
| CK5g Mono velocity change | 1.542 | gainSnap 210.3× |

## Requirements Verification

**Stage:** stage-2 (Phase 2.5b of 2.1–2.6). A requirement closes only when **all** of its acceptance criteria are covered.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-05: Voice modes + glide | should | ⚠️ Partial (was pending) | **Covered:** Poly / Mono / Legato with glide (VM, FK, GL, VS, SU rows; Legato retriggers neither env nor scan, VM3 bitwise).<br>**Open:** feel = HANDS-ON-2.5b rows 1–6, deferred. |
| DSP-06: Mod matrix | must | ✅ Complete (was partial) | **Now covered:** the MPE per-member pressure / slide / bend route to continuous destinations with bipolar depth and move only their own note (MP1, MP6, MR1–MR4). Both acceptance criteria are now met. Feel / controller check = HANDS-ON-2.5a + 2.5b row 7, deferred (the DSP-05 precedent: listening confirmation does not hold an automated criterion open). |
| PERF-01: RT safety | must | ⚠️ Partial | **Now covered:** AL1 (the whole MIDI / MPE / NE script incl. stack overflow and mode switches), NE5 (raw-event buffer overflow, 0 allocs).<br>**Open:** 2.6 file swap / Record-in. |
| PERF-03: Off-thread + reaper | must | ⚠️ Partial | BS1 block-size invariance with sustain, voice modes, MPE and mid-block NE. Open: 2.6 file swap / Record-in; HANDS-ON-2.1. |
| QUAL-01: No artifacts | must | ⚠️ Partial | **Now covered:** Mono retrigger, D32 switch, pedal-up, Mono velocity change (CK5a / d / e / g with firing controls); CK5b / c / f bound-only.<br>**Open:** file swap (2.6), HANDS-ON-2.1–2.5b. |
| PERF-02: CPU budget | must | ⚠️ Partial | Regression held: C25m 27.62 % ≤ 40. Open: typical on factory presets (Stage 4). |
| DSP-01, DSP-04, DSP-05 | must / should | ✅ Complete (unchanged) | Parity 168 / 0 carried. |
| FUNC-01–04, FUNC-09, DSP-02, DSP-03, DSP-07 | must | ⚠️ Partial | Unchanged; their HANDS-ON rows are pending. |

Still pending (later phases): FUNC-06, FUNC-07 (2.6).

**Requirements Summary (after this phase):**
- ✅ Complete: 5 (DSP-01, DSP-04, DSP-05, DSP-06; COMPAT-01 from Stage 1)
- ⚠️ Partial: 13 (FUNC-01–05, FUNC-09, DSP-02, DSP-03, DSP-07, PERF-01–03, QUAL-01)
- ⏸️ Pending (later phase or stage): 6 (FUNC-06–07 stage 2; UI-01–02 stage 3; FUNC-08, COMPAT-02 stage 4)
- ❌ Failed: 0

Total 24, matching REQUIREMENTS.md.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness + param-dump) | ✅ Pass | rc 0 after the touch-rebuild; 0 warnings. |
| build-and-install.sh | ✅ Pass | rc 0, 0 warnings. Only `O-GranulatorInstrument-dev.{vst3,component}` on disk. |
| Standalone | ✅ Pass | Relinked (`OuariconGranulatorInstrument_Standalone`, rc 0); binary 18:30:33 newer than the harness 18:26:45. Alive after 6 s. |
| Render harness (default) | ✅ Pass | **300 PASS / 0 FAIL** (231 + eg25c 33 + eg25d 36). **139** `ctrl FIRED`, **0** `NOT FIRED`. Duplicate-ID set (B1 G1–G4 H1 H2 M1 R1 R1v S2) == the 2.5a set; the FK / VS renames added no duplicates. |
| Parity (`--parity`, fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures, 332 outputs. **168 / 0**; `--rows GV` **2 / 0**. |
| Render goldens | ✅ Pass | Shipping == `renders-2.4.sha256`; legacy == `renders-2.2.sha256`; legacy pinned == `renders-2.1.sha256` 7 / 7. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv`. |
| Fork drift | ✅ Pass | 0 / 13. Clean pin-archive parent root → 0. **NC-a** (byte on `Grain.h`, parent root) rc 1, 1 drift; **NC-b** (byte on `FeedbackChain.h`, local root) rc 1, 1 drift. |
| NE module untouched | ✅ Pass | `git diff --quiet 98836951 HEAD -- modules/tuning/note-expression scripts/juce-patches`. |
| X1 compdb exclusion | ✅ Pass | `ninja -t compdb`: 156 entries referencing O-GranulatorInstrument → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. Control: the parity compdb has 91 `O-Granulator/Source` refs. |
| X2 symbol exclusion | ✅ Pass | `nm -C` finds **0** `ogran::` in the installed VST3, the installed AU and the harness. Control: `proj-parity` has 76. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. `ZZZZ` control rc 2. |
| pluginval 10 VST3 (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, 2 ch in, 0 `!!!` lines. |
| pluginval 10 AU (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, 2 ch in. Only `!!!` is the informational "Current program is -1". |
| `--cpu25a` (run alone) | ✅ Pass | **2 PASS / 0 FAIL**, rc 0. Witness `ref_ns` 40 523 708; calib-before 0.931–0.939 × ref, after/before ≤ 1.006, duty ≥ 0.999.<br>• C25w (same session) 26.13 %.<br>• **C25m 27.62 %** (27.10 … 28.32) ≤ 40, live 243.2 — margin 12.4 points.<br>• **C25tm 6.56 %** ≤ 12, live 80.2.<br>• C25t 5.67 %; Δ C25tm − C25t +0.88.<br>• Δ C25m − C25w +1.49 points. |
| `--cpu25b` (run alone) | ℹ️ INFO | **C25mpe 13.11 %** (13.07 … 13.55), duty ≥ 0.999; witness law recomputes 100 % of ticks (≥ 90 %), 300 400 member writes — valid. No verdict by plan (PD18). |
| Git | ✅ Pass | No uncommitted plugin changes before verify. No `O-GranulatorInstrument*` tag. |

## Human Verification

All pending; deferred by the user on 2026-11-08 until the Stage 3 UI is in place, run together then:
- [ ] HANDS-ON-2.1 rows 1–13
- [ ] HANDS-ON-2.2 rows 1–10
- [ ] HANDS-ON-2.3 rows 1–9
- [ ] HANDS-ON-2.4 rows 1–11 (rows 3–4 carry CK2 / CK3)
- [ ] HANDS-ON-2.5a rows 1–12
- [ ] HANDS-ON-2.5b rows 1–11. Listening calls: Mono re-attack (row 1), Legato smoothness (row 2), glide curve (row 4), MPE controller (row 7, needs hardware), Dorico NE quarter-tones (row 9, VST3 only), and the three bound-only click rows CK5b / c / f (row 10).

## Issues Found

- **None blocking** in the automated scope.
- **CK5g has the thinnest margin** (1.542× vs 2.0×; control 210×). It is the Mono velocity-change 5 ms gain glide at `vel_to_amp` 100 %, the harshest setting. It passes by plan; flag it for HANDS-ON-2.5b row 1 ("a velocity change swells over a few ms").
- **NE zero-length block (INFO).** `processBlock` clears the raw-event copy even when `n == 0`, without calling the engine. The VST3 wrapper skips `processAudio` for a pure flush (`numSamples == numInputs == numOutputs == 0`, `juce_audio_plugin_client_VST3.cpp`), so those events carry into the next block as SUMMARY deviation 11 says. A zero-sample call **with** buses reaches `processBlock` and drops that call's NE events. It is rare and not gated. Revisit in Stage 4 (COMPAT-02 Dorico check) if a host sends one.
- **Three click rows are bound-only** (CK5b / c / f). Grain pitch is latched per grain, so no hard control can be constructed. They go to HANDS-ON-2.5b row 10.
- **Plan deviations reviewed and accepted** (SUMMARY-2.5b §Deviations 1–12): FL7 re-scope (S8, user), checkpoint grouping, C1 scope T3–T10, T2 ordering (measured binary was the unmodified 2.5a build), row ID renames FB → FK and MS → VS (no new duplicate IDs — confirmed), CK5 same-motion references (2.5a RE form), GL6 / VM5 stimulus inside `kCoherP0St`, MP1 sweep ±2000 inside the spawn clamp, NE glide-target shift (NE4), test-only probes (X2 = 0 confirms they are absent from the shipping binaries), NE flush carry (above), bypass-safe CC123. Each keeps a firing control or is bound-only by construction; no threshold was loosened and nothing was re-pinned.
- **CPU spread.** C25m 27.62 % here vs 28.04 % in execute and 26.81 % at 2.5a verify. Witnesses are valid on all runs; this is run-to-run spread. 2.5b adds no grains, so no regression was expected.
- **Carried, by design:** Record-in, `<Slots>`, factory sources, file swap (2.6); MPE upper zone / MIDI 2.0 (v1.1); Dorico / DAW verification (COMPAT-02, Stage 4); voice / MPE telemetry UI (Stage 3); `DevLoaderEditor` removed in 3.1.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, `.planning/STATE.md`, O-GrainScatter) are not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green (harness 300 / 0, 139 controls fire; parity 168 / 0 + GV 2 / 0; true bypass byte-identical; C25m 27.62 % ≤ 40 %). FUNC-05 moves pending → partial (feel = HANDS-ON-2.5b). DSP-06 moves partial → complete (MPE arm in). HANDS-ON-2.1–2.5b (human) are pending, deferred by the user until after Stage 3.

**Ready for next phase:** Yes, under the user's D14 override. Phase 2.6 (Record-in, state / relink, factory sources) is the last Stage 2 phase. Any ❌ in the hands-on sheets after Stage 3 goes back to `/plugin-execute O-GranulatorInstrument 2-dsp` for the owning phase.

**Blockers:** none for 2.6. Open human gates: HANDS-ON-2.1, 2.2, 2.3, 2.4, 2.5a, 2.5b (deferred).
