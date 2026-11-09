# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.5a — CONTEXT

**Date:** 2026-11-08
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:**
- `parameter-spec.md` (LOCKED, 261 params; `lfoN_*` incl. `lfoN_mode`, `menvN_*`, `macroN`, `modM_source/dest/depth` M = 1–16, `sN_*_div` / Tempo mode, 126-entry destination order + 18-entry source order frozen)
- `REQUIREMENTS.md` (DSP-06; PERF-01, PERF-02, QUAL-01 arms)
- `research/ARCHITECTURE.md` §4 (Mod ADSR), §19 (sources), §20 (ControlGrid + ModMatrix), §21 (TempoTracker), A5 (newest-voice rule, D4), Algorithm §1 (read/write ordering, block-size invariance), CPU model, Stage-2 Gate Catalogue
- `ROADMAP.md` § Phase 2.5a (split this session)
- `stages/2-dsp/CONTEXT-2.4.md` (D22–D25), `SUMMARY-2.4.md`, `VERIFICATION-2.4.md`

## Scope split (D26)

The user split ROADMAP Phase 2.5 into two phases. Each has its own discuss → verify cycle. 2.6 is unchanged.

| Phase | Covers |
|-------|--------|
| **2.5a** (this) | Mod sources (2 × Mod ADSR, 4 × LFO global / per-voice, velocity, key, note random, mod wheel, non-MPE channel/poly AT, CC74 slide, channel bend, 4 macros); per-voice ControlGrid + ModMatrix; newest-voice rule; forked TempoTracker + Tempo divisions + Tempo-mode thinning; the matrix CPU verdict |
| **2.5b** | Mono / Legato + glide (note stack), sustain pedal (CC64), MPE lower-zone parser (RPN 0/6, member-channel bend / pressure / slide), VST3 note-expression drain + per-note cents (FUNC-05, the MPE/NE engine path) |

In 2.5a, the Pressure, Slide and Bend sources run in their **non-MPE** form (ARCH §2 "MPE off"). 2.5b reroutes them per member channel and must not change any 2.5a golden made with `mpe_enable` off.

## Goal (2.5a)

DSP-06: every listed source routes through the 16-slot matrix to a continuous destination with bipolar depth, and per-voice sources modulate each voice independently. Tempo sync is live. The final PERF-02 verdict includes the matrix.

## Decisions (locked this session)

### D26 — Split 2.5 into 2.5a / 2.5b
See the table above. ROADMAP is amended. STATUS tracks `2-dsp/2.5a`, then `2-dsp/2.5b`.

### D27 — DSP-06 proof: parity on the shared path + law gates for per-voice
- **Parity arm (extends `--parity`):** null against O-Granulator @ `82406fce`, using the fresh out-of-tree parent build (n1/run.sh precedent), at **−90 dB** after gain alignment. Fitted gain is reported per row and must stay inside a stated band (2.3 PFc6 precedent). The shared path covers:
  - Global LFO 1–4 (every shape, Free + Tempo sync, phase, S&H / smooth hash) → a stream destination, bipolar depth ±.
  - Macro 1–4 → a stream destination (20 ms ExactRamp).
  - Slot ramps (20 ms) on depth edits, and the route-change shadow crossfade (source swap, dest swap, depth sign flip).
  - Every row family has a negative control (perturbed depth, swapped source or wrong phase) that must fail.
  - Parent-only sources (Env follower, Pitch, Onset) are not in the instrument's source table. They are excluded, with no row.
- **Law arm (novel, no parent equivalent):**
  - **Per-voice independence:** 3 held voices, each with a different velocity / key / note random / per-voice LFO phase. Each voice's latched destination tracks its own source. Control: forcing a shared source value makes the per-voice spread collapse to 0 and the row must fail.
  - **Newest-voice rule (D4):** per-voice source → global destination (for example velocity → `reverb_mix`) follows the newest sounding voice. On handover, the max per-sample step ≤ the 20 ms ramp slope bound. The last value is held when no voice sounds. Control: an un-ramped handover hook must exceed the bound.
  - **Mod Env 1–2:** retrigger from current level, times latched at stage entry, 2 ms floors. Golden per-sample shape vs a closed form.
  - **Per-voice LFO:** phase reset to `lfoN_phase` at note-on. S&H draws keyed on `(serial, cycle)`, so steal / allocation order cannot change the content (two allocation orders → identical renders).
  - **Source ranges:** velocity, note random, pressure, mod wheel and macros are unipolar. Key `(note − 60)/64` and slide `(cc74 − 64)/64` are bipolar and clamped. Bend is bipolar and normalised by range. A neutral controller adds exactly 0 (bitwise vs unrouted).
  - **Destination clamp:** `clamp(base01 + Σ depth·src, 0, 1)` at the extremes. 16 routes stacked on one destination saturate without NaN or wrap.
- **Route-edit click gates (QUAL-01):** every MX3-kind edit (add, remove, source swap, dest swap, depth step, sign flip) on a live route is measured with local κ (2.4 S6 precedent) against a same-motion control. The parent's D36 closed with no DSP change (UX1 PASS on its unmodified 2.6 matrix), so there is no fix to fork. The rows prove we inherited that behaviour.

### D28 — Final PERF-02 verdict includes the matrix (≤ 40 %)
- Worst case = the C25w stimulus (16 voices × 4 streams saturating 256, all FX, 4 chains at feedback 100 %, reverb mix 100 %) **plus all 16 matrix slots live**, from per-voice sources (per-voice LFOs + both Mod Envs) to per-voice destinations, at 16 voices. This must be ≤ **40 %** of one core at 48 k / 128. It replaces C25w as the final PERF-02 worst-case verdict.
- Same method as C23 / C25: run alone, `CLOCK_THREAD_CPUTIME_ID`, warm-up + 7 × 10 s, live ≥ 240 asserted, duty witness ≥ 0.9, calibration-loop witness against `tests/golden/cpu-calib-2.3.txt`. C25w is re-run in the same session, so the matrix cost is a measured delta.
- The typical proxy (D20) is re-run with 4 representative routes (global LFO → position, velocity → level, Mod Env 1 → size, macro → filter) as a regression row (≤ 12 %).
- If the 40 % row fails, the D21 lever ladder applies (levers auto in ARCH order; the `grain_limit` 256 → 128 fallback asks first).

## Defaults (no decision needed; binding for plan)

- **Tempo:** forked `TempoTracker.h` stays byte-identical. Sync onsets land within **1 sample** of the synthetic-playhead grid at 48 / 44.1 / 96 k, across tempo changes and loop jumps. Tempo-mode thinning only drops power-of-two onsets. A Free-mode control row must not lock to the grid. LFO Tempo sync uses the ppq phase (parent hybrid).
- **Determinism:** all per-voice random draws key on the note serial, never the voice slot. A block-size null with **every source live** (32/441/512/4096/var) must pass, with a different-seed control.
- **Fork discipline:** `ControlGrid.h`, `ModMatrix.h`, `ModSources.h` and `TempoTracker.h` stay byte-identical to the pin (drift 0 / 13). Per-voice evaluation, the newest-voice rule and the instrument source table go in new Gi files. Any leaf edit goes in `FORK-MANIFEST.md` with reason + sha256.
- **The engine never writes APVTS.** Destination values for the UI are published as `std::atomic<float>`: global destinations directly, per-voice ones from the newest voice. No listener writes another parameter.
- **Allocation gate (PERF-01)** extends across route add / remove / swap, source and mode changes (`lfoN_mode` Global ↔ Per-voice while voices sound), tempo changes and steals with 16 routes live.
- **True bypass:** with all 16 slots unrouted (source None or depth 0), renders-2.4 (and renders-2.2 / FR1 under the 2.4 legacy hooks) stay **byte-identical**. Harness 179/0, `--parity` 133/0 + GV 2/0 carried. Param-dump byte-identical (no new params, no new `<Engine>` keys).
- `lfoN_mode` switch while voices sound: global ↔ per-voice handover uses the 20 ms ramp (click row against a same-motion control).
- HANDS-ON-2.5a is written (LFO feel, per-voice spread, tempo lock, macro sweeps) and **deferred to post-Stage-3** with 2.1–2.4 (user override 2026-11-08).
- Rebuild Standalone explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check.

## Test criteria (Phase 2.5a gate)

- [ ] Parity nulls ≤ −90 dB on global LFO / macro / ramps / shadow crossfade, with the gain band held and controls failing (D27)
- [ ] Each 2.5a source routes to a continuous destination with bipolar depth; per-voice independence with a collapse control (DSP-06)
- [ ] Newest-voice handover with no step beyond the ramp bound; value held when silent; un-ramped control fails (D4)
- [ ] Mod Env shape golden; per-voice LFO phase reset; allocation-order invariance
- [ ] Neutral controllers add exactly 0; 16-route stack clamps without NaN
- [ ] Route-edit click gates (all MX3 kinds) and `lfoN_mode` switch vs same-motion controls (QUAL-01)
- [ ] Tempo onsets within 1 sample; power-of-two thinning; Free-mode control
- [ ] Block-size null with every source live; different-seed control
- [ ] Allocation gate across route edits / mode switches / tempo changes / steals with 16 routes
- [ ] True bypass: renders-2.4 byte-identical unrouted; param-dump identical; fork drift 0
- [ ] CPU worst case with 16 per-voice routes ≤ 40 % with witnesses; typical proxy ≤ 12 % (D28)
- [ ] auval PASS; pluginval 10 VST3 + AU SUCCESS

## Constraints and risks to flag

- **Per-voice evaluation cost:** up to 16 routes × 20 voices per 32-sample cell, plus 80 per-(k,s) destination sets. The sparse per-voice offset vector must not touch unrouted destinations. Headroom is 13.4 pts (C25w 26.59 %).
- **Latched vs ramped destinations:** per-grain-class destinations are read at the spawn sample. `level`, `pan`, fb, reverb and output interpolate within each cell. A per-voice route to a ramped destination needs per-voice ramp state.
- **Newest-voice + steals:** the newest voice can be the one stealing into a spare slot. The handover source is the counted voice, never a 5 ms forced-release spare.
- **Parity alignment:** the parent ticks sources on its own grid. Our grid must be phase-aligned at T₀ (2.4 PB0 enable-at-T_e precedent) or the LFO nulls read as a phase error.
- **2.5b compatibility:** the non-MPE Pressure / Slide / Bend wiring must be reroutable per member channel without moving any 2.5a golden made with `mpe_enable` off.

## Open questions for research

1. The parent `--parity` hooks needed to drive global LFO / macro / route edits from the harness and capture a matched stream destination. Can the existing tapProbe path carry them, or does the out-of-tree build need new hooks?
2. The exact per-voice data layout (offset vectors, per-voice ramp state for ramped destinations) and where the per-voice tick sits relative to the 2.3 FX latch and the 2.4 piece loop.
3. How the parent `ControlGrid` / `ModMatrix` API wraps for a per-voice second pass without a leaf edit (drift 0).
4. Newest-voice handover with steals into spares and during a 20 ms ramp already in flight (ramp from current, never stack).
5. Which destinations are global vs per-voice in the frozen 126-entry table, and the `lfoN_rate` class flip when `lfoN_mode` changes.
6. Tempo-mode thinning: the parent power-of-two rule vs our D3 onset thinning and quotas. Where does the interaction sit?
7. Projected matrix cost against the 13.4-pt headroom (CPU model update).

## Out of scope for 2.5a

- Mono / Legato / glide, sustain pedal, MPE member routing, RPN 0/6, note expression (2.5b)
- Record-in, `<Slots>`, factory sources, file swap (2.6)
- Matrix view, mod rings, LFO phase telemetry UI (Stage 3)
- Typical CPU verdict on factory presets (Stage 4)

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 2-dsp`)
