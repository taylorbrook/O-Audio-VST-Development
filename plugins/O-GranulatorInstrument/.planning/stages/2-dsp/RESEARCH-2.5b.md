# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.5b — RESEARCH

**Researched:** 2026-11-09
**Scope:** Phase 2.5b only (D29–D32): Poly / Mono / Legato with glide, sustain (CC64), the MPE lower-zone parser and member-channel rerouting of Pressure / Slide / Bend, VST3 note expression (per-note cents), and the law-gate proof arms. There is no parent equivalent (O-Granulator @ `82406fce` is an effect), so there is no parity arm.
**Confidence:** HIGH on code facts (current tree at `c9341a94`, the note-expression module 1.1.2, the JUCE 8.0.15 patch, JUCE `juce_MidiRPN.cpp`). MEDIUM on the NE stamped path (§4), which is designed, not built. Every claim is tagged **VERIFIED** (read this session, `path:line`) or **INFERRED** (reasoning, not run).
**Inputs read:**
- `CONTEXT-2.5b.md` (D29–D32, Defaults, 7 open questions); `RESEARCH-2.5a.md` (format); `research/ARCHITECTURE.md` §2, §3, §4, §10, Algorithm §1–§2, risks "MPE / note expression" and "Block-size invariance"; `ROADMAP.md` § 2.5b
- Plugin: `Source/dsp/{Engine.h,Engine.cpp,VoiceAllocator.h,AmpEnv.h,GiVoiceMod.h,ScanPlayhead.h,MidiCursor.h,GiConstants.h}`, `forked/{ControlGrid.h,FORK-MANIFEST.md}`, `Source/{GiParamIds.h,PluginProcessor.h,PluginProcessor.cpp}`; `tests/render-harness/{main.cpp,engine_gates_21.h,engine_gates_25b.h}` (+ grep of eg22–eg25a for MIDI usage)
- `modules/tuning/note-expression/{module.yaml,cpp/NoteExpression.h,cpp/NoteExpression.cpp,cpp/vst3/NoteExpression_VST3.cpp}`; `scripts/juce-patches/note-expression-juce-8.0.15.patch`; `~/JUCE/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h` (patched); `~/JUCE/modules/juce_audio_basics/midi/juce_MidiRPN.cpp`; VST3 SDK `ivstnoteexpression.h`
- References: `plugins/O-simpleWavetable/Source/{MonoStack.h,PluginProcessor.cpp:540-600}`; `plugins/O-Lyrica/Source/{HarpSynthVoice.cpp,PluginProcessor.cpp}` (NE consumer); `plugins/O-Contrabass/tests/render-harness/main.cpp:1431`, `plugins/O-Bowed/tests/render-harness/main.cpp:238` (NE harness injection)
- Memory: `project_o_lyrica_spike_reference`, `critical_juce_synthesiser_voice_steal_allocates`, `pattern_click_kappa_masked_by_level_change`, `pattern_gate_stimulus_below_threshold_is_vacuous`, `pattern_float_and_double_linear_ramp_arrive_a_grid_apart`, `pattern_run_flag_past_dirty_check_latches`

## User Constraints (CONTEXT-2.5b; binding)
- **D29:** Mono = new serial; amp env + both Mod Envs retrigger **from the current level**; scan (Retrigger mode) restarts; per-voice LFOs reset to `lfoN_phase`; Note Random redrawn; velocity source updates; glide moves pitch. Legato = **same serial**, nothing retriggers, only the glide target moves. A note-on with no key held retriggers normally in both modes.
- **D30:** releasing the top key with a lower key held glides back (last-note priority) with no retrigger, both modes. Releasing the last held key releases the cloud.
- **D31:** MPE master (ch 1) bend × `pitch_bend_range` adds to every note. Master AT, master CC74 and poly AT are ignored with MPE on. MPE off = the 2.5a behaviour, unchanged.
- **D32:** Poly → Mono / Legato keeps the newest counted voice (serial kept), force-releases the rest over 5 ms, rebuilds the stack from physical keys. Mono / Legato → Poly: the sounding voice continues as a poly voice. Mono ↔ Legato: no voice action.
- **Defaults:** MonoStack port keyed (channel, note); glide linear in semitones, constant time, latched per event, in the §10 spawn formula, `glide` 0 = exact jump; CC64 ≥ 64 holds the gate, keyDown = physical key; MPE lower zone, member range = last RPN 0 on any member channel (default 48), RPN 6 lower zone only; NE by noteId, latched at note-on and updated live, adds to MPE bend; true bypass (Poly, glide 0, MPE off, no CC64) → renders-2.4 + legacy byte-identical, every 2.5a golden with MPE off unchanged, harness 231/0, `--parity` 168/0 + GV 2/0, param-dump identical, drift 0/13; the engine never writes APVTS; click gates (local κ vs same-motion controls); block-size null with the full script + seed control; allocation gate across the whole MIDI / MPE / NE script; C25m ≤ 40 %, C25tm ≤ 12 % regression; HANDS-ON-2.5b written and deferred; Standalone rebuilt explicitly.

## Project Constraints
- Trunk on `main`. Path-scoped commits only (`git commit -- plugins/O-GranulatorInstrument`). Re-check `git branch --show-current && git status --short` right before each commit. Foreign dirty paths at research time (**VERIFIED**, session-start snapshot): `.claude/agent-memory/{dsp,gui}-agent.md`, `.planning/STATE.md`, `modules/registry.yaml`, untracked `.claude/compaction-snapshot.md`, `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`. Never sweep them in. Never tag.
- Build with `./scripts/build-and-install.sh O-GranulatorInstrument`. Long commands go in the background (600 s watchdog).
- `modules/tuning/note-expression` is a shared module. **Do not edit it** for 2.5b (§4 needs no module edit).

---

## Summary (what the plan must absorb)

1. **Nothing of 2.5b exists yet except two seams.** `voice_mode`, `glide` and `mpe_enable` are unbound (`forked/ControlGrid.h:196`). `applyEvent` is the 2.5a non-MPE form (`Engine.cpp:1833-1914`). Two seams are ready: `ModEnv::retrigger` (from the current level, `GiVoiceMod.h:71-76`) and the per-voice `pressure` / `slide` / `bendSrc` fields (`GiVoiceMod.h:44`, written in `Engine.cpp:1857-1913`). **VERIFIED.**
2. **`AmpEnv` has no retrigger, and its velocity gain would step.** `noteOn` sets `level = 0` (`AmpEnv.h:71-76`). The output is `level · gain` with `gain = g_v` latched at note-on (`:155`). `vel_to_amp` defaults to 70 % (`GiParamIds.h:244`). So a Mono retrigger that updates velocity steps the amplitude by `g_new / g_old` even when the level is continuous. That is a click the D29 gate would catch. **Add `AmpEnv::retrigger (p, fs, newGain)`** (attack from the current level, ModEnv precedent) **plus a linear gain glide over 5 ms** (`kStealMs`). A `gainSnap` hook is the same-motion control. **VERIFIED** (code) / **[decision for plan]** (5 ms gain glide vs keeping the first note's gain for amp only).
3. **Q1: write `GiNoteStack.h`, don't fork `MonoStack`.** `MonoStack` stores bare `int` notes, capacity 32 (`O-simpleWavetable/Source/MonoStack.h`). We need (channel, note) keys, and each entry also has to carry `noteId` + `neSemis`, because a D30 fallback must land on the held key's own NE tuning. `MonoStack` is not in `forked/`, so a new Gi file needs no FORK-MANIFEST row and drift stays 0/13. Keep its semantics (push moves to top, last-note priority, remove by key, drop-oldest on overflow, no allocation). Use capacity **128** (ARCH §3), not 32. **VERIFIED** / **INFERRED**.
4. **Q2: the note-expression module cannot do "by noteId, updated live" as written.** Its table is **pitch-indexed** (`PendingTuningTable = std::array<std::atomic<double>,128>`, `NoteExpression.h`). Correlation only finds a noteId whose NoteOn is **in the same block** (`NoteExpression_VST3.cpp:216-235`). A later-block NE for a held note gets `pitch < 0` and is dropped. `applyPendingTuning` does `exchange(0.0)`, so it is consumed once at note-on. That is the suite's O-Lyrica model (note-on latch only). **Recommended path (no module edit):** `GiVst3Extensions` (plugin-local, `PluginProcessor.h:47`) overrides `onVst3RawEvent`, calls the base, and copies each raw event into a fixed `std::array<Vst3RawEvent, 64>`. The patched struct already carries `sampleOffset`, `noteId`, `pitch`, `channel`, `typeId`, `value` (`juce_VST3ClientExtensions.h:72-88`). The engine then gets a stamped NE list. It correlates the NoteOn noteId to the MIDI note-on by (offset, channel, pitch), stores `noteId` on the voice and the stack entry, and applies `kTuningTypeID` (= **2**, `ivstnoteexpression.h:47-49`) values at their sample. The result is sample-stamped, live, noteId-keyed and block-size invariant. §4. **VERIFIED** (module + patch) / **INFERRED** (design).
5. **Q3: Legato is a target change, not an event, for the schedulers.** It leaves `serial`, `epoch[s]`, `sched[s]`, `tclk`, `scan[s]`, the Mod Envs, the per-voice LFOs and the source values alone. It writes `v.note` (the target) and the glide state. It also sets `quotasDirty`, because `pairCoherence` reads the voice rate at the tick (`Engine.cpp:433`). Mono is a **`retriggerVoice(k, …)`** that runs the per-stream part of `startVoice` (`Engine.cpp:1960-1977`: sched reset, tempo burst + clock restart, scan restart) with a **new serial** and `epoch[s] = 0`. It must **not** run `startVoice`'s law-gain resets (`:1980-1990`, `lawGain.reset` / `gFbLerp.reset`). Those snap the per-pair gain on a sounding voice, which is a click. §2.
6. **Q4: glide is a closed form from absolute time, evaluated per spawn and per tick.** It needs no ramp object. Per voice: `{from, to, t0, N}`, `g(t) = (from − to)·max(0, 1 − (t − t0)/N)` in double, with `t` = the spawn sample (`spawn` `tAbs`) or the tick sample (law). It is block-size invariant by construction, because no accumulated state means no float-vs-double grid drift. A retarget mid-glide takes `from` = the current closed-form value. **The law key must see per-voice pitch:** `fillLawKey` holds only the global `bend` (`Engine.cpp:529`). Add `pvSemis[k]` for each counted voice (glide + member bend·range + NE). They are all +0.0 in bypass, so the comparisons and the output stay the same. A gliding or bending voice then recomputes the law each tick. §3.
7. **Q5: the spare budget is not a constraint.** "4 spares" is a guarantee at note-on only. `forceReleaseVoice` never checks it (`Engine.cpp:2035-2050`), `enforcePolyphony` already makes up to 15 stolen voices at once (`:970-985`), and 20 slots ≥ 16 counted + stolen always holds, because `noteOn` ends the quietest stolen voice when no slot is free (`:1922-1941`). D32 with 16 voices leaves 1 counted + 15 stolen in 16 slots. Sustained voices are counted voices. Sustain + polyphony lowering + D32 in one tick needs no new rule. **VERIFIED** / **INFERRED**.
8. **Q6: use `juce::MidiRPNDetector` for the RPN state machine.** It is a JUCE framework class (no fork), with fixed `states[16]`, `noexcept`, and a POD `std::optional` result (no allocation, `juce_MidiRPN.cpp`). It already handles NRPN interleave (CC 99 / 98 set `isNRPN`), RPN null (127/127 → parameter 16383, never 0 or 6), data entry before a select (MSB ≥ 0x80 → no message), and 14-bit values (CC 6 sends 7-bit; a later CC 38 re-sends 14-bit). RPN 0 / RPN 6 on the master channel are handled by our policy layer (§5). **VERIFIED** (JUCE source) / **[decision for plan]**.
9. **Sustain changes the meaning of `gated`.** Today `isKeyDown() = active && gated` (`VoiceAllocator.h:76`), and `noteOff` matches on it (`Engine.cpp:2001`). Add `Voice::keyDown` (physical). `gated` keeps meaning "gate open". `isKeyDown` reads `keyDown`. Without CC64 the two are always equal, so `chooseVictim` stays bitwise. A note-off under the pedal clears `keyDown` and leaves `gated`. Pedal-up releases every voice with `gated && ! keyDown`. §6.
10. **MPE needs per-channel state next to the 2.5a globals, not in place of them.** Keep `bend` / `chanPressure` / `chanSlide` and their 2.5a write paths exactly as they are, so MPE off stays bitwise. Add `mpeBend[16]`, `mpePressure[16]`, `mpeSlide[16]` and `memberRange` (48), written on every event. They are read only when `mpe_enable` is on: in `initVoiceMod` (initial values) and in member-channel writes. §5.
11. **Pitch formula: append the new terms at the end of the existing sum.** `voiceSemis` today is `key = note − root − fine/100 + bend·range` (`Engine.cpp:278-280`). Adding `+ g(t) + memberBend·memberRange + neSemis` after that leaves the result bitwise identical when each term is +0.0, because `x + 0.0 == x`. ARCH §10's term order (`note + g + b + ne − root − fine`) would re-round the 2.5a path. **Keep the current order and append** [correction to ARCH §10 term order; values equal]. Under MPE the master bend (`bend`, ch 1) stays in its 2.5a slot. `voiceSemis` gains a `t` argument.
12. **Harness:** N2 / N2s / FN3 assert `bound == 251 && touched == 10` (`engine_gates_21.h:2138`, `engine_gates_22.h:1471`, `engine_gates_23.h:359`). Once `voice_mode` / `glide` / `mpe_enable` are read → **`readSet25b()` = 254 bound / 7 unread (rec_*)**, and the three rows are re-scoped (the PD15 precedent). No current row sends CC64, RPN, member channels or NE (grep: every scripted CC / bend / AT is on ch 1, `engine_gates_25{a,b}.h`). The bypass claim is therefore safe at the script level. **VERIFIED.**
13. **CPU: no new verdict.** The only new per-tick cost is the law recompute while a voice glides or member-bends (the same path as routed pairs, `Engine.cpp:1042`). C25m / C25tm re-run as regression rows. An INFO row (16 MPE voices, member bend sweeping + glide) is recommended to measure the law-recompute term. **INFERRED.**

---

## 1. Exact current state — **VERIFIED**

| piece | today (`c9341a94`) | 2.5b |
|---|---|---|
| `ControlGrid.h` (ADAPTED) | binds 251 IDs; `voice_mode`, `glide`, `mpe_enable` never bound (`:196`) | + `voiceMode`, `glideMs`, `mpeEnable` fields + pointers (latched at the tick) → FORK-MANIFEST row re-hash (ADAPTED, the 2.5a precedent, row `:26`) |
| `Voice` | `gated` = key down = gate (`VoiceAllocator.h:60, 75-76`) | + `keyDown`, `noteId` (−1), `neSemis`, glide `{from, to, t0, N}`; `isKeyDown()` → `keyDown` |
| `AmpEnv` | `noteOn` from 0, `gain` latched (`:71-76, :155`) | + `retrigger (p, fs, newGain)` + 5 ms gain glide (Summary 2) |
| `ModEnv` | `retrigger` already exists (`GiVoiceMod.h:71-76`) | used by Mono |
| `applyEvent` | note on / off, ch bend → `bend` + every `bendSrc`, CC 1/33/74/120/121/123, ch AT, poly AT (`Engine.cpp:1833-1914`) | + CC64, RPN via `MidiRPNDetector`, MPE routing branch, Mono / Legato dispatch |
| pitch | `voiceSemis (v, sp)`: `note − root − fine/100 + bend·range` (`:275-284`) | `voiceSemis (v, sp, t)` + `g(t) + mBend·mRange + ne` (appended) |
| law key | 4 × 16 stream fields + `bend` + `range` + 3 hooks (`:497-534`) | + `pvSemis[kVoiceSlots]` (§3.3) |
| NE | `drainAndUpdate()` at the top of `processBlock` (`PluginProcessor.cpp:215`); the table is never read | + stamped raw-event copy in `GiVst3Extensions` → `Engine::process` (§4) |
| params | all 3 already in the 261 (`params.tsv:6,7,9`) | no new params, no new `<Engine>` keys → param-dump identical |

## 2. Voice modes (D29, D30, D32; Q3)

### 2.1 Event dispatch (Mono / Legato)
"The cloud" = the single counted voice in Mono / Legato (invariant: ≤ 1 counted voice outside a D32 transition tick). Physical keys live in the stack.

| event | condition | action |
|---|---|---|
| note-on | stack non-empty **and** the cloud is counted + gated | push key. **Mono:** `retriggerVoice` (new serial, envs from current, scan / sched / tempo restart, LFO re-prepare, NR / velocity / Key update, gain glide), glide → new target. **Legato:** `v.note` = target, glide retarget, `quotasDirty`. |
| note-on | stack empty, **or** the cloud is releasing (no key down, gate closed) | push key. Full retrigger (D29 "normally", new serial) **on the same slot from the current level** if a counted cloud exists, else `startVoice` from 0. **[decision for plan]**: same slot from the current level (ARCH §3 "no reset to 0" in Mono) vs steal + fresh voice. Recommend the same slot: no slot churn, and the click gate covers it. |
| note-off | key is the top and the stack keeps ≥ 1 key | D30: glide to the new top (its `note + neSemis`). No retrigger, same serial, `quotasDirty`. |
| note-off | a non-top key | remove from the stack only |
| note-off | the last key | release the cloud (unless CC64 is down: the gate holds, §6) |

The cloud's `noteId` / `channel` follow the active stack entry. Member expression and NE follow the sounding key (CONTEXT Q1).

### 2.2 What `retriggerVoice` touches (Mono) vs Legato — **INFERRED** from `startVoice` / `initVoiceMod`

| state | Mono retrigger | Legato | note |
|---|---|---|---|
| `serial` | `++serialCounter` | kept | spawns of the old serial are dropped by `spawn`'s check (`Engine.cpp:2331`); grains in flight keep rendering under `envBuf[k]` |
| `epoch[s]`, `sched[s]` | 0, `reset()` (first onset at the retrigger sample) | kept | draws key on `(serial << 24) \| (epoch + idx)` (`:2393`) |
| `tclk`, burst | `restart (t+1)` + burst (PD1) | kept | |
| `scan[s]` | `restart (t, rate)` | kept | Free-run scan is shared, untouched in both |
| amp env | `retrigger` from current + gain glide | kept | Summary 2 |
| Mod Envs | `retrigger` (`GiVoiceMod.h:72`) | kept | |
| per-voice LFOs | `prepare (…, lfoN_phase)` | kept | |
| NR / Velocity src | redrawn (keyed on the new serial) / new | kept | |
| Key src | new note | **[decision for plan]**: D29 does not mention Key. Recommend Key follows the target note in Legato too (it is a pitch source). A route on level / pan would then step per cell. |
| `lawGain`, `gFbLerp` | **not reset** (keep the CellLerp; `quotasDirty`) | not reset (`quotasDirty`) | `startVoice` resets them (`:1980-1990`): only correct for a voice starting from 0 |
| pair views / offsets | `voicePass (k, true)` + `buildPairViews` | next tick | |
| newest-voice rule | new serial → handover + 20 ms ramp, automatic (`updateNewest`, `:1363-1426`) | no handover (same serial) | matches CONTEXT "Legato is not a handover" with no new code |

**Per-voice offset step on Mono retrigger.** An LFO re-prepared to `lfoN_phase`, or a Velocity / NR change, steps the per-voice offsets. On spawn-latched destinations that is harmless (grains latch). On **level / pan** (pair mixer, `pairGL/GR/Lv` CellLerps) it ramps over one cell only (0.67 ms) → a possible κ hit on the "Mono retrigger" click row when such a route exists. **Recommend:** at a Mono retrigger, push a per-voice residual (old − new contribution) for each per-voice level / pan destination through the existing `pushResidual` (`GiVoiceMod.h:279`, 20 ms, the 2.5a route-change path). Measure the row with and without it. **[decision for plan]**.

### 2.3 D32 mode switch (at the grid tick)
`voiceMode` is latched at the tick (`gp.latch`), so the switch lands on an absolute sample: block-size invariant. Poly → Mono / Legato: keep = max-serial counted voice. `forceReleaseVoice` the rest (5 ms). Rebuild the stack from voices with `keyDown`, in serial order (oldest at the bottom).
- **Edge (plan must pin):** if the kept voice is sustained or releasing (no key down) while an older key is still physically held, the stack top is not the sounding pitch. Recommend: the cloud is "detached" until the next note event. A note-on is then a full retrigger (2.1 row 2), and a note-off of the held key does not release a pedal-held cloud. **[decision for plan]**.
- Mono → Poly: nothing (the glide closed form runs out on its own). Mono ↔ Legato: nothing.

## 3. Glide (Q4)

### 3.1 Law — **INFERRED**
Per voice: `glFrom`, `glTo` (key semitones = `note + neSemis`), `glT0` (event sample), `glN = glide_ms · 0.001 · fs` (latched at the event from the tick-latched `gp.glideMs`).
`g(t) = (glFrom − glTo) · max (0, 1 − (t − glT0)/glN)`; the pitch key is `note + neSemis + g(t)`.
- **Constant time:** N does not depend on the interval. **Retarget:** `glFrom` = the current `glTo + g(t_event)`, `glT0 = t_event`.
- **`glide` = 0:** no glide state is written and the term is skipped (exact jump; CONTEXT "no ramp state touched").
- **Poly with glide > 0:** a new voice gets `glFrom = lastKey` (the previous note-on's `note + neSemis`, any mode). The first note ever has no glide (`lastKey` unset).
- Evaluated at the **spawn sample** for grains (pitch is latched per grain, ARCH §10) and at the **tick** for the law (§3.3). There is no per-sample state, so 32 / 441 / 4096 / variable blocks give bitwise-identical spawn pitches.

### 3.2 Golden
The trajectory golden reads the `rate` of every spawned grain of the cloud (per-serial log, eg21 V3 precedent). Check `12·log2(rate·fs/tapeRate) − κ·…` against the closed form at its spawn sample to **≤ 1e-9 st**. Control: a `glideStep` hook (jump at t0) must fail the golden.

### 3.3 The law sees the voice's pitch — **VERIFIED** gap
`pairCoherence` → `voiceRate (v, …)` at the tick (`Engine.cpp:433`). `updateQuotas` recomputes only when `changed` (`:1037-1042`), and the key has the global `bend` only (`:529`). In Poly a voice's note never changed, and a new voice set `quotasDirty`. In 2.5b, Legato, D30 fallback, glide, member bend and NE all change one voice's rate with no dirty flag. **Fix:** append `pvSemis[k]` (counted voices; 0 otherwise) to `fillLawKey` (`kLawKeySize += kVoiceSlots`). Without that row a gliding voice keeps a stale coherence: wrong level, silently. **Gate:** a law-tracking row (glide over 2 oct with `pitch_rand` 0 and coherence on): `lawTarget` at the tick == a fresh `pairLaw` recompute, every tick. Control: a `pvKeyOff` hook.

## 4. Note expression (Q2, Q7) — the stamped path

### 4.1 Why not the module table — **VERIFIED**
- The table is pitch-keyed (128). Two notes on one pitch with different tunings collide, and correlation is not by noteId at the consumer.
- `updatePendingFromEvents` builds its noteId → pitch map **from this block's NoteOns only** (`NoteExpression_VST3.cpp:216-221`). An NE for a note started in an earlier block is dropped (`:236`). "Updated live" fails for every held note after its first block.
- The table has no sample offset: an NE is effective for the whole block.
- O-Lyrica / O-Contrabass / O-Bowed all read it once at note-on (`applyPendingTuning`, exchange-to-0). The harness precedent stores straight into the table (`O-Contrabass …/main.cpp:1431`).

### 4.2 Recommended design — **INFERRED** **[decision for plan]**
1. `GiVst3Extensions::onVst3RawEvent (e)`: call `VST3Extensions::onVst3RawEvent (e)` (the module path stays intact), then `if (n < 64) ne[n++] = e;` into a fixed array (same capacity and drop rule as the module, WR-02).
2. `processBlock`: after `drainAndUpdate()`, pass `(ne, n)` to `engine->process` and set `n = 0`. Also clear it in a `processBlockBypassed` override. Today there is none (grep, **VERIFIED**). Without one, events from bypassed blocks would replay with stale offsets later.
3. The engine walks a second cursor over the NE list, merged with `MidiCursor` by clamped offset. **MIDI first at a tie**, so the NoteOn exists before its NE. Correlation:
   - Raw `NoteOn` → remember `(offset, channel+1, pitch, noteId)` in a fixed 64-slot list for this block. When `noteOn(ch, note)` runs at sample `at`, take the first unconsumed entry with the same `(at − blockStart, ch, note)` → `v.noteId` (and the stack entry's).
   - Raw `NoteExpressionValue` with `typeId == 2` (`kTuningTypeID`; a literal in a SharedCode TU, since no Steinberg header can be included) → `semis = 240·(value − 0.5)`. Write it into every non-free voice and stack entry with that `noteId`. noteId −1 (host gives none) → ignore.
   - Initial tuning: an NE at the note-on's own sample applies before the first spawn, because `runSegment` from `at` runs after every event at `at` (`Engine.cpp` `process` loop, `:582-596`).
4. The NE term sits in the glide target (`note + neSemis`), so a Legato key change or D30 fallback glides to the tuned pitch. A live NE change on the sounding key steps the target with no glide. It moves new grains only (latched), so there is no click.
5. AU / Standalone get no raw events, so `neSemis` stays 0 there.

### 4.3 Harness injection
The Session drives the processor (`engine_gates_21.h:384-446`). Inject with `static_cast<GiVst3Extensions*> (s.p->getVST3ClientExtensions())->onVst3RawEvent (raw)` inside an `s.at (t, …)` callback, which cuts a block at `t`, so `sampleOffset = 0` in every block size. **Gap:** non-zero offsets inside a block need a per-block "before process" hook that gets the block start. Add one to `Session::run` and inject at `t − blockStart`. Do this for the block-size null so NE events land mid-block at 441 / 4096.
- **NE1:** C#4 (61) + NE −50 ¢ at the note-on, κ 100 %, root 60 → every grain `rate == 2^(0.5/12)·tapeRate/fs` within **0.01 ¢**. Controls: the same note with no NE must be 50 ¢ away; an NE whose noteId does not match must not move it.
- **NE2 live:** NE changed 1 s after the note-on (another block) moves the grains spawned after its sample only. The module table would drop this event (control = a `neModuleTable` hook that reads the module table instead must FAIL NE2).
- **NE3:** two notes on one pitch with different noteIds and tunings keep separate rates (the module's pitch-keyed collision would fail this).
- **NE4:** NE + MPE member bend add; NE + Legato glides to the tuned pitch.

## 5. MPE (D31; Q6)

### 5.1 Parser — **VERIFIED** (JUCE) / **[decision for plan]**
`juce::MidiRPNDetector rpn;` (`reset()` in `prepare`). On every CC 6 / 38 / 98–101, call `rpn.tryParse (ch, cc, v)`. On a message:
| message | channel | action |
|---|---|---|
| RPN 0 (pitch-bend sensitivity) | member (2–16) | `memberRange = semis + cents/100` (semis = `is14Bit ? value >> 7 : value`, cents = `is14Bit ? value & 127 : 0`), clamp [0, 96]. Applies to every member channel (MPE spec: zone-wide). JUCE's own `MPEZoneLayout` ignores cents; we keep them. |
| RPN 0 | master (1) | **ignored**: the master range is `pitch_bend_range` (the engine never writes APVTS) |
| RPN 6 (MCM) | master (1) | `memberCount = MSB` (0–15); `memberRange = 48` (the MPE spec resets member sensitivity on an MCM). A note on a channel > memberCount + 1 is handled as non-member (no member expression). MCM 0 → no members. **[decision for plan]**: whether MCM 0 should also mean "MPE off" while `mpe_enable` stays on. Recommend no: the parameter is the switch. |
| RPN 6 | other | ignored (upper zone = v1.1) |
| NRPN, RPN null, other RPNs | any | ignored |
CC 96 / 97 (increment / decrement) are ignored. The parser runs whether MPE is on or off, so turning MPE on mid-session sees the last RPN state, and the output with MPE off does not change.

### 5.2 Routing with `mpe_enable` on
| input | master (ch 1) | member ch m |
|---|---|---|
| pitch bend | `bend` (2.5a slot, × `pitch_bend_range`, all voices) + Bend **source**: unchanged from 2.5a? → **[decision for plan]**: recommend the per-voice Bend source = **member** bend (per-note expression) and the master moves pitch only | `mpeBend[m]` → voices with `channel == m`: pitch term `mBend·memberRange`, Bend source |
| channel AT | ignored (D31) | `mpePressure[m]` → Pressure source of those voices |
| CC 74 | ignored (D31) | `mpeSlide[m]` → Slide source |
| poly AT | ignored (D31) | ignored |
| CC 1 / 33, 64, 120, 121, 123 | as 2.5a / §6 | **[decision for plan]**: recommend CC64 on the master channel only (MPE spec: zone-level messages go to the master). CC121 on m resets that channel's expression. |
New note on member ch m: `vm.pressure = mpePressure[m]`, `slide = mpeSlide[m]`, `bendSrc` / pitch from `mpeBend[m]` (MPE spec initial values). Two notes on one member channel share it (spec-legal).
**Switching `mpe_enable` while notes sound:** recommend that it applies from the next event and sounding voices keep their current values (no retro-mapping, no step). **[decision for plan]**.

### 5.3 Gates (law, no parent)
- **MP1:** 3 held notes on ch 2 / 3 / 4, member bend sweep on ch 3 → voice 3's grain rates follow `memberRange·b`. Voices 2 and 4 are **bitwise** unchanged against a render without the sweep (the 2.5a independence pattern). Control: an `mpeBroadcast` hook (member writes go to every voice) must fail.
- **MP2:** RPN 0 = 12 on ch 2 → member bend +1 = +12 st (default 48 before it). 14-bit LSB 50 → 12.5 st. NRPN 0 / 0 interleaved → no change. RPN null then CC 6 → no change. RPN 0 on ch 1 → no change.
- **MP3:** master bend + member bend + NE add, to 1e-9 st.
- **MP4 (D31 ignores):** master AT / master CC74 / poly AT under MPE → every voice's Pressure / Slide source bitwise unchanged. The same events with MPE off move them (control = the 2.5a behaviour).
- **MP5:** MPE off → the eg25a source rows (`engine_gates_25a.h:451-454` etc.) and every 2.5a golden are unchanged (true bypass row).
- **Re-run under MPE on (Q7):** eg25a source ranges (Pressure / Slide / Bend through member channels), per-voice independence, NV newest-voice handover with member pressure, alloc-order invariance (slot permutation with member expression). These carry the 2.5a meaning into the MPE form (DSP-06 MPE arm).

## 6. Sustain (CC64) — **INFERRED** from `noteOff` / `chooseVictim`
- `sustainDown = v ≥ 64`. **Poly:** a note-off with the pedal down sets `keyDown = false` and leaves `gated` (no env release). Pedal-up: every counted voice with `gated && ! keyDown` → `env.noteOff` + Mod Env `noteOff` (the current `noteOff` body).
- **Re-strike of a sustained pitch in Poly** **[decision for plan]**: CONTEXT reads "takes the oldest-voice-on-the-target-pitch steal rule". That rule fires only at the polyphony limit (`chooseVictim`, `VoiceAllocator.h:137-139`). Below the limit, two voices sound on the same pitch. JUCE `Synthesiser::noteOn` (`juce_Synthesiser.cpp:317-321`, **VERIFIED**) instead releases a sustained voice on the same note + channel with a tail. Recommend the JUCE behaviour (release with tail, not a 5 ms steal), or confirm the literal reading.
- **Mono / Legato:** a note-off removes the key from the stack whatever the pedal (fallback targets physical keys, D30). If the stack is empty and the pedal is down, the cloud's gate holds until pedal-up.
- **CC121** lifts sustain (RP-015) → pending note-offs apply at that sample. **CC123** releases every gated voice and **ignores the pedal** (2.5a behaviour, which is the transport-stop safety), and clears the stack. **CC120** force-releases as in 2.5a and clears the stack.
- **S1 gates:** note-off deferred to pedal-up (env release starts at the pedal-up sample, ± 0); steal with CC64: a sustained voice is "no key down" but not "released" (rule order); Mono stack excludes sustained keys (fallback skips them); CC121 releases sustained voices; pedal-up click row vs a same-motion control.

## 7. Proof arms (gate inventory for the plan)
| family | rows | control that must fire |
|---|---|---|
| VM (D29) | Mono: new serial, amp + Mod Env levels continuous across the retrigger (\|Δ\| ≤ one-sample slope), scan σ = 0 at t, LFO phase = lfoN_phase, NR ≠ old. Legato: serial / epoch / σ / LFO phase / NR / velocity bitwise unchanged, only `v.note` + glide moved | `ampResetZero` (noteOn from 0) fails the continuity row; `legatoAsMono` fails the Legato row |
| FB (D30) | fallback glide to the held key, serial unchanged, no env stage change | `fallbackRetrig` |
| GL | trajectory golden (§3.2), constant time (1 st vs 24 st same N), Poly glide-from-last, glide 0 bitwise = a 2.5a render, law tracking (§3.3) | `glideStep`, `pvKeyOff` |
| MS (D32) | 16 → Mono: newest kept (serial), 15 forced (5 ms), no stuck voice after 1 s, stack = physical keys; Mono → Poly continuity | `modeSwitchHard` (0 ms) |
| SU | §6 | — |
| MP / NE | §5.3, §4.3 | as listed |
| CK (QUAL-01) | local κ (2.4 S6): Mono retrigger, Legato glide, D30 fallback, D32 switch, pedal-up, member bend sweep, Mono velocity change (gain glide) | each a same-motion hard control (`ampResetZero`, `glideStep`, `modeSwitchHard`, `gainSnap`, …). Memory `pattern_click_kappa_masked_by_level_change`: use the 5 ms-frame local κ, not the whole-window one |
| BS (Algorithm §1) | the full script (sustain, Mono / Legato + glide, D32 switch, MPE member bend / AT / CC74, RPN, NE mid-block) at 32 / 441 / 512 / 4096 / variable, bitwise | different seed differs |
| AL (PERF-01) | allocation hook over the same script + stack overflow (> 128 held keys) + 64-event NE overflow | liveness probe (existing A1 pattern) |
| TB | true bypass: renders-2.4 + legacy byte-identical, harness 231 → re-scoped N2 / N2s / FN3, `--parity` 168/0 + GV 2/0, param-dump, drift 0/13 | carried |
| CPU | C25m ≤ 40 %, C25tm ≤ 12 % (regression); **INFO** C25mpe (16 MPE voices, member bend sweeps + glide: the law recompute) | witnesses as 2.5a |

## 8. Pitfalls
1. **Velocity gain step on a Mono retrigger** (Summary 2): continuity of `level` alone is not continuity of the output.
2. **Calling `startVoice` for a Mono retrigger** snaps `lawGain` / `gFbLerp` (`Engine.cpp:1980-1990`) and resets the env to 0. Write `retriggerVoice`.
3. **The law key misses per-voice pitch** (§3.3): a silent wrong-level bug that no output-hash row catches unless coherence is on and the pitch changes.
4. **Term order in `voiceSemis`**: append, don't reorder (Summary 11). Otherwise renders-2.4 move by 1 ulp in the rate and the hashes break.
5. **NE via the module table** loses live updates and collides on pitch (§4.1). The NE2 / NE3 rows are designed to fail on that path.
6. **NE list not cleared on bypassed blocks** (§4.2 step 2).
7. **`kTuningTypeID` literal 2** in a SharedCode TU: no Steinberg header can be included there (module TU split). Comment it with the SDK line (`ivstnoteexpression.h:49`).
8. **MIDI tie order:** `MidiCursor` keeps host order and never forces off-before-on (`MidiCursor.h:28-33`). So a Legato off+on at one sample is a full retrigger when the off comes first and legato when the on comes first. That is deterministic and block-size invariant. Document it, don't "fix" it.
9. **D32 detached cloud** edge (§2.3), and Mono note-on when the cloud is releasing (§2.1 row 2): both need an explicit rule in the plan, or the stack and the cloud disagree.
10. **CC123 vs pedal:** keep the 2.5a release-everything behaviour (transport stop). A pedal-respecting CC123 would hang notes on stop in hosts that send CC123 with no CC64 up.

## 9. Decisions for plan (summary)
1. Amp gain on a Mono retrigger: 5 ms linear glide to the new `g_v` (rec.) vs keep the first note's gain.
2. Mono / Legato note-on while the cloud releases: same slot from the current level (rec.) vs steal + fresh voice.
3. Key source in Legato: follows the target (rec.) vs frozen.
4. Mono retrigger per-voice level / pan offsets: 20 ms residual (rec.) vs a one-cell step.
5. D32 detached cloud rule (§2.3).
6. NE stamped path via `GiVst3Extensions` (rec.) vs the module table (note-on only; fails NE2 / NE3 and CONTEXT "updated live").
7. RPN: `juce::MidiRPNDetector` (rec.); RPN 0 cents kept; MCM 0 ≠ MPE off.
8. MPE Bend source = member bend; CC64 master-only; `mpe_enable` switch applies to the next events.
9. Sustain re-strike in Poly: JUCE release-with-tail (rec.) vs the literal steal-rule reading.
10. `GiNoteStack` capacity 128, entries `{channel, note, noteId, neSemis}`.

## 10. Targets (projection)
Harness 231 → ≈ 275 (VM ≈ 8, FB 3, GL 6, MS 4, SU 6, MP 8, NE 5, CK 7 + controls, BS 1, AL 1, re-scoped N2 / N2s / FN3). `--parity` 168/0 + GV 2/0 unchanged (no parity arm). `--cpu25a` C25m / C25tm regression + C25mpe INFO.
