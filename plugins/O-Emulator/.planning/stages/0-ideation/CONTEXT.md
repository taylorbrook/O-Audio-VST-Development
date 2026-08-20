# O-Emulator — Stage 0 Context (Discuss Phase Findings)

**Date:** 2026-08-20
**Phase:** Stage 0 Research & Planning (research-planning-agent)

## What this plugin is

Authentic retro console audio emulation effect. The differentiator vs. every bitcrusher on the market: audio genuinely passes through each console's codec encode→decode round-trip (SNES BRR, PS1 SPU-ADPCM, NES DPCM, GB 4-bit wave, Genesis 8-bit DAC), at each console's true fixed sample rate, with its true interpolation (SNES/PS1 4-tap Gaussian) and output-stage character. 5 params: Console (choice) + Crush/Age/Reverb/Mix macros.

## Key decisions made during research

1. **One engine skeleton, five configs** — codec/interpolator/output-stage are strategy objects on a shared fixed-chunk pipeline. All five pre-allocated; console switch = pointer swap + 30 ms equal-power crossfade.
2. **Closed-loop brute-force ADPCM encoders** (BRR: 13 shifts × 4 filters per 16-sample block; SPU: × 5 filters per 28-sample block) — the industry-proven approach (C700, SnesPass, brr tools). Cheap at console rates.
3. **SPU reverb is a register-model port** from psx-spx (nocash) spec, running at authentic half-rate 22.05 kHz in ALL console modes, fixed Hall preset, knob = send level. Three independent open-source ports (ipatix lv2-psx-reverb, BodbDearg PlayStation1Vsts, psxverb) confirm topology + preset tables.
4. **Genesis ladder effect** uses the Nuked-OPN2 validated model: +1 LSB on all non-negative samples after 8-bit quantization (doubles the −1→0 gap → level-dependent crossover grit).
5. **NES/GB/Genesis upsample via zero-order hold** — the aliasing images are the sound; per-console output LP dulls them like real RC output stages did.
6. **Constant worst-case latency across modes** (~100–130 samples @ 48 kHz) — no mid-stream PDC renegotiation on console switch. setLatencySamples paired exactly with DryWetMixer::setWetLatency.
7. **Crush is a per-console macro curve**, not a single scalar: encoder drive + shift-floor (SNES/PS1), DPCM timer-rate walk (NES), level-step reduction (GB), DAC-rate hold (Genesis), plus AA-filter opening ≥ 80% for dosed aliasing.
8. **Age = four sub-models**: noise floor (host-rate domain, rate-invariant level), 60 Hz hum family, output-LP dulling, ±15-cent resample-ratio drift (per-sample integrated for offline/realtime parity).
9. **GPL hygiene:** implement codecs and hardware tables (Gaussian ROM, coefficients, presets) from published specs — never port blargg snes_spc / Nuked-OPN2 GPL code into this AGPL repo.

## Constraints carried from requirements

- PERF-02 block-size invariance → fixed-chunk FIFO processing; phase/state never derived from host block boundaries.
- FUNC-02 mix 0% bit-transparent minus latency → exact latency mirroring into wet mixer.
- QUAL-01 bounded output at all extremes → structural (int16 rails inside codec domain) + isfinite non-sticky guards on reverb.
- No mockup yet → parameter-spec.md finalizes at mockup sign-off; draft matches brief exactly, no design-sync conflicts.

## Complexity & strategy

- Score: 5.0 (capped; 5 params = 1.0, 13 components, +1 feedback, +1 modulation)
- Staged implementation: DSP in 4 phases (SNES end-to-end → PS1+reverb → 3 quantizer consoles+switching → Age/Crush polish), GUI in 2 phases.
- Highest risk: SPU reverb port (fallback: tuned 22.05 kHz Schroeder), then closed-loop encoders (fallback: open-loop).

## Open questions (deferred to mockup phase)

- UI detents for Crush's integer-step behaviors (NES rate table, GB level steps)?
- Console selector visual treatment (focal element per brief) — drives choice-relay binding approach.
