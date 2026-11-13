# O-ParametricEQ — Stage 2 (DSP) — CONTEXT

**Date:** 2026-11-12
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Scope:** stage-wide. These decisions bind all Stage 2 phases. Per-phase files use the suite suffix (`RESEARCH-2.1.md`, `PLAN-2.1.md`, …). A per-phase discuss is needed only where a phase raises a new user call.
**Contracts:** `BRIEF.md`, `REQUIREMENTS.md`, `parameter-spec.md` (196 parameters, frozen), `research/ARCHITECTURE.md`, `ROADMAP.md` § Stage 2, `stages/0-ideation/CONTEXT.md` ("Needs the user's call" 1, 2, 4, 5, 7, 8), `stages/1-foundation/VERIFICATION.md`

## Goal

The full audio engine behind the frozen 0.1.0 shell: 24 bands, eight types, 6–96 dB/oct, five placements, zero latency, click-free, with the output stage, auto gain, analyzer taps, display model and listen filter, each phase closed by its gates (G1–G9, G12).

## Decisions

Six calls deferred from Stage 0 (Stage 1 D3), all answered by the user in this session. Five follow the Stage 0 recommendation. **D7 does not.**

### D6 — Slope is inert on Bell, Notch and Band Pass in v1 (user; Stage 0 item 1)
Slope applies to Low Cut, High Cut, Low Shelf, High Shelf and Tilt. For the other three the value is stored, the DSP ignores it, and changing it triggers no design work and no fade. Higher-order band shapes are v1.x.

### D7 — The accuracy polish is in v1 (user; Stage 0 item 2; departs from the recommendation)
ARCHITECTURE.md lists the numerical polish under "Accuracy backlog (not v1)". That line is overridden. Three follow-up answers fix the scope:

- **D7a — Target: near the second-order optimum, one section per bell.** The DSP-01 gate at 44.1 kHz moves from the closed form's level (bell 0.36 / 0.66 / 1.05 dB at ±6 / ±12 / ±30 dB in the model) toward about twice the best single-section fit (reference 0.08 / 0.16 / 0.37 dB), so roughly **0.15 / 0.30 / 0.70 dB**. Research sets the exact tolerance formula from what a robust polish reaches. No extra corrector section: section counts, the CPU budget and the display packet format do not change.
- **D7b — Coverage: bells and shelves, weak spots included.** Bell sections and the second-order shelf sections that the 12–96 dB/oct shelves and tilt are built from. This includes the two INFO-row weak spots: resonant shelves (Q above 1.41) and shelves of 24 dB/oct and steeper with the corner above 10 kHz at 44.1 / 48 kHz. Cuts, Notch and Band Pass keep the closed form.
- **D7c — Hybrid, never worse.** The closed-form design (MLR) stays as the seed and the safety net. A polished design is used only when it is finite, stable (`g, k > 0`) and measurably closer to the prototype than the seed; otherwise that setting keeps the closed form. v1 ships either way. The gate reports which regions are polished and which fall back.

### D8 — Tilt is ±g/2 (user; item 4)
Gain g gives −g/2 below and +g/2 above the pivot, 0 dB at the pivot; total swing g. As ARCHITECTURE.md already specifies.

### D9 — Q 1.0 is Butterworth on cuts, shelves and tilt (user; item 5)
Q is a resonance factor on the highest-Q section (`Q0 ← Q0 · Qu`) at every slope. The default Q of 1.0 is flat. Inert at 6 dB/oct.

### D10 — Above Nyquist, keep the in-band skirt (user; item 7)
Bells above 0.95 Nyquist are re-centred; shelves, tilt and cuts are designed at the true frequency with angles clamped at π; Notch clamps at 0.95 Nyquist and Band Pass at 0.9 Nyquist. As ARCHITECTURE.md "Frequency policy near and above Nyquist".

### D11 — Listen auditions the band's region of the input (user; item 8)
Bell / Notch / Band Pass: matched band-pass at the band's frequency and Q. Low Shelf / Low Cut: 12 dB/oct matched low-pass at the band frequency. High Shelf / High Cut: 12 dB/oct matched high-pass. Tilt: band-pass, Q 0.7. Taken from the input, respecting placement. As ARCHITECTURE.md § 10.

## Consequences of D7 (Claude; for research and plan to confirm)

- **A new phase, 2.1b "Designer polish", between 2.1 and 2.2.** Phase 2.1 lands the closed-form designer and gate G1 at the Stage 0 tolerance first, unchanged: the polish needs it as seed, as fallback and as the baseline to beat. Phase 2.1b adds the polish behind the same designer interface and tightens G1. Nothing from 2.2 on depends on which design produced the section parameters.
- **The 2.1 fallback decision point stays.** If MLR itself fails G1 in C++, the Vicanek 2016 / 2024 fallback becomes the seed. The fallback has no general-Q shelf, so D7b's shelf coverage would need a new call then.
- **The polish runs wherever the designer runs:** on the audio thread on the control grid while a band ramps, and on the message thread for the display. It must therefore be allocation-free, bounded (fixed iteration count), deterministic, and bit-identical between the two threads and between offline and real-time renders. Gate G9 (display vs audio) and the bit-identity rows depend on it.
- **Contracts are not edited in this phase.** `ROADMAP.md` and `ARCHITECTURE.md` are checksummed in STATUS. The plan phase amends them (new phase 2.1b, DSP-01 tolerance in G1 and in Stage 0 decision 14, the "Accuracy backlog (not v1)" section) and refreshes the checksums.

## Constraints carried

- Parameter layout, IDs, ranges, skews, state format and AU capabilities `[1, 1] [2, 2]` are frozen at 0.1.0. Stage 2 registers nothing and renames nothing.
- Skewed raw defaults are inexact: `freq` 999.99988 Hz, `q` 0.99999976 (normalised 0.5 exactly). The engine must not test "Q == 1.0" or "freq == 1000" for a fast path; neutral sleep keys on gain exactly 0 dB, which is exact.
- Phase 2.1 (designer and offline gate) precedes any processor code.
- No always-on stage in the signal path: ramps snap to their target and unity stages are skipped, or the `memcmp` gates fail.
- The control grid is global and independent of the host block size.
- Hard controls must fire before a click row counts; gate stimulus must exceed any threshold in the gate.
- Zero latency in every configuration; no oversampling; double state; no biquad coefficients; no first-order `juce::dsp::IIR::Filter` default constructor.
- DSP-07 engine half (`Band::dynOffsetDb`, held at 0) lands in Phase 2.2 and needs a row in the plan's coverage table (Stage 1 VERIFICATION issue 3).
- Stage 1 RESEARCH pitfall 2 is wrong for JUCE 8.0.15 (a missing parameter already loads at its default). Do not cite it.
- The Stage 1 render harness (36 rows) and the parameter golden must stay green through every Stage 2 phase.
- One path-scoped commit per phase; no tag.

## Open questions for research

Phase 2.1 (closed-form designer):
1. Reproduce the Stage 0 float64 model in C++ and confirm MLR passes G1 at the Stage 0 tolerance, with the cancellation-free forms.
2. Characterise what Stage 0 left unmodelled: Notch above 0.95 Nyquist, Band Pass above 0.9 Nyquist and above 10 kHz.

Phase 2.1b (polish, D7):
3. **Method.** Fixed-iteration Gauss-Newton on the SVF parameters (the Stage 0 trial: 2–5x better in two iterations where it converged, diverged at low Q with a naive match grid) against alternatives: a damped or constrained variant, a better match grid or weighting, a closed-form correction term, or a precomputed correction table. Which is robust over Q 0.025–40, 10 Hz–30 kHz, ±30 dB at 44.1–192 kHz?
4. **Continuity under D7c.** The accept / fall-back choice must not make the section parameters jump as a parameter sweeps across the boundary: a jump is a click source in Phase 2.3 (G3) and a kink in the drawn curve. Options to evaluate: a continuous blend weight in place of a binary switch, or a region boundary fixed in parameter space with a blend zone.
5. **Reciprocity.** Boost / cut within 1e-9 dB is a 2.1 criterion. Does the polish preserve it (polish the boost and invert), or does the criterion relax?
6. **Cost.** Design time per polished section on the audio thread. A sweeping 96 dB/oct shelf redesigns 8 sections per grid point; PERF-02 allows 35 % of one core with all 24 bands sweeping (model about 22 % before the polish). Does the polish fit, and if not, is it applied at ramp end only (with what audible consequence)?
7. **Where it matters.** At 88.2 kHz and above the closed form is already within 0.10 dB + 1.5 %. Is the polish skipped there, or kept for uniformity and bit-identical cross-rate behaviour?
8. **Reachable target for the weak spots.** A shelf resonance at or beyond Nyquist (up to about 5 dB off at ±6 dB, Q 10, 20 kHz) may not be reachable by one section at any fit. Report the optimum for those regions so the gate is set honestly; D7a is "near the optimum", not a fixed number.
9. **Error measure.** Maximum |dB| error on 20 Hz–20 kHz as in G1, or weighted? The polish target and the gate must use the same measure.
10. **Reference.** Extend the "best single biquad" reference (24 sampled settings, Q 0.1–10, fc 1–20 kHz) to the full G1 grid, shelves included, so "near the optimum" is measured rather than extrapolated.

Later phases (unchanged from ROADMAP / Stage 0 "Unverified"):
11. Time-varying SVF stability rests on a proof that was not read; G3 and G5 carry it (2.3).
12. CPU figures are a cost model; smoke reading at the end of 2.2.

## Not blocking

- Stage 1 hands-on DAW smoke (three items in `stages/1-foundation/VERIFICATION.md`) is still open. Worth doing before 2.2 changes the audio path.

## Next Phase

Ready for: research, Phase 2.1 and 2.1b (`RESEARCH-2.1.md`; the polish study may be split into `RESEARCH-2.1b.md` if the planner prefers).
