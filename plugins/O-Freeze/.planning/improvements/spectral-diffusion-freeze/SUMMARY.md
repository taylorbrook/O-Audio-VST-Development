# Summary: Spectral Diffusion Freeze (O-Freeze 2.8.0 → 2.9.0) — Execute, 2026-10-28

Result: T1–T7 complete, all gates PASS on the installed build. Not committed, no tag.

## Tasks
- **T1:**
  - Params appended after DETUNE: ENGINE (Choice, 0), DIFFUSION (0–100 %, 0), AVG_FRAMES (Int 1–16, 8).
  - RNG slots `kSlotSpecPhase` / `kSlotSpecDrift` / `kSlotSpecEngage`. Engage draws use counter `engage·8194 + k` for detune and `+4097` for walk rate.
  - Engine mix is a hand-rolled 50 ms linear ramp, snapped when idle. The Granular branch tests integers (`engineTarget == 0 && engineRampRemaining == 0`), never `e == 0.0f` and never ×1.0f.
  - `startGranularVoices()` extracted draw for draw.
  - Release: `grainDur + 50 ms` unchanged; `max(…, 100 ms)` only while Spectral runs.
  - No new warnings: the 10 unique sites are all v2.8.0 lines; new files have 0.
- **T2:**
  - Out-of-tree v2.8.0 reference (`git archive 26bbaeb6`) built in the scratchpad.
  - 24/24 bit-exact; frozen RMS −15.0 to −18.4 dBFS.
  - Negative control (`densityGain * 1.0000001f`) failed 20/24 (−138.5 dBFS), then reverted.
  - Verdict tightened to exact equality.
  - Re-run after T4+T5 (one combined build) and on the installed T7 binary: 24/24.
- **T3:**
  - FFT built in the constructor; all buffers sized for 2 channels.
  - One `analyseFrame()` serves rolling and fallback analysis.
  - Capture: RMS average; instantaneous frequency `ωH = kπ/2 + princarg(Δφ − kπ/2)`; peaks ±2 bins above −60 dB with regions split at the minimum; DC/Nyquist kept real.
  - Harness: FFT scale 1 (residual 4.2e-7); probe passes (23 hops/s, frozen through freeze and tail, rolling and fallback captures, mid-freeze switch).
- **T4:**
  - `synthesiseFrame()` holds the whole per-hop update.
  - Soft identity lock with λ = max(0, 1−β/0.3)².
  - OU drift walk, σ = Drift × 1.5 dB.
  - Draws shared across L/R; overlap-add primed with 4 frames.
  - D2 constant 0.57: at 0.5 the chord reached −2.97 dB at β=1.
  - Installed levels vs β (0/25/50/75/100 %): chord 0 / −1.70 / −2.44 / −1.92 / −2.43 dB; pink 0 / +0.54 / +0.93 / +2.17 / +2.32 dB.
  - Spectral vs Granular β=0: −0.31 to −0.70 dB (power mean of 6 Granular seeds), so trim = 1.0.
  - Signal: chord −15.4 / −16.7 dBFS (48 k / 192 k); pink −23.5 / −24.5 dBFS.
- **T5:**
  - Block-size gate +4 Spectral configs: bit-exact at 32/441/7 vs 512, 6 seeds distinct each.
  - Peak-to-floor 75.6 / 48.6 / 40.9 / 36.6 / 32.3 dB; circular variance 0.0000 / 0.0458 / 0.1826 / 0.2559 / 0.3426.
  - Metallic residual: N=1 2.33 dB, N=8 1.13 dB (ratio 0.48, limit 0.6).
  - Hop-rate AM: 0.092 / 0.067 dB (limit 0.5).
  - Clicks: engage 0.86, release 0.96, G→S 0.87, S→G 0.59 (limit 1.5).
  - Allocations: 0 at 48 k and 192 k, block 32 (hook proven live first).
  - Timing at 48 k / 32 over 20 s: worst 99 µs, p99.9 72 µs; engage about 327–340 µs.
  - vForce (`vvsincosf` / `vvatan2f`) optimisation took the engage block from 721 µs to about 330 µs. Alt 2 not needed.
- **T6:**
  - Relays and attachments appended in `PluginEditor`; JS ↔ relay parity 15/15.
  - Engine toggle beside Mode.
  - Diffusion / Avg knobs swap into the Size / Grains slots; Reverse dims; LFO label swaps to "Diffusion LFO".
  - Swap is driven by the ENGINE parameter state (preset load / automation update live).
  - DIFFUSION caption tracking 0.8 px, slot move 0.00 px.
  - New i18n keys: 3 labels and 9 tips; `SPECTRAL_TIP_KEYS` applied through a MutationObserver outside the shared i18n code; I18N_EXEMPT gains Granular / Spectral. fr `reviewed: false`, zh `'mt'`.
  - Gates: check-i18n 0, fr/zh lint 0 findings, check-ui-labels PASS (default / Spectral / popover × en/fr/zh-Hans), tip check 426 PASS.
- **T7:**
  - Version 2.9.0; build-and-install OK.
  - pluginval strictness 10 SUCCESS; auval SUCCEEDED.
  - All gates re-run on the installed build.
  - CHANGELOG, README, USER_GUIDE and PLUGINS.md updated.

## Deviations
1. D2 constant 0.57 (was 0.5).
2. Spectral trim 1.0.
3. Null gate requires exact equality (4/24 negative-control renders landed at −144.5 dBFS, under the planned −140 floor).
4. Engine-switch click control is max(Mix-0, each engine steady) — steady Granular alone reaches 1.37× dry depending on seed.
5. Mid-freeze Spectral → Granular draws a fresh drift offset (`engageCounter++`).
6. Rolling-analysis frames are not reset at capture.
7. The T2 re-run after T4 was combined with T5.
8. vForce on macOS, with std:: fallbacks elsewhere — deterministic per platform, not bit-identical across platforms.
9. Editor relays/attachments for ENGINE / DIFFUSION / AVG_FRAMES were missed at first (headless gates blind to it), then added and fully re-validated on the final install.

## Files changed
- Modified: `CMakeLists.txt`, `Source/PluginProcessor.h/.cpp`, `Source/PluginEditor.h/.cpp`, `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `tests/blocksize-null/gate.py`, `tests/i18n-states.json`, `tests/ui_tip_render_check.js`, `CHANGELOG.md`, `README.md`, `docs/USER_GUIDE.md`; root `PLUGINS.md` (O-Freeze row).
- New: `Source/SpectralFreeze.h/.cpp`, `tests/granular-null/gate.py`, `tests/spectral/gate.py`, `tests/render-harness/main.cpp` (console target `O-Freeze-render-test`, built only with `OUARICON_BUILD_TESTS`).

## Open issues
1. Engage block about 330 µs, just over the half-period budget at 32 / 48 k (under the 667 µs block period). Fix would be to spread the primed frames.
2. Translations unreviewed (fr 12 entries `reviewed: false`, zh 12 entries `'mt'`).
3. Pink noise at β=0 is about 2.3–3 dB below the dry level (within gates).
4. README / USER_GUIDE text from the v1.0 era is otherwise stale.
5. The header title still reads "Ouaricon Granular Freeze".
