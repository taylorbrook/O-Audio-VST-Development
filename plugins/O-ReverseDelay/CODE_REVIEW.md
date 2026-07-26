---
plugin: O-ReverseDelay
version: 1.7.1
date: 2026-07-25
depth: deep
verified: 2026-07-25          # /improve-verify PASS over v1.7.2 + v1.7.3
verified_scope: 12 of 13 CLOSED, IN-05 PARTIAL (module half declared out of scope)
scope:
  - Source/PluginProcessor.h
  - Source/PluginProcessor.cpp
  - Source/PluginEditor.h
  - Source/PluginEditor.cpp
  - Source/dsp/CaptureBuffer.h
  - Source/dsp/GrainScheduler.h
  - Source/dsp/ReverseGrain.h
  - Source/dsp/WindowLut.h
  - Source/ui/public/js/app.js
  - Source/ui/public/index.html
  - Source/ui/public/css/styles.css
  - CMakeLists.txt
  - tests/render-harness/CMakeLists.txt (reference)
  - tests/render-harness/main.cpp (reference only)
findings:
  critical: 2
  warning: 5
  info: 6
  total: 13
---

# O-ReverseDelay v1.7.1 — Deep Code Review

## Summary

Two defects reach shipped audio, and both are in v1.6.0/v1.7.0 code rather than in
the v1.0–v1.5 core.

**CR-01** is the v1.0.1 "A2" defect reopened by v1.7.0's delay drift. The engine's
sub-block pass bound (`passBound`, `PluginProcessor.cpp:1444`) is widened to `D`
whenever `delayScatter == 0`, on the reasoning that "every grain's delay IS `D`" —
which stopped being true the moment `driftMul()` was allowed to *reduce* a latched
delay to `0.75·D`. At host block sizes ≥ ~2048 with a short-to-medium delay, grains
spawned late in a pass latch a read position **ahead of the capture write head** and
read one full 14 s ring lap of stale material. It also breaks the 512-vs-4096
bit-identity the whole harness is built on. The reason the 138 probes missed it is
exact and checkable: every drift probe (AV, AW, AX) runs with `delayScatter` at 80 ms
or at its maximum, and `scatterSamples > 0` is precisely the branch that selects the
*safe* bound. `drift-is-live` runs scatter-free but at `block = 512`.

**CR-02** is the freeze rising-edge latch. `freezeLoopSamples` is clamped to `[1, N−1]`
against `capture.getTotalWritten()`, so a session saved with Freeze engaged reopens
with `totalWritten == 0`, latches a **one-sample loop against a freshly cleared ring**,
and writes zeros into the ring for the rest of the session. The wet path is silent and
never recovers, because `freezeEngaged` is set true on that same block and the latch
never re-arms. Probe AP deliberately freezes 3 s after load, which is the one timing
that cannot see it.

Beyond those, the highest-value finding is **WR-01**: the render harness's
`JucePlugin_VersionString` has drifted to `"1.5.0"` again while the plugin ships
1.7.1 — the exact stale-fixture failure v1.5.0 found, wrote up at length, and fixed.
Both preset sentinels key off that string, so probe N's factory audit and probe R's
migration check are not auditing 1.7.1's presets, and the harness and the installed
plugin now ping-pong the `.factory-version` / `.user-migration-version` sentinels.
**WR-02** is its sibling: `GrainScheduler::kMaxSpawnsPerBlock`'s derivation assumes the
scatter-only pass bound, and probe AB reproduces the same wrong derivation and tests
the one configuration where it happens to hold.

What is genuinely clean, checked line by line rather than assumed:

- **The ring invariant.** `kCaptureSeconds = 14.0` with a real `static_assert`
  (`PluginProcessor.h:628`) whose parenthesisation correctly composes drift as a
  multiplier and scatter as an addend. The runtime `maxLatchedDelay` guard
  (`:1352`) computes 6.0 s against a 5.5 s worst case. Correct at the current maxima.
- **RT-safety.** No allocation, lock or file IO anywhere reachable from `processBlock`.
  Coefficients go through `ArrayCoefficients` assigned in place; the scheduler's RNG is
  a template parameter, not a `std::function`; `ScopedNoDenormals` is present.
- **The C++↔WebView bridge is closed in both directions.** 13 `withNativeFunction`
  registrations against 3 in `app.js` + 10 in `preset-manager.js`, names matched
  exactly. 20 `kSliderIds` ≡ `KNOB_IDS`, 4 `kComboIds` ≡ the four `getComboBoxState`
  sites, 1 `kToggleIds` ≡ the one `getToggleState`. `presetmanager_js` uses the
  hyphen-stripped symbol. No dead control.
- **Knob readouts** are exclusively `getScaledValue()`; `scaledToNorm` reproduces
  JUCE's `Math.pow(proportion, skew)` exactly; double-click reset goes through
  `getParameterDefaults`. No JS range table anywhere.
- **JS label safety and TDZ.** Every `textContent` write targets a value node; all
  segment/button copy comes from HTML or `data-*`. All module state is declared in one
  top block and the single `init()` call is the last statement.
- **MSVC hazards.** `kDivisionBeats` is `static constexpr`; the two `launchAsync`
  completions hoist `SafePointer` to a local and take a bare `return` on the null path.
- **Layout.** `setSize(940, 768)` matches `html/body` and `.frame`; 768 ≤ 900.
- **Latency.** `setLatencySamples` is correctly never called — the dry path is
  unaltered and the wet is a delay, so zero reported latency is right.
- **Preset migration.** The two arms carry genuinely separate version gates
  (`< 1.0.1` for `delayTime`, `< 1.5.0` for `grainSize`), the legacy `grainSize` curve
  is reconstructed with *both* its old max and its old skew centre, and all eight
  factory presets are authored in engineering units and pushed through
  `convertTo0to1`. No AsyncUpdater exists anywhere in the plugin, so the
  `cancelPendingUpdate()` hazard is not applicable.
- **The duck follower** genuinely runs per sample with per-block coefficients, and its
  `isfinite` reset is correctly placed after the update and before the divide.

---

## Critical

### CR-01: `driftDepth > 0` with `delayScatter == 0` reopens the A2 unwritten-capture read

**File:** `Source/PluginProcessor.cpp:1444` (with `:1514-1536`)

**What:**
```cpp
const int minDelaySamples = juce::jmax(1,
    static_cast<int>(kDelayTimeMinMs * 0.001 * currentSampleRate));
const int grainDelayFloor = juce::jmin(D, minDelaySamples);
const int passBound       = scatterSamples > 0 ? grainDelayFloor : D;   // <-- 1444
const int passLen         = juce::jmax(1, juce::jmin(numSamples, passBound));
```
The A2 invariant is `passOffset < gD` for every spawned grain. The `scatterSamples > 0`
ternary encodes the assumption that *the only thing that can pull a latched delay below
`D` is scatter*. v1.7.0 broke that assumption at `:1515`:

```cpp
const float driftedD = static_cast<float>(D)
                         * driftMul(driftDepthNorm, driftRateHz, spawnAbs, currentSampleRate);
int gD = static_cast<int>(driftedD);
```
`driftMul` returns `1 + depth·0.25·sin(φ)` ∈ `[0.75, 1.25]`, so with drift on and
scatter off, `gD` can be `0.75·D` while `passLen` is still `min(numSamples, D)`. The
clamp at `:1536` floors `gD` at `grainDelayFloor = jmin(D, minDelaySamples)`, which is
`minDelaySamples` (50 ms) as soon as `D > 50 ms` — far below `passLen`.

The `static_assert` and `maxLatchedDelay` both guard the *upper* end of the read span.
Nothing guards the lower end once drift is in play.

**Why it matters:**
48 kHz, host block 4096, `delayTime` 95 ms (`D` = 4560), `driftDepth` 100 %,
`delayScatter` 0. Then `grainDelayFloor` = 2400, `passBound` = 4560,
`passLen` = 4096, and `gD` ranges over `[3420, 5700]`. A grain spawning at
`passOffset = 3800` on the negative half of the LFO latches
`readAbs = passStartAbs + 3800 − 3420 = passStartAbs + 380` — **380 samples ahead of
the write head**, which step 6 does not write until after step 4 has already rendered.
The grain's head reads the ring slot from one full 14 s lap ago (or silence in the
first 14 s of the session), i.e. exactly the "stale blip at the head of the grain" the
v1.0.0 review's A2 described. With `direction > 0` it is worse: a forward grain reads
`t − gD` at *every* sample of its life, so the whole grain is corrupt, not just its
head. Reachable range at 48 kHz / 4096: `delayTime` ≈ 51–114 ms; at 44.1 kHz / 2048,
`delayTime` ≈ 51–57 ms. The same session renders differently at 512 and at 4096, so an
offline bounce does not match what was monitored — the invariant probes O, W2, AQ and
AX exist to prevent.

Why the harness did not catch it: `drift-ring-clamp` (probe AV) sets
`delayScatter = kDelayScatterMaxMs`; `v170-blocksize-invariance` (probe AX) sets
`delayScatter = 80.0f`. In both, `scatterSamples > 0` selects the *conservative*
`grainDelayFloor` bound, which is exactly the branch that is safe. The one scatter-free
drift probe, `drift-is-live` (probe AW), runs at `block = 512` with
`delayTime = 500 ms`, where `passLen = 512` is two orders of magnitude below the
smallest reachable `gD`. No probe varies block size while drift is on and scatter is
off. `auval` and `pluginval` never see it because the read does not fault, does not
produce a NaN and does not click.

**Fix:** make the pass bound depend on every parameter that can *reduce* a latched
delay, not just on scatter:

```cpp
// Any randomisation that can pull a grain's latched delay BELOW D forces the
// conservative bound. scatter subtracts; drift multiplies by as little as
// (1 - kDriftMaxFraction). Both must be tested here or A2's `i < gD` fails.
const bool delayMayShorten = (scatterSamples > 0) || (driftDepthNorm > 0.0f);
const int  passBound       = delayMayShorten ? grainDelayFloor : D;
```
`grainDelayFloor` is a pure function of the parameters, so `passLen <= grainDelayFloor
<= gD` restores `passOffset < gD` at every host block size, and the fix is bitwise
inert at `driftDepth == 0` (the shipped default, every factory preset and every
pre-v1.7.0 session). Add a probe that renders `driftDepth = 100`,
`delayScatter = 0`, `delayTime = 95 ms`, `direction = 60` at 512 and at 4096 and
asserts `max|512 − 4096| == 0`.

**Risk of fix:** LOW — one boolean; the changed path is only entered when
`driftDepth > 0`, which no shipped preset or pre-v1.7.0 session reaches.

---

### CR-02: a session saved with Freeze engaged reopens with a permanently silent wet path

**File:** `Source/PluginProcessor.cpp:1141-1147` (with `:962-972`, `Source/dsp/CaptureBuffer.h:100-104`)

**What:** `prepareToPlay` deliberately jumps `freezeSmoothed` to the parameter's value
and clears the edge detector:

```cpp
freezeSmoothed.setCurrentAndTargetValue(pFreeze->load() >= 0.5f ? 1.0f : 0.0f);
freezeEngaged     = false;
freezeLoopSamples = 1;
```
The first `processBlock` then sees `frozen && ! freezeEngaged` and latches:

```cpp
freezeLoopSamples = static_cast<int>(
    juce::jlimit(static_cast<juce::int64>(1),
                 static_cast<juce::int64>(juce::jmax(1, capture.getBufferSize() - 1)),
                 capture.getTotalWritten()));
freezeEngaged = frozen;
```
At that instant `capture.getTotalWritten()` is **0** (the ring was just allocated and
cleared by `capture.prepare`), so the `jlimit` floor produces `freezeLoopSamples = 1`.
Because `freezeSmoothed` is already at 1.0, `pushCrossfaded` takes the
`holdWeight >= 1.0f` short circuit into `pushLooped(1)`, which writes
`readAbs(ch, totalWritten − 1)` — the previous ring slot, which is zero. Every
subsequent sample copies the zero it just wrote. The ring stays all-zero for the life
of the hold, and `freezeEngaged` is now `true`, so the latch never re-arms.

**Why it matters:** Freeze is a performance control, so saving a session with it
engaged is ordinary use. On reopening that session the plugin looks correct — the
FREEZE segment is lit, all 20 knobs read their saved values, the grain meter shows
grains active — and the wet output is **exactly zero forever**. The dry passes through,
so it presents as "the plugin stopped working", not as a freeze. The only recovery is
for the user to toggle Freeze off, wait for capture, and toggle it back on; nothing in
the UI suggests that. The same latch fires whenever the user engages Freeze inside the
first block after load, which produces the sibling failure the design explicitly set
out to avoid: with `totalWritten` at, say, 200 samples the hold becomes a 200-sample
(240 Hz at 48 kHz) loop — a tone, not a wash.

Why the harness did not catch it: probe AP (`freeze-*`) engages the hold **3 s after
load**, which was chosen to catch the "advance the head without writing goes silent"
failure and is the one timing at which `totalWritten` is comfortably large. No probe
restores a state with `freeze = 1` and then calls `prepareToPlay`. `auval` and
`pluginval` never set `freeze` and then render.

**Fix:** do not arm a hold against a ring that has nothing in it. In `prepareToPlay`,
start un-frozen when the ring is empty and leave the edge detector armed so the latch
happens once real material exists:

```cpp
// A session can be saved with Freeze engaged, but the RING is not part of that
// state — prepareToPlay has just cleared it. Latching the hold here would loop
// one sample of a zeroed buffer and the wet would be silent for the session.
// Start un-frozen; freezeEngaged stays false, so the first block with material
// arms the hold normally.
freezeSmoothed.setCurrentAndTargetValue(0.0f);
freezeEngaged     = false;
freezeLoopSamples = 1;
```
and floor the latch so a hold can never become a tone:

```cpp
// Never latch a loop shorter than one grain: below that the "hold" is a periodic
// tone at 1/loopSamples rather than a wash — the failure pushLooped() exists to
// avoid, reached from the other end.
const juce::int64 minLoop = juce::jmax<juce::int64> (G, 1);
if (frozen && ! freezeEngaged && capture.getTotalWritten() >= minLoop)
{
    freezeLoopSamples = ...;   // as today
    freezeEngaged     = true;
}
else if (! frozen)
{
    freezeEngaged = false;
}
```
(Keeping `freezeEngaged` false until the latch actually succeeds is what makes the
hold arm itself on a later block instead of being lost.) Add a probe that sets
`freeze = 1`, calls `prepareToPlay`, renders 5 s and asserts the wet RMS is non-zero.

**Risk of fix:** MEDIUM — it changes the documented behaviour of "a session saved with
Freeze engaged must reopen frozen". It still reopens *frozen*; it just captures the
first grain-length of material before holding it, which is the only reading under
which the control does anything at all. Re-run probes AP and the freeze block of
probe M.

---

## Warning

### WR-01: the render harness's `JucePlugin_VersionString` has drifted to 1.5.0 again

**File:** `tests/render-harness/CMakeLists.txt:71-72`

**What:**
```cmake
JucePlugin_VersionString="1.5.0"
JucePlugin_VersionCode=0x10500
```
against `CMakeLists.txt:16`'s `VERSION 1.7.1`. The same file's own comment (`:57-69`)
states that this value is load-bearing, names the exact consequence, and records that
it had already drifted once ("it sat at 1.2.0 while the plugin shipped 1.3.0 and
1.4.0"). It has now drifted across v1.6.0, v1.7.0 and v1.7.1.

**Why it matters:** both preset sentinels compare against `JucePlugin_VersionString`.
`OuariconPresetManager::initializeFactoryPresets` returns early when
`.factory-version` already reads that string (`OuariconPresetManager.h:602-604`), and
`migrateUserPresets()` does the same for `.user-migration-version`
(`PluginProcessor.cpp:353-355`). Concretely:

- A machine whose `.factory-version` was stamped `"1.5.0"` by the v1.6.0 harness build
  makes the v1.7.1 harness **skip the re-seed entirely**, so probe N's factory audit
  runs against on-disk presets that predate `sourceMode`, `duck`, `driftRate` and
  `driftDepth`. The CHANGELOG's v1.7.0 claim that "all eight re-seed and recall at
  worst = 0.0000 tolerance" is therefore not established by that run.
- Running the harness and the installed plugin alternately makes the two sentinels
  ping-pong between `"1.5.0"` and `"1.7.1"`, so **every** processor construction
  re-seeds eight factory files and re-walks the whole user preset library on the
  message thread. Any user preset the harness rewrites is stamped `"version": "1.5.0"`
  even though it was migrated under 1.7.1's ranges.

This is `pattern_test_fixture_mirrors_drift_silently` in its purest form: the fixture
keeps passing precisely because it is describing an older release.

**Fix:** stop mirroring the literal. Derive it from the plugin target so it cannot
drift:

```cmake
get_target_property(_ORD_VERSION OuariconReverseDelay JUCE_VERSION)
target_compile_definitions(O-ReverseDelay-render-test PRIVATE
    JucePlugin_VersionString="${_ORD_VERSION}")
```
and add a probe that `static_assert`s / checks `JucePlugin_VersionString` against the
value baked into the plugin target, so a future divergence fails the run rather than
silently auditing old data. Delete `~/Library/O-ReverseDelay/Presets/.factory-version`
and `.user-migration-version` once before the next harness run so probe N re-seeds.

**Risk of fix:** LOW — build-system only. Expect probes N and R to report new (correct)
numbers on the first run after the sentinels are cleared.

---

### WR-02: `kMaxSpawnsPerBlock`'s derivation assumes the scatter-only pass bound, and probe AB mirrors it

**File:** `Source/dsp/GrainScheduler.h:74-83` (with `Source/PluginProcessor.cpp:1444`, `tests/render-harness/main.cpp:2882-2884`)

**What:** the cap's bound is derived as

```
spawns per pass  = passLen / interval
passLen         <= kDelayTimeMinMs·fs             (the A2 pass bound)
-> spawns <= overlapMax · kDelayTimeMinMs / kGrainSizeMinMs = 16
```
`passLen <= kDelayTimeMinMs·fs` is only true when `scatterSamples > 0`. With scatter at
0 — the shipped default and every factory preset — `passBound = D`, so
`passLen = min(numSamples, D)` and the bound is governed by the **host block size**,
not by `kDelayTimeMinMs`. Probe AB (`spawncap-headroom-*`) reproduces the same
derivation verbatim in its comment and then pins `delayTime` to
`kDelayTimeMinMs`, which is the single configuration in which the false premise
happens to be true (`D == minDelaySamples`, so `passLen <= D` *is* `<= 50 ms·fs`).

**Why it matters:** the real worst case is `delayTime` at its **maximum** with
`grainSize` at its minimum: `interval = G/overlap = 2400/16 = 150` samples at 48 kHz,
`passLen = numSamples`. A 4096-sample block gives 27 spawns per pass, not 16 — the
claimed "8× margin" is already 4.7×. At a 16384-sample block (offline bounce in
several hosts) the nominal is 109 against a cap of 128, and with `jitter` at 100 % the
multiplier is uniform on `[0.1, 1.9]`, so reaching 128 needs a mean multiplier of only
0.85 across 128 draws — roughly 3.2 σ, i.e. a dropped spawn every few minutes of
render rather than the "not a probability worth naming" the header asserts. A dropped
spawn is a missing grain (audibly the same as a pool refusal, so not a Critical), but
`droppedSpawns` is documented as "the scheduler failing to report work it decided to
do" and is asserted `== 0` — so this is a live assertion resting on an unsound bound.

**Fix:** correct the derivation to the real quantity and re-point the probe:

```
passLen  <= min(hostBlockSize, kDelayTimeMaxMs·fs)
interval  = G/overlap >= (kGrainSizeMinMs·fs)/overlapMax
-> spawns <= hostBlockSize · overlapMax / (kGrainSizeMinMs·fs)
```
Add a `spawncap-headroom-longdelay` case to probe AB with
`delayTime = kDelayTimeMaxMs`, `grainSize = kGrainSizeMinMs`, `grainCount = 16`,
`density = 100`, `jitter = 100`, `delayScatter = 0`, run at 4096 and 16384, and assert
`getDroppedSpawnCount() == 0`. If it fails, raise `kMaxSpawnsPerBlock` (it is 4 bytes
per slot) rather than changing the ranges.

**Risk of fix:** LOW for the comment and the probe. Raising the constant, if the new
probe demands it, is a one-line array-size change with no audio effect.

---

### WR-03: `lowCut`/`highCut` smoothing is block-quantised, so cutoff automation is not block-size invariant

**File:** `Source/PluginProcessor.cpp:1369-1389`

**What:**
```cpp
const float lc = juce::jlimit(20.0f,  0.49f * fsF, lowCutSmoothed.skip(numSamples));
const float hc = juce::jlimit(500.0f, 0.49f * fsF, highCutSmoothed.skip(numSamples));
```
`skip(numSamples)` advances the smoother by a whole host block and returns the value
*after* the block; the coefficients are then recomputed once and held for the entire
block. The 20 ms smoothing contract is therefore sampled at the host's block rate.

**Why it matters:** two consequences, both real:

1. **The 512-vs-4096 bit-identity is conditional and nowhere says so.** `NOTES.md` and
   the v1.7.0 CHANGELOG state the invariant unconditionally ("a 512- and a
   4096-sample render of the same input are BIT-IDENTICAL"). It holds only while the
   cutoffs are static, which is the only condition probes O, W2, AQ and AX ever test —
   they set parameters before `prepareToPlay` and never move them, and
   `setCurrentAndTargetValue` starts the smoothers at target. Automate `highCut` and
   the same session bounces differently offline than it monitored, which is the exact
   class of defect the duck follower was deliberately built per-sample to avoid.
2. **At large buffers the smoother does nothing.** A 20 ms ramp at 48 kHz is 960
   samples; a 4096-sample block skips past it in one step, so a swept `highCut` becomes
   a single large biquad coefficient jump per block instead of eight small ones. Inside
   a feedback loop that is a zipper/click risk on a fast sweep. Probe M's click detector
   runs at `block = 512`, where the ramp is resolved in eight steps and the artefact
   does not appear.

**Fix:** decouple the control rate from the host block. Advance the cutoff smoothers
and recompute coefficients on a fixed sub-block (32 or 64 samples) inside the existing
pass loop, so the update grid is a function of the sample rate alone:

```cpp
static constexpr int kCoeffUpdateSamples = 32;   // ~0.7 ms at 48 kHz — control rate,
                                                 // fixed so it does not follow the host
for (int i = off; i < passEnd; i += kCoeffUpdateSamples)
{
    const int n = juce::jmin (kCoeffUpdateSamples, passEnd - i);
    updateDampingCoefficients (lowCutSmoothed.skip (n), highCutSmoothed.skip (n));
    // ... filter n samples
}
```
Note this changes the rendered output for any session that *moves* a cutoff, so it must
be landed with a diff of every probe result and a note in the CHANGELOG. At minimum,
if the change is declined, correct the invariance claim in `NOTES.md` and the header to
"bit-identical for static parameters" and add a probe that renders a `highCut` sweep at
512 and 4096 and reports the divergence rather than leaving it unmeasured.

**Risk of fix:** MEDIUM — it perturbs any render in which a cutoff is being smoothed,
including the first ~20 ms after a preset load. Bitwise-inert for static cutoffs, which
is every existing probe.

---

### WR-04: `getTailLengthSeconds()` is still 10 s after the delay and grain maxima both went to 4 s

**File:** `Source/PluginProcessor.cpp:2044`

**What:**
```cpp
double ReverseDelayProcessor::getTailLengthSeconds() const { return 10.0; }
```
Set at v1.0.0, when `kDelayTimeMaxMs` was 2000 and `kGrainSizeMaxMs` was 500 — i.e.
10 s was about 4× the longest single-generation span. Since then `delayTime` went to
4000 ms (v1.0.1), `grainSize` to 4000 ms (v1.5.0), drift adds +25 % (v1.7.0), and
`regenMakeup` can push the loop into sustain (v1.6.0). The `static_assert` at
`PluginProcessor.h:628` computes the worst-case single-generation read span as
**13.5 s** — already longer than the declared tail, before any feedback.

**Why it matters:** hosts honour `getTailLengthSeconds()` when deciding how far past
the last input event to keep rendering an offline bounce. At `delayTime` 4000 ms,
`grainSize` 4000 ms, `feedback` 70 % and `regenMakeup` 3 dB, the tail runs for tens of
seconds; the host stops at 10 s and the bounce ends mid-wash while the same settings
monitored live decay properly. This is the RESEARCH pitfall 11 the constant exists to
address, no longer sized for the current ranges. The harness renders fixed-length
buffers itself and so cannot see a host-side truncation, and neither `auval` nor
`pluginval` checks tail plausibility.

**Fix:** derive it rather than pinning it, and make it move with the ranges:

```cpp
double ReverseDelayProcessor::getTailLengthSeconds() const
{
    // One generation is gD + 2*G (the same span kCaptureSeconds is sized for), and
    // feedback adds several more. Derived from the range constants so a future
    // range move cannot leave this behind, which is exactly what happened between
    // v1.0.0 and v1.5.0.
    constexpr double kGenerations = 4.0;
    return kGenerations * (kCaptureSeconds - 0.5);   // ~54 s
}
```
or, more conservatively, `kCaptureSeconds` (14 s) at minimum. Note that an over-long
tail costs offline render time but never correctness, while an under-long one truncates
audio.

**Risk of fix:** LOW — reported metadata only, no DSP change. Verify the AU/VST3
bounce length in a host after the change.

---

### WR-05: knob drags use window listeners without pointer capture, so a lost `pointerup` sticks the knob to the cursor and leaves an open host gesture

**File:** `Source/ui/public/js/app.js:319-348`

**What:**
```js
knob.addEventListener("pointerdown", (e) => {
  dragging  = true;
  startY    = e.clientY;
  startNorm = st.getNormalisedValue();
  st.sliderDragStarted();
  window.addEventListener("pointermove", onMove);
  window.addEventListener("pointerup", onUp);
  e.preventDefault();
});
```
`onUp` is the only thing that clears `dragging`, calls `st.sliderDragEnded()` and
removes the two listeners. There is no `setPointerCapture`, no `pointercancel`
handler and no `lostpointercapture` handler.

**Why it matters:** if the `pointerup` is not delivered to the page — the user drags out
of the plugin window and releases over the DAW, the host takes a modal grab, the WebView
loses focus mid-drag, or the OS synthesises a `pointercancel` instead — then `dragging`
stays `true` and both listeners stay attached. Two things follow: every subsequent mouse
move anywhere over the page keeps writing `setNormalisedValue()` with **no button held**,
so the knob follows the cursor until the user clicks; and `sliderDragStarted()` is left
unmatched, so the host's parameter-change gesture stays open — in Logic and Live that
latches automation write on that parameter. Both are silent; nothing in the console
fires. `ui_frontend_check.js` and `ui_tooltip_clamp_check.js` drive synthetic events
that always deliver their `pointerup`, so neither can reach this state.

**Fix:** capture the pointer on the knob and terminate on cancel as well as up:

```js
knob.addEventListener("pointerdown", (e) => {
  dragging  = true;
  startY    = e.clientY;
  startNorm = st.getNormalisedValue();
  st.sliderDragStarted();
  // Capture on the knob itself: a drag that leaves the WebView, or is cancelled by
  // the host taking a modal grab, still terminates — otherwise sliderDragEnded()
  // is never called and the host's automation gesture stays open.
  try { knob.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
  knob.addEventListener("pointermove", onMove);
  knob.addEventListener("pointerup", onUp);
  knob.addEventListener("pointercancel", onUp);
  knob.addEventListener("lostpointercapture", onUp);
  e.preventDefault();
});
```
with `onUp` removing all four and releasing the capture. Keep `onUp` idempotent (it
already early-returns on `!dragging`).

**Risk of fix:** LOW — pointer capture is supported by both WKWebView and WebView2, and
the `try/catch` covers the fallback. Re-run `ui_tooltip_clamp_check.js` to confirm the
drag-suppression behaviour of the tooltip layer is unchanged.

---

## Info

### IN-01: `ReverseGrain::age` is write-only dead state — **Resolved in v1.7.3**

**File:** `Source/dsp/ReverseGrain.h:82` (written at `Source/PluginProcessor.cpp:1603` and `:1762`)

**What:** `age` is zeroed at spawn and incremented by `(end - start)` on every pass, and
is read by nothing in `Source/` or in `tests/render-harness/main.cpp`. It is a leftover
from the steal-oldest pool policy that v1.1.0 removed; `GrainPool::obtain()` is now a
find-inactive round-robin that needs no age ordering. The v1.0.0 review's "Dead code"
item cleared `GrainScheduler::sampleRate` and put `CaptureBuffer::readAbs()` to work,
but missed this one.

**Why it matters:** correctness-neutral, but it is an add per active grain per pass on
the audio thread and, more importantly, it reads as live bookkeeping — a future change
could reasonably assume grains are ordered by age.

**Fix:** delete the field and both writes.

**Risk of fix:** LOW.

---

### IN-02: `PluginEditor.h`'s class contract is three releases stale and now contradicts the .cpp — **Resolved in v1.7.3**

**File:** `Source/PluginEditor.h:10-19`

**What:** the header still declares:

> Stage 4 (Polish) grows the window to 940 × 484 … No visualization, no Timer, no
> C++→JS polling bridge … The native-function surface is exactly ELEVEN

Against `PluginEditor.cpp`: the window is 940 × 768 (`:530`), the native-function
surface is thirteen (`:201-305`, and the .cpp says so), and there *is* a C++→JS polling
bridge — `getGrainMeter`, polled at 15 Hz, which v1.3.0 deliberately reversed decision
D10 to add. The comment block at `:57-64` also still says "17 sliders + 3 combos"
against the 20 + 4 actually built.

**Why it matters:** these headers are treated as contracts elsewhere in this codebase —
the .cpp's own comment tells the reader to "keep that count in sync … an unregistered
fn is a silently dead control". A reviewer diffing against the header's eleven would
conclude two registrations are spurious.

**Fix:** update the header block to 940 × 768, thirteen native functions, 20 sliders /
4 combos / 1 toggle, and record that D10 was reversed at v1.3.0.

**Risk of fix:** LOW.

---

### IN-03: `styles.css` states two contradictory WINDOW-panel height budgets — **Resolved in v1.7.3**

**File:** `Source/ui/public/css/styles.css:418-420` vs `:531-537`

**What:** `:418` says WINDOW "lays out 214 px of content (44 select-cell + 80 knob-cell
+ 72 env-cell + 2 × 9 row-gap) into this panel's 213 px body. It is already 1 px over."
`:536` says the same panel totals "212 of 213 … One px spare", using a 78 px knob-cell.
`NOTES.md`'s Known Issues repeats the 212 figure. The two cannot both be right, and the
difference is entirely the knob-cell (78 vs 80).

Recomputed from the rules as written — `.group-window .knob-cell { gap: 5px }`,
`.group-window .knob { height: 46px }`, `.knob-label { font-size: 9.5px }`,
`.knob-value { font-size: 11px }` at default `line-height: normal` — the knob-cell
renders at 80 px, so `:418` is the correct one and the "1 px spare" note at `:536` and
in `NOTES.md` is wrong.

**Why it matters:** correctness-neutral (`.group` sets no `overflow`, so the 1 px
bleeds harmlessly into the panel's 12 px bottom padding), but this is the same shape of
error v1.7.1 exists to correct: a budget comment that cannot fail the build, disagreeing
with a second budget comment 120 lines away in the same file, with `NOTES.md` mirroring
the wrong one.

**Fix:** delete the superseded `:531-537` budget, keep the measured `:418` one, and
correct `NOTES.md`'s "Known Issues" entry to "1 px over, absorbed by the panel's bottom
padding". Better: add the measurement to `ui_frontend_check.js`
(`scrollHeight <= clientHeight` on `.group-window .group-body`) so the number is
asserted rather than written down twice.

**Risk of fix:** LOW — comments and a test assertion.

---

### IN-04: `envLastCurve` is assigned and never read; the DPR redraw its comment promises does not exist — **Resolved in v1.7.3**

**File:** `Source/ui/public/js/app.js:234` (assigned at `:657`), with `:560-579`

**What:** `envLastCurve` is documented as "last curve received, for a DPR redraw with no
fetch", is written in `fetchEnvelope()`, and is read nowhere. `envResize()`'s comment
says it is "called on every draw rather than once: a window dragged between a retina and
a non-retina display changes devicePixelRatio with no resize event that this page would
otherwise see" — but `envResize()` is only reachable from `drawEnvelope()`, which is only
reachable from `fetchEnvelope()`, which only runs on a parameter change or at init.

**Why it matters:** correctness-neutral, but the stated behaviour is absent: move the
plugin window between a retina and a non-retina display without touching Shape, Tilt or
Taper and the envelope canvas stays at the old backing-store resolution until the next
parameter move. The dead binding makes it look handled.

**Fix:** either wire it up —

```js
const dprQuery = window.matchMedia(`(resolution: ${window.devicePixelRatio}dppx)`);
dprQuery.addEventListener("change", () => { if (envLastCurve) drawEnvelope(envLastCurve); });
```
— or delete `envLastCurve` and the misleading half of the `envResize()` comment.

**Risk of fix:** LOW.

---

### IN-05: the preset sentinels are check-then-write, so the race they are documented to prevent survives — **Partially resolved in v1.7.3**

**File:** `Source/PluginProcessor.cpp:348-431` (and `:265-271`; `OuariconPresetManager.h:601-604`)

**What:** `migrateUserPresets()` reads the sentinel, walks and rewrites the user preset
directory, and only then writes the sentinel (`:429-430`). `initializeFactoryPresets()`
has the same shape. Both run synchronously from the processor **constructor**. The
comment at `:341-347` gives the race as the reason the sentinel exists ("two instances
constructing concurrently would race on the same files") — but a check-then-act sentinel
written after the work does not close it.

**Why it matters:** a host that instantiates several instances in parallel on the first
launch after an upgrade (session load, or a plugin-scan pass that constructs
concurrently) has every one of them pass the sentinel check and rewrite the same files.
In practice the outcome is benign — the transform is per-file version-gated so all
writers produce identical content, and `File::replaceWithText` writes via a temp file —
so this is Info rather than Warning. It is still N× the intended file IO on the message
thread during construction, which is where AU validation is timing-sensitive.

**Fix:** write the sentinel *before* the walk (accepting that an interrupted pass is not
retried — the current comment already accepts the equivalent trade for backups), or take
a lock file / `InterProcessLock` around the whole block. Simplest correct version:

```cpp
// Stamp FIRST: a concurrent constructor must see the claim before the work is
// visible, or the sentinel does not serialise anything.
sentinel.getParentDirectory().createDirectory();
sentinel.replaceWithText (JucePlugin_VersionString);
// ... then walk and rewrite
```

**Risk of fix:** LOW — but note it makes an interrupted migration permanent, so pair it
with the documented recovery (delete the sentinel) already in `NOTES.md`.

---

### IN-06: `prepareToPlay` dereferences the cached parameter atomics without the null guards `reset()` applies — **Resolved in v1.7.3**

**File:** `Source/PluginProcessor.cpp:957-970` vs `:467-477`

**What:** `reset()` guards every cached pointer (`if (pFeedback != nullptr) …`), while
`prepareToPlay` dereferences the same pointers unconditionally
(`pFeedback->load()`, `pMix->load()`, `pLowCut->load()`, `pHighCut->load()`,
`pFreeze->load()`).

**Why it matters:** correctness-neutral today — all twenty-five IDs resolve, and
`ui_frontend_check.js` closes the layout against the relay lists in both directions. But
the two functions disagree about whether the pointers can be null, and the disagreement
picks the wrong direction: a future ID typo in `createParameterLayout()` would make
`reset()` a silent no-op (hard to notice) and `prepareToPlay` a null dereference (a crash
on the first host prepare, in release).

**Fix:** pick one posture. The stronger one is to `jassert` each pointer once in the
constructor immediately after `getRawParameterValue()` and then drop the null checks in
`reset()`, so an ID typo fails loudly in debug and both functions read the same way.

**Risk of fix:** LOW.

---

_Reviewed: 2026-07-25_
_Reviewer: Claude (gsd-code-reviewer), depth: deep_

---

## Resolved

**v1.7.2** (`975fb40`..) — CR-01, CR-02, WR-01 … WR-05. See the CHANGELOG.

**v1.7.3** (`6df057e`) — the Info tier, swept as one gate.

| ID | Resolution |
|----|------------|
| IN-01 | `ReverseGrain::age` and both writes deleted. |
| IN-02 | `PluginEditor.h` contract re-measured from source — 940 × 768, thirteen native fns, 20 sliders / 4 combos / 1 toggle, 25 params, D10 recorded as reversed at v1.3.0. |
| IN-03 | **Resolved against the finding's own recommendation.** See below. |
| IN-04 | Dead `envLastCurve` binding and the absent-behaviour half of `envResize()`'s comment removed. The "wire it up" arm was declined — new runtime behaviour does not belong in a patch sweep. |
| IN-05 | **Partial.** `migrateUserPresets()` now stamps before the walk. `initializeFactoryPresets()` has the same defect but lives in the shared preset-manager module — still open. |
| IN-06 | 25 cached atomics `jassert`-ed once at construction; `reset()`'s null guards dropped so both call sites read identically. Note the finding's `pFreeze` claim was already stale — v1.7.2's CR-02 fix had removed that dereference. |

### IN-03: the finding identified the right defect and the wrong fix

The finding correctly spotted two contradictory budgets, then recommended
keeping the `214`-into-`213` one on the strength of having *recomputed* the
knob-cell from the CSS rules as written. Rendered and measured at the shipping
viewport, the knob-cell is **78 px, not 80**, and the panel lays **212 into 213 —
1 px under, not 1 px over**. The surviving comment is the one the finding
proposed to delete.

This matters beyond the pixel. Recomputation from rules is precisely how the
second budget came to exist, and the review reproduced that method and so
reproduced its error. The resolution is therefore not a corrected comment but
three assertions in `ui_tooltip_clamp_check.js` measuring the rendered rows.

Two further copies of the wrong figure, which the finding did not mention, were
found by grep during the fix: `styles.css`'s top-of-file block and
`PluginEditor.cpp:518`. Four copies total, three of them wrong.

**A second trap surfaced while writing the guard.** The first version asserted
`contentSum <= body.clientHeight` and could never have failed: `.group-body` is
`flex: 1 1 0%` with `min-height: auto`, so it grows with its content rather than
clipping, and `clientHeight` grows with it — the fail-test showed content and
bound moving together from 213 to 222 while still reporting "0 px spare". That
is `pattern_flex1_container_slack_invisible_to_row_sum`, rebuilt by accident
inside the fix for its twin. The shipped bound is the **panel's** fixed content
box (245 less border and padding = 213), the one quantity that does not move
with the thing it bounds. All three assertions were then verified to fail
against a restored pre-v1.4.0 56 px knob and the layout restored bit-identically.

### Still open

- **IN-05 (module half).** `OuariconPresetManager::initializeFactoryPresets()`
  in `modules/persistence/preset-manager/cpp/` (module v1.0.5) writes
  `.factory-version` after its work, with the same non-closing check-then-act.
  Fixing it changes behaviour for every plugin depending on the module and needs
  a module version bump plus a rollout to all dependents — out of scope for a
  single-plugin Info sweep.

_Info tier swept: 2026-07-25_
