# Verification: Spectral Diffusion Freeze

**Plugin:** O-Freeze
**Milestone:** spectral-diffusion-freeze
**Created:** 2026-10-28
**Phase:** Verify

---

## Verification Summary

**Status:** PASSED WITH NOTES
**Version:** 2.8.0 → 2.9.0

Each success criterion was re-run in this session on a fresh build. The execute phase's numbers were not reused.

- **Stale install found and fixed.** The installed binary was stale: the 21:49 install predates the 21:52 edit to `PluginEditor.cpp`. It was rebuilt and reinstalled (`build-and-install.sh`, 0 warnings) before any gate ran.
- **Reference build.** The execute-phase v2.8.0 reference build was gone, so it was rebuilt out of tree from `git archive 26bbaeb6`.
- **Render harness.** It was built out of tree from the working tree with `OUARICON_BUILD_TESTS=ON`, so the main `build/` cache was not reconfigured.

---

## Success Criteria (CONTEXT.md)

| # | Criterion | Status | Evidence (this session, installed build) |
|---|---|---|---|
| 1 | Engine = Granular bit-exact vs v2.8.0 | ✓ PASS | The `granular-null` gate passed. Every config × seed × block size (512/32/441/7) is bit-exact (worst difference 0). Every v2.8.0 state loads as Granular, Diffusion 0 %, Avg 8. |
| 2 | Spectral bit-exact across block sizes; reproducible from seed | ✓ PASS | The `blocksize-null` gate passed. The 4 Spectral configs (β 0, β 0.6 + LFO, threshold β 1, engine switch) are bit-exact at 32/441/7 vs 512, and 6 seeds are pairwise distinct per config. |
| 3 | β sweep monotone; β = 0 tonal | ✓ PASS | Peak-to-floor falls 75.6 → 49.2 → 40.9 → 35.3 → 31.6 dB. Circular variance rises 0.0000 → 0.0403 → 0.1655 → 0.2990 → 0.3801. CV at β 0 is 0.0000 (limit < 0.05). |
| 4 | Avg N = 8 less metallic than N = 1 | ✓ PASS | Residual std is 2.35 dB at N = 1 and 1.13 dB at N = 8. The ratio is 0.48 (limit 0.6); the dry pink control is 0.31 dB. |
| 5 | 0 allocations in processBlock, Spectral engaged | ✓ PASS | The harness ran `--alloc` at 48 k and 192 k, block 32: 0 allocations across 3 captures, including 2 fallback captures and both switch directions. The hook was first proven live with a deliberate malloc. |
| 6 | No click on engage / release / engine switch | ✓ PASS | Click ratios: engage 0.86, release 0.96, Granular → Spectral 0.86, Spectral → Granular 0.50 (limit 1.5 × the same-motion control). Hop-rate AM is 0.092 / 0.067 dB (limit 0.5). |
| 7 | check-i18n, fr/zh lint, check-ui-labels | ✓ PASS | check-i18n: ALL CHECKS PASS (46 plugins). fr-lint: CLEAN. zh-lint: 0 findings. check-ui-labels: ALL CHECKS PASSED. `ui_tip_render_check.js`: 426 passed. |
| 8 | Build without warnings; pluginval 10, auval | ✓ PASS | The rebuild had 0 warnings. pluginval strictness 10: SUCCESS. `auval -v aufx OFCR OuDv`: AU VALIDATION SUCCEEDED. |

### Other requirements

| Requirement | Status | Note |
|---|---|---|
| STFT 8192 / 4×, Hann × Hann, 1/1.5 OLA, DC / Nyquist real | ✓ | Code review of `SpectralFreeze.cpp`. The FFT identity check reports scale 1 (residual 4.2e-7). |
| Frozen output carries signal, 48 k and 192 k | ✓ | Chord −15.4 / −16.7 dBFS; pink −23.5 / −24.5 dBFS. |
| Level across β within ±3 dB (D2, constant 0.57) | ✓ | Chord is 0 to −2.40 dB; pink is 0 to +2.35 dB. |
| Granular vs Spectral β 0 within 1 dB | ✓ | −0.56 dB. |
| Analysis on the write-head clock, held through freeze and tail | ✓ | Harness `--probe`: all 11 checks PASS. |
| Per-bin drift walk, 0.2–3 Hz, deterministic | ✓ | σ = Drift × 1.5 dB (about ±3 dB at 2σ). The rate is log-uniform per bin, drawn per engage. |
| Detune = per-bin spread, drawn once per engage | ✓ | `detuneR` is drawn from the engage counter. |
| Old sessions / automation order preserved | ✓ | The params are appended after DETUNE. Old-state loads were checked in the Granular null. |
| UI: engine toggle, knob swap (D1), Reverse dims, LFO label | ✓ | Headless capture: Spectral shows Diffusion / Avg in the Size / Grains slots, Reverse dimmed, and "Diffusion LFO". Relays and attachments for ENGINE, DIFFUSION and AVG_FRAMES are present in the editor. The JS knob ranges (0–100 and 1–16) match the C++ params. |

---

## Build Verification

- `./scripts/build-and-install.sh O-Freeze`: success, 0 warnings, VST3 and AU installed (log `logs/O-Freeze/build_20261028_215857.log`).
- pluginval `--strictness-level 10` on the installed VST3: SUCCESS.
- auval: `aufx OFCR OuDv` is registered (`O-Freeze-dev`). `auval -v`: AU VALIDATION SUCCEEDED.
- Timing (`--timing`, 48 k, block 32, 20 s frozen):
  - Worst steady block 161.8 µs and p99.9 121.4 µs, against a budget of 333 µs (half of the 667 µs period). PASS.

---

## Known Issues / Notes

1. **[RESOLVED in 2.9.1: engage work spread over 32-sample prime steps; engage-window worst block 60-150 us]** **Engage block exceeds the half-period budget.** The first engage block takes 332 µs; re-engages have a median of 334 µs and a maximum of 410 µs. The budget is 333 µs (half the 667 µs period at 32 / 48 k). It is still under the full block period. The candidate fix is to spread the 4 primed frames across the first hop. This is not gated; it is carried from the SUMMARY open issues.
2. **Translations are unreviewed.** fr has 12 entries at `reviewed: false`; zh has 12 at `'mt'`.
3. **[RESOLVED in 2.9.1: now "Ouaricon Freeze"]** **The header title still reads "Ouaricon Granular Freeze".** That is now inaccurate with a Spectral engine. It was out of scope and left as is.
4. **The headless stub shows wrong readouts.** `scripts/ui-stub/generic-juce-stub.js:154` gives every parameter a neutral 0–1 range at 0.5, so readouts in the capture show 1 % and 1. This affects every knob and is not a binding bug. The real ranges are confirmed in `setupKnobs()`. A DAW / Standalone check is still recommended to confirm live binding.
5. **Binaries are platform-specific.** vForce is used on macOS, so Spectral renders are deterministic per platform but not bit-identical between macOS and other platforms.

---

## Human Verification Needed

- In a DAW, with Engine = Spectral, sweep Diffusion while frozen. Expected: tonal at 0 %, a wash at 100 %, and no zipper.
- Switch the engine while frozen and listen for clicks or level steps.
- Load a v2.8.0 session and confirm it sounds identical.

---

*Generated by improve-milestone verify phase*
