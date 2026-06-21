# Stage 2 (DSP) — VERIFICATION

**Plugin:** O-simpleFM · **Stage:** 2 DSP · **Date:** 2026-06-20 · **Verdict:** ✅ PASS
**Method:** Goal-backward — each ROADMAP test criterion checked against build output, auval, pluginval (strictness 10), and a purpose-built offline render harness that measures the actual spectra.

## Evidence sources
- Build: `ninja O-simpleFM_VST3 O-simpleFM_AU` — clean, **0 warnings from plugin code** (both passes + post-critic-fix rebuild).
- `auval -v aumu OSiF OuDv` → **AU VALIDATION SUCCEEDED** (render @ 6 SR, 1-ch, MIDI, Latency property, 17-param set/schedule/ramp, bad-max-frames, reset-retention).
- `pluginval --strictness-level 10` (VST3) → **SUCCESS, exit 0** — audio processing + automation across 15 SR/block-size combos (64–1024), state restoration, parameter thread-safety, bus-layout restoration, fuzz parameters. No failures/leaks/NaN.
- Offline render harness (`O-simpleFM-render-test`) → **ALL PASS** (5/5), measuring real output spectra.

## Phase 2.1 — Core PM voice (FUNC-01/02/05, DSP-01/02/03)
| Criterion | Verdict | Evidence |
|-----------|---------|----------|
| Loads as instrument, MIDI poly, no crash | ✅ | auval Music Device (aumu) + MIDI test PASS; pluginval no crash; 16 pre-allocated voices |
| Index 0 = pure sine; index up adds sidebands (no zipper) | ✅ | harness: index0 → a440=0.63, a220/a880≈0 (pure carrier); index→sidebands sb@2200 0→0.030; `SmoothedValue` on index |
| Integer ratio harmonic / non-integer inharmonic | ✅ | PM core `fc·ratioEff`, ratioSnap at read site; carrier-null test uses ratio 4 with correct sideband placement |
| Fixed-mode holds Hz while carrier tracks pitch; ratio key-tracks | ✅ | `fm = fixedMode ? fixedHz : fc·ratio` (FMVoice.h); contract-verified by critic |
| Amp ADSR shapes notes; no stuck/silent voices | ✅ | voice lifetime gated on `ampEnv.isActive()`; pluginval non-releasing-audio PASS; auval reset-retention PASS |
| No note-on/off clicks; no denormal stalls | ✅ | phases never reset mid-note; `ScopedNoDenormals`; pluginval clean |

## Phase 2.2 — Mod env → index + feedback (FUNC-03/04, DSP-05/06)
| Criterion | Verdict | Evidence |
|-----------|---------|----------|
| Mod env sweeps timbre independent of amplitude | ✅ | independent `modEnv`; multiplicative `I_inst = baseIndex·((1−depth)+depth·modEnv)` |
| depth 1.0 + sustain 0 → pure-sine tail; carrier null near I≈2.405 | ✅ | **harness carrier-null: carrier/sideband ratio = 0.0001** (Bessel J₀ zero reproduced) |
| Feedback enriches mod → saw/noise smoothly; **no screech/limit-cycle, no NaN** | ✅ | harness feedback@100%: peak=0.93, rms=0.375, finite; two-sample avg + history clamp + isfinite scrub + note-on reset |
| Velocity→index only when velToIndex>0 | ✅ | `velFactor = (1−velToIndex)+velToIndex·velLevel` → 1.0 at 0 |
| No zipper on feedback/index automation | ✅ | `SmoothedValue` on index + feedback; pluginval automation (sub-block 32) PASS |

## Phase 2.3 — Anti-aliasing + viz tap (DSP-03, PERF-01, QUAL-01)
| Criterion | Verdict | Evidence |
|-----------|---------|----------|
| No audible aliasing across index/feedback/pitch (2× OS + ceiling) | ✅ (headless) | key-tracked Carson ceiling @ oversampled Nyquist + 2× polyphase-IIR decimation; harness sidebands land at exact theoretical freqs. *Critical-listening audit deferred to Stage 4 (manual).* |
| Latency reported | ✅ | `setLatencySamples(getLatencyInSamples())`; auval + pluginval Latency PASS |
| Viz ring fed each block; analyzer finite, no alloc/crash | ✅ | `VizRing` fixed `std::array<atomic<float>,8192>` copy-only write; editor 30 Hz Timer pumps `FmVizAnalyzer` (4096 BH FFT + scope); copy-before-FFT honored; pluginval open-editor-whilst-processing PASS |

## Stage-level success criteria
- [x] Builds clean (VST3 + AU); auval SUCCEEDS.
- [x] Synth produces audio on note-on; silent at rest; 16-voice poly; no stuck/silent voices.
- [x] PM core correct (index→sidebands, ratio→harmonicity, fixed/ratio modes).
- [x] Amp + mod envelopes independent; depth 1.0 + sustain 0 → pure-sine tail.
- [x] Feedback stable to 100% — no NaN, no Nyquist screech.
- [x] 2× oversampling active, latency reported; AA chain in place.
- [x] Viz data path real-time-safe (copy-only audio thread; FFT on Timer).
- [x] No new APVTS params; 17-param contract intact; state persistence passes (auval reset-retention + pluginval state restoration).

## Critic review (adversarial DSP gate)
- **No BLOCKERS.** Contract adherence verified across all dimensions (radians, multiplicative routing depth 1.0, no mid-note phase reset, OS wraps summed render, FFT on message thread, amp-gated lifetime, feedback clamp-history placement, ceiling at oversampled Nyquist).
- **W1 (channel pinning)** — FIXED: oversampler built for fixed `kMaxChannels=2`; runtime layout always ≤ 2 → no overrun. Validated by pluginval bus-layout restoration.
- **W2 (over-large block overrun, Release crash potential)** — FIXED: `processBlock` chunks into ≤ `maxBlockSize` slices; OS internal buffer never exceeded. Validated by pluginval variable-block (64–1024) sweep.
- **NOTE 1 (ceiling re-armed per sample)** — FIXED: ceiling target computed once per render call.
- **NOTE 4 (fastSine NaN)** — FIXED: `isfinite` guard added to the reusable primitive.
- **NOTE 2 (feedback bandwidth not in ceiling)** — accepted; deferred to Stage 4 aliasing audit (contract's tiered fallback covers it).
- **NOTE 3 (MIDI addEvent alloc under extreme MIDI)** — accepted/low-risk; `ensureSize(4096)` + per-chunk rebuild; documented.

## Deferred to later stages (not Stage 2 failures)
- Critical-listening aliasing/artifact audit + extreme-setting sweep → Stage 4 (manual + pluginval already clean).
- Visual confirmation of spectrum/scope rendering → Stage 3 (WebView); data path proven headlessly now.
- Factory presets / preset tour → Stage 4.
