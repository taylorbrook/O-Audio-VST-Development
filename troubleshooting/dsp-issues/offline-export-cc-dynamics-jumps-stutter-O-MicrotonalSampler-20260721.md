---
plugin: O-MicrotonalSampler
date: 2026-07-21
problem_type: dsp_issue
component: juce_processor
symptoms:
  - "Dorico offline audio export has sudden jumps in dynamic level; real-time playback of the same passage is correct"
  - "After first fix (v1.23.8): export hairpins show constant stuttering (rapid sawtooth) of the dynamic level instead of jumps — still export-only"
  - "Notes starting mid-hairpin begin at the wrong dynamic and snap mid-note (export only)"
root_cause: event_timing
juce_version: 8.0.14
resolution_type: code_fix
severity: critical
tags: [offline-render, audio-export, dorico, cc11, expression, asyncupdater, message-thread, dynamics-crossfade, thread-pacing]
---

# Troubleshooting: Offline-Export-Only Dynamic Jumps / Stutter from AsyncUpdater-Mediated CC Dynamics

## Problem

CC11-driven dynamics (equal-power layer crossfade + dB-linear `dynamic_range` gain) sounded perfect in Dorico real-time playback but produced sudden dynamic-level jumps in offline audio export. The first fix converted the jumps into a per-block stutter (sawtooth) during hairpins — again export-only. Both defects were thread-pacing races between the render thread and the message thread; no `isNonRealtime()` branch was involved.

## Environment

- Plugin: O-MicrotonalSampler (v1.23.3–v1.23.7 affected; fixed v1.23.8 + v1.23.9)
- JUCE Version: 8.0.14
- Affected: `PluginProcessor.cpp` processBlock CC11 scan / `handleAsyncUpdate`, `MicrotonalSamplerVoice.cpp` startNote seed + per-block smoother target
- Date: 2026-07-21 (both fixes confirmed by user in Dorico export)

## Symptoms

- Exported audio (Dorico "Export Audio") had abrupt dynamic-level steps a whole hairpin in size; the identical passage played back live was smooth.
- v1.23.8 interim state: export hairpins stuttered continuously (per-block sawtooth between adjacent dynamic levels); live playback still clean.
- auval, pluginval strictness-10, and the offline render harness all pass in every affected version — none of them drive a CC stream against a wall-clock-starved message thread, so this class is invisible to the standard gates.

## What Didn't Work

**Attempted Solution 1 (v1.23.8, partial):** Audio-thread `liveExpression` atomic written by the processBlock CC11 scan, with an epsilon-guarded reconciliation branch that adopted the `expression` APVTS value on CC-quiet blocks (to keep UI-knob/host-automation moves working).
- **Why it failed:** the epsilon only recognises the async forward's echo AFTER it lands. While a forward is in flight the param is stale by the whole message-thread lag — offline, that lag spans entire hairpins. Every CC-quiet block adopted the stale param, then the next CC block jumped forward again: a per-block sawtooth ("constant stuttering"), export-only because in real time the lag is sub-epsilon.

## Solution

Two-part fix, both required:

**v1.23.8 — audio path must not depend on the message-thread round-trip.** processBlock stores CC11 into an audio-thread atomic BEFORE staging the async host forward; voices and the Velocity-mode post-mix smoother read that atom (via `currentExpression()`), not the APVTS atom. `handleAsyncUpdate` stays as the UI/host-automation mirror only.

**v1.23.9 — gate param adoption on the pending slot being drained.**

```cpp
// PluginProcessor.cpp — processBlock (after the CC11 scan)
if (latestCC11 >= 0)
{
    const float ccVal = (float) latestCC11 / 127.0f;
    liveExpression.store (ccVal, std::memory_order_relaxed);   // audio path, FIRST
    lastForwardedExpression = ccVal;
    pendingCC11Value.store (latestCC11, std::memory_order_relaxed);
    triggerAsyncUpdate();                                       // UI/host mirror only
}
else if (pExpression != nullptr
         && pendingCC11Value.load (std::memory_order_relaxed) < 0) // LOAD-BEARING gate
{
    // No CC this block AND no forward in flight: param is trustworthy,
    // adopt genuine knob/automation moves (epsilon vs our own echo).
    const float paramVal = pExpression->load();
    if (std::abs (paramVal - lastForwardedExpression) > (0.5f / 127.0f))
    {
        liveExpression.store (paramVal, std::memory_order_relaxed);
        lastForwardedExpression = paramVal;
    }
}
```

```cpp
// handleAsyncUpdate — clear pending only AFTER the param write
// (was: exchange(-1) first, which opened a stale window for the gate above)
const int v = pendingCC11Value.load (std::memory_order_acquire);
if (v >= 0)
{
    if (auto* ep = parameters.getParameter ("expression"))
        ep->setValueNotifyingHost ((float) v / 127.0f);

    int expected = v;   // CAS keeps a NEWER value staged mid-dispatch;
    pendingCC11Value.compare_exchange_strong (expected, -1,
                                              std::memory_order_acq_rel);
}
```

Voice side (v1.23.8): `startNote` seeds `dynamicsSmoother` and the per-block target from `currentExpression()` (live atom with APVTS fallback for bare test voices) instead of the lagged param.

## Why This Works

1. **Root cause (jumps):** the v1.12.1 RT-safety staging (processBlock → pending atomic → `triggerAsyncUpdate` → `setValueNotifyingHost`) made the message thread the ONLY writer of the audio path's dynamics input. `AsyncUpdater` is wall-clock-paced and coalescing; offline render outruns it, so N CC steps collapse into one late param write and the voice traverses the whole delta (crossfade + up to 40 dB gain) in one 20 ms ramp.
2. **Root cause (stutter):** reconciling against the param without knowing whether a forward was in flight treated the *lagging* forward as a user move. The pending slot is exactly that in-flight knowledge — but only if it is cleared AFTER the param write (load + CAS, not exchange-first), so `pending == -1` ⇒ param reflects the last forward.
3. The audio thread is now self-sufficient (relaxed atomic ops only, RT-safe); the message thread is demoted to a visibility mirror whose timing can no longer affect sound.

## Prevention

- Any plugin that forwards a MIDI CC to a parameter via AsyncUpdater AND consumes that parameter in the render path has this bug latently. Grep for the trio `pendingCC..Value` / `triggerAsyncUpdate` / `getRawParameterValue` feeding a smoother; audit all Ouaricon plugins that copied the O-MicrotonalSampler CC staging.
- Rule: the audio thread must never wait on a message-thread round-trip for a value it produced itself. Stage for the host/UI, but apply locally first.
- When reconciling a self-forwarded parameter, an epsilon-vs-last-forwarded check is NOT sufficient — you must also know no forward is in flight, and the in-flight flag must clear only after the write it tracks.
- Test gap: none of auval / pluginval-10 / the offline harness exercise "render thread far ahead of message thread with a live CC stream". A Dorico (or any DAW) offline export of a hairpin passage is the only current repro — do that manually after touching any CC→dynamics path.
- Memory pattern: `pattern_offline_render_asyncupdater_dynamics_gap` (auto-memory) mirrors this doc.

## Related Issues

- See also: [stale-asyncupdate-stomps-restored-state-O-simplePhysicalModelSynth-20260716.md](../parameter-issues/stale-asyncupdate-stomps-restored-state-O-simplePhysicalModelSynth-20260716.md) — same underlying hazard class (AsyncUpdater timing vs. authoritative value), different failure mode (deferred update stomping restored state vs. lagged delivery starving the audio path).
