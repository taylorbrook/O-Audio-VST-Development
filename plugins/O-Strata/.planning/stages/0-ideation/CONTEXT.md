# O-Strata — Stage 0 Context (v2, live wave-terrain oscillator)

**Date:** 2026-09-08 (re-plan; the 2026-09-07 baked-design context is preserved at `superseded-baked-v1/stages/0-ideation-CONTEXT.md`)
**Inputs:** BRIEF.md v2, REQUIREMENTS.md v2.0.0 (28 IDs), parameter-spec-draft.md v2, evidence/*.md, research §1 / §5 / §6 / §7.2 / §7.3, terrain-bench outputs, the verified fork in `Source/`
**Outputs:** `research/ARCHITECTURE.md` (v2), `ROADMAP.md` (v2), this file, STATUS.md
**Gate report:** `gate-report.json` in this folder is the 2026-09-08 04:45 gate-0-to-1 record from the first pass (bypassed: no source to build before the fork) — kept as history.

## Goal

Turn the live-terrain brief into an implementable architecture and a phased plan that honours the fork already built, settling the twelve research items the baked plan never needed.

## Discuss findings

1. **The fork is an asset, not a blank slate.** `Source/` is O-Prism v1.24.0 renamed, wavetable library stripped, pluginval / auval verified. The oscillator interface is recorded (`WavetableOscillator.h:45-63`) and the voice call sites are exactly enumerable, so a `TerrainOscillator` with the same public methods drops in without touching the voice loop's structure.
2. **The spatial-frequency convention decides the symmetry defaults.** The Stage 0 sweep showed sin(πF x)·sin(πF y) at F = 1, r = 0.5, c = (0.13, 0.21), aspect 0.7 gives h1 = 0 dB (5 partials); the 2πF convention at the same defaults gives h1 = −46 dB. The gate is also not monotone in F (F = 2 fails at that centre, F = 4 is −8.6 dB), so it must run per preset.
3. **Degree-16 Chebyshev is narrower than the brief's framing.** Fit ≥ 94 % up to Terrain Freq 2 (Cosine Wells 76 %) and collapses beyond; per-pitch truncation makes high-K orbits dull, not aliased, at the top (Epitrochoid 7 at C6: D_max = 1). Bandlimited mode therefore clamps F ≤ 2, freezes Pitch Track, and block-rates the terrain destinations. It stays the showcase, not the default.
4. **Pitch tracking must only scale down.** Harmonic count is not proportional to F at low F (orbit geometry dominates), so scaling F up below C4 would fail DSP-01 at C2. Hard knee at C4, block-rate.
5. **`juce::dsp::Oversampling` cannot be used per oscillator.** It is block-only and pairs up + down; the oscillator synthesises at OS·fs and needs only a per-sample decimator, and the per-sample FM cross-feed forbids block rendering. JUCE's `FilterDesign` still supplies the allpass coefficients in `prepare`.
6. **Feedback is bounded by clamps alone.** Worst-case gain 1; the two-sample average is what makes Damp 0 usable (DX7 anti-hunting precedent). The displacement's DC is the intended spatial compression; the output DC blocker handles the audio.
7. **CPU: §7.2's 8.8 % is kernel-only.** The full-path model (decimator, smoothing, feedback, DC blocker) lands at ≈ 15 % for the oscillator delta at the default patch; two savings bring it to ≈ 11.4 %. PERF-02's "of one core" wording includes the inherited engine and should read "oscillator delta".
8. **No unit-test framework, CI runs no tests.** The Stage 2 gate is the render harness (O-Bowed's console-app pattern + the Stage 1 smoke pattern with `JUCE_WEB_BROWSER=0`), eleven gates H1–H11, all printing measured numbers, negative controls on the branch they target.

## Decisions taken this phase

| # | Decision | Rationale (one line) |
|---|----------|----------------------|
| D1 | Feedback = leaky integrator on a two-sample average, `d` ±0.5, `p` clamped, `y` clamped, NaN-scrubbed, reset at note-on; damp law a = 1 − 2^(−1 − 9·Damp), rate/OS-corrected; 5 Hz DC blocker on the oscillator output; Feedback 0 bit-identical (0·finite) | Boundedness from clamps; anti-hunting from the average; DC on `d` is the feature |
| D2 | Pitch tracking F_eff = F · min(1, C4/f_note)^track, block-rate, glide-target frequency | Constant count below C4 (DSP-01 at C2), 1/f above; one `pow` per block |
| D3 | Bandlimited: degree-16 triangle (153 coeffs), F clamped [0.25, 2], Pitch Track inert, truncation D_max = ⌊0.5fs/(K f)⌋ − 1 with a 2-diagonal taper; terrain destinations block-rate for global sources / bypassed for per-voice sources; F-lattice is v1.1 | Fit table; shared set across voices; keep Phase 2.4 bounded |
| D4 | PNG → Chebyshev at import on the 64² node grid after a node-matched box filter; projected at F = 1 only; fit % shown | 0.3 ms; honest quadrature; no image lattice |
| D5 | Hand-rolled per-sample polyphase-IIR decimator (2×: tw 0.06 / −70 dB, 4 coeffs; 4×→2× stage tw 0.15 / −60 dB), coefficients from `FilterDesign` in `prepare`, both stages pre-allocated per partial; constant +1 sample latency report on top of the distortion latency | Block API breaks FM cross-feed; latency 1.15 / ≈1.4 / 0 samples cannot be reported per path |
| D6 | Delete `WavetableOscillator` / `WavetableData` / `WavetableGenerator` and table plumbing in Phase 2.1; `TerrainOscillator` replaces in place; reaper generalised to a type-erased `Retirable`; sine placeholder survives through the Stage 1 second pass | No wavetable mode by brief; dormant code rots in a repo without tests; v1.1 Baked would re-fork anyway |
| D7 | Mod destinations appended (46), indices 1/2 relabelled "OscA/B Size", enum names unchanged; per-voice `SmoothedValue` on the 22 base values + ModWheel / Aftertouch ramps (5 ms); Terrain Freq mod log-domain ±2 octaves, others additive clamped | Preset indices stay valid; O-Prism's call pattern kept; fixes host and CC steps |
| D8 | Quality default = 2× | CPU parity with Bandlimited, identical behaviour for all orbits and PNGs, live terrain mod |
| D9 | Symmetry defaults c = (0.13, 0.21), aspect 0.7, r = 0.5, F = 1 under the πF convention; gate runs per preset with a (0,0)/Aspect 1 negative control; harness prints neighbouring-centre h1 for fixing | Measured 0 dB at defaults; non-monotone in F |
| D10 | Clean-room libraries: 6 terrains (πF convention; Mitsuhashi normalised 1.747; Wells' fractional power via integer-power lerp), 11 orbits (Butterfly closed with a 2π-periodic wing term; Superellipse via 33×129 LUT; Squarcle fast tanh); K per orbit: Ellipse 1, Limaçon 2, Epi p → p+1, Hypo p → p−1, Superellipse / Butterfly / Squarcle approximate | Licence (GPL vs AGPL), closure once per cycle, `pow`-free |
| D11 | PERF-02 gate = oscillator delta via a harness-only `terrainKernelBypass` baseline; planned savings: ramps shared per oscillator, fb = 0 block skip; fallback float phase path | Full-path model ≈ 15 % vs kernel-only 8.8 % |
| D12 | Harness: `tests/render-harness/` console app, `JUCE_WEB_BROWSER=0`, gates H1–H11, harness-only switches (`feedbackPathEnabled`, `terrainKernelBypass`, `HarnessIdentityX`, allocation counter), goldens as `.sha256` | Only executable gate in a repo with no test framework |

## Constraints carried into implementation

- No `pow`, `std::function`, allocation, lock or file I/O in the per-sample path (DSP-05 / PERF-01); `FilterDesign` and LUT builds only in `prepare`.
- Publish / retire only on the message thread; `prepareToPlay` and `setStateInformation` never publish.
- `getLatencySamples()` is non-virtual: always `setLatencySamples` from `prepareToPlay` and the timer.
- Every `AudioParameterChoice` ≥ 2 entries; choice lists append-only after v1.0.0; each range change after v1.0.0 needs its own migration gate (the Stage 1 second pass is the last free one).
- Path-scoped commits only; two sessions share the checkout.
- New research documents under `research/` need the 10-field frontmatter; plugin-local `.planning/research/ARCHITECTURE.md` is a contract (no frontmatter, matches siblings).

## Open questions (for Taylor)

1. **PERF-02 wording:** amend "≤ 12 % of one core" to "oscillator delta ≤ 12 %" (the whole-plugin number includes the inherited engine, which §7.2 never counted)? The harness will print both either way.
2. **Listen to *Terrain* first** (BRIEF Next Step 1) — the feedback damp law (D1) and the Bandlimited limits (D3) are the two places where the listening result could change a decision before Stage 2.
3. **Bandlimited F-lattice in v1.0 or v1.1?** v1.1 recommended; ≈ 40 lines and 13 coefficient sets if the listening pass wants LFO → Terrain Freq in Bandlimited mode.
4. **Mod-destination host strings:** short O-Prism style ("OscA CtrX", "OscA Fdbk") vs longer; decide at mockup v2.

## Next

1. `design UI for O-Strata` (ui-mockup skill) → `mockups/v2-ui.yaml/.html` → lock `parameter-spec.md` v2 (STATUS `mockup_finalized`, `ready_for_implementation: true`).
2. Set STATUS `stage: 1`, `phase: discuss`; run `/plugin-discuss O-Strata 1-foundation` for the second pass.
