# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.5b — CONTEXT

**Date:** 2026-11-09
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:**
- `parameter-spec.md` (LOCKED, 261 params; `polyphony`, `voice_mode` Poly/Mono/Legato, `glide` 0–2000 ms skew 0.3, `pitch_bend_range` 0–48 st, `mpe_enable`)
- `REQUIREMENTS.md` (FUNC-05; DSP-06 MPE arm; PERF-01, PERF-03, QUAL-01 arms; COMPAT-02 is Stage 4)
- `research/ARCHITECTURE.md` §2 (MIDI / MPE / NE front end), §3 (voice allocator, note stack, glide, sustain), §10 (spawn pitch formula incl. `glideOffset + bend + neCents/100`), A5 (newest-voice rule, D4), D5 (Dorico NE in v1), Algorithm §1 (block-size invariance gate incl. sustain / MPE)
- `ROADMAP.md` § Phase 2.5b
- `stages/2-dsp/CONTEXT-2.5a.md` (D26–D28), `SUMMARY-2.5a.md`, `VERIFICATION-2.5a.md`

## Starting point (code at 523f4e84)

- `VoiceAllocator.h` is Poly-only. Its header already says 2.5 "only changes what keyDown means" (CC64).
- `Engine.cpp` MIDI handling is the 2.5a non-MPE form: CC1/33 wheel, CC74 → every voice, CC121 reset, channel AT → every voice, poly AT → voices on that note, channel bend.
- `ControlGrid.h` never binds `voice_mode`, `glide`, `mpe_enable` (comment at :196).
- `PluginProcessor` already subclasses `Ouaricon::NoteExpression::VST3Extensions` (note-expression 1.1.2) and calls `drainAndUpdate()` each block. Nothing consumes the tuning yet.
- **No parent equivalent.** O-Granulator @ `82406fce` is an effect with no voice modes, MPE or NE, so 2.5b has no parity arm. Proof is by law gates (the 2.5a law-arm pattern). References: `O-simpleWavetable/Source/MonoStack.h` (true-legato stack, verbatim from O-simpleSubtractive) and O-Lyrica (NE reference, memory `project_o_lyrica_spike_reference`).

## Goal (2.5b)

FUNC-05: Poly / Mono / Legato with glide. The MPE / NE engine path: member-channel bend, pressure and slide move only their own note, and VST3 per-note cents tune the note. DSP-06 closes its MPE arm. COMPAT-02 (DAW / Dorico) stays Stage 4.

## Decisions (locked this session)

### D29 — Mono vs Legato on an overlapping note
- **Mono** = a new note on the single cloud. It gets a **new note serial**. Amp env + both Mod Envs retrigger **from the current level** (no reset to 0). Retrigger-mode scan restarts. Per-voice LFOs reset to `lfoN_phase`. Note Random is redrawn and the velocity source updates. Glide moves the pitch.
- **Legato** = pitch only. **Same serial.** Nothing retriggers: envs, scan, per-voice LFO phase, Note Random and the velocity source all keep the first note's values. Only the glide target moves.
- A note-on with **no key held** retriggers normally in both modes (new serial).

### D30 — Fallback on release of the top key
Releasing the top key while a lower key is still held: **both modes glide back** to the held note (last-note priority) **without retriggering**. Same serial, envs / scan / LFO / Note Random / velocity unchanged. This is the O-simpleWavetable / O-simpleSubtractive true-legato `MonoStack` precedent. Releasing the last held key releases the cloud.

### D31 — MPE master channel (ch 1)
- Master bend × `pitch_bend_range` **adds to every note**, on top of member bend and NE cents.
- Master channel AT and master CC74 are **ignored** with MPE on. Pressure and Slide come only from each note's member channel. Poly AT is ignored with MPE on.
- With `mpe_enable` off, the 2.5a non-MPE behaviour stays exactly as it is.

### D32 — `voice_mode` change while notes sound
- **Poly → Mono / Legato:** the newest counted voice keeps sounding (it becomes the mono cloud, serial kept). Every other voice force-releases over 5 ms (the `polyphony`-lowering precedent). The note stack is rebuilt from the keys still physically down.
- **Mono / Legato → Poly:** the sounding voice continues as an ordinary poly voice. New notes allocate normally.
- **Mono ↔ Legato:** no voice action. The new rule applies from the next note event.

## Defaults (no decision needed; binding for plan)

- **Note stack:** port `MonoStack` (fixed capacity, last-note priority, push-moves-to-top, no allocation). Velocity is not stored (D30 keeps the sounding velocity). Under MPE the stack must identify a note by (channel, note) so member expression follows the active key (research Q1).
- **Glide:** linear in semitones over `glide` ms, constant time, latched per note event. Mono / Legato always glide when `glide` > 0. In Poly with `glide` > 0, a new voice starts at the last played note's pitch (ARCH §3). `glideOffset` enters the §10 spawn formula, so grain pitch is read at the spawn sample (latched per grain, as all pitch is). `glide` = 0 is an exact jump (no ramp state touched).
- **Sustain (CC64):** value ≥ 64 = down. The pedal holds the gate; a note-off while sustained is applied on pedal-up. `keyDown` = physical key, so the steal policy's "no key down" rule now differs from "released" (VoiceAllocator header). Mono / Legato: a released-but-sustained key leaves the stack (fallback targets physical keys only); the cloud's gate holds until pedal-up if no key is down. Re-striking a sustained pitch in Poly takes the "oldest voice already on the target pitch" steal rule. CC121 (Reset All Controllers, RP-015) also lifts sustain, so pending note-offs apply; CC123 / CC120 release / silence as in 2.5a.
- **MPE parser (lower zone):** ch 1 master, ch 2–16 members. Member bend range = the last RPN 0 received on any member channel (default **48**). RPN 6 accepted for the lower zone only (upper zone v1.1). RPN state machine on CC 101 / 100 / 6 / 38, per channel, fixed arrays (ARCH §2 `rpnState[16]`, `memberBendRange[16]`). Member bend / pressure / CC74 write only the voices whose channel matches. A new note on a member channel takes that channel's current bend / pressure / slide (MPE spec initial values). Two notes on the same member channel share its expression (spec-legal, no special case).
- **Pitch:** `bend_st = masterBend·pitch_bend_range + memberBend·memberRange + neCents/100` (MPE on); channel bend × `pitch_bend_range` (MPE off, unchanged). The ±48 clamp applies after, as in §10.
- **Note expression (D5):** `drainAndUpdate()` stays at the top of `processBlock`. Per-note cents correlate by `noteId`, never pitch (module contract). Latched at note-on and updated live; the next spawn reads the new value. NE works with MPE off and on; with MPE on it adds to member bend.
- **Newest-voice rule (D4):** in Mono / Legato the one cloud is the newest voice. A Legato key change is **not** a handover (same serial, no step). A Mono retrigger is a handover and uses the 2.5a 20 ms ramp.
- **True bypass:** `voice_mode` Poly, `glide` 0, `mpe_enable` off, no CC64 → renders-2.4 and the 2.4 legacy renders stay **byte-identical**, and every 2.5a golden made with `mpe_enable` off is unchanged. Harness 231/0, `--parity` 168/0 + GV 2/0 carried. Param-dump byte-identical (no new params, no new `<Engine>` keys). Fork drift 0/13 (new code goes in Gi files; any leaf edit goes in `FORK-MANIFEST.md`).
- **The engine never writes APVTS.** No listener writes another parameter.
- **Click gates (QUAL-01):** local κ (2.4 S6 precedent) against same-motion controls for: Mono retrigger from current level, Legato glide, D30 fallback, D32 mode switch (5 ms release), sustain pedal-up release, MPE member bend sweep. Each with a hard control that must fire.
- **Block-size null:** the ARCH Algorithm §1 script extended with sustain, Mono / Legato with glide, MPE member expression and NE events, at 32 / 441 / 512 / 4096 / variable, bitwise. Different-seed control.
- **Allocation gate (PERF-01):** across the whole MIDI / MPE / NE script, including mode switches, sustain, RPN messages and stack overflow (> capacity held keys).
- **CPU:** no new verdict (2.5b adds no grains). C25m re-run as a regression row (≤ 40 %) and C25tm (≤ 12 %).
- **HANDS-ON-2.5b** is written (glide feel, Legato smoothness, sustain, MPE controller, Dorico NE preview) and **deferred to post-Stage-3** with 2.1–2.5a (user override 2026-11-08).
- Rebuild Standalone explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check.

## Test criteria (Phase 2.5b gate)

- [ ] Mono: overlapping note = new serial, envs retrigger from current level (no reset), scan restarts, LFO phase resets (D29)
- [ ] Legato: overlapping note = same serial; envs, scan, LFO phase, Note Random, velocity unchanged; only pitch moves (D29, FUNC-05)
- [ ] Fallback glides back without retrigger in both modes (D30)
- [ ] Glide: linear-in-semitone trajectory golden at the spawn samples; constant time; Poly glide-from-last-note; glide 0 exact
- [ ] Sustain: note-off deferred to pedal-up; steal-policy keyDown rule; Mono stack excludes sustained keys
- [ ] Mode switch while sounding resolves per D32 with no stuck note and no click beyond the control
- [ ] MPE: member bend / pressure / slide move only their own note (others bitwise unchanged); RPN 0 sets member range (default 48); master bend adds; master AT / CC74 and poly AT ignored (D31)
- [ ] NE −50 ¢ on C#4 renders a quarter-sharp C4 (harness raw-event injection, rate within ±0.01 ¢); NE + MPE bend add
- [ ] `mpe_enable` off: every 2.5a golden unchanged; true bypass byte-identical; param-dump identical; drift 0/13
- [ ] Click gates vs same-motion controls (QUAL-01); block-size null with the full script + seed control
- [ ] Allocation gate across the whole MIDI / MPE / NE script (PERF-01)
- [ ] C25m ≤ 40 %, C25tm ≤ 12 % (regression)
- [ ] auval PASS; pluginval 10 VST3 + AU SUCCESS

## Constraints and risks to flag

- **Serial semantics are load-bearing.** Every per-voice random draw, the per-voice LFO S&H and Note Random key on the serial. Legato must keep it, Mono must mint a new one, or the 2.5a allocation-order invariance rows lose their meaning.
- **Retrigger from current level** in Mono needs the amp env and both Mod Envs to restart attack from their present output, not from 0 (ARCH §3). A reset-to-0 is a click.
- **Steal spares vs Mono:** D32's 5 ms releases use the 4 steal spares. A switch with 16 sounding voices must handle spares that are already full (ARCH §3 quietest-spare rule).
- **NE event timing:** the module drains once per block. A per-note tuning change cannot be sample-stamped finer than the block unless research finds a stamped path. That is acceptable for latched-at-spawn pitch only if block-size invariance still holds (it may need to be excluded from the bitwise null and gated separately).
- **Two expression sources on one voice** (member channel + NE noteId) must address the same voice. A noteId → slot map must survive steals into spares.

## Open questions for research

1. `MonoStack` keyed on (channel, note) under MPE: adapt the verbatim leaf (FORK-MANIFEST entry) or write `GiNoteStack.h`?
2. The note-expression 1.1.2 API surface: how `applyPendingTuning` / noteId reach the engine without allocation, and whether events carry sample offsets.
3. Where Legato's pitch-only change sits relative to the scan Retrigger and the per-(k,s) schedulers (no new note-on event for the voice).
4. Where `glideOffset` is evaluated (per spawn vs per grid tick) and whether it needs per-voice ramp state.
5. Sustain + `polyphony` lowering + D32 releases, and the spare-slot budget with all three at once.
6. RPN parser edge cases: NRPN interleave (CC 99 / 98), RPN null (127/127), 14-bit data entry, RPN 0 on the master channel (master range is the parameter, not RPN).
7. Harness raw-event injection for NE (the O-Lyrica precedent) and which 2.5a rows must be re-run under MPE on.

## Out of scope for 2.5b

- Record-in, `<Slots>`, factory sources, file swap (2.6)
- MPE upper zone, MPE+ / MIDI 2.0 (v1.1)
- Dorico playback template / DAW verification (COMPAT-02, Stage 4)
- Voice / MPE telemetry UI (Stage 3)

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 2-dsp`)
