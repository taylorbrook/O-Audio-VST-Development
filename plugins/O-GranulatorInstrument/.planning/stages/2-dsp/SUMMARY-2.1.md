---
plugin: O-GranulatorInstrument
stage: 2
phase: "2.1"
title: "Slots, tape, voices, one stream"
executed: 2026-11-07
version: 0.1.0 (unchanged, P9)
fork_pin: 82406fce
result: executed — automated gates green; HANDS-ON-2.1 pending (human)
---

# O-GranulatorInstrument — Stage 2 / Phase 2.1 SUMMARY

Notes play. A file loaded into a slot is decoded, resampled (Kaiser, offline) and built into a `MipTape<4>`. The audio thread takes it with no lock or allocation. Up to 16 counted voices (20 slots) each run stream `s1`:
- a Free-mode scheduler, thinned to a water-filled integer quota
- a 3-mode scan playhead and key-tracked pitch (±48 st)
- a region-fit spawner and Hann grains
- the per-voice output law, the amp env and −6 dB voice headroom

Voices then pass the wet safety knee and `output_gain`, all on an absolute 32-sample grid with MIDI applied at sample offsets. `DevLoaderEditor` (`OGI_DEV_LOADER`, plugin target only) drives the real load API.

## Results

| Gate | Result |
|---|---|
| Build | VST3 / AU / Standalone / render-harness / param-dump rc 0. **0 warnings** in plugin sources (the only warnings are the pre-existing note-expression module and VST3 SDK ones). |
| Render harness | **88 PASS / 0 FAIL**: the 55 Stage 1 rows (R0) plus 33 Phase 2.1 rows. Every negative control fires. Runtime ≈ 6 s. |
| param-dump | **Byte-identical** to the Stage 1 capture and to `.planning/params.tsv` (P10). Goldens untouched. |
| Console exclusion (P4) | The console TUs (harness, param-dump) carry 0 `OGI_DEV_LOADER` / `DevLoaderEditor` references and 28 `OGI_TEST_HOOKS` references. The editor TU is compiled only into the plugin SharedCode. |
| Fork drift (F1) | `tests/fork-drift-check.sh` → **0 drift**, 13 leaves. NC-a (byte flipped in a scratch parent tree) → check 1 fires, rc 1. NC-b (byte flipped in a scratch local VERBATIM leaf) → check 2 fires, rc 1. |
| Install | `build-and-install.sh` → `O-GranulatorInstrument-dev.{vst3,component}` (dual-variant sweep clean). |
| auval | `aumu OuGI OuDv` **AU VALIDATION SUCCEEDED**. Negative control `ZZZZ` → "didn't find the component". |
| pluginval 10 | VST3 ×2 **SUCCESS**, AU **SUCCESS** (strictness 10, run one at a time, editor tests against `DevLoaderEditor`). `Main bus num input channels: 2` (SC active). The AU log's only `!!!` line is the informational "Current program is -1". |
| Standalone | Alive after 6 s. |
| HANDS-ON-2.1 | **Pending (human)**: `stages/2-dsp/HANDS-ON-2.1.md`, 13 rows incl. the chooser-UAF row. |

### Phase 2.1 rows (metric | threshold | margin | control)

| ID | Measured | Threshold | Control |
|---|---|---|---|
| Q0 | 0 / 10 000 vectors differ (8 070 capped, 3 514 with ties) | bitwise | `noSort` → 5 590 differ ✔ |
| Q1a | liveMax 32/64/128/256 at caps 32/64/128/256 | ≤ cap, ≥ cap/2 | (Q1b) |
| Q1b | R3 kills / spawns in 2.5 s steady state: 0/32, 0/80, 0/160, 0/320 | 0 | `thinningOff` cap 64 → 8 000 R3 kills ✔ |
| Q2 | thinned: B 32 ≥ m_B 32, A 32 ≤ m_A 32 (A filled 64); unthinned period: B 32 ≥ 32 | at L_max + 1 cell | unthinned + `overQuotaOff` → B 1 vs m_B 32 ✔ |
| Q3 | thinned 256 → 32 at L_max + 5 ms + 1 cell; unthinned + D25 256 → 32 within 0.5 s | ≤ 32 | `thinningOff + d25Off` → 256 ✔ |
| Q4 | per-voice law error −0.196 / −0.233 / −0.394 dB (1/4/16 v) | ±0.5 dB (margin 0.106) | old-vs-old 0 dB; thinning off → −39.7 dB off-law ✔ |
| T1 | 0 / 185 606 settled samples differ vs `MipRing<4>::push` | bitwise | input +1e-3 → all 5 levels differ ✔ |
| T2 | 0 / 1 000 reads differ vs `MipSnapshot` | bitwise | input +1e-3 → 64/64 ✔ |
| K1 | DC error 0 / 0 (44.1→96 k, 96→44.1 k) | ≤ 1e-6 | unnormalised → 1.8e-6 / 1.5e-6 ✔ |
| K2 | passband worst 0.0001 dB to 0.9·min/2; stopband worst −113.3 dB | 0.01 dB / −90 dB (margins 0.0099 / 23.3) | β = 2 → −58.8 dB ✔ |
| K3 | 48→48 k bit-exact | bitwise | — |
| L1 | 40 s @ 44.1 k → 1 323 000 frames, trimmed; 20 s @ 192 k → 2 880 000 (15 s), trimmed | exact | 10 s file → untrimmed ✔ |
| L2 | mono → L ≡ R on every level | bitwise | stereo → 46 834 / 48 760 differ ✔ |
| P1 | latched-rate rel err 0 (kt 100 and kt 0) | 1e-12 | kt 100 → 3 distinct rates ✔ |
| P2 | root 0.0000 ¢, root+12 −0.0000 ¢ (2 s grain, central 50 %) | ±1 ¢ (margin 1.0) | fine +5 ¢ → −5.000 ¢ ✔ |
| P3 | +48 st 4 kHz → 32 kHz bin −110.8 dB; +24 st 15 kHz → 36 kHz bin −105.2 dB | ≤ −60 dB (margin 45.2) | 2.5 kHz @ +48 → 40 kHz present at −4.6 dB ✔ |
| P4 | root+60 → +48 clamp, level 4, rate 16; R2: 0 / 1 000 last-1 k-frame reads differ, 0 clamped | exact | — |
| S1 | start-position error ≤ 2e-9 frames; 0 % holds; ping-pong reflects | 1e-6 | Retrigger vs Free-run 2nd-note start differs ✔ |
| S2 | sorted region; 10 ms = 480 frames; end clamp; params unchanged | exact | — |
| V1 | 4 voices, 4 distinct rates, rel err 0 | exact | — |
| V2 | 2 steals (serials 2, 3); d2 peak ratio vs no-steal 0.965 | ≤ 2.0 (margin 1.035) | 0 ms ramp → 5.34 ✔ |
| V3 | 0 / 201 grains differ under slot permutation | bitwise | seed + 1 → 201 differ ✔ |
| V4 | 8 force-released; 4 sounding after 5 ms + 2 cells; d2 ratio 0.466 | ≤ 2.0 | — (INFO: 0 ms ramp → 4.64) |
| M1 | 0 stuck (zero-length note, off→on retrigger, 600 events/block, 300/300 started) | 0 | — |
| E1 | stage ends within one cell (all 4 sets); velocity law worst 0.0004 dB; click worst 1.000 | 32 smp / ±0.05 dB / ≤ 2.0 | — |
| H1 | worst 10 ms RMS −18.8 dB; recovery −0.00 dB; d2 1.093; retire after 288 smp | ≥ −24 / ≥ −3 / ≤ 2.0 / ≤ 336 | — |
| H2 | pending ≤ 1; 51 published, 50 superseded freed, 0 deferred, 1 live tape | as stated | no entry counter → 19 deferred (leak) ✔ |
| H3 | retired after 1 024 smp with 0 references; fade-only hold observed | 0 refs | `retireScan256` → 23 early retires ✔ |
| H4 | 48 → 96 k mid-note: pitch err 0; crossfade 1 920 smp; d2 0.915 | 1e-12 / 1920 ± 32 / ≤ 2.0 | — |
| H5 | 200 interleaved prepare/setState/load/process: consistent, settled, 0 leaked tapes | — | — |
| N1 | blocks 441/512/4096/8192(large-block probe)/variable vs 32: 0 samples differ (size_rand 50 %) | bitwise | seed change → 191 997 differ ✔ |
| N2 | 236 unread params randomised → 0 differ | bitwise | `s1_density` → 383 differ ✔ |
| A1 | 0 audio-thread allocations over 252 blocks (15 steals, 3 swaps, 1 SR crossfade, 44 kills) | 0 | liveness `malloc(64)` → 1 ✔ |

## Fork (D11)

13 leaves are in `Source/dsp/forked/`, renamed (ogran→ogi, OGRAN_TEST_HOOKS→OGI_TEST_HOOKS), with `FORK-MANIFEST.md` (pin blob sha256 + local sha256 for every leaf). D43 (`FeedbackChain.h`) and D47a (`ModSources.h`) are included at the pin.
- **ADAPTED:** `EngineConstants.h` and `ControlGrid.h`.
- **ADAPTED (pending):** `ScaleQuantiser` (2.2), `ModSources` and `ModMatrix` (2.5), `UiTelemetry` (2.6). These stay byte-for-byte rename-only until their phase.
- **VERBATIM:** the other 7.

At fork time there was 0 upstream drift (`git diff 82406fce HEAD -- plugins/O-Granulator/Source/dsp` empty).

## Plan flags adopted (research flags)

- **P1 integer-quota thinning:** `T_k = max(fs/density, (L_max+1)/m_k)`. Q1b proves 0 steady-state R3 kills. If the literal real-q reading is preferred, drop Q1b only.
- **P2 `s1_size_rand` wired** (one CounterRng draw per onset, keyed on serial). It keeps N1's seed control non-vacuous.

## Deviations from the plan

1. **Pitch formula = ARCHITECTURE :407** `st = κ·(note − root − fine/100 + bend·pbr) + s1_pitch`. The plan's shorthand put fine outside κ with the opposite sign. ARCHITECTURE is the definition: fine tunes the *file*, so it is subtracted inside κ. P2's control therefore reads −5 ¢ for fine +5.
2. **EngineConstants:** `kMaxReachS` 10 → 34 and `kRingSeconds` 41 → 65, in addition to the planned edits. Without them the parent's reach `static_assert` fails at `kMaxRate 16`. The instrument never allocates that ring. Recorded in the manifest.
3. **Kaiser resampler: 64 zero-crossings per side** (RESEARCH said 32). With 32, 0.9·min/2 sits inside the transition band (−0.30 dB, K2 fail). The cost is offline only (loader thread).
4. **Engine fix found by Q3:** with every live slot busy, `acquire`'s native steal takes an *already-killing* grain. The new grain is then a net +1 alive, the D25 extra kill only broke even, and a lowered `grain_limit` never converged while the pool stayed full (stuck at 255 with cap 32). That path now also kill-fades the top-priority victim, as the R3 path does (`GiSpawner.h`).
5. **Gate-design changes** (stimulus, not thresholds):
   - **Q1b:** the steady window is 2.5 s, because at cap 32 the period is ≈ 1 s and 0.5 s was vacuous.
   - **Q2:** under integer thinning, A's excess drains at exactly B's spawn rate, so the over-quota term cannot change the thinned outcome. The thinned assertion is kept, and the control runs in a new `periodThinOff` regime: spawn period unthinned, quotas finite.
   - **H3:** a test-only `testSetPoolFadeMs(20)` lets a fade spare outlive the 5 ms swap kills. At equal 5 ms fades the 256-vs-288 scan difference is unreachable.
   - **E1:** the attack click is compared at equal envelope level. The attack peaks at 1.0 while the reference is measured at the sustain level.
   - **T1/T2:** the control is an input bump of 1e-3. The 1-ulp double tap perturbation cannot move a float output, so it is printed as INFO.
6. **Q0 control:** `WaterFillTest::noSort`. Tie order is result-invariant by construction, so a tie-order control would never fire.
7. **Wet safety** is P8's `1.9953·tanh(x/1.9953)`, not the forked `wetKnee`.
8. **Loader** refuses sources shorter than 10 ms (this keeps Stage 1 row H1's 16-frame failure case). The trim also caps at 2.88 M *native* frames.
9. **`prepareToPlay` clears voices and the pool** (SR rebuilds arrive through the 20 ms dual-read crossfade).

## Carried forward

- **Goldens re-baseline in 2.2 is expected (P11).** 2.1 ignores the spray, jitter and spread defaults, so the 2.2 output shift is not a regression.
- **`kVoiceGainDb = −6 dB` is provisional**, tuned in the 2.2 listening pass.
- `kModSources 12 / kModDests 113 / kSrcTelemetry 11` stay at parent values until 2.5 (spec: 18 / 126). Flagged in the manifest.
- `DevLoaderEditor`, `OGI_DEV_LOADER` and the `createEditor` branch are removed in Phase 3.1.
- Slots do not persist across session reopen until 2.6 (`<Slots>` state).
- Module-gap candidate (G5): `MipTape` duplicates the grain-engine halfband (8 lines). `BandlimitedReader.h` was not edited.

## Files

- New: `Source/dsp/{GiConstants,SlotTape,VoiceQuotas,KaiserResampler,MipTape,SlotHandoff,MidiCursor,VoiceAllocator,AmpEnv,ScanPlayhead,VoiceGrain,GiSpawner,GiRenderer}.h`, `Source/dsp/{SlotLoader,Engine}.{h,cpp}`, `Source/dsp/forked/*.h` (13) + `FORK-MANIFEST.md`, `Source/PluginEditorDevLoader.{h,cpp}`, `tests/fork-drift-check.sh`, `tests/render-harness/engine_gates_21.h`, `stages/2-dsp/HANDS-ON-2.1.md`.
- Modified: `CMakeLists.txt` (dsp sources, dev-loader TU, `OGI_DEV_LOADER` option on the plugin target only, `Source/dsp{,/forked}` include paths), `Source/PluginProcessor.{h,cpp}`, `tests/render-harness/main.cpp` (CoreFoundation pre-include before `using namespace juce`, plus the `eg21::runAll()` call).
- **No tag.**
