---
plugin: O-Octagon
stage: ideation
status: creative_brief_complete
last_updated: 2026-08-10
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief and requirements are finalized for O-Octagon. Ready to proceed to Stage 0 planning or UI mockup.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (Logic-native 8-channel DBAP spatializer for an irregular, non-flat concert array)
- Architecture inherited as locked constraints from `research/logic-pro-multichannel-octaphonic-dbap.md`
- 18 musical parameters + 42 venue values specified
- Preset strategy settled (two separate stores)
- Signal flow, UI concept, use cases captured
- 30 requirements extracted with acceptance criteria
- v1.1+ deferrals recorded explicitly

## Next Steps

1. Stage 0 planning — DSP architecture and roadmap (`/plan O-Octagon`)
2. UI mockup — two-screen design, Room + Venue (`/start O-Octagon` → option 3)
3. Measure Roy Barnett Recital Hall (8 × x/y/z metres + front/rear ear heights)

## Context to Preserve

**Key Decisions (ideation):**
- One source puck + stereo Width (L/R as two sub-points, spread ⊥ to bearing from room centre, metres)
- Motion is host automation only — no internal path generator, no gesture record in v1.0
- Outside the hull → gain attenuation + air-absorption LPF, both defeatable; internal reverb deferred
- v1.0 extras = per-speaker trim only. Binaural fold-down, VBAP A/B, and the Quad variant all deferred to v1.1+
- Two preset stores: venue (geometry/trims/label map) never written by a musical preset
- Weights are 8 automatable APVTS floats plus scene buttons that write all 8 at once
- Rake defines source Z — `srcZ = 0` rides the sloped audience plane, not the stage floor
- `srcX`/`srcY` stored normalised to the venue bounding box so musical presets stay venue-portable

**Locked by prior research — do NOT re-litigate:**
- mono/stereo in → `AudioChannelSet::create7point1()` out; 7.1 is only an 8-channel carrier
- Never `octagonal()` or `discreteChannels(8)` — Logic ignores both
- Not multi-output (Logic multi-out is `aumu` only; `aufx` gets one bus)
- Build the speaker→buffer map ONCE in `prepareToPlay()` via `getChannelIndexForType()` — a wrong map is SILENT
- DBAP per the **2011-04-14 revised** equations (original eqs 3-6 and 9-10 are wrong)

**Open for Stage 0 (non-architectural):**
1. Stereo-track fallback policy in `isBusesLayoutSupported()`
2. Verify-ping signal, level ceiling, dwell, cycle rate
3. Numeric defaults — blur cap, hull attenuation and air-LPF curves, meter ballistics
4. Default venue scale in metres for the traced layout
5. Venue data storage — separate `ValueTree` vs 42 APVTS parameters

**Related work in repo:**
- `research/logic-pro-multichannel-octaphonic-dbap.md` — the locked architecture
- `research/spatial-audio-per-grain-spatialization.md §1` — VBAP math, for the deferred A/B mode
- `plugins/O-Orbit` (v1.0.0, installed) — sibling VBAP orbiter; motion vocabulary for a future version

**Files Created:**
- plugins/O-Octagon/.planning/BRIEF.md
- plugins/O-Octagon/.planning/REQUIREMENTS.md
- plugins/O-Octagon/.planning/STATUS.md
