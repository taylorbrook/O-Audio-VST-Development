# O-Octagon — monitor fold-down: build record (v1.7.0)

**Status:** ✅ **COMPLETE — shipped as v1.7.0 on 2026-08-26.**

This file began as a handoff written while the integration was blocked, and is kept as the build
record. The eight edits below were all applied; the section headings now describe what is in the
tree, not what is pending.

**Shipped as v1.7.0** — the concurrent i18n work took v1.6.0.

---

## Why this was built in two halves

Mid-task, a second session was found actively editing `PluginEditor.cpp`, `PluginProcessor.cpp/.h`,
`index.html`, `app.js` and `styles.css` — the complete overlap with this feature's integration
surface — with files written three minutes earlier, `VERSION` already moved to 1.6.0, and HEAD
advanced by three commits since session start. Editing them concurrently would have clobbered ~507
uncommitted lines through a shared `.git/index`
(`pattern_shared_checkout_index_race_between_sessions`).

The work was therefore split at the seam with no overlap: **the DSP core, its standalone harness and
this specification were built first**, all in new files. The i18n session then committed
(`8dcb1317`) and moved to O-ReverseDelay, at which point the integration was applied against its
committed work rather than around its uncommitted work.

**The lesson worth keeping:** the split cost nothing. `MonitorFold` depends only on `VenueSnapshot`
and `VenueGeometry`, so it was fully testable — and one real design bug was found and fixed in it —
before a single contended file was touched.

---

## What shipped

| Path | State |
|---|---|
| `Source/DSP/MonitorFold.h` / `.cpp` | The fold. Clean under the production warning gate in Release **and** Debug. |
| `tests/monitor-fold/` | Standalone harness — **15 checks, 0 failures**. Read its README before touching the pan. |
| Probes CY, CZ, DA, DB | In the render harness. **61 probes total, 0 failures.** CZ negative-controlled. |

Measured: ITD 31 samples @48 kHz for a source at 90° (model max 0.6558 ms = 31.5 samples); ILD
mirrors to ±16.9 dB; coherent-8 worst-case fold peak 0.627 against a source peak of 1.0.

---

## The eight edits (all applied)

### 1. `Source/Data/VenueSnapshot.h`

Add beside `speakerToBuffer`:

```cpp
    /** v1.7.0 — the two out[] SLOTS whose buffer channel carries ChannelType left / right.

        SLOTS INTO out[], NOT BUFFER CHANNELS. renderChunk()'s out[i] is already
        buffer.getWritePointer (speakerToBuffer[i]), so the fold writes through the SAME pointers
        every other write in this plugin goes through and adds no second output-indexing site.
        Resolved on the message thread by ochan::resolveMonitorSlots().

        {-1, -1} when the pair cannot be resolved — the monitor is then refused, never guessed. */
    std::array<int, 2> monitorSlot { -1, -1 };
```

`std::array<int,2>` keeps the `is_trivially_copyable_v` static_assert satisfied.

### 2. `Source/DSP/ChannelMap.h` / `.cpp`

```cpp
/** Resolves the monitor pair: the out[] slots carrying ChannelType left and right.

    NOT speakers 1 and 2. The monitor must reach the physical outputs a headphone amp is plugged
    into, which under the measured CoreAudio Emagic 7.1 order (OutputOrder.h) are outputs 1-2 =
    left/right. Speakers 1 and 2 land wherever the venue's label assignment puts them, which for
    any non-default wiring is not the headphone pair at all.

    Returns false and leaves `out` untouched unless BOTH resolve to distinct slots.
*/
bool resolveMonitorSlots (const juce::AudioChannelSet& outSet,
                          const std::array<int, kNumSpeakers>& speakerToBuffer,
                          std::array<int, 2>& out);
```

```cpp
bool ochan::resolveMonitorSlots (const juce::AudioChannelSet& outSet,
                                 const std::array<int, kNumSpeakers>& speakerToBuffer,
                                 std::array<int, 2>& out)
{
    const int bufL = outSet.getChannelIndexForType (juce::AudioChannelSet::left);
    const int bufR = outSet.getChannelIndexForType (juce::AudioChannelSet::right);

    if (bufL < 0 || bufR < 0 || bufL == bufR)
        return false;

    // INVERT speakerToBuffer. It is a permutation of 0..7 (isPermutationOf0to7 guarantees it), so
    // exactly one slot maps to each buffer channel and this loop cannot find two.
    std::array<int, 2> resolved { -1, -1 };

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        if (speakerToBuffer[(std::size_t) i] == bufL) resolved[0] = i;
        if (speakerToBuffer[(std::size_t) i] == bufR) resolved[1] = i;
    }

    if (resolved[0] < 0 || resolved[1] < 0 || resolved[0] == resolved[1])
        return false;

    out = resolved;
    return true;
}
```

### 3. `Source/PluginProcessor.h`

```cpp
    /** v1.7.0 — the monitor fold-down arm. DELIBERATELY NOT AN APVTS PARAMETER AND DELIBERATELY
        NOT PERSISTED — see setMonitorArmed() and getStateInformation(). */
    std::atomic<bool> monitorArmed { false };

    /** Set by the audio thread when the arm was honoured but the fold was SUPPRESSED anyway —
        an offline render. The banner reads it so an operator who bounced with the monitor up is
        told why the bounce is clean rather than left wondering. */
    std::atomic<bool> monitorSuppressed { false };

public:
    bool isMonitorArmed()     const noexcept { return monitorArmed.load (std::memory_order_acquire); }
    bool isMonitorSuppressed() const noexcept { return monitorSuppressed.load (std::memory_order_acquire); }

    /** Message thread. Returns whether the arm took. Refused in SAFE mode and on an unresolved
        monitor pair — the same shape as startVerifyPing()'s precondition. */
    bool setMonitorArmed (bool shouldArm);
```

### 4. `Source/PluginProcessor.cpp`

**`publishSnapshot()`** — before `venuePublisher.publish (snapshot);`:

```cpp
    // v1.7.0. Resolved HERE, on the message thread, beside the speaker map it inverts — the audio
    // thread performs no channel lookup of its own. Left {-1,-1} on failure, which the fold treats
    // as "refuse", never as "guess".
    if (! ochan::resolveMonitorSlots (getBusesLayout().getMainOutputChannelSet(),
                                      speakerToBuffer, snapshot.monitorSlot))
        snapshot.monitorSlot = { -1, -1 };
```

**`setMonitorArmed()`** — new:

```cpp
bool OOctagonProcessor::setMonitorArmed (bool shouldArm)
{
    if (! shouldArm)
    {
        monitorArmed.store (false, std::memory_order_release);
        return true;
    }

    // SAFE mode already outputs a stereo fold; folding a fold is meaningless, and there is no
    // eight-speaker rig to fold FROM. Same precondition shape as startVerifyPing().
    if (! mappedOutputAvailable (getTotalNumOutputChannels()))
        return false;

    if (venuePublisher.read().monitorSlot[0] < 0)
        return false;

    // MUTUALLY EXCLUSIVE WITH THE PING, AND THAT IS NOT TIDINESS. The ping names a PHYSICAL
    // speaker; folding it to headphones answers a question about wiring with a signal that has
    // left the wiring. Whichever the operator asked for last wins.
    verifyPing.abort();

    monitorArmed.store (true, std::memory_order_release);
    return true;
}
```

**`startVerifyPing()`** — add the reciprocal, right before `verifyPing.start (...)`:

```cpp
    monitorArmed.store (false, std::memory_order_release);   // see setMonitorArmed()
```

**`processBlock()`** — after the existing `mapped` / ping-abort block:

```cpp
    // ══ THE PRIMARY CONSTRAINT: THE MONITOR MUST NOT CONTAMINATE A RENDER ═════════════════════
    //
    // isNonRealtime() is set by the wrapper for an offline bounce (Logic's Bounce and Bounce in
    // Place). The fold is bypassed STRUCTURALLY there — not attenuated, not faded: never engaged,
    // so MonitorFold::isRunning() stays false and not one sample is clocked. Probe CZ asserts the
    // offline render is BIT-IDENTICAL to a never-armed one.
    //
    // THIS IS THE THIRD OF FOUR GUARDS AND NOT THE STRONGEST. It does not fire for a REALTIME
    // bounce. The guard that covers that is that `monitorArmed` is neither a parameter nor
    // persisted, so a session cannot come back armed — see getStateInformation().
    const bool monitorOn = monitorArmed.load (std::memory_order_acquire)
                        && mapped
                        && ! isNonRealtime();

    monitorSuppressed.store (monitorArmed.load (std::memory_order_acquire) && ! monitorOn,
                             std::memory_order_release);

    gainStage.process (buffer, numIn, numOut, mapped, snapshot, snapshotParameters(),
                       &verifyPing, monitorOn);
```

> The meter loop below it needs **no** change: it already meters the written buffer post-ping, so
> in monitor mode it correctly shows six silent lanes. That is the fold being visible, exactly as
> N8's fold is.

**`processBlockBypassed()`** — beside the existing `verifyPing.abort()`:

```cpp
    monitorArmed.store (false, std::memory_order_release);   // D11's rule, extended to the monitor
```

**`getStateInformation()`** — add the comment, and **no code**:

```cpp
    // ── monitorArmed IS DELIBERATELY ABSENT FROM THIS FUNCTION ────────────────────────────────
    //
    // Guard 2 of 4, and the one that covers the case isNonRealtime() cannot: a REALTIME bounce.
    // If the arm were persisted here — as tooltipsEnabled legitimately is — a session could reopen
    // armed and a realtime bounce would carry a headphone fold into the delivered file with six
    // channels silent. Not persisting makes that state unreachable across a reload.
    //
    // IF YOU ARE ADDING PERSISTENCE HERE BECAUSE RE-ARMING IS ANNOYING, YOU ARE REMOVING THE
    // FEATURE'S PRIMARY SAFETY PROPERTY. Every competitor treats monitor mode as transport state
    // for the same reason.
```

### 5. `Source/DSP/GainStage.h` / `.cpp`

- `#include "MonitorFold.h"`, add member `MonitorFold monitorFold;`
- `prepare()` step 2: `monitorFold.prepare (sampleRateToUse, snapshot);`
- `process()` signature gains a trailing `bool monitorOn = false`, so every existing harness call
  site compiles unchanged — the `VerifyPing* ping = nullptr` precedent.
- In `process()`'s control-boundary branch:

```cpp
        if (phase == 0)
        {
            updateControl (snapshot, p);

            // v1.7.0. OUTSIDE updateControl() so that function's signature and its dirty check are
            // untouched: the fold is a function of the ROOM, and its own generation gate is the
            // right place for that, not the parameter memcmp.
            monitorFold.updateGeometry (snapshot);
            monitorFold.setEngaged (monitorOn && snapshot.monitorSlot[0] >= 0);
        }
```

- `renderChunk()`, REAL arm, as the **last** statement after the ping overwrite:

```cpp
        // ── v1.7.0 — THE MONITOR FOLD, LAST ───────────────────────────────────────────────────
        //
        // After the ping so its six-lane mute is authoritative (they are mutually exclusive
        // upstream; this is the backstop, not the gate). isRunning() false is the STRUCTURAL
        // BYPASS the bit-identity claim rests on — nothing is clocked, exactly as delayEngaged
        // false clocks no delay line.
        if (monitorFold.isRunning())
            monitorFold.fold (out, snapshot.monitorSlot[0], snapshot.monitorSlot[1], start, count);
```

### 6. `Source/PluginEditor.cpp`

Two native functions, plus three properties on `getStatus`:

```cpp
    obj->setProperty ("monitorArmed",      processorRef.isMonitorArmed());
    obj->setProperty ("monitorSuppressed", processorRef.isMonitorSuppressed());
    obj->setProperty ("monitorAvailable",  ! processorRef.isSafeMode()
                                        && ! processorRef.isChannelMapInvalid());
```

```cpp
    options = options.withNativeFunction ("setMonitorArmed",
        [this] (auto& args, auto complete)
        {
            const bool ok = processorRef.setMonitorArmed (args.size() > 0 && (bool) args[0]);
            complete (juce::var (ok && processorRef.isMonitorArmed()));
        });

    options = options.withNativeFunction ("getMonitorArmed",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.isMonitorArmed()));
        });
```

> `getMonitorArmed` is polled, **not** pushed once at init:
> `pattern_webview_one_shot_state_push_stale_on_preset_load`. The arm can be cleared by the
> processor without the page asking — bypass, a SAFE-mode flip, a ping start — so a one-shot read
> goes stale within seconds.

### 7. UI — `index.html`, `styles.css`, `app.js`

`.monitor-banner` is `.safe-banner`'s sibling, frame-level (D13), authored copy never rewritten,
JS toggles `[hidden]`. Suggested markup:

```html
      <div class="monitor-banner" id="monitor-banner" role="status" hidden
           data-tip-title="Monitor fold-down"
           data-tip="Headphone fold of the eight solved feeds into outputs 1-2, other six muted. Never included in an offline bounce, and never remembered across a reload.">
        <span class="monitor-tag">MONITOR</span>
        <span class="monitor-copy" id="monitor-copy">Headphone fold &#8212; rig outputs muted</span>
      </div>
```

Use `--alert` as SAFE and MAP do. The banner must be **visually loud** — it is the only defence
against a realtime bounce.

When `monitorSuppressed` is true the copy becomes
`"Suppressed for offline render &#8212; bounce is clean"`, which is the reassuring half.

### 8. `CMakeLists.txt`

`VERSION 1.7.0`, and add to `target_sources`:

```cmake
        Source/DSP/MonitorFold.cpp
```

---

## Probes (appended — CY, CZ, DA, DB)

Next free ids are **CY, CZ, DA, DB**. (`DC` is deliberately skipped — the harness already uses the
string `"DC"` for a DC-input signal name and the collision reads badly in output.)

```cpp
    //==========================================================================
    // CY — THE STRUCTURAL BYPASS. A session that never arms the monitor clocks NOTHING.
    //
    // This is the bit-identity claim's mechanism, asserted directly: fold() is the only writer,
    // isRunning() gates every call to it, and monitorSamples counts every sample it processes. A
    // zero here means the v1.5.0 signal path was reached literally, not approximately.
    //
    // CROSS-VERSION IDENTITY IS A DIFFERENT TEST AND IS NOT THIS ONE — that is the v1.5.0 backup
    // comparison in the regression phase (pattern_reanchor_cross_version_digest_probe). Saying so
    // here stops this probe from being read as more than it is.
    {
        constexpr int total = 4096 * 4;

        OOctagonProcessor proc;
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        setParam (proc, "srcX", 0.30f);
        setParam (proc, "srcY", 0.70f);

        juce::AudioBuffer<float> out (8, total);

        oo::instr::resetMonitorCounter();
        renderInto (proc, out, total, { 1, 7, 64, 333, 4096 }, {});

        const bool clean = oo::instr::monitorSamples.load() == 0;
        const bool live  = out.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("CY monitor-disarmed-is-structural-bypass", clean && live,
               juce::String ("monitorSamples = ")
                   + juce::String ((juce::int64) oo::instr::monitorSamples.load())
                   + (clean ? " (the fold never ran)" : " — THE FOLD CLOCKED WHILE DISARMED")
                   + (live ? "" : "; SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // CZ — THE PRIMARY CONSTRAINT. AN OFFLINE RENDER IS BIT-IDENTICAL TO A NEVER-ARMED ONE.
    //
    // "Must not silently contaminate a render" is the requirement this whole feature was shaped
    // around, and this is the probe that holds it. Three processors, identical material:
    //
    //   a — never armed                          (the reference)
    //   b — ARMED, setNonRealtime (true)         (must equal a, BIT FOR BIT)
    //   c — ARMED, realtime                      (must DIFFER from a, or b's match is vacuous)
    //
    // c is not decoration. Without it a fold that was broken, silent, or never wired would pass
    // this probe perfectly (pattern_probe_must_target_the_branch_the_fix_changed).
    {
        constexpr int total = 4096 * 4;

        auto build = [&] (OOctagonProcessor& p)
        {
            negotiate (p, mono, set71);
            applyRotatedLabels (p);
            setParam (p, "srcX", 0.22f);
            setParam (p, "srcY", 0.63f);
        };

        OOctagonProcessor a, b, c;
        build (a); build (b); build (c);

        b.setNonRealtime (true);

        const bool armedB = b.setMonitorArmed (true);
        const bool armedC = c.setMonitorArmed (true);

        juce::AudioBuffer<float> outA (8, total), outB (8, total), outC (8, total);

        renderInto (a, outA, total, { 4096 }, {});
        renderInto (b, outB, total, { 4096 }, {});
        renderInto (c, outC, total, { 4096 }, {});

        const bool offlineClean = bitIdentical (outA, outB);
        const bool realtimeFolds = ! bitIdentical (outA, outC);

        check ("CZ monitor-cannot-contaminate-offline-render",
               offlineClean && realtimeFolds && armedB && armedC,
               juce::String (offlineClean
                                 ? "armed + isNonRealtime: bit-identical to never-armed"
                                 : juce::String ("OFFLINE RENDER WAS CONTAMINATED — ")
                                       + firstDifference (outA, outB))
                   + (realtimeFolds ? "; realtime arm DOES fold (probe can fail)"
                                    : "; REALTIME ARM CHANGED NOTHING — probe vacuous")
                   + (armedB && armedC ? "" : "; AN ARM WAS REFUSED — probe vacuous"));
    }

    //==========================================================================
    // DA — TWO LANES CARRY, SIX ARE HARD ZERO, AND THE TWO ARE left/right.
    //
    // ON A NON-IDENTITY MAP, which is the entire point: applyRotatedLabels puts "L" on speaker 8
    // and "R" on speaker 1, so the monitor pair resolves to out[] slots 7 and 0 and lands in the
    // BUFFER channels left/right occupy. A probe on the default map would be byte-identical to one
    // asserting "channels 0 and 1" and would test nothing (the C1 argument, reused).
    {
        constexpr int total = 4096 * 3;

        OOctagonProcessor proc;
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        setParam (proc, "srcX", 0.35f);

        const bool armed = proc.setMonitorArmed (true);

        juce::AudioBuffer<float> out (8, total);
        renderInto (proc, out, total, { 4096 }, {});

        const auto set = proc.getBusesLayout().getMainOutputChannelSet();
        const int  bufL = set.getChannelIndexForType (juce::AudioChannelSet::left);
        const int  bufR = set.getChannelIndexForType (juce::AudioChannelSet::right);

        // Measure only the settled tail — the first 5 ms is the engage crossfade, during which the
        // six rig lanes are legitimately still fading and are NOT yet zero.
        const int from = total / 2;

        int    sounding = 0;
        float  worstSilent = 0.0f;

        for (int ch = 0; ch < 8; ++ch)
        {
            const float mag = out.getMagnitude (ch, from, total - from);

            if (ch == bufL || ch == bufR) { if (mag > 1.0e-4f) ++sounding; }
            else                            worstSilent = juce::jmax (worstSilent, mag);
        }

        check ("DA monitor-writes-left-right-mutes-six",
               armed && sounding == 2 && worstSilent == 0.0f,
               juce::String ("monitor pair = buffer ch ") + juce::String (bufL) + "/"
                   + juce::String (bufR) + " (out[] slots 7/0 under the rotated map); "
                   + juce::String (sounding) + " of 2 sounding; worst rig-lane magnitude "
                   + juce::String (worstSilent, 9)
                   + (armed ? "" : " — ARM REFUSED, probe vacuous"));
    }

    //==========================================================================
    // DB — THE FOLD IS LIVE, POSITION-DEPENDENT, AND WITHIN A PLAUSIBLE ILD BAND.
    //
    // The band is both rails on purpose. A ">" comparison passed at an ILD of 108 dB during
    // development, which was the far ear being SILENT — an uncompressed constant-power pan had
    // driven it to zero and taken the ITD and the head shadow out of the signal path with it. The
    // near ear looked perfect throughout. Asserting a BAND is what makes that failure visible, and
    // asserting the MIRROR is what catches a flipped atan2 convention.
    {
        auto ildDbForSource = [&] (float srcX)
        {
            constexpr int total = 4096 * 4;

            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);
            setParam (proc, "srcX", srcX);
            setParam (proc, "srcY", 0.5f);
            proc.setMonitorArmed (true);

            juce::AudioBuffer<float> out (8, total);
            renderInto (proc, out, total, { 4096 }, {});

            const auto set = proc.getBusesLayout().getMainOutputChannelSet();
            const int  bufL = set.getChannelIndexForType (juce::AudioChannelSet::left);
            const int  bufR = set.getChannelIndexForType (juce::AudioChannelSet::right);

            const int from = total / 2;
            const double eL = juce::jmax (1.0e-12f, out.getRMSLevel (bufL, from, total - from));
            const double eR = juce::jmax (1.0e-12f, out.getRMSLevel (bufR, from, total - from));

            return 20.0 * std::log10 (eR / eL);
        };

        const double right = ildDbForSource (0.95f);
        const double left  = ildDbForSource (0.05f);

        const bool banded   = right > 3.0 && right < 25.0 && left < -3.0 && left > -25.0;
        const bool mirrored = std::fabs (right + left) < 4.0;

        check ("DB monitor-fold-is-position-dependent", banded && mirrored,
               juce::String ("srcX 0.95 -> ") + juce::String (right, 1)
                   + " dB, srcX 0.05 -> " + juce::String (left, 1)
                   + " dB (want +/-3..25, mirrored within 4 dB)"
                   + (banded   ? "" : " — OUT OF BAND: 0 dB means the fold is wired to nothing, "
                                      ">25 dB means the far ear is silent")
                   + (mirrored ? "" : " — NOT MIRRORED: the azimuth convention is flipped"));
    }
```

---

## Completion record

- [x] Rebased onto the i18n session's commits; 4-way version check re-run
- [x] Edits 1–8 applied
- [x] `instr::monitorSamples` moved into `DbapSolver.h` and folded into `instr::resetCounters()`;
      the `MonitorFold.h` stopgap deleted and probe CY switched to `resetCounters()` / `get()`
- [x] Probes CY, CZ, DA, DB appended — **and CZ confirmed able to fail:** with guard 3 removed it
      reports "OFFLINE RENDER WAS CONTAMINATED" while CY/DA/DB stay green
      (`pattern_probe_must_target_the_branch_the_fix_changed`)
- [x] `ui_frontend_check.js` §21's derived module list — no new page module was needed; the two
      new receivers and the native-fn count were reviewed into their whitelists with rationale
- [x] Build, `auval` PASS, pluginval strictness 10 SUCCESS
- [x] CHANGELOG, NOTES.md, PLUGINS.md, FEATURE-REVIEW.md, parameter-spec.md

### Still outstanding — human gates only

- [ ] **Hall / headphone listen.** The only test for whether the fold actually *images*. Every
      automated gate here measures level, timing and silence; none of them can hear.
- [ ] **Manual:** arm the monitor, bounce offline, confirm the file has eight live rig channels.
      Probe CZ asserts this, but on the harness's own render path rather than through Logic.
- [ ] **Manual:** arm the monitor, save and reload the session, confirm it returns **disarmed**.

### Two additions made during integration that this spec did not originally have

1. **A fifth guard: the editor destructor disarms.** The banner is the sole defence against a
   realtime bounce, and a closed window has no banner — an armed monitor behind a closed editor is
   the one configuration where the fold runs with every warning switched off. The invariant is now
   *the fold cannot outlive its own warning*.
2. **`getMonitorArmed` was specified and then removed.** `getStatus` already carries `monitorArmed`
   on the poll the banner needs, so a second reader was a native function nothing called — a dead
   bridge, which §3 of `ui_frontend_check.js` caught (`pattern_webview_native_fn_bridge_gap`).

## Two things deliberately NOT done

1. **No HRTF convolution.** Scoped out as milestone-sized: it needs an embedded HRIR dataset
   (licence, megabytes of binary data, the `juce_add_binary_data` hyphen-stripping trap) and 16
   partitioned convolvers. The geometric model plus head shadow is what ships.
2. **No `setLatencySamples()` for the ITD lines.** Reporting the ~0.66 ms would move host PDC and
   therefore change the render — the exact contamination this feature exists to be incapable of.
   `MonitorFold.h` says this at length so a future reader does not "fix" it.
