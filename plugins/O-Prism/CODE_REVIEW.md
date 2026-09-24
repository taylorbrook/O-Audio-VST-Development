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
verified: 2026-09-23T00:00:00Z
verified_resolution: 1.26.1 (CR-03, WR-03, WR-07, WR-09 + the startNote pitch-wheel seed);
  1.27.0 (CR-01, CR-02);
  1.27.1 (no finding — retraction of the v1.27.0 "FX mod NaN" note + the
  advanceGlobalLfoPhases finite guard and both harness rate declarations)
  1.27.2 (WR-01)
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
> | WR-03 | ✅ Resolved in **v1.26.1** | `clearUserWavetableOverride(0/1)` unconditionally, ahead of the `userWtState` block (so legacy states with no child are covered too). |
> | WR-07 | ✅ Resolved in **v1.26.1** | Dead-band guard removed; all four `setRate` calls unconditional. |
> | WR-09 | ✅ Resolved in **v1.26.1** | Early return in `processBlock` on `getNumChannels() == 0`, still publishing `blockGeneration`. Covers `PrismVoice.cpp` `getWritePointer(0)` via the same chokepoint. |
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
**File:** `Source/dsp/NoiseGenerator.cpp:73-85`

Brown (`:89` `rateScale`), Vinyl (`:121` `cutNorm = 2000/fs`) and Wind (`:150, :161`) all
scale their coefficients by the sample rate. The Paul Kellet pink filter uses the poles
`0.99765 / 0.96300 / 0.57000`, which are fixed for 44.1 kHz.

**Failure:** the same preset's noise bed is measurably brighter at 48 kHz and noticeably so
at 96 kHz — the pole frequencies scale with fs, so the −3 dB/oct slope shifts up. Same
class as `pattern_noise_bed_level_is_rate_dependent`.

**Fix:** warp the three pole coefficients by `44100 / fs` in `prepare()`.

### WR-06: Two clock-seeded RNGs make any render non-reproducible
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

### IN-01: Half of `WavetableGenerator` is dead — ~105 LOC
`Source/dsp/WavetableGenerator.h:33-53`, `.cpp:35-127`. `generateProceduralTable`,
`generateSaw`, `generateSquare`, `generateTriangle`, `generateSine` and the `WaveShape`
enum have no callers anywhere — `WavetableFactory` owns every factory table and the
importer/editor call only `generateMipmaps` / `generateMipmapsForFrame`. Bonus: the three
additive generators are O(N²) (`2048` harmonics × `2048` samples = 4.2M `std::sin` calls
each), so deleting them also removes dead construction-time cost.

### IN-02: `generateMipmaps` duplicates `generateMipmapsForFrame` verbatim — ~55 LOC
`Source/dsp/WavetableGenerator.cpp:129-193` vs `:195-248`. Identical FFT setup, identical
per-level bin zeroing, identical mirror loop. Extract one file-static helper taking
`(table, frameIndex, fft, fftBuffer, workBuffer)`; the whole-table path keeps its
allocate-FFT-once property by passing its own buffers in.

### IN-03: Dead public API
- `SVFFilter::setKeyTrack` (`SVFFilter.cpp:75-86`) — `PrismVoice` computes key tracking
  itself at `:505-510` and calls `setCutoff`. As written the method is also wrong: it
  multiplies `cutoffHz` cumulatively on each call.
- `ModulationMatrix::clearOffsets` (`ModulationMatrix.cpp:97-100`) — `evaluate()` already
  fills zero.
- `TuningEngine::getScaleFrequencies` (`TuningEngine.cpp:786-800`).
- `TuningEngine::clearPitchBend` / `clearAllPitchBends` — **do not delete these**; see
  CR-03, they should be called.

### IN-04: SVF 24 dB second stage is `processSingleSVF` inlined with a magic constant
`Source/dsp/SVFFilter.cpp:156-171` repeats the TPT core and recomputes
`h = 1/(1 + 2*0.707*g + g*g)` **every sample** from a literal `0.707`. Give
`processSingleSVF` an `R2` parameter and cache the Butterworth `h` next to `g`/`R2` in
`updateCoefficients()`. Carried over from the v1.9.0 improve backlog (items 6 and 10),
never applied.

### IN-05: `processNotch` is `processSingleSVF` plus one line
`Source/dsp/SVFFilter.cpp:124-133` — the whole body is identical to `:103-111`; only the
return differs (`yLP + yHP`). Fold it in as `case 6`.

### IN-06: The mod matrix rescans all 16 slots for every sample of every voice
`ModulationMatrix::evaluate()` (`ModulationMatrix.cpp:70-87`) is called per sample at
`PrismVoice.cpp:555`. It zeroes all 26 destinations and walks all 16 slots regardless of
how many are enabled. At 16 voices and 48 kHz that is ~32M operations per second of
overhead when a typical patch uses two or three routes.

**Fix:** in `updateFromAPVTS` (already once per block) compact the enabled slots into a
small `activeSlots` array and record which destinations they touch; `evaluate()` then
clears only those destinations and walks only the active list.

### IN-07: `getFrameHarmonics` builds a `juce::dsp::FFT` on every call
`Source/dsp/WavetableEditor.cpp:74` — `juce::dsp::FFT tempFFT (11);` allocates its twiddle
tables per call, purely because the method is `const` and the `fft` member is not. Mark the
member `mutable` and drop the local. Called on every harmonic-editor refresh.

### IN-08: `readSample` can index one past the frame for a tiny negative phase
`Source/dsp/WavetableOscillator.cpp:151-155`. `phase -= std::floor(phase)` on `-1e-20`
gives `1.0 - 1e-20`, which rounds to exactly `1.0` in double; then `samplePos = 2048.0`,
`idx0 = 2048`, and `getSample(level, frame, idx0 + 1)` reads index 2049 — one past the
2049-element frame, and past the end of `data` for the last frame at the last level.

**Not currently reachable** — every path into `readSample` already wraps (Bend returns
`pow(p, e)` with `p ≥ 0`, FM wraps, Sync/Off read a wrapped accumulator). Listed because
closing it is `idx0 = std::min(idx0, WavetableData::kTableSize - 1);` and the failure mode
is a heap OOB.

### IN-09: Distortion "Fold" aliases at 2× oversampling
`Source/dsp/DistortionProcessor.cpp:34` (`2^1 = 2x`), `:94-96`
(`data[i] = std::sin(x * kPi)`). Sine folding generates unbounded harmonic order; at drive
1.0 the pre-gain is 10×, so `sin(10x·π)` produces components an order of magnitude above
the source that 2× cannot contain. The other three shapes are saturating and mostly fine at
2×. Either raise oversampling for this mode specifically or swap to a bounded triangle fold.

### IN-10: Planning artifacts are stale
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
