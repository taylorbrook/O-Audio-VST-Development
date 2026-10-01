# Baseline: O-SimpleReverb 1.14.0

Output of `O-SimpleReverb-render-check --baseline`. Measured through `processBlock` only
(`tests/render-check/measure.h`): 48 kHz, block 512, WET 100 / DRY 0, CHARACTER 0, LOW CUT off,
a unit impulse in both channels after 0.5 s of silence. Left channel unless a column says L / R.

## Decay time and stereo

RT60 is T30 (Schroeder integral, line over -5..-35 dB, extrapolated to 60 dB). Mid = 354-1414 Hz,
8 kHz = 5657-11314 Hz, 125 Hz = 88-177 Hz. Target = v2.0.0's base RT60 x DECAY, at every SIZE.
L/R correlation is zero-lag, full band, from 0.25 to 0.75 x mid RT60 after the first arrival.

| Type | DECAY | SIZE | Target (s) | Mid RT60 (s) | vs target | 8 kHz RT60 (s) | 125 Hz RT60 (s) | L/R correlation | IR length (s) |
|---|---|---|---|---|---|---|---|---|---|
| Booth | 0.5x | 50 | 0.20 | 0.668 | +234 % | 0.350 | 0.681 | +0.522 | 6.0 |
| Booth | 1.0x | 50 | 0.40 | 0.719 | +80 % | 0.405 | 0.707 | +0.537 | 6.0 |
| Booth | 2.0x | 50 | 0.80 | 10.400 | +1200 % | 2.934 | 11.237 | +0.448 | 20.4 |
| Booth | 1.0x | 0 | 0.40 | 0.687 | +72 % | 0.395 | 0.687 | +0.525 | 6.0 |
| Booth | 1.0x | 100 | 0.40 | 0.754 | +88 % | 0.416 | 0.739 | +0.509 | 6.0 |
| Room | 0.5x | 50 | 0.55 | 0.777 | +41 % | 0.386 | 0.799 | +0.052 | 6.0 |
| Room | 1.0x | 50 | 1.10 | 1.038 | -6 % | 0.679 | 1.029 | +0.011 | 6.0 |
| Room | 2.0x | 50 | 2.20 | 10.773 | +390 % | 4.240 | 11.281 | -0.010 | 20.8 |
| Room | 1.0x | 0 | 1.10 | 0.866 | -21 % | 0.591 | 0.860 | +0.030 | 6.0 |
| Room | 1.0x | 100 | 1.10 | 1.278 | +16 % | 0.795 | 1.275 | +0.009 | 6.0 |
| Hall | 0.5x | 50 | 1.50 | 0.967 | -36 % | 0.646 | 0.980 | +0.010 | 6.0 |
| Hall | 1.0x | 50 | 3.00 | 1.734 | -42 % | 1.272 | 1.769 | +0.005 | 6.0 |
| Hall | 2.0x | 50 | 6.00 | 11.049 | +84 % | 5.996 | 11.352 | -0.016 | 21.0 |
| Hall | 1.0x | 0 | 3.00 | 1.137 | -62 % | 0.897 | 1.135 | +0.022 | 6.0 |
| Hall | 1.0x | 100 | 3.00 | 3.494 | +16 % | 2.137 | 3.630 | +0.018 | 6.0 |
| Spring | 0.5x | 50 | 1.25 | 0.750 | -40 % | 0.445 | 0.759 | +0.424 | 6.0 |
| Spring | 1.0x | 50 | 2.50 | 0.904 | -64 % | 0.669 | 0.877 | +0.351 | 6.0 |
| Spring | 2.0x | 50 | 5.00 | 10.848 | +117 % | 5.048 | 11.282 | +0.327 | 20.9 |
| Spring | 1.0x | 0 | 2.50 | 0.804 | -68 % | 0.606 | 0.797 | +0.366 | 6.0 |
| Spring | 1.0x | 100 | 2.50 | 1.025 | -59 % | 0.743 | 0.989 | +0.323 | 6.0 |
| Plate | 0.5x | 50 | 1.25 | 0.867 | -31 % | 0.540 | 0.840 | +0.022 | 6.0 |
| Plate | 1.0x | 50 | 2.50 | 1.279 | -49 % | 0.925 | 1.227 | +0.010 | 6.0 |
| Plate | 2.0x | 50 | 5.00 | 10.935 | +119 % | 5.343 | 11.258 | -0.013 | 21.0 |
| Plate | 1.0x | 0 | 2.50 | 0.984 | -61 % | 0.747 | 0.945 | -0.008 | 6.0 |
| Plate | 1.0x | 100 | 2.50 | 1.803 | -28 % | 1.216 | 1.799 | +0.022 | 6.0 |
| Ambient | 0.5x | 50 | 3.50 | 1.026 | -71 % | 0.849 | 1.006 | +0.000 | 6.0 |
| Ambient | 1.0x | 50 | 7.00 | 2.165 | -69 % | 1.795 | 2.126 | +0.038 | 6.0 |
| Ambient | 2.0x | 50 | 14.00 | 11.166 | -20 % | 7.974 | 11.300 | -0.014 | 21.1 |
| Ambient | 1.0x | 0 | 7.00 | 1.252 | -82 % | 1.114 | 1.223 | +0.006 | 6.0 |
| Ambient | 1.0x | 100 | 7.00 | 6.510 | -7 % | 4.435 | 6.673 | +0.015 | 9.3 |

## Impulse response structure

First arrival = first sample within 40 dB of the IR's peak. Early arrivals = local maxima within
20 dB of the largest sample in the 60 ms after the first arrival (count L / R, and how many of L's
have no R arrival within 0.1 ms). Echo density is Abel & Huang's (1.0 = Gaussian), 20 ms window,
100 ms after the first arrival; mixing time is when it first reads 0.9. -60 dB = the decay curve's
own crossing, no extrapolation.

| Type | DECAY | SIZE | First arrival L / R (ms) | Early arrivals L / R | L without R | Echo density @ 100 ms | Mixing time (ms) | Peak (dBFS) | Energy (dB) | -60 dB at (s) |
|---|---|---|---|---|---|---|---|---|---|---|
| Booth | 0.5x | 50 | 28.29 / 28.29 | 111 / 125 | 52 | 0.92 | 62 | -13.4 | 7.0 | 0.62 |
| Booth | 1.0x | 50 | 28.29 / 28.29 | 110 / 128 | 53 | 0.97 | 58 | -13.4 | 7.2 | 0.66 |
| Booth | 2.0x | 50 | 28.29 / 28.29 | 108 / 124 | 51 | 0.83 | 178 | -13.4 | 13.4 | 9.63 |
| Booth | 1.0x | 0 | 28.29 / 28.29 | 110 / 129 | 53 | 0.97 | 62 | -13.4 | 7.2 | 0.64 |
| Booth | 1.0x | 100 | 28.29 / 28.29 | 110 / 128 | 53 | 0.97 | 58 | -13.4 | 7.3 | 0.69 |
| Room | 0.5x | 50 | 40.29 / 40.81 | 99 / 119 | 60 | 0.92 | 85 | -16.3 | 4.2 | 0.73 |
| Room | 1.0x | 50 | 40.29 / 40.81 | 99 / 113 | 65 | 0.88 | 95 | -16.4 | 5.5 | 0.97 |
| Room | 2.0x | 50 | 40.29 / 40.81 | 98 / 111 | 64 | 0.81 | 318 | -16.5 | 11.8 | 9.88 |
| Room | 1.0x | 0 | 40.29 / 40.81 | 100 / 113 | 66 | 0.89 | 92 | -16.5 | 5.2 | 0.82 |
| Room | 1.0x | 100 | 40.29 / 40.81 | 98 / 112 | 64 | 0.89 | 95 | -16.4 | 6.0 | 1.19 |
| Hall | 0.5x | 50 | 78.06 / 78.58 | 101 / 111 | 66 | 0.92 | 48 | -21.8 | 0.8 | 0.98 |
| Hall | 1.0x | 50 | 78.06 / 78.58 | 98 / 111 | 63 | 0.90 | 65 | -21.4 | 3.0 | 1.72 |
| Hall | 2.0x | 50 | 78.06 / 78.58 | 96 / 111 | 64 | 0.89 | 70 | -21.1 | 9.3 | 10.34 |
| Hall | 1.0x | 0 | 78.06 / 78.58 | 98 / 109 | 65 | 0.89 | 42 | -21.6 | 1.8 | 1.16 |
| Hall | 1.0x | 100 | 78.06 / 78.58 | 97 / 111 | 63 | 0.90 | 62 | -21.3 | 5.0 | 3.34 |
| Spring | 0.5x | 50 | 45.52 / 45.54 | 102 / 111 | 56 | 0.99 | 12 | -23.6 | -0.3 | 0.76 |
| Spring | 1.0x | 50 | 45.52 / 45.54 | 102 / 111 | 57 | 0.97 | 12 | -23.6 | 0.6 | 0.90 |
| Spring | 2.0x | 50 | 45.52 / 45.54 | 102 / 108 | 61 | 1.05 | 12 | -23.6 | 8.7 | 10.46 |
| Spring | 1.0x | 0 | 45.52 / 45.54 | 102 / 110 | 57 | 0.97 | 12 | -23.6 | 0.3 | 0.81 |
| Spring | 1.0x | 100 | 45.52 / 45.54 | 102 / 111 | 58 | 0.99 | 12 | -23.6 | 0.9 | 1.01 |
| Plate | 0.5x | 50 | 33.29 / 33.81 | 97 / 118 | 60 | 0.85 | 130 | -16.6 | 5.5 | 0.78 |
| Plate | 1.0x | 50 | 33.29 / 33.81 | 97 / 115 | 63 | 0.82 | 162 | -16.8 | 7.1 | 1.14 |
| Plate | 2.0x | 50 | 33.29 / 33.81 | 98 / 113 | 66 | 0.79 | 185 | -15.9 | 13.0 | 9.56 |
| Plate | 1.0x | 0 | 33.29 / 33.81 | 97 / 115 | 62 | 0.82 | 172 | -16.8 | 6.5 | 0.89 |
| Plate | 1.0x | 100 | 33.29 / 33.81 | 97 / 114 | 63 | 0.82 | 170 | -16.8 | 7.9 | 1.60 |
| Ambient | 0.5x | 50 | 62.12 / 62.65 | 102 / 108 | 65 | 0.91 | 68 | -22.5 | -0.0 | 1.04 |
| Ambient | 1.0x | 50 | 62.12 / 62.65 | 100 / 110 | 60 | 0.89 | 162 | -22.3 | 2.7 | 2.10 |
| Ambient | 2.0x | 50 | 62.12 / 62.65 | 99 / 110 | 60 | 0.87 | 178 | -22.1 | 8.8 | 10.51 |
| Ambient | 1.0x | 0 | 62.12 / 62.65 | 102 / 110 | 61 | 0.91 | 85 | -22.4 | 0.9 | 1.27 |
| Ambient | 1.0x | 100 | 62.12 / 62.65 | 99 / 111 | 59 | 0.89 | 278 | -22.1 | 6.3 | 6.15 |

## Early arrivals at DECAY 1.0x, SIZE 50

Times in ms after the left channel's first arrival, with the sign of the sample; first eight.

| Type | L | R |
|---|---|---|
| Booth | 0.00+, 0.52+, 1.65+, 2.17+, 3.65+, 4.17+, 5.08-, 5.44+, ... | 0.00+, 0.52+, 1.65+, 2.17+, 3.65+, 4.17+, 5.08-, 5.44+, ... |
| Room | 0.00+, 1.65+, 3.65+, 5.08-, 5.44+, 6.94+, 7.73-, 8.50+, ... | 0.52+, 2.17+, 4.17+, 6.12-, 7.46+, 7.77-, 9.02+, 9.77-, ... |
| Hall | 0.02+, 1.67+, 3.67+, 5.10-, 5.46+, 6.96+, 7.75-, 8.52+, ... | 0.54+, 2.19+, 4.19+, 6.15-, 7.48+, 7.79-, 9.04+, 9.79-, ... |
| Spring | 0.04-, 1.69-, 3.19+, 3.69-, 5.12+, 5.48-, 6.08+, 6.77+, ... | 0.56-, 2.21-, 2.96+, 3.71+, 4.21-, 4.62+, 5.38+, 6.00-, ... |
| Plate | 0.00+, 1.65+, 3.65+, 5.08-, 5.44+, 6.94+, 7.73-, 8.15+, ... | 0.52+, 2.17+, 4.17+, 6.12-, 7.46+, 7.77-, 9.02+, 9.77-, ... |
| Ambient | 0.02+, 1.67+, 3.67+, 5.10-, 5.46+, 6.96+, 7.75-, 8.52+, ... | 0.54+, 2.19+, 4.19+, 6.15-, 7.48+, 7.79-, 9.04+, 9.79-, ... |

## Do two types share resonances?

Pearson correlation of the tail's dB spectrum between types at DECAY 1.0x, SIZE 50: 200-2000 Hz in
0.5 Hz steps, Hann window starting 0.25 x mid RT60 (at least 50 ms) after the first arrival and one
mid RT60 long (0.25..2 s), a 100 Hz moving average removed so that an EQ tilt does not count.
1.0 = the same resonant frequencies, 0 = unrelated.

| | Room | Hall | Spring | Plate | Ambient |
|---|---|---|---|---|---|
| Booth | +0.75 | +0.72 | +0.78 | +0.73 | +0.72 |
| Room | | +0.91 | +0.79 | +0.92 | +0.91 |
| Hall | | | +0.76 | +0.93 | +0.96 |
| Spring | | | | +0.76 | +0.76 |
| Plate | | | | | +0.94 |

## When each band arrives (DECAY 1.0x, SIZE 50)

First sample of a narrow band (f / 1.06 .. f x 1.06) within 20 dB of that band's largest in the
160 ms after the first arrival, in ms, less the same reading of the band filter itself. A
dispersive spring arrives later as frequency rises (a chirp); anything else reads about 0.

| Type | 300 Hz | 600 Hz | 1 kHz | 2 kHz | 3 kHz | 3.8 kHz | 3 kHz - 1 kHz |
|---|---|---|---|---|---|---|---|
| Booth | 7.7 | 2.1 | 0.8 | 0.4 | 0.1 | 0.1 | -0.6 |
| Room | 17.7 | 1.5 | 0.8 | 0.4 | 0.1 | 0.1 | -0.6 |
| Hall | 17.8 | 1.6 | 0.9 | 0.4 | 0.2 | 0.1 | -0.7 |
| Spring | 14.1 | 6.9 | 4.1 | 5.8 | 2.0 | 2.0 | -2.1 |
| Plate | 6.6 | 2.1 | 0.8 | 0.4 | 0.1 | 0.1 | -0.7 |
| Ambient | 17.8 | 2.2 | 0.9 | 0.4 | 0.2 | 0.1 | -0.7 |

## CPU

Wall time inside `processBlock` per second of audio, white noise in, block 512, median of 5 runs of
5 s, at each type's defaults (WET 25 / DRY 100). One core of the machine that ran this; a figure to
compare on the same machine, never a pass/fail.

| Type | 48 kHz | 96 kHz |
|---|---|---|
| Booth | 0.33 % | 0.65 % |
| Room | 0.32 % | 0.66 % |
| Hall | 0.36 % | 0.75 % |
| Spring | 0.43 % | 0.90 % |
| Plate | 0.36 % | 0.74 % |
| Ambient | 0.36 % | 0.75 % |

---

## Reading the baseline

Everything above this line is tool output (commit `02bac73f`, v1.14.0, `Source/` untouched). The
notes below are written by hand and are what stage 1's gates should take from it.

- **DECAY 2.0x gives every type the same tail.** Booth, Room, Hall, Spring, Plate and Ambient all
  read 10.4-11.2 s at DECAY 2.0x, SIZE 50: the room size closes to 1.0 on every type, so the
  Freeverb feedback is the same 0.98 whatever TYPE says. Booth goes from 0.72 s at 1.0x to 10.4 s.
- **SIZE is a decay control.** Mid RT60 moves with SIZE on every type (Hall 1.14 -> 3.49 s, Ambient
  1.25 -> 6.51 s) while the first arrival does not move by a sample. The new "RT60 holds across
  SIZE" and "SIZE moves the structure" gates both fail here, as expected.
- **Mixing time is not a structure measure.** It moves with SIZE in v1.14.0 (Ambient 85 -> 278 ms)
  although no delay changes, because it follows the decay. The "SIZE moves the structure" gate
  must read first-arrival or tap times; on mixing time v1.14.0 would pass it for the wrong reason.
- **The types share their resonances**: 0.72-0.96 between any two (the gate is < 0.3).
- **L/R correlation** is near 0 for Room, Hall, Plate and Ambient already. Only Booth (+0.45..+0.54)
  and Spring (+0.32..+0.42), the two types with width below 1, fail `|r| < 0.3`.
- **No chirp.** Every band arrives together (3 kHz - 1 kHz = -0.6..-0.7 ms; Spring -2.1 ms, low
  bands slightly *later* through its three allpasses). The Spring gate asks for >= +2 ms.
- **Early arrivals** are a dense click train (about 100 within 20 dB in the first 60 ms), the tank's
  eight combs each repeating the input-side taps. Not discrete output taps.
- **HF decay**: the 8 kHz RT60 is 28-89 % of mid (shortest share at DECAY 2.0x, longest on Ambient).
- **CPU** 0.32-0.43 % of a core at 48 kHz, 0.65-0.90 % at 96 kHz.
- **Measurement floor.** On synthetic noise of known T60 the measurer is unbiased (mean within
  1.2 %), but a single realisation scatters 2.8 % at 0.2 s, 2.3 % at 0.4 s, 0.8 % at 3 s. A +/-10 %
  gate on Booth at DECAY 0.5x (0.20 s) has about three standard deviations of room, not ten.
