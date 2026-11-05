# Stage 2 — DSP · Phase 2.4 — Listening checkpoint (run at verify)

**Plugin:** O-Granulator · **Phase:** 2.4 (freeze, feedback network, output reverb) · **Written:** 2026-11-05 (execute T13, PLAN P45)

This is the second and last Stage-2 human listening checkpoint (D3). Execute did not wait on it; verify records a verdict per item.

## Setup (every item)

- Host: any DAW or the Standalone, **generic editor** (Stage 3 has not landed the UI). Install with `./scripts/build-and-install.sh O-Granulator`.
- **Monitors down.** Items 2, 6 and 7 reach resonant / self-sustaining corners. The D9 knee caps the wet path at +6 dBFS, but +6 dBFS is still loud.
- Start every item from **default parameters**, then apply only the listed changes. Stream 1 alone unless the item says otherwise (s1_enable On, s2–s4 Off, which is the default).
- Mix 100 % (wet only) unless stated. Output gain 0 dB.
- Source: a sustained, tonal, mid-level signal (pad, sung vowel, bowed string) at about −18 dBFS, plus a drum loop for transients where noted.

## Items

### 1 — Freeze at spray 0: buzz vs texture (ARCH Algorithm §3; deviations 11–12)

| Parameter | Value |
|---|---|
| s1_position | 0 % |
| s1_spray | 0 % |
| s1_jitter | 0 % |
| s1_size / s1_density | 100 ms / 10 g/s, then 500 ms / 10 g/s |
| freeze | toggle On / Off every few seconds |

Listen for:
- A periodic buzz at the spawn rate (10 Hz) while frozen. The gate measured ρ_H = −0.001 (buzz control 0.99), so this should read as a stable cloud.
- **Engage and release clicks.** Two execute deviations target exactly this:
  - the deferred head-stop: the head stops about 5 ms after the edge, once the crossing grains have faded;
  - the slope-matched release continuation.
  Gates: engage κ 1.19× (the planned form measured 23.8×), release 1.15×.
- Whether the ~5 ms freeze latency is perceptible on a drum loop (expected: no).

Fallbacks (one line each):
- **Wider minimum spray.** In `Spawner.h` `frozenDelay`, change `std::max (spray * span, rl)` to `std::max (spray * span, 2.0 * rl)`.
- **Head-stop vs virtual-head.** This is the ARCH Algorithm §3 fallback, not pre-wired. It is a 2.5 decision if item 1 fails.

### 2 — Feedback character (D12: does saturation read as "quieter"?)

| Parameter | Value |
|---|---|
| s1_position | 5 % (≈ 400 ms loop at the 8 s buffer) |
| s1_size / s1_density | 200 ms / 50 g/s |
| feedback | 50 %, 80 %, 100 % |
| fb_saturation | sweep 0 → 100 % at each feedback setting |
| fb_diffusion | 0 % |

Listen for:
- Tail length at 50 / 80 / 100 %. The R29 bound law (signed off) costs −0.3 … −6 dB of loop gain vs the ARCH law.
- Whether more saturation reads as "quieter" (D12: the ceiling falls from 0 to −18 dB).

Fallbacks (one line each):
- **D12 quieter.** `EngineConstants.h`: `kSatSpan = 7.0f` → `3.0f`.
- **Tails too short.** Use the R29 per-step ripple table: `OutputLaw.h` `fbTapGain` → `1/max (1, S1 + r(step))`. This needs the table built in the bank constructor, so it is a small task, not one line.

### 3 — Diffusion and tone in the loop

| Parameter | Value |
|---|---|
| Item 2 base | feedback 80 % |
| fb_diffusion | sweep 0 → 100 % |
| fb_low_cut | sweep 20 → 2000 Hz |
| fb_high_cut | sweep 1 k → 20 kHz |

Listen for:
- Diffusion smearing echoes into a wash (the FB2d echo-density autocorrelation fell 0.53 → −0.03).
- Zipper or clicks on the cutoff sweeps (20 ms log-domain ramps; coefficients per cell).
- Muddy or thin loops at the extremes.

### 4 — Reverb size / damp range

| Parameter | Value |
|---|---|
| reverb_mix | 30 %, then 100 % |
| reverb_size | sweep 0 → 100 % |
| reverb_damp | sweep 0 → 100 % |
| reverb_mix automation | 30 → 0 → 30 % |

Listen for:
- Usable size range (RT60 0.2 s … 12 s nominal).
- Damp range.
- Mix → 0 fades the tail in about 50 ms (R35: the module has no "let it ring" mode). Is that acceptable?
- Re-enable from 0 starts a cold tank, with no click.

### 5 — R22: crush 100 % silences grains below −12 dBFS

| Parameter | Value |
|---|---|
| s1_crush | 100 % |
| s1_crush_prob | 100 % |
| Source level | −18 dBFS, then −6 dBFS |

Listen for: at −18 dBFS (typical live level), crush 100 % mutes most grains (D7 mid-tread: silence stays 0). Is that a usable extreme or a fault?

Fallback: noted in RESEARCH-2.3 R22. D7 is unchanged unless the ear says otherwise.

### 6 — D10 resonance at res 100 %

| Parameter | Value |
|---|---|
| s1_filter_type | LP, then HP, then BP |
| s1_filter_res | 100 % |
| s1_filter_cutoff | sweep 200 Hz → 8 kHz |

Listen for: characterful vs harsh. D10 halves the LP/HP peak to about +10 dB (it was +20 dB). BP keeps 0 dB (D8).

### 7 — Drive + feedback sustain: feature (drone) or fault? (R30, signed off)

| Parameter | Value |
|---|---|
| Item 2 base | — |
| s1_drive | 100 % |
| feedback | 100 % |
| Source | short notes, then silence |

Listen for:
- Whether the loop self-sustains at the guard level after the input stops. By design it does: FX small-signal gain exceeds 1 in the loop, and the decay gate is scoped to FX gain ≤ 1.
- Repeat with LP res 100 % and with crush 100 % instead of drive.

Fallback (not recommended in RESEARCH §7): divide the tap by the stream's latched FX gain. That restores linear decay but makes driven loops about 12 dB quieter.

## Verdict sheet (filled at verify)

| # | Item | Verdict | Fallback taken |
|---|---|---|---|
| 1 | Freeze buzz / clicks / latency | | |
| 2 | Feedback character, D12 | | |
| 3 | Diffusion / tone | | |
| 4 | Reverb range, mix → 0 | | |
| 5 | R22 crush 100 % | | |
| 6 | D10 resonance | | |
| 7 | Drive + feedback sustain | | |
