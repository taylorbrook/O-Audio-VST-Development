# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-10-05
**Participants:** User, Claude
**Source contracts:** BRIEF.md, research/ARCHITECTURE.md (18 resolved questions, §A1–A8), ROADMAP.md §Stage 2 (4 phases), parameter-spec.md (21 params, locked), stages/1-foundation/{CONTEXT,VERIFICATION}.md.

ARCHITECTURE.md already fixes the algorithms. This discussion covered only the decisions it left open: execution cadence, velocity curve, mono legato, and the Interp-Off step click.

## Requirements Confirmed

- **Phase 2.1, Bank Engine:** `WavetableBank`, `BankFactory` (Sine→Saw, Sine→Square, Pulse Width, Formant, Drive per §A3–A7), `MipmapBuilder` (11 levels, `Kmax_L = min(1023, 1024 >> L)`, DC + Nyquist bins zeroed), and `SharedResourcePointer<BuiltInBanks>`. Offline FFT gates per ROADMAP: 32 × 2048, peaks 1.0 ± 0.5 dB, DC ≤ −80 dB, Saw frame k = h1..k, Square evens ≤ −60 dB and frame 32 top = h31, Drive h3/h5/h7 strictly monotonic with frame 1 h3 ≈ −46 dB, level L ≤ −120 dB above Kmax_L. Build time is logged.
- **Phase 2.2, Voice + Oscillator:** `WtVoice` (double phase, linear in-cycle read + guard sample, frame lerp vs cycle-wrap latch), strict mip level, mid-rise quantizer (Full = bit-identical bypass), amp ADSR (dirty-checked `setParameters`), Poly 16 / Mono, ±2 st bend, output stage (−60 dB = 0 gain), `isfinite` scrub, lead-voice display atomics. Gates: FUNC-01 ±1 cent A0–C8; **QUAL-02** C8 at 44.1/48/96 kHz ≤ −100 dB inharmonic, A0–C8 sweep ≤ −70 dB; DSP-01/02/03; FUNC-07; PERF-01 alloc gate.
- **Phase 2.3, Modulation → Position:** 20 ms knob `SmoothedValue`, a global per-sample LFO (5 shapes, free 0.01–20 Hz / 16-division tempo sync from PPQ, BPM fallback 120), deterministic S&H RNG, per-voice mod ADSR × `env_amount`, clamp to [0,1], and a per-voice 2 ms one-pole (Interp On only, seeded at note-on). Gates: DSP-05, FUNC-05 (synthetic AudioPlayHead), FUNC-06, and a click detector on Square/S&H at 100% depth.
- **Phase 2.4, Bank Switch / Import / Persistence (HIGH risk):** per-block bank resolve, a frozen-cycle 5 ms crossfader (triggers: bank pointer, mip level, interp, bandlimit), the atomic imported-bank publish + verbatim Prism REG-01 reaper, the §A8 importer worker, and `IMPORTED_BANK` (flac16 + `pcm16gz` fallback reader). Gates: QUAL-03, DSP-06, FUNC-03 (215 / 256 / reject), COMPAT-03 (WAV/AIFF/FLAC at 44.1/48/96), FUNC-04 bit-identical reload, reaper quiescent + rendering rules, and no leak under rapid repeated imports.

## Constraints Identified

- **No UI exists until Stage 3.** Import must therefore be triggerable from code. Expose a public processor entry point (e.g. `requestImport (const juce::File&)`) plus a status query that the harness drives. Stage 3 wires the button and the `importStatus` event to the same entry point.
- ASan hangs at init on macOS 26 (project memory). Lifetime safety in 2.4 is gated by a scripted import-during-held-notes render, the alloc gate, and reaper counter assertions, not by sanitizers.
- The reaper needs the entry counter, because the generation reaper freezes when the host idles the audio thread (project memory). Test both the quiescent rule and the +2 rendering rule.
- Do **not** port O-Prism's trilinear mip blend: it fails QUAL-02 at −15.6 dB. Zero the Nyquist bin.
- `IIR::Filter` is not used here. Every per-voice `SmoothedValue` / one-pole must still be seeded at note-on, and `prepareToPlay` must not fade in from silence (project memory: gain ramp without seeding).
- Amp ADSR `setParameters` is dirty-checked, never called per block unconditionally (project memory: per-block ADSR kills release).
- Determinism: S&H RNG reseeded in `prepareToPlay` and phases reset at note-on, so offline renders are byte-stable for golden gates. Render jobs must not share a seed (common-random-numbers trap).
- The installed binary is verified with pedalboard via `uv run --python 3.12 --with pedalboard`, with params set by raw_value.
- Build/install via `./scripts/build-and-install.sh O-simpleWavetable`. Commits are path-scoped (`plugins/O-simpleWavetable`, `PLUGINS.md`) using the shared-checkout temp-index discipline.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Execution cadence | **2.1 + 2.2 → build/install → user listening checkpoint → 2.3 + 2.4** | Catches tone and feel problems (bank character, aliasing A/B, stepping) before the high-risk import/reaper work. The generic editor is the test surface. |
| Velocity → amplitude | **Squared:** `gain = (v/127)²` | Perceptually even, and soft playing gets genuinely quiet. |
| Mono legato | **True legato:** an overlapping note changes pitch only; amp and mod env continue | Subtractive `renderMonoLegato` pattern, as ARCHITECTURE §11 specifies. Last-note priority, and the release returns to the previous held note. |
| Interp-Off step on Pulse/Imported | **Accept as-is** (no micro-fade) | Stepping is the lesson, and the small click on non-sine-phase frames is honest. Revisit only if Stage 4 listening finds it harsh (Risk 5 fallback: 1 ms micro-fade for Pulse/Imported). |
| Algorithms | **As ARCHITECTURE.md** (linear read, strict 11-level mips, latched stepping, frozen-cycle crossfade, mid-rise, FLAC16 state, message-thread cycle/FFT) | Already resolved and numerically verified in Stage 0. Not re-litigated. |
| Prior sign-offs carried in | Empty Imported = silence + prompt; ±2 st bend; mid-rise; reject < 2048 samples; `lfo_div` default 1/1; per-frame normalize capped at +24 dB | Stage 1 CONTEXT, user 2026-10-05 |
| Mid-rise DC on digital-silent imported slices (Risk 8) | Accept (default, not asked) | Affects only bit-reduced + silent slices. The DSP-03 gate uses built-in banks. |

## Open Questions (for research)

- The exact Prism REG-01 reaper source to port verbatim (file and commit in O-Prism), and whether the entry-counter fix is already in it.
- JUCE 8.0.15 FLAC writer/reader to and from `MemoryOutputStream` / `MemoryInputStream` for mono 16-bit at an arbitrary nominal rate. Confirm lossless int16 round trip.
- Which O-simpleSubtractive `renderMonoLegato` code to reuse, and whether its voice-steal handling matches the 16-voice Poly path here.
- The shape of the harness: a standalone test executable vs. a pedalboard-driven installed-binary gate for each phase, plus the reuse of existing click-detector / alloc-gate helpers (see index_dsp_gate_design).
- Imported-bank memory: whether to alias the mip levels for an imported bank (§Core 1 option) or accept about 23 MB per instance.

## Next Phase

Ready for: research phase
