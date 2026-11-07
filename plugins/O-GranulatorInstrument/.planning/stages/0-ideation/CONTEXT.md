# O-GranulatorInstrument — Stage 0 Context (Discuss/Research Findings)

**Date:** 2026-11-07
**Phase:** Stage 0 Research & Planning (research-planning-agent)
**Tier / depth:** 6 / DEEP · **Complexity:** 5.0 (raw 27.0) · **Strategy:** phased (Stage 2 × 6, Stage 3 × 3)
**Parent:** O-Granulator @ 3043b72a (Stage 3, 3.3 planned)

## Decisions (the 7 open questions)

| # | Decision | One-line rationale |
|---|----------|--------------------|
| D1 | **Fork the leaves now; converge into grain-engine 1.2.0 after O-Granulator ships.** The fork happens at Phase 2.1 start, pinned to the commit that closes 3.3. It uses FORK-MANIFEST.md (sha256 per file), namespace `ogi`, and a drift check at every verify with a hunk-by-hunk review. | The parent Engine is ring/loop/analysis-shaped and is under "audio must not move" in 3.3. Its leaves are stable and reusable verbatim. |
| D2 | **30 s per slot, with a 2.88 M-frame cap (15 s at 192 k).** The working tape is a session-rate MipTape<4> (×1.9375). The pristine copy is native-rate, level 0. Allocation is lazy, per loaded length. Four full slots take ≈ 135 MB at 48 k and ≈ 225 MB at 96 k. | Writes stay 1:1 with output samples, and Reset / SR change need no disk access. |
| D3 | **`grain_limit` 32/64/128/256, default 256; 512 dropped.** Budget: worst ≤ 40 %, typical ≤ 12 % of one core at 48 k/128. Degradation is deterministic: onset **thinning** to the water-filled (voice, stream) quotas in steady state, and steal-with-fade only at transitions. Cap lowering converges. | Measured parent cost is 17.29 % at 128 grains, which projects to ≈ 34 % at 256 and ≈ 62 % at 512. Steal-oldest under 16-voice overload would truncate every grain and exceed any spare budget. |
| D4 | **Per-voice source → global destination = newest sounding voice.** Held when silent, with a 20 ms handover. The UI marks these routes "mono". | Deterministic and stable; the "last key" convention. Loudest flips; disallowing removes useful routes. |
| D5 | **Dorico note expression in v1: yes.** Implemented with MPE in Phase 2.5, DAW-verified in Stage 4. The NE `VST3Extensions` subclass also sets `getPluginHasMainInput() = false`. | The module is proven in 10+ plugins and the patch marker is present. Per-grain pitch is latched at spawn, so a cents offset is one add. |
| D6 | **Record-in availability depends on signal presence, not bus state.** With no bus, arming is inert and the UI says "unavailable". A capture with peak < −90 dBFS is discarded and the UI says "no sidechain signal". The output is always cleared, so sidechain audio never leaks. | An AU host hands an unrouted sidechain silence (O-Comp 1.10.0 lesson). |
| D7 | **1× record head per slot in the loop region.** The write is convex, `wc ← (1−λ)wc + λ·r[n−32]` with λ = 0.95·feedback. Write-before-read per 32-grid piece. The tap is the amp-weighted mean over voices. There are 4 per-stream chains. Freeze = heads stop (chains warm). Reset = off-thread rebuild + 20 ms dual-read swap. Feedback 0 = no writes. | A convex write of bounded signals cannot grow for any voice count. It matches "the copy drifts, Reset restores". |

## Other decisions
- **Engine shape:** the parent's absolute 32-sample pass engine, plus a voice dimension. MIDI events are applied at their offsets inside pieces. RNG draws are keyed by note serial (not voice slot).
- **Voices:** 20 slots (≤ 16 counted + 4 steal spares). A custom allocator (WtSynthesiser policy port) with no JUCE Synthesiser or MPE classes, because both allocate on the audio thread in 8.0.15. Custom exponential ADSRs with 2 ms floors.
- **Pitch:** final clamp ±48 st (MipTape L = 4). The key is never quantised; `sN_scale` quantises the spread from unison only.
- **Slots:** off-thread decode, offline Kaiser sinc resample, an audio-owned `pending` exchange, and an SPSC retire ring (audio is the sole producer). This avoids the generation-reaper idle-host freeze by construction. `SlotInfo` sits under a lock that only off-audio threads take.
- **Spawner:** the region fit (no crossing of the loop seam), plus a record-head clamp in modular head-relative coordinates while a head writes.
- **Factory sources:** embedded BinaryData FLAC (`factory:<id>`), not a content folder.
- **State:** `<Slots>` holds the source scheme, hash, an opt-in FLAC-24 embed (≤ 16 MB), resolve order and relink. Working copies are not saved.

## Constraints carried forward (suite memory)
- Draw-before-obtain.
- Serial-keyed CounterRng streams.
- No block-relative scheduling.
- Double read positions.
- Nyquist-clamped filters.
- No `IIR::Filter` default ctor.
- Move-to-spare-slot steals.
- Cap-lowered-live convergence gate.
- Density norm with N_eff (full windows under thinning).
- int64 seed.
- No nested param writes.
- `setLatencySamples` only in `prepareToPlay`.
- Fork drift: diff every hunk.

## Design Sync for the mockup / parameter-spec.md
Parameters stay at 261 with no IDs added or removed:
- S1: `grain_limit` 4 entries.
- S2: `pitch_bend_range` covers MPE master only; members use RPN 0 (48).
- S3: `feedback` = the convex write amount.
- S6: ±48 clamp.
- S7: newest-voice rule.
- S9: `<Slots>` state keys.
- S10: factory sources embedded.
- S11: Embed action.

See ARCHITECTURE.md "Design Sync".

## Open items (blocking Stage 1)
- Mockup (lanes, voice section, Record-in states, MOD instrument sources, mono badges) → LOCKED parameter-spec.md, with the 126 destinations and 18 sources frozen.
- Phase 2.1 also waits for O-Granulator 3.3 to close (fork pin).
