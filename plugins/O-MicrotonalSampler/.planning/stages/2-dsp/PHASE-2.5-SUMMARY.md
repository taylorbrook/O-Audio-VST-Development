---
title: "O-MicrotonalSampler Phase 2.5 — Implementation Summary"
created: 2026-04-27
stage: 2-dsp
phase: 2.5
status: gate_5_pass
commit_sha: pending
---

# Phase 2.5 — Implementation Summary

## Status

**Phase 2.5 Gate 5 PASS** — code complete + automated gate green.

Triple build (VST3 + AU + Standalone) green via `cmake --build`. Cache-clearing
fresh install completed per CLAUDE.md protocol. `pluginval --strictness 5
--validate-in-process --skip-gui-tests` reports `SUCCESS`. `auval -v aumu
OMtS OuDv` reports `AU VALIDATION SUCCEEDED`. Awaiting atomic commit by user.

Subjective sustain-loop checks (sustained sine, vibrato cello, transient
percussion fallback, short-region edge case) and the Apple Silicon CPU
benchmark are deferred to `/plugin-verify` time.

## Tasks Completed (code level)

| # | Task | Files Touched | Status |
|---|------|---------------|--------|
| 29 | `LoopDetector` module created — pure-function `detectLoop(buf, sampleRate) → LoopRegion`. RMS scan (1024-window, stride 256, search range `[N*0.40, N - 1024 - 64]`); variance gate (`rms[min] / meanRms > 0.7` → invalid, EC-7); zero-crossing snap for `loopStart` (±64, falling-edge preferred); target loop length `max(2048, sampleRate / 50)` with same-direction zc match for `loopEnd`; re-scan with shrunken target if no headroom; defensive `<16`-sample length guard; final headroom check (`loopEnd <= N - 2`) for cubic-context safety. Allocation lives on the loader thread (single mono mixdown vector per slot). | `Source/LoopDetector.h`, `Source/LoopDetector.cpp` | DONE |
| 30 | `LoopDetector` wired into `SampleLoader::run` — invoked once per slot AFTER stereo-promotion + SR-conversion, BEFORE `slot` is moved into `builtSlots`. Invalid result → `loopStart = loopEnd = 0` (one-shot fallback, EC-7). DBG logs "loop detected"/"no loop region" with sample range. CMake: `Source/LoopDetector.h` + `Source/LoopDetector.cpp` added to `target_sources`. | `Source/SampleLoader.cpp`, `plugins/O-MicrotonalSampler/CMakeLists.txt` | DONE |
| 31 | 8-entry equal-power crossfade LUT (`loopXfadeLut()`) added to anonymous namespace in `MicrotonalSamplerVoice.cpp`. Built once at static-init via an immediately-invoked lambda (since `std::cos` / `std::sin` aren't yet portably constexpr in JUCE 8's supported toolchains). Index `i ∈ [0..7]` maps to fade position `x = i / 8`; first = outgoing weight, second = incoming. `<array>` include added. | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 32 | Loop-wrap + 8-sample crossfade implemented in voice DSP. Two new free helpers in the anonymous namespace: `readSlotWithLoop(buf, N, pos, lpStart, lpEnd) → float` (consolidates one-shot `EC-4` clamp + standard cubicInterp + crossfade region into a single `noexcept` call) and `wrapLoopPosition(pos&, lpStart, lpEnd)` (idempotent `while`-loop wrap, defensive against high-playRate skip-overs). Both `renderNextBlock` and `renderTailRamp` per-sample loops simplified — same helper for both, applied independently to `slotLow` and `slotHigh` cursors. Phase 2.3 one-shot path stays bit-exact. | `Source/MicrotonalSamplerVoice.cpp`, `Source/MicrotonalSamplerVoice.h` | DONE |
| 33 | Phase 2.5 Gate 5 verification — orchestrator ran triple build (10/10 ninja targets), cache clear + fresh install per CLAUDE.md protocol, `pluginval --strictness 5 --validate-in-process --skip-gui-tests` (SUCCESS), `auval -v aumu OMtS OuDv` (AU VALIDATION SUCCEEDED). Atomic commit deferred to user. Subjective DAW checks deferred to `/plugin-verify`. | (build artefacts) | DONE |

## Crossfade Geometry

Within `[loopEnd - 8, loopEnd)` the renderer reads two cubicInterp samples:

- **`outSample`** = `cubicInterp(buf, N, pos, /*no-wrap*/ 0, 0)` — 4-tap context
  spans `(loopEnd-9 .. loopEnd+1)` at the start of the fade and
  `(loopEnd-2 .. loopEnd+2)` at the last fade sample. The `i+2` tap reads
  natural source continuation past `loopEnd` (the recording's tail), giving
  the "outgoing" component a smooth pre-wrap decay.
- **`inSample`** = `cubicInterp(buf, N, pos - loopLen, lpStart, lpEnd)` — 4-tap
  context wraps via `cubicInterp`'s loop helper. At the start of the fade the
  context taps land at `(loopStart-9 wrapped, loopStart-8 wrapped, loopStart-7 wrapped, loopStart-6 wrapped)`,
  i.e. the audio that would already be playing AFTER the wrap. By the last
  fade sample, taps land at `(loopEnd-2, loopEnd-1, loopStart, loopStart+1)`,
  bridging the wrap discontinuity.

Output = `outSample * w.first + inSample * w.second`, where `w` is the
equal-power LUT entry for the integer floor of `pos - (loopEnd - 8)`.

The 4-tap `i+2` tap of the outgoing read fetches up to `loopEnd + 2`, which
is why `LoopDetector::detectLoop` enforces `loopEnd <= N - 2` (otherwise the
out-of-bounds tap clamps to `buf[N-1]` and the fade gets an unintended click).

## LoopDetector Behaviour Summary

| Input | Behaviour | Outcome |
|-------|-----------|---------|
| Sustained sine / vibrato cello (≥ 5 s, steady RMS in 40-100 % zone) | RMS scan finds a low-RMS region; variance gate `rms[min] / meanRms ≤ 0.7`; zc snap succeeds | `valid = true`, looped playback |
| Kick-drum impulse / transient percussion | All RMS values ≈ 0 OR `rms[min] / meanRms > 0.7` (no clear quiet region) | `valid = false`, one-shot fallback |
| Short sample where the only quiet region < 16 samples after zc snap | `loopEnd - loopStart < 16` defensive guard | `valid = false`, one-shot fallback |
| `loopEnd > N - 2` (no cubic-context headroom) | Final headroom check | `valid = false`, one-shot fallback |
| Buffer < 1024 + 64 + 1 samples after the 40 % start | `searchEnd <= searchStart` | `valid = false`, one-shot fallback |

## Real-Time Safety Audit

- **`processBlock` allocation-free**: no new heap touch in audio path.
- **`renderNextBlock` allocation-free**: per-sample loop calls `readSlotWithLoop`
  (free function, all-stack) and `wrapLoopPosition` (no allocs). LUT is a
  function-local `static const std::array<…, 8>` initialized once at first
  call (Meyers singleton-style); subsequent calls hit only the cached array.
- **`renderTailRamp` allocation-free**: same helpers; per-sample writes into
  pre-allocated steal buffers. `noexcept` preserved.
- **LoopDetector allocation cost**: single `std::vector<float>` mixdown per
  slot, on the **loader thread** (RT-safe by isolation). No alloc in the
  audio path. Logs only via `DBG` (compiled out in Release).
- **Lock-free**: no mutex / `juce::CriticalSection` / `std::lock_guard` introduced.
- **TSan-clean**: no inter-thread reads/writes added. Loop fields populated
  on the loader thread BEFORE `std::atomic_store` of the new `SampleMap`
  shared_ptr — voices snapshot the map under the same atomic guard used
  in Phase 2.3.

## Files Touched (Phase 2.5 only)

| File | Action |
|------|--------|
| `Source/LoopDetector.h` | Created — pure-function `LoopRegion detectLoop(buf, sampleRate)` declaration in `LoopDetector` namespace. |
| `Source/LoopDetector.cpp` | Created — RMS scan + variance gate + zero-crossing snap + length guard implementation. |
| `Source/SampleLoader.cpp` | Modified — added `LoopDetector.h` include; called `LoopDetector::detectLoop` per slot AFTER stereo-promote, populated `slot.loopStart`/`slot.loopEnd` (or 0/0 on invalid) with DBG logs. |
| `Source/MicrotonalSamplerVoice.h` | Modified — updated docstring header to include Phase 2.5 description. |
| `Source/MicrotonalSamplerVoice.cpp` | Modified — added `<array>` include; added `loopXfadeLut()` static-init LUT, `readSlotWithLoop` and `wrapLoopPosition` free helpers in anonymous namespace; replaced per-slot reads in `renderNextBlock` and `renderTailRamp` with `readSlotWithLoop`; replaced advance-only cursor steps with advance + `wrapLoopPosition`; expanded file-level Phase 2.5 implementation notes. |
| `plugins/O-MicrotonalSampler/CMakeLists.txt` | Modified — added `Source/LoopDetector.h` and `Source/LoopDetector.cpp` to `target_sources`. |
| `.planning/stages/2-dsp/PHASE-2.5-SUMMARY.md` | Created (this file). |
| `.planning/STATUS.md` | Updated resume point to Phase 2.5 code-complete (Stage 2 substages: all 5 done). |

**No other source files touched** (`PluginProcessor.cpp/.h`, `PluginEditor`,
`SampleMap.h`, `FilenameParser`, `MicrotonalSamplerSound.h` — unchanged).
No new module dependencies. No new external libs.

## Items Deferred to /plugin-verify (subjective DAW + CPU benchmark)

Per the project's pattern, the DSP agent does not exercise these
autonomously. Becomes Gate 5 acceptance items for the user to verify
before declaring Stage 2 closed:

1. **Triple build** (VST3 + AU + Standalone) — green in Release config. ✓ done by orchestrator.
2. **Cache-clearing install** per CLAUDE.md. ✓ done by orchestrator.
3. **`pluginval --strictness 5 --validate-in-process --skip-gui-tests`** — `SUCCESS`. ✓ done by orchestrator.
4. **`auval -v aumu OMtS OuDv`** — `AU VALIDATION SUCCEEDED`. ✓ done by orchestrator.
5. **Subjective DAW test** — load a SampleMap and verify:
   - **Sustained sine (5 s, 440 Hz):** loops at the same level forever, no audible crossfade artifact. Loop detector should pick a steady region in the 40-100 % zone.
   - **Vibrato cello sample** (synthesize: 440 Hz with 5 Hz vibrato, 5 s): loops without pitch jump or click.
   - **Transient percussion** (synthesize: kick-drum impulse): falls back to one-shot (`region.valid == false`) → ADSR release ends voice (EC-7).
   - **Short loop region edge case:** sample where the only low-RMS region is < 16 samples → defensive guard fires, falls back to one-shot.
   - **Acceptance suite re-run:** all previous gate fixtures (4-sample-set, 4-layer, 16-voice steal) — no regressions.
6. **CPU benchmark:** Apple Silicon, 48 kHz / 256 buffer, 16 sustained voices at full polyphony with looping samples → measure CPU% via Logic Pro CPU meter or `pluginval --benchmark-voices`. Target: ≤ 5 % (PERF-02). If above 5 %, investigate cubic-Hermite hot path; consider whether the conditional pre-filter from Gate 1 was overzealous.
7. **Latency audit:** `getLatencySamples()` returns 0 (PERF-04). The getter is non-virtual in JUCE 8; default value is 0; no override needed.
8. **Full requirement traceability** — verify FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01..04, COMPAT-02, QUAL-01 all satisfied by walking REQUIREMENTS.md acceptance criteria one by one. Update `STATUS.md` with a green check per requirement.

If any subjective check fails, file a defect and pause Stage 2 close-out.

## Recipe for User to Close Gate 5

```bash
# Build (project root) — already done by orchestrator
cd /Users/taylorbrook/Dev/VST-development/build
cmake --build . --target O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone

# Cache clear + fresh install (per CLAUDE.md) — already done by orchestrator
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-MicrotonalSampler-dev.component
cp -R /Users/taylorbrook/Dev/VST-development/build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/VST3/O-MicrotonalSampler-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R /Users/taylorbrook/Dev/VST-development/build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/AU/O-MicrotonalSampler-dev.component ~/Library/Audio/Plug-Ins/Components/

# pluginval (strictness 5, allocation-guard ON) — already done; SUCCESS
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness 5 --validate-in-process --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3

# auval — already done; AU VALIDATION SUCCEEDED
auval -v aumu OMtS OuDv

# Atomic commit (only after build + pluginval + auval green)
cd /Users/taylorbrook/Dev/VST-development
git add plugins/O-MicrotonalSampler/Source/LoopDetector.h
git add plugins/O-MicrotonalSampler/Source/LoopDetector.cpp
git add plugins/O-MicrotonalSampler/Source/SampleLoader.cpp
git add plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.h
git add plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp
git add plugins/O-MicrotonalSampler/CMakeLists.txt
git add plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.5-SUMMARY.md
git add plugins/O-MicrotonalSampler/.planning/STATUS.md
git commit -m "feat(O-MicrotonalSampler): loop auto-detect + 8-sample equal-power crossfade - Phase 2.5 Gate 5 PASS"
```

## Next Phase

**Stage 2 verify** — `/plugin-verify O-MicrotonalSampler 2-dsp`

Gate 5 closes the 5-stage Stage 2 plan. After verify, the plugin can move
to **Stage 3 (GUI)**: WebView UI mockup → integration → parameter binding.
