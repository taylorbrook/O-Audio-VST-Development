---
phase: O-Prism code review
reviewed: 2026-09-22T00:00:00Z
version_reviewed: 1.26.0
depth: deep
files_reviewed: 38
files_reviewed_list: all Source/*.cpp|h, Source/dsp/*.cpp|h (excl. generated GeometryTablesData.h), ui/public/index.html, ui/public/js/*.js
findings:
  critical: 3
  warning: 9
  info: 10
  total: 22
status: issues_found
verified: 2026-09-24T00:00:00Z
verified_resolution: 1.26.1 (CR-03, WR-03, WR-07, WR-09 + the startNote pitch-wheel seed);
  1.27.0 (CR-01, CR-02);
  1.27.1 (no finding — retraction of the v1.27.0 "FX mod NaN" note + the
  advanceGlobalLfoPhases finite guard and both harness rate declarations);
  1.27.2 (WR-01);
  1.28.0 (WR-02, WR-06, WR-08 fixed; WR-04 and WR-05 RETRACTED with evidence —
  WR-04 is inert in the only case it occurs, WR-05's prescribed fix measurably
  makes the rate match worse);
  1.28.1 (Info tier: IN-02, IN-03a, IN-03b, IN-04, IN-05, IN-06, IN-07, IN-08,
  IN-10 resolved; IN-01 REFUTED — the "dead" code is called by
  WavetableFactory::createFactoryLibrary; IN-03c SKIPPED as module-owned;
  IN-09 DEFERRED)
open_findings: IN-09 only (Fold aliases at 2x oversampling — deferred to its own
  MINOR: the oversampling factor is fixed in the DistortionProcessor ctor and
  initProcessing allocates, so it cannot follow a parameter on the audio thread,
  and the distortion latency is reported to the host)
supersedes: .planning/CODE-REVIEW.md (v1.18.1, all 47 findings resolved)
---

# O-Prism: Code Review Report

> **Resolution log**
>
> | Finding | Status | Where |
> |---------|--------|-------|
> | CR-01 | ✅ Resolved in **v1.27.0** | All seven mutating `WavetableEditor` ops write into a PRIVATE shadow buffer (`acquireShadow()`), never the published one. A publish is a swap: `commitPendingEdits()` makes the shadow live and hands back the buffer it displaced, which the processor holds in a single `coolingEditBuffer` slot until the audio thread cannot reach it, then returns as the next shadow. The new pointer is stored **before** the old buffer starts cooling — the `+2` rule only makes it safe once a block has started (repointing voices) and finished. Mutations route through the new `editWavetable()` wrapper so a call site cannot publish without cycling the buffer it replaced. |
> | CR-02 | ✅ Resolved in **v1.27.0** | 13 stores → `memory_order_release`, 3 loads → `memory_order_acquire` (the 11 + 3 cited, plus the 2 new stores in `publishEditedWorkingTable`). The invariant is documented at the `userTablePtrA/B` declaration in `PluginProcessor.h`. Static-only verdict: a memory-ordering edge has no single-threaded observable and the arm64 store-buffer window it closes does not reproduce on demand. |
> | CR-03 | ✅ Resolved in **v1.26.1** | `PrismVoice::releaseNotePitchBend()` at both note-end points + `clearAllPitchBends()` on All Notes Off / All Sound Off. Deliberately *not* placed in `stopNote`'s tail-off branch as prescribed: `Synthesiser::noteOn` tail-offs the old voice of a re-struck note **before** the new voice runs `startNote`, so clearing at note-off would strip a live bend from the re-strike. Guarded on `isNoteHeld()` for the same collision. The `isNoteHeld()` skip strands the entry of a **held** note whose voice is stolen; `startNote` now seeds the entry from `currentPitchWheelPosition`, so the stranded value is overwritten at the next strike and the residual is unobservable. |
> | WR-01 | ✅ Resolved in **v1.27.2** | `PrismSynthesiser`, a `juce::Synthesiser` subclass, overrides `renderVoices` and advances the global phase by *that sub-block's* length right after the voices seed from it; `processBlock` no longer advances. Sub-block lengths sum to the block length, so the end-of-block phase `fxLfo[]` reads is unchanged to the last bit. Coupling now recorded at the declaration: `juce::Synthesiser` skips `renderVoices` entirely on a zero-channel output, so the advance goes with it — the WR-09 early return keeps the two in agreement, but they must change together. |
> | WR-02 | ✅ Resolved in **v1.28.0** | `readSample` selected its mipmap level from the member `frequency`. A cached `warpHarmonicScale` — recomputed by BOTH `setWarpType` (outside its change guard, so it cannot go stale) and `setWarpAmount` — now scales the frequency the level selector sees; `getLevelSelectFrequency()` exposes the product so the contract is assertable rather than inferred from a spectrum. Sync/Window and Bend take `1 + 3*warpAmount`; Off and FM stay 1.0, because FM's index is `fmInput` and moves every sample. The Sync predicate mirrors `isSyncMode` exactly, 0.001f floor included. Sync/Bend at high warp now sound duller and cleaner — that is the trade. No built-in negative control is possible (pre-fix the accessor returns `frequency` unconditionally); falsified by source revert, 7 `[W]` assertions fail. |
> | WR-04 | ⚠️ **RETRACTED in v1.28.0** — not a live defect; prescribed guard applied anyway | The write aliasing is real but occurs ONLY on a one-channel block, and there `rightData` aliases `leftData` for *reading* too, so `inputL == inputR`. `DelayProcessor` is symmetric end to end — same maximum delay, same lowpass type and 8 kHz cutoff on both feedback filters, ONE shared `delaySamples` smoother driving both reads, both feedback states reset to 0 — and PingPong's cross-feedback is itself symmetric. Nothing breaks the symmetry, so `wetL == wetR` for all time (measured max \|L-R\| exactly 0.0) and the overwritten store held a bit-identical value. The review's premise, "in PingPong mode the two lines carry genuinely different signal", holds in stereo and fails in mono. The guard is applied because it is free, makes the mono intent explicit, and is the correct arithmetic if an asymmetric mono path is ever added — but it changes no audio, which `[M2]` asserts. |
> | WR-05 | ⚠️ **RETRACTED in v1.28.0** — do NOT re-apply | Mechanism real for a pole in isolation; impact claim unsupported, and every prescribed correction is WORSE. Worst octave-band deviation from the 44.1 kHz reference over 125 Hz–16 kHz, as power per Hz: **as shipped 0.160 dB @ 48 kHz / 0.631 dB @ 96 kHz**; poles warped `a' = a^(44100/fs)` 0.650 / 5.010; that plus the white-noise PSD rescale 0.282 / 1.632. 0.160 dB at 48 kHz is not "measurably brighter" by any standard. The reason both corrections fail: preserving each pole's frequency and each section's DC gain is not the same as preserving the SUMMED response, and the sum is what is audible — the three sections plus the flat direct term combine into a magnitude already nearly rate-invariant, the frequency-axis compression at a higher rate offsetting the 1/fs fall in white-noise power per Hz almost exactly. Coefficients unchanged; a retraction note sits at them, and `[P-neg]` runs both corrections through the same measurement so re-applying either one fails the gate. |
> | WR-06 | ✅ Resolved in **v1.28.0** | `PrismVoice::setVoiceIndex(i)` from the construction loop, before `prepare()`: index mixed by Knuth's 2654435761, each component offset by the golden-ratio constant `0x9E3779B9` so separation lands in the HIGH bits an LCG propagates. `fxLfo[]` seeded on the same terms in `prepareToPlay`. `prepare()` rewinds every stream to its seed — that is what makes a *render* reproducible, since the host calls it once per bounce. `resetWithRandomPhases` takes a supplied `phaseSeed` instead of `this`, keeping both original properties (same set per note, different set per voice) without the heap address. Two deliberate non-choices: `NoiseGenerator::reset()` does NOT re-seed (it fires per note-on; re-seeding would restart the same burst on every note, a pitched artefact), and per-voice seeds must stay distinct (a shared seed makes all 16 voices phase-coherent). This is the finding that unblocks a byte-stable offline render gate. |
> | WR-08 | ✅ Resolved in **v1.28.0** | The one-pole now runs on `log2(frequency)`; `currentLog`/`targetLog` are kept in step with `currentFreq`/`targetFreq` at every mutation and `getNextFrequency` returns `exp2(currentLog)`. Shape unchanged — same coefficient, same asymptotic approach — only the domain moves. `toLog` floors at 1e-6 Hz so a zero target cannot poison the state with `-inf`. One `exp2` per sample and only while a glide runs; the convergence early-out returns before it. At the halfway point in PITCH the frequency is now the geometric mean 185.0010 Hz (exact 184.9972) where the old code passed 294.33 Hz. Built-in negative control: the linear-Hz one-pole is reproduced in the gate and fails both assertions — pitch half-life spread 39.113 % vs 0.0000 %, direction asymmetry 0.4466 vs 0.000000000. |
> | WR-03 | ✅ Resolved in **v1.26.1** | `clearUserWavetableOverride(0/1)` unconditionally, ahead of the `userWtState` block (so legacy states with no child are covered too). |
> | WR-07 | ✅ Resolved in **v1.26.1** | Dead-band guard removed; all four `setRate` calls unconditional. |
> | WR-09 | ✅ Resolved in **v1.26.1** | Early return in `processBlock` on `getNumChannels() == 0`, still publishing `blockGeneration`. Covers `PrismVoice.cpp` `getWritePointer(0)` via the same chokepoint. |
> | IN-01 | ⚠️ **REFUTED in v1.28.1** — nothing to delete | The premise "`WavetableFactory` owns every factory table" is false. `WavetableFactory::createFactoryLibrary()` (`WavetableFactory.cpp:178`) *delegates* factory tables 0–3 to `generateProceduralTable` at `:184-187`, and it is called from `PluginProcessor.cpp:560` and `tests/dsp_quality_check.cpp:434,487`. Deleting the "dead" ~105 LOC would have removed the Saw, Square, Triangle and Sine factory wavetables. The secondary observation stands and is deliberately NOT acted on: the three additive generators are O(N²) (~4.2M `std::sin` each), but that is live construction cost and reducing it would change table contents — a DSP change, not an Info sweep. |
> | IN-02 | ✅ Resolved in **v1.28.1** | One file-static `buildFrameMipmaps` replaces ~55 lines of verbatim duplication; both callers pass their own FFT and scratch buffers, so the whole-table path keeps allocate-once. The guard-sample write is deliberately NOT in the helper — it is the one place the paths differ (whole-table sweeps all guards once at the end, single-frame sets only its own, per level) and folding it in would end the equivalence. Side effect: the duplicated `bin * 2` indexing was the source of 8 `-Wsign-conversion` warnings, now 4. |
> | IN-03a | ✅ Resolved in **v1.28.1** | `SVFFilter::setKeyTrack` deleted from header and implementation. Zero callers (`PrismVoice` computes key tracking itself and calls `setCutoff`), and wrong as written — it multiplied `cutoffHz` into itself cumulatively on every call. |
> | IN-03b | ✅ Resolved in **v1.28.1** — by deletion, after one wrong turn | `ModulationMatrix::clearOffsets` deleted as prescribed. Worth recording the intermediate state: the sweep first tried to resolve this *by making the function live*, as IN-06's once-per-block wipe. That wipe was itself a regression (see IN-06), so the function went back to having no caller and the finding resolves by deletion after all. The tidy-looking pairing of two findings was the wrong answer. |
> | IN-03c | ⏭️ **SKIPPED in v1.28.1** — module-owned | `TuningEngine::getScaleFrequencies` is genuinely uncalled, but `Source/TuningEngine.{h,cpp}` is a vendored copy of `modules/tuning/scala-tuning-engine`, where the identical method sits at `cpp/TuningEngine.cpp:793`. Deleting it from one consumer makes O-Prism a customized divergence that the next `/module-upgrade` would fight, and helps none of the other consumers. Belongs to a module change. |
> | IN-04 | ✅ Resolved in **v1.28.1** | `g`, `R2`, `h` and `butterR2`/`butterH` are all pure functions of cutoff, resonance and rate, so all four now live in `updateCoefficients()`; `processSingleSVF` takes its coefficient pair as arguments, letting the second stage run the shared core instead of open-coding it. **The literal `0.707` is preserved exactly** — it sits 1.510e-4 relative below true Butterworth (`2*0.707 = 1.414` vs `sqrt(2) = 1.4142135623730951`, measured), and the comment now states the literal and the measured gap instead of the old false claim that `R2 = sqrt(2)`. Substituting the exact value would change rendered output for every patch on LP24/HP24/BP24. Falsified by doing exactly that: `[S1]` fails with 207,360 mismatches — precisely the cascade sample count — worst \|diff\| 3.186e-4. |
> | IN-05 | ✅ Resolved in **v1.28.1** | `processNotch` folded in as `case 6: return yLP + yHP;`. Same operations in the same order. One nuance preserved deliberately: for an out-of-range `filterType` the pre-fix 24 dB path ran the second stage's integrators and then returned *stage 1*; `filterType` is an APVTS Choice over 0–6 so this is unreachable, but it is reproduced verbatim so the refactor is bit-identical even on the branch that cannot execute. |
> | IN-06 | ✅ Resolved in **v1.28.1** | `updateFromAPVTS()` compacts passing slots into `activeSlots` and reuses the existing `destRouted` set (added for REG-03 in v1.27.0 — half the prescribed fix already existed); `evaluate()` walks and clears only those, and both per-sample bounds checks are hoisted. **The trap, and the wrong answer that had to be backed out:** clearing only routed destinations strands a destination that stops being routed, so it needs a clear elsewhere — but a blanket `destOffsets.fill(0.0f)` in `updateFromAPVTS()` is WRONG, because that function runs once per MIDI *sub-block* and `PrismVoice` reads `ModDest::Pitch` one sample late (`PrismVoice.cpp:599`, above `evaluate()` at `:627`, whereas `FiltACutoff` at `:756` is read after). The blanket wipe changed the rendered pitch on the first sample after every MIDI event: max \|diff\| 0.380141199 at 15 CC/block, `lfo-subblock-check` 3/21 failing. Shipped shape is a **transition clear** — zero exactly the destinations that just left the routed set. Two complementary negative controls: removing the clear fails `[N2]`/`[N3]` (offset frozen at 0.999999940); the blanket wipe passes `dsp-quality-check` 0-failed while failing `lfo-subblock-check` 3/21. Neither gate alone covers this change. |
> | IN-07 | ✅ Resolved in **v1.28.1** | `WavetableEditor::fft` marked `mutable`; the per-call `juce::dsp::FFT tempFFT (11)` and its 2048-point twiddle allocation are gone from `getFrameHarmonics`. Safe to share — all five uses (`:213, 254, 305, 527, 545`) are message-thread, reached only from the WebView native functions in `PluginEditor.cpp`; the audio thread never touches it. |
> | IN-08 | ✅ Resolved in **v1.28.1** | `idx0 = std::min (idx0, WavetableData::kTableSize - 1)` in `readSample`. Premise confirmed exactly: `-1e-20` wraps to *exactly* 1.0 in double, giving `idx0 = 2048` and an `idx0 + 1` read of index 2049 in a 2049-element frame, through a `getSample` that is a bare unchecked vector index. Unreachable from the plugin (all internal callers wrap; the phase parameters are non-negative) but reachable through the public `resetWithPhase`, which is how the gate drives it. Audio-neutral, and that is *why* it sat latent: unclamped, `frac` is exactly 0.0, so the interpolation is `a + 0.0*(b-a)` and the out-of-bounds `b` is multiplied away. Hence `[X7]`, the only assertion here that can fail on a clamp removal: it plants an infinity where the overflowing read lands and exploits `0.0 * inf == NaN`. Removing the clamp renders `nan`; `[X7-nv]` proves the trap is armed. |
> | IN-09 | ⏸️ **DEFERRED in v1.28.1** to its own MINOR | Real — 2× oversampling (`DistortionProcessor.cpp:34`) cannot contain `std::sin (x * kPi)` (`:95`) at a 10× pre-gain. Neither prescribed fix is PATCH-shaped. Raising oversampling per mode: the factor is fixed in the constructor and `initProcessing` allocates, so it cannot follow a parameter on the audio thread — and the distortion is the plugin's only latency source with its latency reported to the host (`PluginProcessor.cpp:765, 1151`), so reported latency would move when the user changes distortion type. A bounded triangle fold: changes Fold's timbre for every existing patch and preset. **The only finding left open in this report.** |
> | IN-10 | ✅ Resolved in **v1.28.1** | `.planning/IMPROVE-STATE.md` moved to `.planning/archive/` with an ARCHIVED header (it described the v1.8.0 → v1.9.0 cycle as in progress against a plugin shipping 1.28.0, and three items it listed as open were re-found as IN-04/IN-05). `.planning/CODE-REVIEW.md` gained a `superseded_by` front-matter key — its numbering is its own, and its IN-08 is not this report's IN-08. `.planning/SIMPLIFICATION-AUDIT.md` now states it was audited against v1.17.0 with drifted line numbers; its Phase 3 is still genuinely open, so the `/simplify-phase3` pointer in the v1.17.1 CHANGELOG entry was deliberately left alone. |
>
> All verified still present at the cited `file:line` before editing — the v1.26.1
> four against v1.26.0, CR-01 and CR-02 against v1.26.1.
> Everything else in this report is **open**.
>
> **Regression round, 2026-09-23 (folded into v1.27.0, pre-commit).** Four
> regressions introduced by the fixes above, numbered REG-* to keep them
> distinct from this report's pre-existing findings. REG-01: the first shape of
> CR-01 cloned 20 MiB per `requestAnimationFrame` frame on the harmonic-drag
> path, against an uncapped retire queue whose only sweep ran off a 500 ms timer
> with a `processBlock`-keyed expiry rule — unbounded with the transport
> stopped. REG-02: `setStateInformation` never called `stopEditing`, so a
> restore killed live preview and the next edit republished over it. REG-03: the
> WR-07 guard removal left four `std::exp2` unconditional in the per-sample voice
> loop. REG-04: the WR-09 early return skipped `vst3Extensions.drainAndUpdate()`,
> stranding a Note Expression tuning delta for a later note. All four resolved
> and gated by `tests/edit_rotation_check.cpp` (`O-Prism-edit-rotation-check`,
> 15/15) plus a static ordering check for REG-04; six negative controls, the
> last reproducing the pre-fix numbers exactly (201 tables held, 200
> allocations, against a bound of 3 and 2). See the v1.27.0 CHANGELOG entry.
>
> **`/improve-verify` 2026-09-23 — PASS.** All four CLOSED, no regression survived
> verification. Gated by `tests/bend_state_check.cpp` (new, `O-Prism-bend-state-check`),
> 14/14: a released bent note re-strikes at 261.636 Hz / 0.07 cents (CR-03); a note
> struck under a held wheel bends, on a note no earlier check touched (the seed);
> eight zero-channel blocks — one carrying a note-on — return, and the instance still
> renders (WR-09); a restore with no override clears it, with the `userWavetables`
> child both present-and-empty and absent (WR-03). Each has a negative control:
> reverting the seed + both clear sites fails CR-03 at 277.213 Hz and the seed at
> 329.584 Hz; reverting only the early return crashes `processBlock` with
> `EXC_BAD_ACCESS address=0x0` in the mono gain loop; reverting the two
> `clearUserWavetableOverride` calls fails both WR-03 checks. WR-07 is static-only —
> the guard is gone and `LFO::setRate` is one divide with no phase side effect, so
> there is no between-sample state to observe. `auval -v aumu OuPr OuDv` and
> pluginval strictness 10 both SUCCEED on the installed 1.26.1 bundles.
>
> **`/improve-review` 2026-09-23 (v1.27.0) — CR-01, CR-02 CLOSED.** Gated by
> `tests/wavetable_cow_check.cpp` (new, `O-Prism-wavetable-cow-check`), 38/38. For
> each of the seven mutating ops: the edit publishes a *new* table, the displaced
> buffer stays byte-identical to a pre-call snapshot (the CR-01 assertion — in-place
> mutation is exactly what makes it differ), the new buffer nonetheless differs from
> that snapshot (non-vacuity), and frame count and finiteness hold. Osc A is parked
> on a multi-frame factory table by walking the bank for one with ≥ 2 frames: the
> default has a single frame, which silently skipped `reverseOrder` and the global
> normalize. Negative control: restoring the v1.26.1 in-place `WavetableEditor.cpp`
> fails 14/14 — exactly the two CR-01 assertions across all seven ops — while the
> non-vacuity and shape checks still pass, so the gate isolates the race and not the
> edit. The bundled render-thread-plus-hammered-edits run is a smoke test over the
> retire path under contention, **not** a verdict: it does not fail on pre-fix code.
> CR-02 is static-only — a `grep` for `memory_order_relaxed` on `userTablePtr` must
> return nothing. `bend-state-check` 14/14 and `geometry-check` 0-failed still green;
> `auval -v aumu OuPr OuDv` and pluginval strictness 10 both SUCCEED on the installed
> 1.27.0 bundles.
>
> **v1.27.1 — the "FX mod NaN" that came out of that round was NOT a finding.**
> It never entered this report, and it should not be re-opened as one. The v1.27.0
> CHANGELOG recorded five processor-level mod destinations rendering NaN and blamed
> `juce::jlimit`; both halves were wrong. `getSampleRate()` is set by
> `setPlayConfigDetails`, not `prepareToPlay`, and `edit_rotation_check` and
> `wavetable_cow_check` called only the latter — so they rendered at rate 0, where
> `rateHz / 0` is inf and `phase -= std::floor (phase)` makes that a sticky NaN in
> `advanceGlobalLfoPhases`. Those five destinations are simply the ones fed from
> `fxModMatrix`, whose LFO sources are the poisoned global phases; the other twenty
> are voice-level. No host reaches it. Fixed anyway in v1.27.1 (a finite guard, since
> the NaN was unrecoverable once produced) together with both harnesses, and gated by
> `tests/fx_mod_nan_check.cpp` (`O-Prism-fx-mod-nan-check`), 159/159, whose negative
> control is built in: 9/159 fail on the pre-fix guard with no file revert.
>
> **`/improve-verify` 2026-09-24 (v1.28.0) — PASS.** All five Warning-tier findings
> dispositioned as claimed: **WR-02, WR-06, WR-08 CLOSED**; **WR-04, WR-05 retractions
> upheld**. No regression survived verification.
>
> *Survived-to-disk.* The installed `-dev` AU and VST3 **binaries are byte-identical
> (SHA-256) to a fresh build from committed HEAD** — `3c58ae94…` (AU) and `ba87bd89…`
> (VST3) — so the shipped bundle is provably this source. Version coherent end to end:
> CMakeLists `1.28.0`, CHANGELOG top entry, PLUGINS.md row and both bundles'
> `CFBundleShortVersionString` all agree. No alternate-variant orphan on disk (only
> `-dev`, per the dual-variant sweep rule).
>
> *Closure evidence.* **WR-06**: `updateWarpHarmonicScale` aside, a full sweep of
> `Source/` finds **no unseeded RNG left** — every `juce::Random` is constructed with an
> explicit seed and `WavetableFactory`'s three `std::mt19937` are fixed at 42/99/77; the
> only `Time::currentTimeMillis` is a filename generator. The per-render rewind claim
> holds structurally: `prepareToPlay` → `voice->prepare()` (PluginProcessor.cpp:736) →
> `noiseGen.prepare` / `lfo1..4.prepare`, each of which re-applies its stored seed, so the
> *same instance* is reproducible across bounces, not merely a fresh one. All seven
> RNG-owning components of `PrismVoice` are covered (lfo1–4, noiseGen, oscA, oscB);
> `subOsc` and `glide` own none. **WR-02**: the level-select predicate mirrors
> `getNextSampleStereo`'s `isSyncMode` **exactly** — same `(Sync || Window) && warpAmount
> > 0.001f` — and the recompute sits outside `setWarpType`'s change guard, so no stale
> scale can survive a mode switch. All four warp paths are coherent: Sync/Window scale by
> the slave ratio, Bend by its exponent, Off and FM stay 1.0. **WR-08**: `currentLog` and
> `targetLog` are kept in step at **all five** mutation points (`reset`, `startFrom`,
> `setTarget` incl. its snap branch, and both arms of `getNextFrequency`); the `exp2` is
> gated behind the convergence early-out, so it costs nothing once a note has arrived —
> this matters because REG-03 in v1.27.0 was exactly an unconditional `exp2` in the
> per-sample voice loop. **WR-04**: `isBusesLayoutSupported` is confirmed **not**
> overridden anywhere in `Source/`, so the mono negotiation the retraction depends on is
> genuinely reachable; one shared `delaySamples.getNextValue()` drives both reads, which
> is the symmetry the inertness argument rests on.
>
> *Gates, all re-run from a clean build (exit 0, zero warnings): **494 checks, 0 failed**.*
> `dsp-quality-check` 64/64 (the new gate) · `fx-mod-nan-check` 159/159 ·
> `geometry-check` 166/166 · `wavetable-cow-check` 41/41 · `edit-rotation-check` 28/28 ·
> `lfo-subblock-check` 21/21 · `bend-state-check` 15/15. Every negative control fired as
> documented: `[G1-neg]` pitch half-life spread 39.113 % vs 0.0000 %, `[G2-neg]`
> direction asymmetry 0.4466 vs 0.000000000, `[P-neg]` both prescribed WR-05 corrections
> measurably worse at both rates, `[M3]` the asymmetric case the WR-04 guard would matter
> for. `auval -v aumu OuPr OuDv` **SUCCEEDED** and pluginval **strictness 10 SUCCESS** on
> the installed 1.28.0 bundles.
>
> *Three Info-tier observations, none blocking (no new WR/CR number assigned).*
> **(a)** The WR-05 retraction note at `NoiseGenerator.cpp:109–121` and the gate header at
> `dsp_quality_check.cpp:95–99` cite **0.085 dB / 0.546 dB** "as shipped", but the live
> gate measures **0.160 dB / 0.631 dB** — the CHANGELOG and the table above are correct
> and the code comments are stale from a measurement taken before WR-06 fixed the noise
> seeds. The conclusion is unaffected (0.160 ≪ 0.650), but the comment is the artefact a
> future maintainer reads before deciding whether to "fix" WR-05, so it should carry the
> reproducible numbers. **(b)** `frequency * warpHarmonicScale` is written **twice** —
> `getLevelSelectFrequency()` (WavetableOscillator.h:81) and `readSample` inline
> (WavetableOscillator.cpp:225). The `[W]` contract assertions read the accessor while the
> audio path uses the duplicate, so a future edit to one alone would leave the gate green
> and the audio wrong; `readSample` should call the accessor. **(c)** Nothing asserts that
> the processor's 16 voices actually receive **16 distinct** indices. `[D]` proves
> reproducibility and `[D-osc]` proves seed separation at the unit level, but both would
> still pass if `PluginProcessor.cpp:572` regressed to a constant — which is precisely the
> phase-coherent-unison failure WR-06's distinctness requirement guards against. Verified
> correct by reading (`setVoiceIndex (i)`), i.e. static-only. **(d)** The CHANGELOG's stated
> reason for leaving WR-02's *audible* improvement ungated — "aliasing in a sync'd or
> phase-distorted oscillator is periodic at the master f0, so it lands *on* the harmonics
> rather than between them and no inharmonic-energy metric separates it" — holds only when
> `fs / f0` is an **integer**. The sync'd output is periodic at f_master, so its partials sit
> at `k·f0`, but a partial above Nyquist folds to `|k·f0 − n·fs|`, which is a multiple of f0
> only if `fs ≡ 0 (mod f0)`. Measured on a naive hard-synced saw at ratio 4 (a proxy for the
> pre-fix surplus, not the plugin itself): inharmonic energy **3.45 %** at f0 = 1000 Hz
> (`fs/f0` = 48, i.e. the leakage floor) against **11.0 %** at f0 = 1017 Hz and 11.0 % at
> f0 ≈ 1046.5 Hz — a clean 3× separation. The premise that no such metric can discriminate
> is therefore not established, and a `[W]`-tier aliasing gate at a deliberately
> non-commensurate f0 looks constructible. Worth retesting before the listening row is
> accepted as the only possible evidence; cf.
> `pattern_exact_cycle_gate_needs_exactly_representable_f0`, which is this trap's mirror
> image.


> **`/improve-review` 2026-09-24 (v1.27.2) — WR-01 CLOSED.** Verified still present at
> the cited `Source/PrismVoice.cpp:452-464` / `Source/PluginProcessor.cpp:796` before
> editing. Gated by `tests/lfo_subblock_check.cpp` (new, `O-Prism-lfo-subblock-check`),
> 21/21. The observable is that MIDI carrying no modulation must not change the sound:
> CC#20 is untouched by `processBlock`'s MIDI scan and by the empty
> `PrismVoice::controllerMoved`, so all it does is force a sub-block boundary. 15 CC per
> block 32 samples apart (16 sub-blocks) and 3 per block (4 sub-blocks) now render
> **bit-identically** to the undivided block, for LFO1 → Pitch and LFO1 → FiltA Cutoff —
> Pitch integrates the LFO error and the filter does not, so neither one's sensitivity is
> carrying the result. Determinism is constructed rather than assumed around still-open
> WR-06: Sine shape (not S&H), noise and sub at their 0.0 defaults, and osc A/B Phase
> above zero so `startNote` takes `resetWithPhase` instead of `resetWithRandomPhases`.
> [A0] asserts two instances render bit-identically before anything else, so a later
> equality cannot pass for the wrong reason. Negative control (restore the single
> `processBlock` advance, delete the override): **4/21 fail, exactly the four [A]
> assertions** — 0.359 and 0.345 max |diff| on Pitch, 0.251 and 0.067 on FiltA Cut —
> while [A0], [B] non-vacuity, [C] traversal, [D] the free-run-off control and [E] the
> end-of-block phase all stay green. [C] and [E] pass pre-fix *because* the global phase
> itself was never wrong: it always advanced by the full block length, and the damage was
> only in what the voices sampled on the way through. Regression: `fx-mod-nan-check`
> 159/159, `edit-rotation-check` 28/28, `wavetable-cow-check` 41/41, `bend-state-check`
> 15/15, `geometry-check` PASS. `auval -v aumu OuPr OuDv` and pluginval strictness 10
> both SUCCEED on the installed 1.27.2 bundles.
>
> **`/improve-verify` 2026-09-24 (v1.27.2) — PASS.** WR-01 CLOSED; no regression survived
> verification. Survived-to-disk: CMakeLists `VERSION`, CHANGELOG top entry, NOTES.md,
> PLUGINS.md row and both installed bundles' `CFBundleShortVersionString` all read 1.27.2,
> the installed VST3/AU binaries are byte-identical to a fresh build of the current tree,
> and `nm` finds `OPrismAudioProcessor::PrismSynthesiser::renderVoices` in the installed
> VST3 — the fix is in the shipped binary, not just the source. Closure re-derived from
> JUCE 8.0.15 `juce_Synthesiser.cpp` rather than from the resolution's account: the three
> `renderVoices` call sites in `processNextBlock` partition the block exactly (the `break`
> path renders the full remainder, the sub-`minimumSubBlockSize` path renders nothing and
> consumes nothing), so the redistribution is exact, and all three sit under
> `if (targetChannels > 0)` — the zero-channel coupling recorded at the declaration.
> `advanceGlobalLfoPhases` now has exactly one caller. **Negative control re-run
> independently** (revert both hunks, rebuild the gate only, sources restored and
> SHA-verified against a pre-revert backup): **4/21 fail, exactly the four [A]
> assertions**, at 0.359234661 / 0.345356464 (Pitch) and 0.251120985 / 0.066679746
> (FiltA Cut) — the CHANGELOG's recorded magnitudes, reproduced. [A0], [B], [C], [D], [E]
> stayed green. All six gates green on the restored tree: `lfo-subblock-check` 21/21,
> `fx-mod-nan-check` 159/159, `edit-rotation-check`, `wavetable-cow-check`,
> `bend-state-check` all 0-failed, `geometry-check` PASS (0 failed).
> `auval -v aumu OuPr OuDv` SUCCEEDED and pluginval strictness 10 SUCCESS on the installed
> bundles. One latent coupling noted, **not a live defect**: only the `AudioBuffer<float>`
> `renderVoices` overload is overridden, so enabling double-precision processing (O-Prism
> does not override `supportsDoublePrecisionProcessing()`, and has no double `processBlock`)
> would silently stop the advance entirely — a third member of the same family as the two
> couplings already recorded at the declaration.

**Reviewed:** 2026-09-22 (v1.26.0, 📦 Installed)
**Scope:** everything that landed since the last review closed at v1.19.1 — v1.19.2
through v1.26.0 (Note Expression, global free-run LFOs, the retired-table reaper,
the Geometry bank, zh-Hans, the v1.26.0 UI restructure) plus a full re-read of the
RT path.

## Summary

The v1.19.0/v1.19.1 batch did real work and it holds: the Nyquist clamp and defensive
wrap in `WavetableOscillator` are in place, the CR-03 `loadFromDisk()` mass-free is gone
(`replaceOrInsertFromFile` + the generation-counter reaper replaced it), the tuning-engine
mutation is deferred to the message thread, the FX param atomics are cached, the delay
line is sized from the real sample rate, and the importer hardens against hostile headers.
`launchAsync` captures a `SafePointer` at all six file choosers, and every user-supplied
string that reaches the DOM (wavetable names, `.scl` scale names) goes through
`textContent`, not `innerHTML` — no injection path.

What is left clusters in three places.

**Wavetable lifetime is half-fixed.** v1.19.0 solved *freeing* a table the audio thread
might read. It did not solve *mutating* one: every wavetable-editor operation rewrites the
live, published working table in place (CR-01). And every pointer publish crosses threads
on `memory_order_relaxed`, which on arm64 — the primary platform — establishes no
happens-before edge to the buffer contents (CR-02).

**One user-visible correctness bug.** Per-note pitch bend is written but never cleared, so
a released note keeps its bend and re-strikes detuned (CR-03). `clearPitchBend` and
`clearAllPitchBends` exist, fully written, and have no callers. The same table has a
second, opposite hole found while fixing this: `startNote` discarded
`currentPitchWheelPosition`, so a note struck under a held wheel started unbent. Both
directions are closed in v1.26.1.

**The v1.23.0 global free-run LFO didn't account for JUCE's MIDI sub-block splitting**
(WR-01), so free-running LFOs stutter under dense MIDI.

Beyond that: two aliasing paths (Sync/Bend warps read the wrong mipmap level; the Fold
distortion at 2×), a session-restore leak, and a solid block of dead code — an entire dead
half of `WavetableGenerator`, plus 55 lines duplicated verbatim inside it.

---

## Critical

### CR-01: Wavetable editor mutates the live published table in place — data race with the audio thread
**✅ Resolved in v1.27.0** — see the resolution log above.
**Files:** `Source/dsp/WavetableEditor.cpp:98-171` (`setFrameHarmonics`), `:218-266`
(`normalizeFrames`), `:268-292` (`fadeEdges`), `:294-306` (`reverseFrames`), `:308-336`
(`reverseOrder`), `:338-383` (`smoothFrames`); published at `Source/PluginProcessor.cpp:1127-1134`

`startEditing()` stores `wavetableEditor.getWorkingTable()` into `userTablePtrA/B`, so from
that moment voices read that exact buffer every block for live preview. Every editor
operation then writes straight into it from the message thread: level 0 is overwritten via
`std::copy`, then `generateMipmapsForFrame` rewrites all 10 mipmap levels and the guard
sample for that frame.

There is no synchronisation of any kind between the two threads for the *contents*. The
v1.19.0 reaper (`retireTable` + `blockGeneration`) makes it safe to *free* a table the
audio thread may hold — it says nothing about writing to one it is holding right now. The
v1.18.1 review noted this as a "secondary" part of CR-02 and it was never addressed.

**Failure:** hold a pad chord, open the wavetable editor on that oscillator, drag a
harmonic bin. The audio thread reads level-0 samples and mipmap levels that are half-old
and half-new — torn floats, i.e. audible garbage, and formally UB. Every ops-bar button
(Normalize, Fade, Reverse, Reverse Order, Smooth) is the same path.

**Fix:** the machinery to do this correctly already exists. Build each edit into a fresh
`WavetableData` (clone → mutate → `generateMipmapsForFrame`), publish the new pointer with
a release store, and hand the old one to `retireTable()`. That is the `pattern_retired_map_reaper_rt_free`
shape the plugin already uses for delete/save.

### CR-02: Wavetable pointers are published to the audio thread with `memory_order_relaxed`
**✅ Resolved in v1.27.0** — see the resolution log above.
**Files:** `Source/PluginProcessor.cpp:1051, 1056, 1065, 1070, 1131, 1133, 1148, 1152, 1160, 1164, 1206` (stores);
`:1082` (`resolveActiveTable` load), `:1101-1102` (`isUserTableActive`)

`userTablePtrA/B` are `std::atomic<const WavetableData*>` and every store and load uses
`std::memory_order_relaxed`. The message thread fully constructs a `WavetableData` — a
`std::vector<float>` of `10 × numFrames × 2049` floats, up to ~21 MB for a 256-frame table
— and then publishes the pointer. The audio thread loads it in `resolveActiveTable` and
dereferences it in the same block.

A relaxed store/load pair creates no synchronizes-with edge, so nothing orders the buffer
writes before the pointer store. This is not theoretical here: the build targets Apple
Silicon, and arm64 is weakly ordered — the audio thread is permitted to observe the new
pointer while the sample data is still in the writer's store buffer.

**Failure:** import a wavetable or select a user table mid-playback and the first blocks
after the swap can read uninitialised heap. Intermittent, hardware-dependent, and exactly
the kind of bug that never reproduces on x86 CI.

**Fix:** `std::memory_order_release` on every store, `std::memory_order_acquire` on every
load. No structural change, no cost on x86, one `dmb ish` on arm64 per publish (message
thread) and a cheap acquire on the audio side.

### CR-03: Per-note pitch bend is never cleared — a released note keeps its bend and re-strikes detuned
**Files:** `Source/PrismVoice.cpp:730-739`, `Source/TuningEngine.cpp:767-784`,
`Source/PluginProcessor.h:216-225`

`PrismVoice::pitchWheelMoved` writes `tuningEngine->setPitchBend(currentMidiNote, bend)`
into a 128-entry table keyed by MIDI note. JUCE's `Synthesiser::handlePitchWheel` only
delivers wheel messages to voices where `isPlayingChannel()` is true — i.e. *active*
voices. Once a note is released and `clearCurrentNote()` runs, that note number stops
receiving updates, and its entry in `notePitchBends` is frozen at whatever it last was.

Nothing ever clears it. `TuningEngine::clearPitchBend` and `clearAllPitchBends` are both
fully implemented and **have zero callers anywhere in the codebase** — `stopNote` doesn't
call them, and the all-notes-off / all-sound-off branch in `processBlock`
(`PluginProcessor.cpp:784-785`) only clears `noteStates`.

**Failure:** play C4, push the wheel to +1, release C4, return the wheel to centre, play C4
again. `startNote` reads `tuningEngine->getFrequency(60)` at `PrismVoice.cpp:204`, which
applies the stale `+1.0` bend — the note sounds a full pitch-bend-range sharp (default 2
semitones, up to 48) with the wheel visibly centred. It stays wrong until the wheel is
moved again *while that note is held*. Every note that was ever bent carries its own stale
offset, so a passage can come back in several different wrong tunings at once.

Secondary: the Tuning tab's held-notes readout goes through `getActiveNotes()` →
`tuningEngine.getFrequency(i)`, so it displays the same wrong Hz.

**Fix:** call `clearPitchBend(currentMidiNote)` from `PrismVoice::stopNote` (both the
tail-off and immediate branches), and `clearAllPitchBends()` in the
`isAllNotesOff() || isAllSoundOff()` branch of `processBlock`.

---

## Warning

### WR-01: Free-running LFOs replay the same phase in every MIDI sub-block
**✅ Resolved in v1.27.2** — see the resolution log at the top.
**Files:** `Source/PrismVoice.cpp:452-464`, `Source/PluginProcessor.cpp:796`

`juce::Synthesiser::renderNextBlock` splits the buffer at MIDI events whenever the gap is
at least `minimumSubBlockSize` (default 32; `setMinimumRenderingSubdivisionSize` is never
called here), calling `PrismVoice::renderNextBlock` once per sub-block. The voice seeds
`lfoN.setPhase(processor->getGlobalLfoPhase(n))` at the top of *every* one of those calls,
but `advanceGlobalLfoPhases` runs once per `processBlock`, after `renderNextBlock` returns.

So with k sub-blocks, the free-running LFO is rewound to the same start phase k times and
only advances through the last sub-block's worth of samples.

**Failure:** hold a chord with an LFO in free-run mode and send continuous aftertouch or
CC1 — both of which this plugin reads as mod sources, so it is a realistic performance
gesture. At ~30 events per block the LFO advances roughly 1/30th as far as it should and
audibly stutters. Tempo-synced and per-note LFOs are unaffected.

**Fix:** advance `globalLfoPhase` per sub-block (pass `numSamples` down), or seed each
voice from `globalLfoPhase + (subBlockStart / sampleRate) * rate`.

### WR-02: Mipmap level is chosen from the un-warped frequency, so Sync and Bend alias
**✅ Resolved in v1.28.0** — see the resolution log above.
**Files:** `Source/dsp/WavetableOscillator.cpp:164-170` vs `:245-246` and `:196-201`

`readSample` picks its mipmap level from the member `frequency`:
`levelFloat = log2(max(frequency, baseFreq) / baseFreq)`. But two warp modes multiply the
effective harmonic content without touching `frequency`:

- **Sync** (`:245-246`) runs the slave accumulator at `phaseIncrement * syncRatio` where
  `syncRatio = 1 + warpAmount * 3`, i.e. up to 4×.
- **Bend** (`:198-200`) applies `pow(phase, 1 + warpAmount * 3)`, compressing phase into
  the start of the cycle and generating harmonics far above the unwarped spectrum.

Both read a mipmap band-limited for the base pitch, so the extra harmonics fold.

**Failure:** Sync at warp amount 1.0 on a note around C6 aliases audibly; sweep warp back
to 0 and it cleans up. Same for Bend at high amounts.

**Fix:** scale the frequency used for level selection — `frequency * syncRatio` in Sync
mode, `frequency * bendExponent` (a reasonable proxy) in Bend mode.

### WR-03: `setStateInformation` never clears a user-wavetable override
**File:** `Source/PluginProcessor.cpp:1282-1293`

The restore only *applies* an override when the saved name is non-empty:

```cpp
if (nameA.isNotEmpty()) selectUserWavetable (0, nameA);
if (nameB.isNotEmpty()) selectUserWavetable (1, nameB);
```

There is no `else` and no unconditional clear. `userTableNameA/B` and `userTablePtrA/B`
are processor members that survive the state swap, and `resolveActiveTable` gives the user
pointer priority over the factory index unconditionally (`:1082-1083`).

**Failure:** load session A (Osc A → user table "Alpha"), then load session B in the same
plugin instance (Osc A → factory table "Vowel Morph"). Session B plays "Alpha". The UI
reads the same resolver so it agrees with the wrong answer; only removing "Alpha" from
disk or reselecting by hand recovers.

**Fix:** `clearUserWavetableOverride(0); clearUserWavetableOverride(1);` before the
`userWtState` block.

### WR-04: Mono output drops the left delay line entirely
**⚠️ RETRACTED in v1.28.0 — not a live defect.** The aliasing only happens on a
one-channel block, and there `inputL == inputR` and the processor is symmetric end to
end, so `wetL == wetR` for all time (measured max |L-R| exactly 0.0) and the
overwritten store held a bit-identical value. The premise quoted below — that in
PingPong "the two lines carry genuinely different signal" — is true in stereo and
false in mono. The prescribed guard was applied anyway; it changes no audio. See the
resolution log above.
**File:** `Source/dsp/DelayProcessor.cpp:95, 120-121`

```cpp
auto* rightData = block.getNumChannels() > 1 ? block.getChannelPointer (1) : leftData;
...
leftData[i]  = wetL;
rightData[i] = wetR;   // same pointer in mono — wetL is discarded
```

In mono the two writes target the same address, so the second wins and the left delay
line's output is computed and thrown away. In PingPong mode the two lines carry genuinely
different signal, so mono ping-pong is the right line only.

Reachable: `isBusesLayoutSupported` is not overridden, and `juce::AudioProcessor`'s default
returns `true` for any layout, so a host — or the Standalone on a mono output device — can
negotiate one channel against the declared stereo bus (`PluginProcessor.cpp:512-513`).

**Fix:** `leftData[i] = (dataR != nullptr) ? wetL : 0.5f * (wetL + wetR);` or sum the two
lines explicitly in the mono case.

### WR-05: Pink noise is the only noise type with no sample-rate correction
**⚠️ RETRACTED in v1.28.0 — do NOT re-apply the fix below.** The mechanism is real for
a pole in isolation, but the summed response of the economy filter is already nearly
rate-invariant, and both prescribed corrections measurably make it worse (as shipped
0.160 dB / 0.631 dB worst band at 48 / 96 kHz; poles warped 0.650 / 5.010; warped plus
the PSD rescale 0.282 / 1.632). See the resolution log above.
**File:** `Source/dsp/NoiseGenerator.cpp:73-85`

Brown (`:89` `rateScale`), Vinyl (`:121` `cutNorm = 2000/fs`) and Wind (`:150, :161`) all
scale their coefficients by the sample rate. The Paul Kellet pink filter uses the poles
`0.99765 / 0.96300 / 0.57000`, which are fixed for 44.1 kHz.

**Failure:** the same preset's noise bed is measurably brighter at 48 kHz and noticeably so
at 96 kHz — the pole frequencies scale with fs, so the −3 dB/oct slope shifts up. Same
class as `pattern_noise_bed_level_is_rate_dependent`.

**Fix:** warp the three pole coefficients by `44100 / fs` in `prepare()`.

### WR-06: Two clock-seeded RNGs make any render non-reproducible
**✅ Resolved in v1.28.0** — see the resolution log above.
**Files:** `Source/dsp/LFO.h:61` + `LFO.cpp:43, 91` (S&H), `Source/dsp/NoiseGenerator.h:44`

Both are default-constructed `juce::Random`, whose constructor calls `setSeedRandomly()` —
seeded from `Time::getHighResolutionTicks()`, `Time::currentTimeMillis()` and the object
address. Every plugin instantiation draws a different stream.

**Failure:** two bounces of the same project with S&H on an LFO or any noise level above
zero differ sample-for-sample. More importantly, this plugin can never have a byte-stable
offline render gate — the exact trap recorded as
`pattern_sh_lfo_clock_seeded_random_breaks_harness_determinism`.

`WavetableOscillator::resetWithRandomPhases` (`:101`) is a milder version: seeded from
`this`, so it is stable within a process but varies across runs and builds
(`pattern_random_start_phase_seeded_from_this_breaks_render_diff`).

**Fix:** seed explicitly and deterministically — e.g. `juce::Random(voiceIndex * 2654435761u)`
in `prepare()`, with the voice index threaded in from the processor's voice-construction
loop.

### WR-07: LFO-rate modulation sticks inside its dead band
**File:** `Source/PrismVoice.cpp:559-568`

```cpp
const float lr1 = modMatrix.getModOffset (ModDest::LFO1Rate);
if (std::abs (lr1) > 0.0001f) lfo1.setRate (lfo1Rate * std::exp2 (lr1 * 2.0f));
```

The base rate is applied once per sub-block at `:443`. Inside the sample loop the rate is
only *re-*applied when the offset is outside the dead band — so once the modulator enters
the dead band, the LFO keeps the last modulated rate for the rest of the sub-block instead
of returning to `lfo1Rate`. Same shape as
`pattern_conditional_coeff_update_leaks_enabled_flag`.

**Failure:** a slow modulator crossing zero leaves a short rate "notch" at every zero
crossing. Bounded to one sub-block, so subtle — but it is a correctness hole, and the
guard buys nothing (`setRate` is one divide).

**Fix:** drop the guard and always call `setRate`, or make the else-branch restore the base
rate.

### WR-08: Glide interpolates in linear Hz, not in pitch
**✅ Resolved in v1.28.0** — see the resolution log above.
**File:** `Source/dsp/GlideProcessor.h:82`

`currentFreq = currentFreq * glideCoeff + targetFreq * (1 - glideCoeff)` is a one-pole in
the frequency domain. Perceived pitch is logarithmic, so a glide from C2 to C5 covers the
top octave in a fraction of the time it spends crawling through the bottom one.

For a plugin whose entire premise is microtonal pitch accuracy this is the wrong domain.

**Fix:** smooth `log2(freq)` and exponentiate: `currentLog = currentLog*c + targetLog*(1-c);
return std::exp2(currentLog);`. Two `exp2`/`log2` per note-start, one `exp2` per sample —
or precompute a per-sample multiplicative ratio and keep it to one multiply.

### WR-09: Channel-0 access without a channel-count check
**Files:** `Source/PluginProcessor.cpp:948-955`, `Source/PrismVoice.cpp:497`

The `else` branch of the width/volume stage assumes exactly one channel and calls
`buffer.getSample(0, …)` / `setSample(0, …)` unguarded; `PrismVoice::renderNextBlock`
likewise calls `outputBuffer.getWritePointer(0)` with no check. A zero-channel block — which
some hosts pass while probing, and which pluginval exercises — dereferences a null pointer.

**Fix:** early-return on `buffer.getNumChannels() == 0` at the top of `processBlock`.

---

## Info

### IN-01: Half of `WavetableGenerator` is dead — ~105 LOC — ⚠️ **REFUTED in v1.28.1**
`Source/dsp/WavetableGenerator.h:33-53`, `.cpp:35-127`. `generateProceduralTable`,
`generateSaw`, `generateSquare`, `generateTriangle`, `generateSine` and the `WaveShape`
enum have no callers anywhere — `WavetableFactory` owns every factory table and the
importer/editor call only `generateMipmaps` / `generateMipmapsForFrame`. Bonus: the three
additive generators are O(N²) (`2048` harmonics × `2048` samples = 4.2M `std::sin` calls
each), so deleting them also removes dead construction-time cost.

### IN-02: `generateMipmaps` duplicates `generateMipmapsForFrame` verbatim — ~55 LOC — ✅ **Resolved in v1.28.1**
`Source/dsp/WavetableGenerator.cpp:129-193` vs `:195-248`. Identical FFT setup, identical
per-level bin zeroing, identical mirror loop. Extract one file-static helper taking
`(table, frameIndex, fft, fftBuffer, workBuffer)`; the whole-table path keeps its
allocate-FFT-once property by passing its own buffers in.

### IN-03: Dead public API — ✅ **Resolved in v1.28.1** (a, b) / ⏭️ **SKIPPED** (c, module-owned)
- `SVFFilter::setKeyTrack` (`SVFFilter.cpp:75-86`) — `PrismVoice` computes key tracking
  itself at `:505-510` and calls `setCutoff`. As written the method is also wrong: it
  multiplies `cutoffHz` cumulatively on each call.
- `ModulationMatrix::clearOffsets` (`ModulationMatrix.cpp:97-100`) — `evaluate()` already
  fills zero.
- `TuningEngine::getScaleFrequencies` (`TuningEngine.cpp:786-800`).
- `TuningEngine::clearPitchBend` / `clearAllPitchBends` — **do not delete these**; see
  CR-03, they should be called.

### IN-04: SVF 24 dB second stage is `processSingleSVF` inlined with a magic constant — ✅ **Resolved in v1.28.1**
`Source/dsp/SVFFilter.cpp:156-171` repeats the TPT core and recomputes
`h = 1/(1 + 2*0.707*g + g*g)` **every sample** from a literal `0.707`. Give
`processSingleSVF` an `R2` parameter and cache the Butterworth `h` next to `g`/`R2` in
`updateCoefficients()`. Carried over from the v1.9.0 improve backlog (items 6 and 10),
never applied.

### IN-05: `processNotch` is `processSingleSVF` plus one line — ✅ **Resolved in v1.28.1**
`Source/dsp/SVFFilter.cpp:124-133` — the whole body is identical to `:103-111`; only the
return differs (`yLP + yHP`). Fold it in as `case 6`.

### IN-06: The mod matrix rescans all 16 slots for every sample of every voice — ✅ **Resolved in v1.28.1**
`ModulationMatrix::evaluate()` (`ModulationMatrix.cpp:70-87`) is called per sample at
`PrismVoice.cpp:555`. It zeroes all 26 destinations and walks all 16 slots regardless of
how many are enabled. At 16 voices and 48 kHz that is ~32M operations per second of
overhead when a typical patch uses two or three routes.

**Fix:** in `updateFromAPVTS` (already once per block) compact the enabled slots into a
small `activeSlots` array and record which destinations they touch; `evaluate()` then
clears only those destinations and walks only the active list.

### IN-07: `getFrameHarmonics` builds a `juce::dsp::FFT` on every call — ✅ **Resolved in v1.28.1**
`Source/dsp/WavetableEditor.cpp:74` — `juce::dsp::FFT tempFFT (11);` allocates its twiddle
tables per call, purely because the method is `const` and the `fft` member is not. Mark the
member `mutable` and drop the local. Called on every harmonic-editor refresh.

### IN-08: `readSample` can index one past the frame for a tiny negative phase — ✅ **Resolved in v1.28.1**
`Source/dsp/WavetableOscillator.cpp:151-155`. `phase -= std::floor(phase)` on `-1e-20`
gives `1.0 - 1e-20`, which rounds to exactly `1.0` in double; then `samplePos = 2048.0`,
`idx0 = 2048`, and `getSample(level, frame, idx0 + 1)` reads index 2049 — one past the
2049-element frame, and past the end of `data` for the last frame at the last level.

**Not currently reachable** — every path into `readSample` already wraps (Bend returns
`pow(p, e)` with `p ≥ 0`, FM wraps, Sync/Off read a wrapped accumulator). Listed because
closing it is `idx0 = std::min(idx0, WavetableData::kTableSize - 1);` and the failure mode
is a heap OOB.

### IN-09: Distortion "Fold" aliases at 2× oversampling — ⏸️ **DEFERRED to its own MINOR**
`Source/dsp/DistortionProcessor.cpp:34` (`2^1 = 2x`), `:94-96`
(`data[i] = std::sin(x * kPi)`). Sine folding generates unbounded harmonic order; at drive
1.0 the pre-gain is 10×, so `sin(10x·π)` produces components an order of magnitude above
the source that 2× cannot contain. The other three shapes are saturating and mostly fine at
2×. Either raise oversampling for this mode specifically or swap to a bounded triangle fold.

### IN-10: Planning artifacts are stale — ✅ **Resolved in v1.28.1**
`.planning/IMPROVE-STATE.md` still describes the v1.8.0 → v1.9.0 cycle as in progress
("Completed Fixes (3/40)") against a plugin that ships 1.26.0 — several of its listed items
(the `0.707` constant, the SVF core unification, `stereoWidth` in `allSliderIds()`) are
still open and now appear here as IN-04/IN-05. `.planning/CODE-REVIEW.md` is the v1.18.1
review (resolved) and `.planning/SIMPLIFICATION-AUDIT.md` is v1.17.0. Delete or archive
`IMPROVE-STATE.md`; this file supersedes the review.

---

## Verified clean

Re-checked against known suite-wide patterns and found correct in v1.26.0:

- **CR-01 (v1.18.1) Nyquist clamp** — `setFrequency` clamps at `0.5 * sampleRate` and
  `readSample` wraps defensively; the sync re-seed wraps too (`WavetableOscillator.cpp:66, 151, 256`).
- **CR-03 (v1.18.1) mass-free on save** — `saveAsUserWavetable` now calls
  `replaceOrInsertFromFile` and hands the replaced table to `retireTable`; the other
  oscillator is re-resolved by name *before* the retire (`PluginProcessor.cpp:1197-1210`).
- **Retired-table reaper** — the `blockGeneration + 2` rule is sound: the generation only
  reaches `retiredAt + 2` after a full `processBlock` has both started (repointing every
  voice in `updateWavetableAssignments`) and finished.
- **RT safety of the tuning path** — `getFrequency` reads lock-free atomics;
  `setBuiltInPreset` / `setCustomIntervals` / `rebuildFrequencyTable` are all deferred to
  `handleAsyncUpdate` on the message thread, and `pendingTuningPresetChange` correctly
  distinguishes a preset change from a scalar change so a loaded `.scl` is not clobbered.
- **`launchAsync` UAF** — all six choosers capture `juce::Component::SafePointer` before
  the call (`PluginEditor.cpp:267, 299, 322, 350, 477, 519`).
- **WebView injection** — user-supplied strings (wavetable names, `.scl` scale names,
  generated scale names) all reach the DOM via `textContent` / `createElement`
  (`index.html:4105-4110, 4594, 4865`). The only `innerHTML` interpolations carry
  developer-owned catalogue data or numbers.
- **Importer hardening** — int64 length computation, absolute 8M-sample ceiling, channel
  clamp, `isfinite` sample-rate guard (`WavetableImporter.cpp:56-78`).
- **`ValueTree` XML round-trip** — `uiLanguage` uses `isVoid()` + `toString()`, the only
  correct guard (`critical_valuetree_xml_roundtrip_loses_type`).
- **`ADSR::setParameters`** — called only in `startNote`, never per block
  (`pattern_adsr_setparameters_per_block_kills_release`).
- **FX activity gates** — the WR-07 active→inactive reset is present and correct on all
  five effects.
- **EQ coefficients** — `ArrayCoeffs` assignment into pre-grown storage, no RT heap alloc
  (`pattern_arraycoefficients_rt_safe_iir`).
- **SVF NaN guard** — `flushIfNonFinite` resets state rather than propagating
  (`pattern_biquad_nan_guard_sticky_silence`).

---

## Suggested batching

| Batch | Findings | Rationale |
|-------|----------|-----------|
| **v1.26.1 (PATCH)** | CR-03, WR-03, WR-07, WR-09 | Contained, user-visible correctness. CR-03 is the one a player will actually hit. |
| **v1.27.0 (MINOR)** | ✅ CR-01, ✅ CR-02 — ✅ WR-01 in v1.27.2 | Thread-safety work on the wavetable publish path + the sub-block LFO. Touches real structure; wants its own version. |
| **v1.27.x** | WR-02, WR-04, WR-05, WR-06, WR-08, IN-09 | DSP quality and determinism. WR-06 unblocks a byte-stable render gate, so it is worth doing before the rest so the others can be regression-tested against goldens. |
| **Cleanup sweep** | IN-01..IN-08, IN-10 | ~200 LOC removed, no behaviour change. `/simplify` territory. |

---

## Resolved

| Tier | Findings | Closed in |
|------|----------|-----------|
| Critical | CR-03 | v1.26.1 |
| Critical | CR-01, CR-02 | v1.27.0 |
| Warning | WR-03, WR-07, WR-09 | v1.26.1 |
| Warning | WR-01 | v1.27.2 |
| Warning | WR-02, WR-06, WR-08 fixed; WR-04, WR-05 retracted with evidence | v1.28.0 |
| Info | IN-02, IN-03a, IN-03b, IN-04, IN-05, IN-06, IN-07, IN-08, IN-10 | v1.28.1 |
| Info | IN-01 refuted (the cited code is called); IN-03c skipped (module-owned) | v1.28.1 |

**Open: IN-09 only.**

The v1.28.1 sweep is carried by the single commit
`refactor(O-Prism): v1.28.1 — Info-tier review sweep (8 findings)`; resolutions are
keyed to the version rather than to a SHA, because the annotation and the commit
that contains it cannot reference each other in one atomic change. `git log
--oneline -- plugins/O-Prism` resolves it.

Three things a later reader should not have to rediscover:

1. **A review finding can be wrong about its own premise.** IN-01 read as an
   obvious ~105-LOC deletion and would have removed four factory wavetables. The
   check that caught it was grepping for callers across `Source/`, `tests/` *and*
   `modules/` rather than trusting the finding's own claim about who owns what.
2. **Two findings that look like they solve each other may not.** IN-03b (dead
   `clearOffsets`) and IN-06 (per-sample wipe) appeared to cancel: make the dead
   function the per-block clear and both close. The pairing was wrong, because
   `updateFromAPVTS()` runs per MIDI sub-block and one destination is read a sample
   late. Only `lfo-subblock-check` distinguished the tidy answer from the correct
   one.
3. **An invisible defect needs a gate that can see it.** IN-08's out-of-bounds read
   could not change any rendered value, so no value assertion could fail on it. The
   assertion that works plants an infinity at the exact address the overflow lands
   on and lets `0.0 * inf == NaN` surface the read.
