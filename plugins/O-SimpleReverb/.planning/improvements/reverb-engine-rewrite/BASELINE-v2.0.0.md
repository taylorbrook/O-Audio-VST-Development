# Baseline: O-SimpleReverb 2.0.0

Output of `O-SimpleReverb-render-check --baseline`. Measured through `processBlock` only
(`tests/render-check/measure.h`): 48 kHz, block 512, WET 100 / DRY 0, CHARACTER 0, LOW CUT off,
a unit impulse in both channels after 0.5 s of silence. Left channel unless a column says L / R.

## Decay time and stereo

RT60 is T30 (Schroeder integral, line over -5..-35 dB, extrapolated to 60 dB). Mid = 354-1414 Hz,
8 kHz = 5657-11314 Hz, 125 Hz = 88-177 Hz. Target = v2.0.0's base RT60 x DECAY, at every SIZE.
L/R correlation is zero-lag, full band, from 0.25 to 0.75 x mid RT60 after the first arrival.

| Type | DECAY | SIZE | Target (s) | Mid RT60 (s) | vs target | 8 kHz RT60 (s) | 125 Hz RT60 (s) | L/R correlation | IR length (s) |
|---|---|---|---|---|---|---|---|---|---|
| Booth | 0.5x | 50 | 0.20 | 0.197 | -2 % | 0.151 | 0.210 | -0.086 | 6.0 |
| Booth | 1.0x | 50 | 0.40 | 0.397 | -1 % | 0.302 | 0.413 | -0.098 | 6.0 |
| Booth | 2.0x | 50 | 0.80 | 0.803 | +0 % | 0.603 | 0.812 | -0.091 | 6.0 |
| Booth | 1.0x | 0 | 0.40 | 0.405 | +1 % | 0.302 | 0.402 | -0.067 | 6.0 |
| Booth | 1.0x | 100 | 0.40 | 0.393 | -2 % | 0.304 | 0.378 | -0.034 | 6.0 |
| Room | 0.5x | 50 | 0.55 | 0.556 | +1 % | 0.381 | 0.574 | -0.013 | 6.0 |
| Room | 1.0x | 50 | 1.10 | 1.102 | +0 % | 0.714 | 1.072 | -0.004 | 6.0 |
| Room | 2.0x | 50 | 2.20 | 2.181 | -1 % | 1.387 | 2.265 | -0.023 | 6.0 |
| Room | 1.0x | 0 | 1.10 | 1.074 | -2 % | 0.705 | 1.122 | -0.003 | 6.0 |
| Room | 1.0x | 100 | 1.10 | 1.093 | -1 % | 0.746 | 1.134 | -0.047 | 6.0 |
| Hall | 0.5x | 50 | 1.50 | 1.530 | +2 % | 0.884 | 1.645 | -0.004 | 6.0 |
| Hall | 1.0x | 50 | 3.00 | 2.995 | -0 % | 1.712 | 3.146 | -0.010 | 6.0 |
| Hall | 2.0x | 50 | 6.00 | 6.027 | +0 % | 3.332 | 6.304 | +0.003 | 9.0 |
| Hall | 1.0x | 0 | 3.00 | 2.995 | -0 % | 1.670 | 3.317 | +0.006 | 6.0 |
| Hall | 1.0x | 100 | 3.00 | 3.017 | +1 % | 1.771 | 3.084 | +0.001 | 6.0 |
| Spring | 0.5x | 50 | 1.25 | 1.251 | +0 % | 1.202 | 1.008 | +0.337 | 6.0 |
| Spring | 1.0x | 50 | 2.50 | 2.505 | +0 % | 2.273 | 1.908 | +0.300 | 6.0 |
| Spring | 2.0x | 50 | 5.00 | 4.988 | -0 % | 4.417 | 3.460 | +0.330 | 9.0 |
| Spring | 1.0x | 0 | 2.50 | 2.497 | -0 % | 2.331 | 1.837 | +0.338 | 6.0 |
| Spring | 1.0x | 100 | 2.50 | 2.505 | +0 % | 2.262 | 1.964 | +0.325 | 6.0 |
| Plate | 0.5x | 50 | 1.25 | 1.266 | +1 % | 1.060 | 1.449 | +0.022 | 6.0 |
| Plate | 1.0x | 50 | 2.50 | 2.514 | +1 % | 1.846 | 2.745 | +0.023 | 6.0 |
| Plate | 2.0x | 50 | 5.00 | 4.940 | -1 % | 3.043 | 5.514 | +0.004 | 9.0 |
| Plate | 1.0x | 0 | 2.50 | 2.571 | +3 % | 1.688 | 2.801 | +0.010 | 6.0 |
| Plate | 1.0x | 100 | 2.50 | 2.515 | +1 % | 1.997 | 2.698 | +0.006 | 6.0 |
| Ambient | 0.5x | 50 | 3.50 | 3.505 | +0 % | 2.382 | 3.683 | +0.004 | 6.0 |
| Ambient | 1.0x | 50 | 7.00 | 6.983 | -0 % | 4.561 | 7.263 | +0.002 | 9.9 |
| Ambient | 2.0x | 50 | 14.00 | 14.165 | +1 % | 8.845 | 14.715 | +0.001 | 23.6 |
| Ambient | 1.0x | 0 | 7.00 | 7.067 | +1 % | 4.439 | 7.210 | -0.020 | 10.0 |
| Ambient | 1.0x | 100 | 7.00 | 7.044 | +1 % | 4.757 | 7.246 | +0.011 | 10.0 |

## Impulse response structure

First arrival = first sample within 40 dB of the IR's peak. Early arrivals = local maxima within
20 dB of the largest sample in the 60 ms after the first arrival (count L / R, and how many of L's
have no R arrival within 0.1 ms). Echo density is Abel & Huang's (1.0 = Gaussian), 20 ms window,
100 ms after the first arrival; mixing time is when it first reads 0.9. -60 dB = the decay curve's
own crossing, no extrapolation.

| Type | DECAY | SIZE | First arrival L / R (ms) | Early arrivals L / R | L without R | Echo density @ 100 ms | Mixing time (ms) | Peak (dBFS) | Energy (dB) | -60 dB at (s) |
|---|---|---|---|---|---|---|---|---|---|---|
| Booth | 0.5x | 50 | 3.75 / 3.98 | 30 / 31 | 26 | 0.97 | 2 | 0.8 | 7.2 | 0.18 |
| Booth | 1.0x | 50 | 3.75 / 3.98 | 39 / 48 | 33 | 1.03 | 2 | -0.3 | 7.5 | 0.35 |
| Booth | 2.0x | 50 | 3.75 / 3.98 | 60 / 75 | 50 | 1.05 | 5 | -1.4 | 8.4 | 0.69 |
| Booth | 1.0x | 0 | 3.38 / 3.50 | 66 / 68 | 49 | 0.97 | 2 | -2.5 | 7.2 | 0.35 |
| Booth | 1.0x | 100 | 4.52 / 4.98 | 13 / 14 | 12 | 0.95 | 5 | 2.0 | 8.2 | 0.35 |
| Room | 0.5x | 50 | 17.52 / 18.29 | 20 / 45 | 14 | 0.96 | 22 | -3.8 | 4.0 | 0.53 |
| Room | 1.0x | 50 | 17.52 / 18.29 | 24 / 45 | 18 | 0.98 | 22 | -4.9 | 4.4 | 1.00 |
| Room | 2.0x | 50 | 17.52 / 18.29 | 26 / 47 | 20 | 0.96 | 22 | -6.0 | 5.2 | 1.97 |
| Room | 1.0x | 0 | 16.27 / 16.65 | 64 / 72 | 42 | 1.00 | 15 | -7.1 | 4.1 | 1.01 |
| Room | 1.0x | 100 | 20.06 / 21.58 | 19 / 20 | 11 | 0.93 | 50 | -2.6 | 5.2 | 1.00 |
| Hall | 0.5x | 50 | 55.06 / 56.58 | 25 / 33 | 15 | 0.97 | 58 | -7.5 | 2.6 | 1.41 |
| Hall | 1.0x | 50 | 55.06 / 56.58 | 25 / 33 | 15 | 0.98 | 55 | -8.6 | 3.2 | 2.76 |
| Hall | 2.0x | 50 | 55.06 / 56.58 | 26 / 33 | 16 | 0.98 | 55 | -9.7 | 4.2 | 5.49 |
| Hall | 1.0x | 0 | 52.52 / 53.29 | 57 / 67 | 36 | 1.02 | 35 | -10.8 | 2.9 | 2.79 |
| Hall | 1.0x | 100 | 60.12 / 63.15 | 9 / 11 | 7 | 0.89 | 135 | -6.3 | 3.8 | 2.74 |
| Spring | 0.5x | 50 | 32.19 / 36.15 | 93 / 70 | 70 | 0.83 | 10 | -23.0 | -0.4 | 1.31 |
| Spring | 1.0x | 50 | 32.19 / 36.15 | 93 / 70 | 67 | 0.83 | 10 | -24.1 | 0.9 | 2.56 |
| Spring | 2.0x | 50 | 32.19 / 36.15 | 94 / 70 | 68 | 0.82 | 10 | -25.2 | 2.3 | 5.02 |
| Spring | 1.0x | 0 | 23.94 / 26.90 | 99 / 72 | 70 | 0.94 | 2 | -25.0 | 0.9 | 2.57 |
| Spring | 1.0x | 100 | 43.17 / 48.48 | 97 / 48 | 82 | 1.14 | 12 | -23.0 | 1.0 | 2.54 |
| Plate | 0.5x | 50 | 13.06 / 14.71 | 94 / 74 | 72 | 0.81 | 188 | -13.1 | 6.1 | 1.18 |
| Plate | 1.0x | 50 | 13.06 / 14.71 | 94 / 74 | 72 | 0.77 | 210 | -14.2 | 6.3 | 2.24 |
| Plate | 2.0x | 50 | 13.06 / 14.71 | 94 / 74 | 72 | 0.76 | 210 | -15.3 | 6.6 | 4.40 |
| Plate | 1.0x | 0 | 11.58 / 12.75 | 105 / 88 | 72 | 0.92 | 92 | -17.8 | 5.9 | 2.24 |
| Plate | 1.0x | 100 | 15.15 / 17.48 | 104 / 78 | 82 | 0.70 | 255 | -15.2 | 6.7 | 2.25 |
| Ambient | 0.5x | 50 | 41.33 / 43.23 | 24 / 30 | 14 | 0.95 | 62 | -11.7 | 1.3 | 3.21 |
| Ambient | 1.0x | 50 | 41.33 / 43.23 | 24 / 30 | 14 | 0.94 | 88 | -12.8 | 2.3 | 6.40 |
| Ambient | 2.0x | 50 | 41.33 / 43.23 | 24 / 30 | 14 | 0.94 | 88 | -13.9 | 3.4 | 12.91 |
| Ambient | 1.0x | 0 | 38.17 / 39.10 | 48 / 64 | 31 | 0.98 | 35 | -15.1 | 2.3 | 6.47 |
| Ambient | 1.0x | 100 | 47.65 / 51.44 | 6 / 8 | 6 | 0.76 | 128 | -10.6 | 2.6 | 6.35 |

## Early arrivals at DECAY 1.0x, SIZE 50

Times in ms after the left channel's first arrival, with the sign of the sample; first eight.

| Type | L | R |
|---|---|---|
| Booth | 0.00+, 0.67-, 1.44+, 2.23-, 3.12+, 4.04-, 4.98+, 5.38+, ... | 0.23+, 0.96-, 2.50-, 2.85+, 4.35-, 5.27+, 5.73-, 7.48+, ... |
| Room | 0.00+, 2.19-, 4.79+, 6.00+, 7.44-, 8.58+, 9.50+, 10.40+, ... | 0.77+, 3.17-, 3.90+, 6.00+, 8.31-, 8.58+, 9.50+, 14.52-, ... |
| Hall | 0.00+, 4.38-, 9.56+, 14.85-, 16.50+, 20.79+, 24.10+, 25.15+, ... | 1.52+, 6.29-, 7.77+, 16.60-, 19.00+, 24.10+, 25.15+, 29.02-, ... |
| Spring | 0.71+, 1.23-, 1.88-, 2.42-, 2.90-, 3.35-, 3.79-, 4.42+, ... | 4.60+, 5.04-, 5.58-, 6.06-, 6.48-, 7.23-, 7.58-, 7.94-, ... |
| Plate | 0.00+, 3.79-, 5.08-, 7.58-, 8.88+, 9.50-, 10.17-, 11.38-, ... | 1.65+, 5.25-, 6.42-, 8.85-, 10.02+, 10.96-, 12.46-, 13.62+, ... |
| Ambient | 0.00+, 5.46-, 11.96+, 18.56-, 23.79+, 25.98+, 33.12+, 33.56-, ... | 1.90+, 7.88-, 9.71+, 20.75-, 23.73+, 33.10-, 33.71-, 36.27-, ... |

## Do two types share resonances?

Pearson correlation of the tail's dB spectrum between types at DECAY 1.0x, SIZE 50: 200-2000 Hz in
0.5 Hz steps, Hann window starting 0.25 x mid RT60 (at least 50 ms) after the first arrival and one
mid RT60 long (0.25..2 s), a 100 Hz moving average removed so that an EQ tilt does not count.
1.0 = the same resonant frequencies, 0 = unrelated.

| | Room | Hall | Spring | Plate | Ambient |
|---|---|---|---|---|---|
| Booth | -0.06 | -0.01 | -0.06 | -0.04 | -0.04 |
| Room | | -0.01 | +0.04 | -0.05 | +0.00 |
| Hall | | | +0.01 | +0.04 | -0.03 |
| Spring | | | | +0.05 | +0.03 |
| Plate | | | | | +0.00 |

## When each band arrives (DECAY 1.0x, SIZE 50)

First sample of a narrow band (f / 1.06 .. f x 1.06) within 20 dB of that band's largest in the
160 ms after the first arrival, in ms, less the same reading of the band filter itself. A
dispersive spring arrives later as frequency rises (a chirp); anything else reads about 0.

| Type | 300 Hz | 600 Hz | 1 kHz | 2 kHz | 3 kHz | 3.8 kHz | 3 kHz - 1 kHz |
|---|---|---|---|---|---|---|---|
| Booth | -0.1 | 0.1 | 0.0 | 0.0 | 0.0 | 0.0 | +0.0 |
| Room | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | +0.0 |
| Hall | 0.2 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | +0.0 |
| Spring | 0.5 | 0.8 | 1.0 | 1.5 | 2.8 | 4.1 | +1.7 |
| Plate | 1.3 | 0.7 | 0.4 | 0.2 | 0.1 | 0.1 | -0.3 |
| Ambient | 0.1 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | +0.0 |

## CPU

Wall time inside `processBlock` per second of audio, white noise in, block 512, median of 5 runs of
5 s, at each type's defaults (WET 25 / DRY 100). One core of the machine that ran this; a figure to
compare on the same machine, never a pass/fail.

| Type | 48 kHz | 96 kHz |
|---|---|---|
| Booth | 0.62 % | 1.32 % |
| Room | 0.65 % | 1.40 % |
| Hall | 0.66 % | 1.38 % |
| Spring | 1.17 % | 2.34 % |
| Plate | 0.34 % | 0.81 % |
| Ambient | 0.65 % | 1.39 % |
