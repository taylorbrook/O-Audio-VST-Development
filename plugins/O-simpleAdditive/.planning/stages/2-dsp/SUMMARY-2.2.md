# Stage 2 / Phase 2.2 — Scan/Morph + Mod-Env + LFO — SUMMARY

**Date:** 2026-06-22
**Goal:** The wavetable dimension — Frame A → Frame B spectral morph driven by manual scan, a global sine LFO, and a per-voice mod-envelope; zipper-free (FUNC-02/03, DSP-03/06).
**Gate (incremental-DSP mode):** clean build + `auval` — **MET**.

## What was implemented

### `Source/AdditiveVoice.h`
- **Frame B presets** — `OSimpleAdditive::fillFrameB(source, out[16])` free function + `FrameBSource` enum (`Sine/Saw/Square/Odd`). Raw (un-normalized) amplitude vectors; `refillTable()`'s shared headroom divide keeps the morph endpoints balanced against Frame A:
  - **Sine** = H1 only; **Saw** = `1/k`; **Square** = odd-only `1/k`; **Odd** = odd-only `1/m` (flatter hollow vs Square's `1/k`). Matches ARCHITECTURE.md §"Frame B preset shapes".
- **Spectral morph** in `refillTable(float scan)` — `active_k = frameA_k + scan·(frameB_k − frameA_k)` (per-partial linear *spectral* lerp), applied **before** band-limit + sum, so what's heard == what will be displayed. Phase-coherent → zipper-free (DSP-03).
- **Per-voice mod-env** — second `juce::ADSR modEnv` (independent of amp env): `setSampleRate` in prepare, `noteOn`/`noteOff`/`reset` mirrored with the amp env. Advanced per-sample in the render loop so its timing is exact; voice lifetime stays keyed on the **amp** env only (a long mod release never keeps a silent voice alive).
- **Scan resolve (control-rate)** — `scanTarget = clamp(scanBase + lastModEnv·scanEnvAmount, 0, 1)` per block, fed to a 20 ms `SmoothedValue` (`scanSmoothed`). The table refills only when the smoothed scan moves > `kScanRefillEps` (1e-4), so a static patch keeps the once-per-note refill cadence while a moving morph refills at control rate. `lastModEnv` is the mod-env value from the end of the previous block (sub-block latency, harmless under the 20 ms smoother).
- **Note-on seed** — `startNote` seeds `scanSmoothed` to the note's starting scan (mod-env ≈ 0 at retrigger) with `setCurrentAndTargetValue` (no ramp from a stale value) and refills the table at that scan.
- **`setParams` extended** — now `(frameA[16], frameB[16], scanBase, scanEnvAmount, ampParams, modParams)`; dirty-checks both Frame A and Frame B (exact compare).

### `Source/PluginProcessor.{h,cpp}`
- **Global scan LFO** — single sine shared by all voices (all notes morph in phase). `lfoPhase` member advanced once per block by `scanLfoRate·numSamples/fs`; `scanBase = scanPosition + sin(2π·lfoPhase)·scanLfoDepth` (bipolar swing around the manual position).
- **Frame B resolved once per block** via `fillFrameB((int) frameBSource, frameB)` and pushed to all voices (per-voice dirty-check absorbs the unchanged case).
- **`pushParamsToVoices(int numSamples)`** — now also reads `frameBSource`, `scanPosition`, `scanLfoRate`, `scanLfoDepth`, `scanEnvAmount`, and the mod ADSR; advances the LFO; pushes the new 6-arg `setParams`. Called from `processBlock` with the block size.

## Verification
- **Build:** VST3 + AU compile clean (only the pre-existing Stage-1 `processorRef` unused-private-field warning from the `GenericAudioProcessorEditor` placeholder — resolves when the WebView editor lands in Stage 3). DSP code warning-free.
- **auval:** `aumu OSiA OuDv` → **AU VALIDATION SUCCEEDED**. Render tests pass at 11025/22050/44100/48000/96000/192000 Hz, mono + stereo, 64–4096 frames; bad-max-frames handled; parameter set/ramp PASS; **MIDI test PASS** (voice triggers/renders/releases without NaN, denormal, or crash with the new morph + dual-env path exercised).
- **No-regression:** default patch (`scanPosition`/`scanLfoDepth`/`scanEnvAmount` = 0) gives `scan = 0` ⇒ `active_k = frameA_k` — bit-identical to the Phase 2.1 "first sound" (Frame A drawbars). The morph adds a dimension without changing the default tone.

## Audible criteria — for DAW/Standalone confirmation (not auval-checkable)
- [ ] scan=0 → Frame A; scan=100% → Frame B; intermediate = smooth spectral morph (FUNC-02).
- [ ] Manual scan, LFO sweep, and mod-env each move the morph pointer; all three sum (FUNC-03).
- [ ] Held note morphs over time under LFO and/or mod-env; scope waveform visibly morphs (UI confirmation in Stage 3).
- [ ] No zipper noise on scan automation or fast LFO (DSP-03).

## Not in 2.2 (deferred to Phase 2.3)
- Spectral-decay tilt (`spectralDecay`, `velToDecay`), bit-depth quantizer (`bitDepth`), lock-free viz tap + active-spectrum snapshot. `refillTable()` still carries the marked extension point where spectral-decay composes (after morph, before band-limit + sum).
