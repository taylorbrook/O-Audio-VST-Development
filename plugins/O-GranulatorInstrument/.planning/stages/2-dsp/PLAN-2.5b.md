---
title: "O-GranulatorInstrument Stage 2 (DSP) — Phase 2.5b Execution Plan"
created: 2026-11-09
juce_version: "8.0.15"
phase: "2.5b"
summary: "Seven waves: (W0) pre-flight + baselines (harness 231/0, renders-2.4 / legacy, fresh parent build + parity 168/0 + GV 2/0) and --cpu25a on the UNMODIFIED 2.5a engine; (W1) plumbing with no behaviour: ControlGrid binds voice_mode / glide / mpe_enable (re-hash, drift 0/13), readSet25b 254/7 re-scopes N2 / N2s / FN3, Voice gains keyDown / noteId / neSemis / glide state, AmpEnv::retrigger + 5 ms gain glide (unit only), GiNoteStack.h (cap 128), voiceSemis(v, sp, t) with appended zero terms, law key + pvSemis; (W2) voice modes: CC64 sustain (JUCE release-with-tail re-strike, USER), Mono retriggerVoice / Legato target change / D30 fallback, closed-form glide, D32 switch at the tick with the detached-cloud rule -> commit C1 (harness 231/0, bypass held); (W3) MPE lower zone (juce::MidiRPNDetector, per-channel state beside the 2.5a globals) + the NE stamped path through GiVst3Extensions; (W4) gates eg25c (33 rows) + eg25d (36 rows), Session pre-block hook, harness 231 -> 300, parity 168/0 carried; (W5) CPU regression C25m <= 40 % / C25tm <= 12 % + C25mpe INFO; (W6) install, validators, HANDS-ON-2.5b (deferred), SUMMARY / STATUS, commit C2. Checkpoint (renders-2.4 + legacy byte-identical) after every engine task. No tag; version 0.1.0; no new params; CMakeLists.txt not edited."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 2 / Phase 2.5b Execution Plan (Mono / Legato / glide, sustain, MPE, note expression)

## Goal

**FUNC-05:** Poly / Mono / Legato with glide (D29, D30, D32). **DSP-06 MPE arm:** member-channel bend, pressure and slide move only their own note (D31). **D5:** VST3 per-note cents tune the note, by `noteId`, live and sample-stamped. Sustain (CC64) holds the gate.

There is no parent equivalent (O-Granulator @ `82406fce` is an effect), so there is **no parity arm**. Proof is by law gates, click gates against same-motion controls, the block-size null, the allocation gate, and true bypass (every 2.5a artefact unchanged with `voice_mode` Poly, `glide` 0, `mpe_enable` off, no CC64).

**Out of scope** (CONTEXT-2.5b): Record-in, `<Slots>`, factory sources (2.6); MPE upper zone, MPE+ / MIDI 2.0 (v1.1); Dorico template / DAW checks (COMPAT-02, Stage 4); voice / MPE telemetry UI (Stage 3).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT-2.5b.md` | D29–D32, Defaults, gate list. **Binding, not reopened.** D11–D28 still bind. |
| `stages/2-dsp/RESEARCH-2.5b.md` | §1–§10 (cited **R§x**), Summary 1–13 (**RSx**), Pitfalls **P1–P10** (R§8). Authoritative over ARCHITECTURE where they differ ([correction] ARCH §10 term order). This plan's PDs decide every R§9 item. |
| `stages/2-dsp/{PLAN,SUMMARY,VERIFICATION}-2.5a.md` | What exists: harness 231/0 (115 controls), `--parity` 168/0 + GV 2/0, `--cpu25a` C25m 26.81 % / C25tm 6.37 %, hook names, the checkpoint pattern, local κ (2.4 S6), PD10 MIDI source rules, the 2.5b seam fields. |
| `research/ARCHITECTURE.md` | §2 (MIDI / MPE / NE), §3 (allocator, stack, glide, sustain), §10 (spawn pitch), A5 (D4), Algorithm §1 (block-size null). |
| `parameter-spec.md` (LOCKED) + `tests/golden/params.tsv` | `voice_mode`, `glide` (0–2000 ms, skew 0.3), `pitch_bend_range`, `mpe_enable`, `polyphony`. **Not edited.** |
| This plugin | `Source/dsp/{Engine.h,Engine.cpp,VoiceAllocator.h,AmpEnv.h,GiVoiceMod.h,ScanPlayhead.h,MidiCursor.h,GiConstants.h}`, `Source/dsp/forked/{ControlGrid.h,FORK-MANIFEST.md}`, `Source/{GiParamIds.h,PluginProcessor.h,PluginProcessor.cpp}`, `tests/render-harness/{main.cpp,engine_gates_21.h…25b.h,render_hashes.h}`, `tests/golden/{renders-2.1,renders-2.2,renders-2.4}.sha256`, `tests/fork-drift-check.sh`. |
| References (read, never edit) | `plugins/O-simpleWavetable/Source/MonoStack.h` (stack semantics); `modules/tuning/note-expression/cpp/{NoteExpression.h,vst3/NoteExpression_VST3.cpp}` (1.1.2); `scripts/juce-patches/note-expression-juce-8.0.15.patch`; `~/JUCE/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h:72-88` (`Vst3RawEvent`); `~/JUCE/modules/juce_audio_basics/midi/juce_MidiRPN.cpp`; `~/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp:317-321` (re-strike precedent). |

Plan-time facts (verified 2026-11-09): last code commit under `Source/ tests/ CMakeLists.txt` = `98836951`; `Engine.cpp` 3120 lines (`prepare:81`, `voiceSemis:275`, `fillLawKey:497`, `process:559`, `gridTick:613`, `enforcePolyphony:970`, `updateQuotas:1001`, `updateNewest:1363`, `applyEvent:1833`, `noteOn:1916`, `startVoice:1947`, `noteOff:1996`, `forceReleaseVoice:2035`, `endVoice:2062`, `spawn:2328`); `EngineTestHooks` at `Engine.h:207`; `kLawKeySize` at `Engine.h:609`; processor `drainAndUpdate()` at `PluginProcessor.cpp:215`, `engine->process` at `:254`, no `processBlockBypassed` override; `GiVst3Extensions` at `PluginProcessor.h:47`; `readSet25a()` at `engine_gates_21.h:291`, N2 literal at `:2138`; harness `Session` at `engine_gates_21.h:382`, `run` at `:446`; ControlGrid sha256 `2274bdb6…`; test headers `engine_gates_25a.h` / `25b.h` are **taken** by 2.5a → 2.5b uses **`engine_gates_25c.h` / `engine_gates_25d.h`**.

---

## Plan-phase decisions (resolve R§9)

- **PD1 [R§9.1 adopted] Mono amp gain = 5 ms linear glide.**
  - `AmpEnv::retrigger (p, fs, newGain)`: attack restarts from the current `level` (the `ModEnv::retrigger` precedent, `GiVoiceMod.h:71-76`); `gain` moves linearly from its current value to `g_v (newVel)` over `kStealMs` (5 ms) as a sample-counted ramp advanced inside `process` (no float accumulation across blocks: remaining-count + fixed step, both latched at the retrigger sample).
  - `noteOn` is unchanged (from 0, gain latched) → bypass bitwise.
  - Hooks: `ampResetZero` (retrigger = `noteOn`, level → 0), `gainSnap` (gain steps).
- **PD2 [R§9.2 adopted] Mono / Legato note-on while the cloud is releasing** (no key down, gate closed, still counted) = a full retrigger **on the same slot from the current level** (`retriggerVoice`, new serial), in both modes. No counted cloud → `startVoice` from 0 (normal allocation).
- **PD3 [R§9.3 adopted] Key source follows the target note in Legato** (and on a D30 fallback). It is a pitch source; Velocity / Note Random stay frozen per D29.
- **PD4 [R§9.4 adopted] Mono retrigger residual.** For each per-voice-routed level / pan destination of the cloud, push `old − new` contribution through `pushResidual` (`GiVoiceMod.h:279`, 20 ms). Hook `noPvResidual` (row VM6n). If VM6 shows the residual is not needed (κ with and without both ≤ 2.0×), keep it anyway (it is the 2.5a route-change path) and record both numbers.
- **PD5 [R§2.3 adopted] D32 detached cloud.** After Poly → Mono / Legato, if the kept (newest) voice has no key down while other keys are still physically held, the cloud is **detached**: the stack is rebuilt from physical keys but the cloud does not follow it. The next note-on is a full retrigger on the cloud's slot (PD2). A note-off of a held key does not release a pedal-held cloud; the last physical key released with the pedal up releases it. `detached` is a single engine flag, cleared by the next note-on or the cloud's end.
- **PD6 [R§4.2 adopted] NE stamped path, no module edit.**
  - `GiVst3Extensions::onVst3RawEvent (e)` calls the base first (the module table stays intact for nobody-reads compatibility), then `if (neCount < 64) neBuf[neCount++] = e;` (fixed `std::array<Vst3RawEvent, 64>`; drop beyond 64, counter `neDropped`). **T10 first confirms from the patch that `onVst3RawEvent` runs on the audio thread inside the same `process` call that precedes `processBlock`** (else → **S8**).
  - `processBlock`: after `drainAndUpdate()`, pass `(neBuf.data(), neCount)` to `engine->process`, then `neCount = 0`. New `processBlockBypassed` override: `neCount = 0`, then the base `AudioProcessor::processBlockBypassed (buffer, midi)` (P6).
  - Engine: `struct GiNeEvent { int offset, channel, pitch, noteId, type; double value; }` built from the raw list (no Steinberg header; `kTuningTypeID = 2` literal, comment `ivstnoteexpression.h:49`, P7); a second cursor merged with `MidiCursor` by clamped offset, **MIDI first at a tie**. Raw NoteOns go into a 64-slot per-block correlation list; `noteOn (ch, note)` at sample `at` takes the first unconsumed `(at − blockStart, ch, note)` entry → `v.noteId` + stack entry. Tuning events: `semis = 240·(value − 0.5)` written to every non-free voice and stack entry with that `noteId`; noteId −1 ignored.
  - `Engine::process` gains `const GiNeEvent* ne = nullptr, int nNe = 0` (default: the 2.5a call is unchanged).
  - AU / Standalone: no raw events → `neSemis` stays 0.
  - Hook `neModuleTable` (consumer reads the module's pitch-keyed table at note-on instead) = NE2 / NE3 control.
- **PD7 [R§5.1 adopted] RPN.** `juce::MidiRPNDetector rpn` (`reset()` in `prepare`), fed every CC 6 / 38 / 98–101 whether MPE is on or off. RPN 0 on a member channel → `memberRange = semis + cents/100` clamped [0, 96] (zone-wide; cents kept). RPN 0 on ch 1 ignored (master range = `pitch_bend_range`). RPN 6 on ch 1 → `memberCount = MSB`, `memberRange = 48`; a note on a channel > memberCount + 1 is non-member. **MCM 0 ≠ MPE off** (the parameter is the switch). NRPN, RPN null, other RPNs, CC 96 / 97 ignored.
- **PD8 [R§5.2 adopted] MPE routing.** With `mpe_enable` on: master bend → `bend` (2.5a slot, pitch of every voice) **only**; the per-voice **Bend source = member bend**. Member bend / AT / CC74 → `mpeBend / mpePressure / mpeSlide[m]` and the voices whose `channel == m` (pitch term `mBend·memberRange`, Bend / Pressure / Slide sources). Master AT, master CC74, poly AT ignored (D31). **CC64 honoured on ch 1 only** under MPE (zone-level); CC121 on a member channel resets that channel's expression only; CC121 on ch 1 as 2.5a + lifts sustain. A new member note takes the channel's current values. **`mpe_enable` switched while notes sound applies from the next event**; sounding voices keep their current values (no retro-mapping, no step). With MPE off the 2.5a write paths are untouched (per-channel arrays are written but never read).
- **PD9 [USER 2026-11-09] Sustain re-strike in Poly = JUCE release-with-tail.** A note-on whose (channel, note) matches a voice with `gated && ! keyDown` (held only by the pedal) sends that voice into its normal env release (Mod Envs too), then allocates the new note normally. Not a 5 ms steal; no stacking. (Supersedes the CONTEXT "oldest-voice-on-target-pitch" reading, which only fires at the polyphony limit and stays as it is there.)
- **PD10 [R§9.10 adopted] `GiNoteStack.h`** (new, `ogi::GiNoteStack`): capacity **128**, entries `{channel, note, noteId, neSemis}`; push moves an existing (channel, note) to the top; last-note priority; remove by (channel, note); drop-oldest on overflow (counter `stackDropped`); no allocation. Not a `MonoStack` fork → no FORK-MANIFEST row.
- **PD11 [RS11, P4] Pitch: append, don't reorder.** `voiceSemis (v, sp, t)` = the current expression `note − root − fine/100 + bend·range` **then** `+ g(t) + mBend·memberRange + neSemis` (each +0.0 in bypass → bitwise). Every caller passes `t` (spawn sample, or the tick sample for the law).
- **PD12 [R§3.3 adopted] Law key sees per-voice pitch.** `fillLawKey` appends `pvSemis[kVoiceSlots]` (= `g(t_tick) + mBend·memberRange + neSemis + (note − firstNote of this serial)` for counted voices, +0.0 otherwise); `kLawKeySize += kVoiceSlots`. Hook `pvKeyOff` (GL6n).
- **PD13 [R§3.1 adopted] Glide = closed form.** Per voice `{glFrom, glTo, glT0, glN}` (double); `g(t) = (glFrom − glTo)·max (0, 1 − (t − glT0)/glN)`; `glN = glideMs·0.001·fs` from the tick-latched `gp.glideMs` at the event. Retarget: `glFrom = glTo + g(t_event)`, `glT0 = t_event`. `glide` = 0 writes no glide state (`glN = 0` → term skipped). Poly with glide > 0: `glFrom = lastKey` (previous note-on's `note + neSemis`, any mode; unset before the first note). Hook `glideStep` (GL1n).
- **PD14 [RS9] `Voice` fields.** `keyDown` (physical), `noteId` (−1), `neSemis` (0), glide state; `isKeyDown()` reads `keyDown`. Without CC64 `keyDown == gated` always → `chooseVictim` bitwise.
- **PD15 [RS12] `readSet25b`.** `eg21::readSet25b()` = `readSet25a()` + `voice_mode`, `glide`, `mpe_enable` = **254 bound / 7 unread** (`rec_target`, `rec_arm`, `rec_trigger`, `rec_threshold`, `rec_length`, `rec_length_sync`, `rec_gain`). N2 / N2s / FN3 switch to it with literals 254 / 7; titles say `readSet25b`. Any other carried row needing a re-scope → **S8**.
- **PD16 [P8] MIDI tie order is host order.** `MidiCursor` is not changed. Off-then-on at one sample = retrigger; on-then-off = legato. Deterministic and block-size invariant; documented in SUMMARY and pinned by row FB3.
- **PD17 [CONTEXT Defaults] True bypass + checkpoints.**
  - Bypass = Poly, glide 0, MPE off, no CC64, no NE: no glide state, appended pitch terms +0.0, `pvSemis` +0.0, `keyDown == gated`, MPE arrays never read.
  - **Checkpoint after every engine task (T3–T10):** build the harness (BG); `$H --dump-render-hashes "$S/ck-<T>.sha256"` == `renders-2.4.sha256`; `--dump-render-hashes-legacy` == `renders-2.2.sha256`. A move → **S3** attributed to that task. **T5, T7, T9, T10 also run the full `$H` (231/0)** — the 2.5a goldens (eg25a / eg25b) are the MPE-off bypass witness. Renders are **never re-pinned** in 2.5b.
- **PD18 [CONTEXT Defaults] CPU = regression only.** `--cpu25a` unchanged, re-run: C25m ≤ 40 %, C25tm ≤ 12 % (witnesses as 2.5a). New `--cpu25b`: **C25mpe INFO** = C25m + `mpe_enable` on, 16 notes on 15 member channels with member bend sweeping (0.5 Hz) + Legato-free Poly glide 300 ms retriggered every 500 ms; witness law recomputes ≥ 0.9 × ticks. No lever ladder; a regression FAIL → **S2**.
- **PD19 Test hooks** (all under `OGI_TEST_HOOKS`, in `EngineTestHooks`):
  - Switches: `ampResetZero`, `gainSnap`, `legatoAsMono`, `retrigViaStartVoice`, `noPvResidual`, `fallbackRetrig`, `glideStep`, `pvKeyOff`, `modeSwitchHard` (D32 releases 0 ms), `pedalUpHard` (pedal-up = 0 ms gate cut), `mpeBroadcast`, `neModuleTable`.
  - Counters: `serialsMinted`, `retriggers`, `legatoMoves`, `fallbacks`, `modeSwitches`, `sustainDeferred`, `restrikeReleases`, `rpnMessages`, `memberWrites`, `neApplied`, `neDropped`, `stackDropped`, `lawRecomputes`. `resetCounters()` zeroes them.
  - Probes: `getVoiceForTest (k)` extended (keyDown, noteId, neSemis, glide state, env level, amp gain), `getStackForTest()`, `getMpeForTest()`.
  - Harness: `Session::beforeBlock` callback `(blockStart, blockLen)` (eg21) so NE raw events can be injected at `t − blockStart` mid-block.
- **PD20 Housekeeping.**
  - Version **0.1.0**; **no tag**. No new params, no `<Engine>` keys → param-dump byte-identical.
  - `CMakeLists.txt` **not edited** (new sources are headers: `GiNoteStack.h`, `GiMpe.h`; tests are headers).
  - Not edited: every `forked/` leaf except `ControlGrid.h` (re-hashed, cols 5 / 7); `modules/tuning/note-expression/*`; `MidiCursor.h`; `cpu_calib.h`; `tests/parity/*`; pg22–pg25a; eg22–eg25b except the N2s / FN3 literals.
  - **Two commits**, path-scoped: **C1** after W2 (voice modes + sustain + glide, harness 231/0, bypass held) and **C2** at the end.
  - HANDS-ON-2.5b written and deferred post-Stage-3 with 2.1–2.5a.
  - Standalone rebuilt explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check.

---

## Tasks

**Roles.** **[agent]** = dsp-agent writes files (no Bash). **[run]** = build / gate runner: builds, gates, validators, hash refreshes, commits. **BG** = `run_in_background`, log to `$S/<name>.log` (`$S` = the executing session's scratchpad). `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`. Paths relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

**Audio-path rules (every engine task).** No locks (except the 2.2 scale try-lock), no allocation, no vector growth, no `std::function` on the audio thread; fixed arrays only. No listener writes a parameter; the engine never writes APVTS. Every random draw keys on the note serial, never the slot. Glide is evaluated from absolute time, never accumulated.

### Wave 0 — pre-flight, baselines, CPU reference

**T1. Pre-flight + baselines** [run]
- Output `$S/preflight.txt`. Any mismatch → STOP and ask.
  - **Late changes:** `git log --oneline 98836951..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` empty (else map: `Engine.*` / `VoiceAllocator.h` / `AmpEnv.h` → T4–T10; `PluginProcessor.*` → T10; `forked/*` → T3; harness → T3, T11–T13).
  - **Git:** branch `main`; record `git status --short`; foreign paths never staged (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, `plugins/O-GrainScatter/*`, any other session's files).
  - **Contracts:** `parameter-spec.md` sha256 (file + body) = STATUS `contract_checksums`; `shasum -a 256 Source/dsp/forked/*.h` = FORK-MANIFEST col 5; `git diff --quiet 98836951 HEAD -- modules/tuning/note-expression scripts/juce-patches`.
  - **Baseline (BG):** build harness + param-dump; `$H` = **231 / 0**, 0 NOT FIRED → `$S/h-25a.log`, duplicate-ID set → `$S/dup-25a.txt`; param-dump == `.planning/params.tsv` → `$S/pd-25a.txt`; render dumps == `renders-2.4` / `renders-2.2`; drift 0 / 13.
  - **Parity (BG):** `tests/parity/run.sh "$S/parity"` rc 0; `$H --parity "$S/parity/out"` **168 / 0**, `--rows GV` **2 / 0**; residual lines → `$S/res-base.txt`.
  - Standalone phony target present.

**T2. `--cpu25a` on the unmodified 2.5a engine** [run]
- Depends on T1 with every BG job finished. **No `Source/` edit before this is done.**
- `$H --golden-dir tests/golden --cpu25a` alone (BG) → `$S/cpu25a-base.log`; valid witnesses (≤ 3 tries, then STOP). Record C25m / C25tm / C25w / C25t.

### Wave 1 — plumbing, no behaviour

**T3. ControlGrid rows + manifest + readSet25b (PD15)** [agent; runner re-hashes]
- Files: `Source/dsp/forked/ControlGrid.h`, `Source/dsp/forked/FORK-MANIFEST.md`, `tests/render-harness/engine_gates_21.h`, `engine_gates_22.h`, `engine_gates_23.h`.
- ADAPTED region only: bind `voice_mode` (choice index), `glide` (ms, through the skewed range, never inverted by hand), `mpe_enable` (bool) into `GiGridParams` (`voiceMode`, `glideMs`, `mpeEnable`), latched at the tick. Header comment: never bound after 2.5b = record. FORK-MANIFEST ControlGrid col 7 += "; 2.5b: voice_mode / glide / mpe_enable"; runner refreshes col 5.
- `readSet25b()` after `readSet25a()`; N2 / N2s / FN3 → 254 / 7.
- Done: checkpoint passes; drift 0 / 13 after the refresh; `$H` 231 / 0.

**T4. Voice fields, AmpEnv retrigger, GiNoteStack, appended pitch terms, law key (PD1, PD10–PD14)** [agent]
- Files: `Source/dsp/VoiceAllocator.h`, `Source/dsp/AmpEnv.h`, `Source/dsp/GiNoteStack.h` (new), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on T3.
- `Voice` + `keyDown` / `noteId` / `neSemis` / glide; `isKeyDown()` → `keyDown`; `startVoice` sets `keyDown = true`, `noteOff` clears both as today. `AmpEnv::retrigger` + gain ramp (unit-callable, not wired). `GiNoteStack` (PD10). `voiceSemis (v, sp, t)` with the appended terms (all callers updated, PD11). `fillLawKey` + `pvSemis` (PD12). Hooks declared.
- Done: checkpoint passes; the agent reports the before / after of `voiceSemis` and `fillLawKey`.

### Wave 2 — voice modes (commit C1)

**T5. Sustain CC64 (PD9, R§6)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on T4.
- `sustainDown` (≥ 64). Note-off under the pedal: `keyDown = false`, gate held, counter `sustainDeferred`. Pedal-up: every counted voice with `gated && ! keyDown` → env + Mod Env `noteOff` at that sample. Poly re-strike → release-with-tail (PD9, counter `restrikeReleases`). CC121 lifts sustain (pending offs apply); CC123 releases every gated voice ignoring the pedal; CC120 as 2.5a. Hook `pedalUpHard`.
- Done: checkpoint + full `$H` 231 / 0.

**T6. Mono / Legato dispatch, `retriggerVoice`, fallback, glide (D29, D30, PD1–PD4, PD13)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/dsp/GiVoiceMod.h`.
- Depends on T5.
- Dispatch per R§2.1 + PD2. `retriggerVoice (k, …)` per R§2.2: new serial, `epoch[s] = 0`, `sched[s].reset()`, tempo burst + clock restart, scan restart, `AmpEnv::retrigger` (PD1), Mod Env `retrigger`, per-voice LFOs `prepare` at `lfoN_phase`, NR / velocity / Key update, PD4 residuals, `voicePass (k, true)` + `buildPairViews`, `quotasDirty`. **Never** `startVoice`, never `lawGain.reset` / `gFbLerp.reset` (P2). Legato: `v.note` + glide retarget + Key (PD3) + `quotasDirty`. D30 fallback: glide to the new top's `note + neSemis`, same serial. Stack under CC64 per R§6 (sustained keys leave the stack; cloud gate holds until pedal-up). Glide per PD13 incl. Poly glide-from-last. Cloud `channel` / `noteId` follow the active entry. Hooks `ampResetZero`, `gainSnap`, `legatoAsMono`, `retrigViaStartVoice`, `noPvResidual`, `fallbackRetrig`, `glideStep`.
- Done: checkpoint passes (Poly everywhere in the recipes).

**T7. D32 mode switch at the tick (PD5)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on T6.
- At the tick where the latched `voiceMode` changes: Poly → Mono / Legato keeps the max-serial counted voice, `forceReleaseVoice` the rest (5 ms; spare budget per RS7 — no new rule), rebuilds the stack from `keyDown` voices in serial order, sets `detached` per PD5. Mono / Legato → Poly and Mono ↔ Legato: no voice action. Hook `modeSwitchHard`; counter `modeSwitches`.
- Done: checkpoint + full `$H` 231 / 0.

**T8. Commit C1** [run]
- Depends on T7. `git branch --show-current` (= `main`) + `git status --short` immediately before; `git status --short -- plugins/O-GranulatorInstrument` lists only T3–T7 files; `git add plugins/O-GranulatorInstrument/Source/dsp/GiNoteStack.h`; `git commit -- plugins/O-GranulatorInstrument` with `phase: O-GranulatorInstrument 2-dsp/2.5b voice modes (CC64 sustain + release-with-tail re-strike, Mono retriggerVoice / Legato / D30 fallback, closed-form glide, D32 switch; readSet25b 254/7; harness 231/0; renders-2.4 + legacy byte-identical)`. `git log -1 --stat` only plugin paths. **No tag.**

### Wave 3 — MPE + note expression

**T9. MPE lower zone + RPN (PD7, PD8, D31)** [agent]
- Files: `Source/dsp/GiMpe.h` (new: per-channel `mpeBend / mpePressure / mpeSlide[16]`, `memberRange`, `memberCount`, RPN policy), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on T8.
- `MidiRPNDetector` fed always; routing branch only when `gp.mpeEnable`; 2.5a globals and their write paths untouched (RS10). `initVoiceMod` reads member initial values under MPE. Pitch term `mBend·memberRange` (PD11 slot). Hook `mpeBroadcast`; counters `rpnMessages`, `memberWrites`.
- Done: checkpoint + full `$H` 231 / 0.

**T10. NE stamped path (PD6)** [agent; runner confirms the threading fact first]
- Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on T9. Runner first reads the patch / wrapper and records where `onVst3RawEvent` is called (audio thread, same `process` call) → else **S8**.
- Raw-event copy, `processBlockBypassed` override, `GiNeEvent` conversion, merged cursor (MIDI first at a tie), per-block NoteOn correlation, tuning writes to voices + stack entries, `neSemis` into the glide target. Hook `neModuleTable`; counters `neApplied`, `neDropped`.
- Done: checkpoint + full `$H` 231 / 0; module untouched (`git diff --quiet -- modules/tuning/note-expression`).

### Wave 4 — gates

**T11. `engine_gates_25c.h` — voice modes, glide, switch, sustain (33 rows)** [agent]
- Files: `tests/render-harness/engine_gates_25c.h` (new, `eg25c::runAll()`).
- Depends on T10.

  | ID | Stimulus | Pass | Control |
  |---|---|---|---|
  | VM1 | Mono, hold C4 vel 0.4, press E4 vel 0.9 mid-decay | new serial; amp + Mod Env levels continuous (\|Δ\| ≤ one-sample slope); σ = 0 at t; LFO phase = `lfoN_phase`; NR ≠ old; velocity src = 0.9 | VM1n `ampResetZero` → continuity fails |
  | VM2 | as VM1 | amp gain linear over 5 ms to `g_v (0.9)` (≤ 1e-6) | VM2n `gainSnap` → step |
  | VM3 | Legato, same notes | serial / epoch / σ / LFO phase / NR / velocity bitwise unchanged; `v.note` + glide + Key moved (PD3) | VM3n `legatoAsMono` → fails |
  | VM4 | both modes: note-on with no key held; note-on during the release tail | full retrigger, new serial; release-tail case = same slot from current level (PD2) | — |
  | VM5 | Mono retrigger with coherence on | `lawGain` / `gFbLerp` continuous across t | VM5n `retrigViaStartVoice` → snap |
  | VM6 | Mono retrigger, per-voice LFO → s1_level + vel → s1_pan | per-cell \|Δ\| ≤ 1.05 × 20 ms ramp bound | VM6n `noPvResidual` → one-cell step |
  | FB1 | Mono + Legato: hold C4, E4, release E4 | glide to C4; serial same; env stage unchanged; `fallbacks` = 1 | FB1n `fallbackRetrig` → serial changes |
  | FB2 | 3 keys; release a non-top key; then release all | stack-only removal; last release releases the cloud | — |
  | FB3 | Legato off+on at one sample, both host orders, blocks 32 / 4096 | off-first = retrigger, on-first = legato, identical across block sizes (PD16) | — |
  | GL1 | Mono glide 300 ms C4 → C6 | every spawn's semis == closed form ≤ 1e-9 st | GL1n `glideStep` → fails |
  | GL2 | 1 st vs 24 st, same `glide` | identical N (constant time) | — |
  | GL3 | retarget at 40 % of a glide | continuous at t_event; new N from there | — |
  | GL4 | Poly glide 200 ms: first note, then a second | first: no glide; second starts at the first's pitch | — |
  | GL5 | `glide` 0, Mono overlapping notes | no glide state written; spawn semis exact jump | — |
  | GL6 | glide over 2 oct, `pitch_rand` 0, coherence on | `lawTarget` == fresh `pairLaw` every tick | GL6n `pvKeyOff` → stale |
  | MS1 | 16 Poly voices → Mono at a tick | newest kept (serial); 15 forced (5 ms); 0 voices active after 1 s except the cloud; stack == physical keys | — |
  | MS2 | Mono → Poly with the cloud sounding, then 3 new notes | cloud continues bitwise; new notes allocate | — |
  | MS3 | Mono ↔ Legato with a held cloud | bitwise == no switch until the next note event | — |
  | MS4 | PD5: kept voice pedal-held, older key held | detached; next note-on = retrigger on the cloud slot; held-key off does not release | — |
  | SU1 | Poly, CC64 down, note-off, pedal-up at +1 s | release starts at the pedal-up sample (± 0) | — |
  | SU2 | at the polyphony limit with sustained voices | victim order: sustained = "no key down", not "released" | — |
  | SU3 | re-strike a sustained C4 (same ch) | old voice releases with tail; new voice fresh; `restrikeReleases` = 1; same note on another channel untouched | — |
  | SU4 | Mono under CC64: release top key, release last key | fallback skips sustained keys; gate holds until pedal-up | — |
  | SU5 | CC121 / CC123 / CC120 under the pedal | CC121 lifts sustain (pending offs at that sample); CC123 releases all + clears stack; CC120 force + clear | — |
  | SU6 | polyphony 16 → 4 + CC64 + Poly → Mono in one tick | slots ≤ 20; no stuck voice after 2 s; no NaN | — |

  Rows: 25 + 8 controls (VM1n, VM2n, VM3n, VM5n, VM6n, FB1n, GL1n, GL6n) = **33**.

**T12. `engine_gates_25d.h` — MPE, NE, clicks, invariance, bypass (36 rows) + `--cpu25b`** [agent]
- Files: `tests/render-harness/engine_gates_25d.h` (new, `eg25d::runAll()`, `eg25d::cpu25b()`), `tests/render-harness/engine_gates_21.h` (`Session::beforeBlock`, PD19).
- Depends on T11.
  - **MP1, MP1n** — 3 notes on ch 2 / 3 / 4, member bend sweep on ch 3: voice 3 follows `memberRange·b` (≤ 1e-9 st); voices 2 / 4 bitwise == no-sweep render. `mpeBroadcast` → fails.
  - **MP2** — RPN table: RPN 0 = 12 on ch 2 → +1 bend = +12 st (48 before); 14-bit LSB 50 → 12.5 st; NRPN 0/0 interleaved → no change; RPN null then CC 6 → no change; RPN 0 on ch 1 → no change.
  - **MP3** — master + member + NE add ≤ 1e-9 st.
  - **MP4** — D31: master AT / master CC74 / poly AT under MPE → every voice's sources bitwise unchanged; the same events with MPE off move them.
  - **MP5** — a new note on ch 5 after ch 5 bend / AT / CC74 → starts at those values.
  - **MP6** — member pressure / slide move only their voice; CC121 on ch 3 resets only ch 3.
  - **MP7** — `mpe_enable` toggled with notes held: sounding values unchanged at the toggle (bitwise); next events follow the new mode.
  - **MP8** — MCM: RPN 6 = 3 on ch 1 → note on ch 6 non-member; MCM resets range to 48; MCM 0 with MPE on → no member routing, MPE still on.
  - **MR1–MR4** — 2.5a rows re-run under MPE on through member channels: source ranges (Pressure / Slide / Bend), per-voice independence, NV handover with member pressure, allocation-order invariance (slot offset 0 vs 7, bitwise).
  - **NE1, NE1c** — C#4 + NE −50 ¢ at the note-on, κ 100 %, root 60: every grain rate == `2^(0.5/12)·tapeRate/fs` within 0.01 ¢. Control: no NE → 50 ¢ away, and a mismatched noteId → no move.
  - **NE2, NE2n** — NE changed 1 s after the note-on, mid-block at 441 / 4096: only grains spawned after its sample move. `neModuleTable` → the live change is lost (fails).
  - **NE3** — two notes on one pitch, different noteIds and tunings → separate rates (`neModuleTable` would collide; asserted in NE2n's run).
  - **NE4** — NE + Legato glides to the tuned pitch; D30 fallback lands on the held key's own NE tuning.
  - **NE5** — 70 NE events in one block → 64 applied, `neDropped` 6, no allocation; events sent during a bypassed block never replay.
  - **CK5a–g** (local κ, 5 ms frames, vs same-motion references, ≤ 2.0×): Mono retrigger, Legato glide, D30 fallback, D32 switch, pedal-up release, member bend sweep, Mono velocity change.
  - **CK5an, CK5dn, CK5en, CK5gn** — hard controls `ampResetZero`, `modeSwitchHard`, `pedalUpHard`, `gainSnap` > 2.0× with `ctrl FIRED` asserted. CK5b / c / f have no constructible hard control (pitch is latched per grain, so a step makes no click by construction) → bound only, noted in SUMMARY and HANDS-ON (the 2.4 CK2 / CK3 precedent).
  - **BS1, BS1n** — the full script (CC64, Mono + Legato with glide, D32 switch, MPE member bend / AT / CC74, RPN, NE mid-block via `beforeBlock`) at 32 / 441 / 512 / 4096 / variable → 0 diffs; seed + 1 differs.
  - **AL1** — 0 allocations across the BS1 script + 130 held keys (stack overflow, `stackDropped` > 0) + 70-event NE block; every event counter > 0 (liveness).
  - **TB5** — true bypass: Poly, glide 0, MPE off, no CC64: renders-2.4 + legacy byte-identical via the shipping path.
  - **TB6** — MPE off with RPN / member-channel traffic already parsed: eg25a source rows (`engine_gates_25a.h:451-454`) and NV1 bitwise unchanged.
  - Rows: 9 MP + 4 MR + 7 NE + 11 CK + 2 BS + 1 AL + 2 TB = **36**.
  - `cpu25b()`: C25mpe INFO per PD18 with its witness.

**T13. `main.cpp` wiring** [agent]
- Files: `tests/render-harness/main.cpp`.
- Depends on T12. `#include` eg25c / eg25d after eg25b; `runAll()` in order; `--cpu25b` (rc = INVALID count); banner + usage.

**T14. Build + full harness** [run; agent fixes]
- Depends on T13.
- BG build; `$H` → **300 PASS / 0 FAIL**, 0 NOT FIRED (231 + 33 + 36) → `$S/h-25b.log`; duplicate-ID set ⊇ `$S/dup-25a.txt` with no new duplicates; TB0 / TB3 / TB4 / TB5 / TB6, RB1 7/7, FR1 14/14; param-dump == `$S/pd-25a.txt`; drift 0 / 13 + NC-a / NC-b fire; X1 (compdb) 0 parent refs.
- VM / FB / GL / MS / SU / MP / MR / NE / BS / AL failure → **S5**; CK → **S6**. **Never loosen a threshold or re-pin a render.**

**T15. Parity carried** [run]
- Depends on T14. BG: `tests/parity/run.sh` (incremental); `$H --parity` **168 / 0**, GV 2 / 0; residuals == `$S/res-base.txt` to 0.1 dB. Any move → **S3**.

### Wave 5 — CPU regression

**T16. `--cpu25a` + `--cpu25b`** [run]
- Depends on T15 (nothing else running; Logic closed).
- `$H --golden-dir tests/golden --cpu25a` alone (BG) → `$S/cpu25a.log`: **C25m ≤ 40 %**, **C25tm ≤ 12 %**, witnesses valid; Δ vs T2 printed (INFO). Then `--cpu25b` alone → C25mpe INFO + law-recompute witness. NOVERDICT ≤ 3 tries, then STOP. Valid-witness FAIL → **S2**.

### Wave 6 — install, validators, hands-on, bookkeeping

**T17. Install + Standalone + validators + X2** [run]
- `./scripts/build-and-install.sh O-GranulatorInstrument` (BG; clears the AU cache, sweeps both variants). `ninja -C build OuariconGranulatorInstrument_Standalone` (BG); `.app` newer than T14's harness and alive after 6 s. `auval -v aumu OuGI OuDv` PASS + ZZZZ control. pluginval 10 VST3 + AU, one at a time, BG, `--strictness-level 10 --timeout-ms 600000`, 2 ch in, Logic closed. X2: `nm -C` on installed binaries + `$H` → 0 `ogran::`.

**T18. `HANDS-ON-2.5b.md` (deferred)** [agent]
- Files: `stages/2-dsp/HANDS-ON-2.5b.md`, HANDS-ON-2.5a layout, "Deferred with HANDS-ON-2.1–2.5a" banner. Rows (Standalone + Logic AU + Dorico VST3):
  1. Mono: overlapping phrases retrigger from the current level — no click, envelope feel.
  2. Legato: smooth pitch-only moves; nothing restarts.
  3. Fallback glides back to the held key in both modes.
  4. Glide 0 / 50 / 300 / 2000 ms: constant time feel; Poly glide-from-last.
  5. Mode switch while holding a chord: one voice survives, no click, no stuck note.
  6. Sustain pedal: held notes, re-strike under pedal releases with tail; CC121 / transport stop.
  7. MPE controller (Seaboard / Linnstrument / Osmose): per-note bend, pressure, slide independent; master bend adds.
  8. MPE bend range from the controller's RPN 0.
  9. Dorico NE: quarter-tone preview + playback at the right pitch; NE + glide in Legato.
  10. CK5b / c / f (no hard control constructible) — listening check.
  11. 2.1–2.5a patches in Poly / MPE off sound unchanged.
- Not self-certified.

**T19. SUMMARY, STATUS, commit C2** [agent writes, run commits]
- Files: `stages/2-dsp/SUMMARY-2.5b.md`, `.planning/STATUS.md` (`stage_2_phase_2_5b_executed_hands_on_pending`; next = 2.5b verify).
- SUMMARY: every row with margin + control; counts vs targets (harness 300, parity 168 + GV 2); glide golden error; κ table with controls; NE rate error; C25m / C25tm / C25mpe + Δ vs T2; PD1–PD20 as decided; the T10 threading fact; PD16 tie-order note; every deviation with its reason.
- **Commit C2:** branch + status immediately before; plugin status lists only this plan's files (anything else → STOP); `git add` new paths (`GiMpe.h`, `engine_gates_25c.h`, `engine_gates_25d.h`, `SUMMARY-2.5b.md`, `HANDS-ON-2.5b.md`); `git commit -- plugins/O-GranulatorInstrument`. Never `-A` / `-a`. `git log -1 --stat` only plugin paths. **No tag.**

---

## Dependency graph

```
T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 (C1) → T9 → T10 → T11 → T12 → T13 → T14 → T15 → T16 → T17 → T18 → T19 (C2)
           (checkpoint after each of T3–T10; full $H after T5, T7, T9, T10)
```

- T2 finishes before any `Source/` edit.
- T3 → T10 strictly sequential (shared `Engine.{h,cpp}`).
- T16 runs alone after every build and parity job.

## Execution notes

- **Agent split.** dsp-agent writes T3–T7, T9–T13 code, T18, T19's SUMMARY. The runner does every build, checkpoint, gate, validator, hash refresh and both commits.
- **Background:** ninja, `run.sh`, `build-and-install.sh`, the Standalone link, pluginval, cold auval, `--cpu25a`, `--cpu25b`, the full harness (600 s watchdog).
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument 2-dsp`.

## Success criteria (CONTEXT-2.5b gate)

| Criterion | Proven by | Target |
|---|---|---|
| Mono = new serial, retrigger from current level, scan / LFO restart (D29) | VM1, VM2, VM4, VM5, VM6 (+ controls) | continuity; controls fire |
| Legato = same serial, pitch only (D29, FUNC-05) | VM3, VM3n | bitwise |
| Fallback glides back without retrigger (D30) | FB1, FB1n, FB2, NE4 | serial kept |
| Glide golden, constant time, Poly glide-from-last, glide 0 exact, law tracks pitch | GL1–GL6, GL1n, GL6n | ≤ 1e-9 st |
| Sustain: deferred off, keyDown rule, re-strike with tail (PD9), Mono stack excludes sustained keys | SU1–SU6 | exact sample |
| Mode switch per D32, no stuck note | MS1–MS4, CK5d / CK5dn | 0 stuck |
| MPE member expression independent; RPN 0 / 6; master adds; D31 ignores | MP1–MP8, MP1n, MR1–MR4 | bitwise / ≤ 1e-9 st |
| NE −50 ¢ quarter-sharp; live; noteId-keyed; adds to MPE bend | NE1–NE5, NE1c, NE2n | ≤ 0.01 ¢ |
| `mpe_enable` off: 2.5a goldens unchanged; true bypass byte-identical; param-dump identical; drift 0 / 13 | TB0 / TB3 / TB4 / TB5 / TB6, checkpoints, carried 231 rows | 14/14, identical, 0 / 13 |
| Click gates vs same-motion controls (QUAL-01) | CK5a–g + 4 hard controls | ≤ 2.0×; controls > 2.0× |
| Block-size null with the full script + seed control | BS1, BS1n | 0 diffs / differs |
| Allocation gate across MIDI / MPE / NE (PERF-01) | AL1 | 0 allocations |
| CPU regression | `--cpu25a` C25m, C25tm; C25mpe INFO | ≤ 40 %, ≤ 12 % |
| Carried | harness, parity, GV | **231 → 300 / 0**; parity 168 / 0; GV 2 / 0 |
| auval PASS; pluginval 10 VST3 + AU; parent code excluded | T17, X1, X2 | all PASS |
| HANDS-ON-2.5b written, deferred | T18 | 11 rows |

**Requirements this phase moves** (verify sets the final status): **FUNC-05** pending → automated arm complete (feel = HANDS-ON); **DSP-06** partial → automated arm complete (MPE form in); **PERF-01** MIDI / MPE / NE arm in; **QUAL-01** voice-mode / sustain / MPE click gates in. COMPAT-02 stays Stage 4.

## Pitfalls → task + gate (RESEARCH P1–P10)

| # | Pitfall | Task | Gate |
|---|---|---|---|
| P1 | Velocity gain step on a Mono retrigger | T4 / T6 (PD1) | VM2 + VM2n, CK5g + CK5gn |
| P2 | `startVoice` used for a Mono retrigger | T6 | VM5 + VM5n, VM1 + VM1n |
| P3 | Law key misses per-voice pitch | T4 (PD12) | GL6 + GL6n |
| P4 | `voiceSemis` term reorder | T4 (PD11) | checkpoint T4, TB5 |
| P5 | NE via the module table | T10 (PD6) | NE2n, NE3 |
| P6 | NE list not cleared on bypassed blocks | T10 | NE5 |
| P7 | `kTuningTypeID` literal | T10 | NE1 |
| P8 | MIDI tie order | PD16 | FB3, BS1 |
| P9 | Detached cloud / note-on while releasing | T6 / T7 (PD2, PD5) | VM4, MS4 |
| P10 | CC123 vs pedal | T5 | SU5 |

## Stop-and-ask conditions (execute; never pre-decided)

- **S2 — CPU regression.** C25m > 40 % or C25tm > 12 % with valid witnesses. STOP with the per-tick profile (the likely term is the PD12 law recompute).
- **S3 — bypass broken.** A checkpoint, TB0 / TB3 / TB4 / TB5 / TB6, RB1 / FR1, a carried 2.5a row or a parity residual moves. STOP and find the statement-shape change. **Never re-baseline.**
- **S5 — law arm** (VM / FB / GL / MS / SU / MP / MR / NE / BS / AL). Structural; STOP with the diagnosis. Never loosen.
- **S6 — clicks.** A CK5 row over 2.0×, or a hard control that cannot fire after one stimulus fix. STOP: a ramp-length change is a D29 / D32 amendment — consent required.
- **S8 — scope.** `onVst3RawEvent` not on the audio thread inside `process`; a note-expression module edit appears necessary; a carried row other than N2 / N2s / FN3 needs a re-scope. STOP: the user's call.
- **Inherited:** T1 contract / late-change mismatch; INVALID after one stimulus step; three NOVERDICTs; unexpected paths at either commit.

## Risks

- **R1 Serial semantics.** Every per-voice draw keys on the serial; VM1 / VM3 + MR4 pin new-vs-kept.
- **R2 Bypass shape.** Eight engine tasks touch the audio path; per-task checkpoints + full `$H` after T5 / T7 / T9 / T10 attribute any move.
- **R3 NE threading.** The stamped path relies on `onVst3RawEvent` running in the audio `process` call; T10 checks it before writing.
- **R4 Vacuous click controls.** Three CK rows have no constructible hard control; they are bounded and listed for HANDS-ON rather than given a control that cannot fire.
- **R5 Law recompute cost.** Gliding / bending voices recompute the law per tick; C25mpe INFO measures it.
- **R6 Concurrent sessions.** Foreign dirty paths exist; both commits path-scoped with branch + status re-checked immediately before.
- **R7 Test hooks in shipping.** All under `OGI_TEST_HOOKS`; X2 and validators run on the shipping build.
