# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.4 — CONTEXT

**Date:** 2026-11-08
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:**
- `parameter-spec.md` (LOCKED, 261 params; `feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion`, `freeze`, `reverb_mix/size/damp`, `output_gain`; actions Reset all / per slot)
- `REQUIREMENTS.md` (DSP-04, DSP-05; PERF-01, PERF-02, QUAL-01 arms)
- `research/ARCHITECTURE.md` D7, §6 (slot handoff, dual-read swap), §16 (feedback tap `g_fb`), §17 (feedback network + record heads), §18 (reverb + wet safety + output gain), §9 record-head clamp, Algorithm §3 (stability), CPU model, Stage-2 Gate Catalogue
- `ROADMAP.md` § Phase 2.4
- `stages/2-dsp/CONTEXT-2.3.md` (D18–D21), `SUMMARY-2.3.md`, `VERIFICATION-2.3.md`

## Goal (ROADMAP Phase 2.4)

DSP-04 (feedback into per-stream working copies, Freeze, Reset, voice-count-aware runaway guard) and DSP-05 (output reverb). Plus the final PERF-02 worst-case verdict at 40 %.

Components:
- Per-(k,s) `g_fb` and an amp-weighted **mean** tap per stream (÷ max(1, Σaₖ)).
- 4 × forked `FeedbackChain` (already in `Source/dsp/forked/`) with 32-sample returns.
- Record heads: exactly 1×, loop-region wrap latched per wrap, run condition per ARCH §17, convex write `wc ← (1−λ)·wc + λ·r[n−32]`, λ = 0.95·feedback, write-before-read per 32-sample piece.
- Spawner record-head clamp in head-relative modular coordinates, with forced-crossing telemetry.
- Freeze, Reset (off-thread rebuild from pristine + 20 ms dual-read crossfade swap), SR rebuild path (parked heads, `tapeRate/fs` correction).
- `ReverbUnit` (reverb-unit 1.2.0, atomic bind) → wet safety knee → `output_gain`.

Still excluded: modulation / voice modes / MPE / NE / tempo (2.5), Record-in / `<Slots>` / factory sources (2.6).

## Decisions (locked this session)

### D22 — DSP-04/05 proof: leaf nulls vs parent + law gates for the novel loop
- **Leaf null arm (extends `--parity`):** O-Granulator @ `82406fce`, fresh out-of-tree parent build (n1/run.sh precedent), **−90 dB** after gain alignment:
  - `FeedbackChain`: identical input bus → identical return, across `fb_low_cut` / `fb_high_cut` / `fb_saturation` / `fb_diffusion` sweeps incl. extremes, and the 32-sample return delay.
  - `ReverbUnit` + wet knee: same pre-reverb bus → same output, `reverb_mix/size/damp` sweeps. Our side has **no dry path**; compare against the parent's wet-only path (research confirms the parent hook needed).
  - Each row family has a negative control (perturbed parameter or swapped order) that must fail.
  - Fitted gain per row reported and inside a stated band (2.3 PFc6 precedent), so alignment cannot hide a gain error.
- **Law arm (the tape loop has no parent equivalent):** ARCH Algorithm §3 gates:
  - 60 s, 16 voices held, feedback 100 %, density 200, size 500 ms, pitch +12, all streams +6 dB, res 100 %: tape peak ≤ max(pristine peak, 0.7); no NaN / Inf / denormal.
  - **Runaway guard (voice-count aware):** same stimulus at 1 vs 16 voices; tap level does not scale with voice count (mean, not sum); control = sum tap (÷1) must grow and fail.
  - Freeze → tape hash unchanged over 10 s (taken after the λ ramp settles, D24).
  - Reset → tape hash == pristine build (also when frozen: stays frozen).
  - Feedback 0 → tape hash == pristine after 60 s of play (bitwise; DSP-01 parity untouched).
  - Write-before-read: no grain reads a frame written in the same piece out of order; block-size null with feedback 80 % + freeze toggles (32/441/512/4096/var), different-seed control.
- Head clamp: telemetry 0 forced crossings whenever the region admits a feasible start; no grain read inside the unsettled mip margin; a short-region row where crossings are forced must report them (control).

### D23 — Ceiling = parent D9 wet-safety knee (+6 dBFS)
- The "limiter ceiling" of DSP-04 is the parent knee `1.9953·tanh(x/1.9953)` (identity ≤ −6 dBFS, asymptote +6 dBFS), measured **before** `output_gain`. No 0 dBFS hard clamp. Parity with the parent.
- Gate: knee output ≤ +6 dBFS on (a) the 60 s feedback stress (D22) and (b) the carried **FZ1** stimulus from 2.3 (pre-knee bus 1.53 / 1.71 / 1.73 at 44.1 / 48 / 96 k). Pre-knee peak reported as INFO.
- `output_gain` is applied after the knee and is excluded from the ceiling row (user-owned gain).

### D24 — Freeze / feedback transitions: λ ramped over 20 ms
- Freeze ramps λ → 0 over **20 ms** (same ramp as `feedback`); unfreeze ramps back. The heads stop advancing once λ reaches 0. The chains keep running on the grain output (warm), and their return is discarded while frozen (parent rule).
- Click gates (QUAL-01) vs same-motion controls: freeze toggle, feedback step 0 → 100 %, head start / stop (run condition flips), Reset swap. A hard-stop variant (test hook) is the control and must read worse or equal (recorded).
- The freeze hash is taken from the first sample after the ramp ends.

### D25 — Final PERF-02 worst case at 40 % (default, carried from D19)
- Re-run the D19 worst case with **feedback 100 % on 4 chains + reverb (mix 100 %)** added: ≤ **40 %** of one core at 48 k / 128. This is the final PERF-02 worst-case verdict.
- Same method as C22 / C23: run alone, `CLOCK_THREAD_CPUTIME_ID`, warm-up + 7 × 10 s, live ≥ 240 asserted, duty witness ≥ 0.9 (contended counts as FAIL), calibration-loop witness against `tests/golden/cpu-calib-2.3.txt`. C23w is re-run in the same session, so chains + reverb are a measured delta (projection ≈ +3.5 pts).
- The typical proxy (D20) is re-run with reverb at its default (mix 0) as a regression row (≤ 12 %).
- Reset dual-read fade (2× reads on one stream for 20 ms) is an **INFO** row, not part of the verdict.
- D21 lever ladder and the "fallback asks" rule still apply if 40 % fails.

## Defaults (no decision needed; binding for plan)

- `FeedbackChain.h` and other forked leaves stay byte-identical to the pin (drift 0 / 13). Gi-side wiring goes in new Gi files; any leaf edit goes in `FORK-MANIFEST.md` with reason + sha256.
- Thread safety per ARCH: Reset / SR rebuild run off-thread, are installed via `SlotHandoff` (`pending[s].exchange`, SPSC retire ring), and have no locks or allocation on audio. Reset is a native function, not a parameter. No listener writes another parameter.
- Allocation gate (PERF-01) extended across: Reset (per slot + all), Freeze toggles, feedback on/off, SR change rebuild, and steals with feedback on.
- Host-idle bounded memory: 100 Reset requests with no processBlock → `pending` holds ≤ 1 tape per slot (PERF-03 arm).
- NaN scrubs on the loop return and the tape write; injected NaN in the chain → scrub fires, tape stays finite (control).
- With feedback 0, freeze off and reverb mix 0, every existing render golden (renders-2.2, FR1) stays **byte-identical**: the new path must be a true bypass. Harness 150/0 and `--parity` 99/0 + GV 2/0 carried.
- SR rebuild: a 44.1 k pristine into a 96 k session plays at correct pitch while parked (rate correction), and the heads resume only after the new tape lands.
- No new parameters, no new `<Engine>` keys (the working copy is not saved; reload = pristine). Param-dump stays byte-identical.
- HANDS-ON-2.4 is written (feedback character, freeze, reset smoothness, reverb tails fade on mix → 0) but **deferred to post-Stage-3** with 2.1–2.3, per the user's 2026-11-08 override.
- Rebuild Standalone explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check.

## Test criteria (Phase 2.4 gate — ROADMAP + D22–D25)

- [ ] FeedbackChain and ReverbUnit + knee nulls vs parent ≤ −90 dB, gain band held, controls fail (D22)
- [ ] 60 s feedback-100 % stress at 16 voices: tape peak ≤ max(pristine, 0.7), knee out ≤ +6 dBFS, no NaN / Inf / denormal (DSP-04, D23)
- [ ] FZ1 carried ceiling row ≤ +6 dBFS at 44.1 / 48 / 96 k (D23)
- [ ] Runaway guard: 1 vs 16 voices, tap does not scale; sum-tap control fails (DSP-04)
- [ ] Reset hash == pristine (incl. while frozen); Freeze hash unchanged 10 s post-ramp; feedback 0 hash == pristine after 60 s (DSP-04)
- [ ] Click gates: freeze toggle, feedback step, head start / stop, Reset swap vs same-motion controls (QUAL-01, D24)
- [ ] Head clamp: 0 forced crossings when feasible, forced-crossing control reports; no reads in the unsettled mip margin
- [ ] Reverb audible (wet energy row) and tails decay on mix → 0 (DSP-05)
- [ ] Block-size null with feedback 80 % + freeze toggles; different-seed control
- [ ] Allocation gate across reset / freeze / SR change / steals with feedback; host-idle bounded memory
- [ ] True bypass: renders-2.2 / FR1 byte-identical with feedback 0 + reverb mix 0; param-dump identical; fork drift 0
- [ ] CPU worst case with chains + reverb ≤ 40 % with witness; typical proxy ≤ 12 %; reset-fade INFO (D25)
- [ ] auval PASS; pluginval 10 VST3 + AU SUCCESS

## Constraints and risks to flag

- **Record-head clamp in modular coordinates** is the riskiest new piece (ARCH Known Challenges). Short loop regions and reverse grains are the edge cases.
- **Mip settling at the head:** writes update incremental mips; grains reading pitched at level l near the head must stay outside `margin(l)`. The loop-wrap sliver (≤ 63·(2^l − 1) samples) is accepted per ARCH §7 and must not be a gate failure.
- **Dual-read swap + parked head:** the record head must not write into either tape during the 20 ms fade; Reset during a running fade must queue, not stack (pending ≤ 1).
- **Feedback 0 bitwise** depends on λ being exactly 0.0f after the ramp (not a denormal tail) and on the writer being skipped, not writing `1·wc + 0·r`.
- **CPU headroom:** C23w 26.01 % + ≈ 3.5 pts projected → ≈ 30 % vs 40 %. Comfortable, but the dual-read fade doubles one stream's read cost (INFO).
- **Parent reverb hook:** the parent has a dry/wet path; the null needs its wet-only output (or mix 100 %) — research confirms.

## Open questions for research

1. Parent hooks for a standalone FeedbackChain and ReverbUnit + knee null: does the parent `--parity` build expose them, or does the out-of-tree build need extra test hooks?
2. Exact `margin(level)` for the head clamp and how the parent's reach clamp (GR-ARCH §5) maps onto a moving head in a loop region.
3. Click-gate thresholds and same-motion controls for freeze / feedback step / Reset (parent precedents, memory `step-click-gate-needs-same-motion-control`).
4. Where the 32-sample piece loop sits in `Engine.cpp` today, and how write-before-read slots into the existing render order (ARCH render order §7–8).
5. Reverb "audible" + "tails fade on mix → 0" as automated rows (energy thresholds, decay time).
6. SR rebuild: what already exists from 2.1 (`SlotLoader`, `SlotHandoff`) vs what 2.4 adds (dedupe by (slot, rate), parked heads).

## Out of scope for 2.4

- Modulation of feedback / reverb / output destinations (2.5)
- Record-in capture, `<Slots>` state, factory sources (2.6)
- Typical verdict on factory presets (Stage 4)
- Feedback / freeze / reset UI, record-head and evolving overview visuals (Stage 3)

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 2-dsp`)
