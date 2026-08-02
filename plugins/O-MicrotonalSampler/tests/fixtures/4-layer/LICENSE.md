# Test Fixture Provenance — O-MicrotonalSampler 4-velocity-layer set

Source: **procedurally synthesized in-house** by `generate.py` in this same folder
Generator seed: none — the output is a pure deterministic sine, no RNG is involved
Dependencies to re-run: Python 3, `numpy`, `soundfile`
License: **AGPL-3.0-or-later**, together with the rest of this repository
Copyright: © 2026 Ouaricon Audio

Files (with the MD5 recorded 2026-08-01):

| File | Layer amplitude | MD5 | Bytes |
|---|---|---|---|
| `C4_v1.wav` | 0.25 | `e61541dcb9fa97591b962ce9578448b2` | 288044 |
| `C4_v2.wav` | 0.50 | `e17c9f40cd3cb97c0ec1798d295ac831` | 288044 |
| `C4_v3.wav` | 0.75 | `d431ab86f95f9b78b5f4c05b5e7ab0a3` | 288044 |
| `C4_v4.wav` | 1.00 | `8977070edfff6b9e34cf7c531bab5771` | 288044 |

Format: WAVE PCM, 24-bit, **stereo**, 48 000 Hz, exactly **1.000 s** (48 000 frames) per
file. Content is a 440 Hz sine; the four layers differ **only** in amplitude.

## These are test fixtures — they have never shipped in a binary

This matters enough to state directly, because it corrects a claim in the release-readiness
audit.

These four files are **not referenced by any `juce_add_binary_data` target** anywhere in the
repository. A grep of every `CMakeLists.txt` under `plugins/` for `4-layer` or `C4_v`
returns no matches (verified 2026-08-01). They are loaded from disk by the Phase 2.3 Gate 3
test only. **No signed, notarised, or distributed plugin binary has ever contained them.**

`PUBLIC-RELEASE-READINESS.md` §2.2 originally listed this folder among sample sets
"compiled into distributed plugin binaries via `BinaryData`". That characterisation did not
hold for these files; §2.2 has since been corrected.

## Use

The `FilenameParser` maps `C4_vN` → `(midiNote = 60, velocityLayer = N − 1)`. Because the
four layers differ only in amplitude, a velocity sweep across them makes the equal-power
crossfade measurable: two adjacent layers at 0.707 each sum to a known middle amplitude.

440 Hz is the ET frequency of A4 (MIDI 69), not C4 (MIDI 60), and that is intentional — the
loader records `slot.midiNote = 60` from the filename and the voice computes `playRate` so
that "play C4" reproduces the audio at the C4 microtonal frequency. The absolute source
frequency is irrelevant to the crossfade test; only the per-layer amplitude matters.

## Reproducibility

```bash
cd plugins/O-MicrotonalSampler/tests/fixtures/4-layer
python3 generate.py
md5 *.wav        # must match the table above
```

**No third-party content.** Nothing here was sampled, recorded, or derived from any
commercial sample library or third-party recording — these are synthesized reference tones.
This document is provenance evidence, not a third-party notice.
