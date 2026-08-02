# Built-in Source Sample Provenance — O-simpleGrain

Source: **procedurally synthesized in-house** by `plugins/O-simpleGrain/tools/generate_samples.py`
Generator seed: `SEED = 20260624` (deterministic)
Dependencies to re-run: Python 3, `numpy`, `scipy`
License: **AGPL-3.0-or-later**, together with the rest of this repository
Copyright: © 2026 Ouaricon Audio

Files (with the MD5 recorded 2026-08-01):

| File | MD5 | Bytes |
|---|---|---|
| `fire.wav`  | `9f638958f0bcdb09897c72e0ff06db10` | 344024 |
| `piano.wav` | `68efbe27e85979e723e1d777c2209e46` | 396944 |
| `voice.wav` | `cc22efdae4229249e026a4def47f71d7` | 291104 |
| `water.wav` | `19155402570691acce2512565b445836` | 344024 |

Format: WAVE PCM, 24-bit, **mono**, 44 100 Hz, peak-normalised to −1 dBFS.

Use: these four files are the **final shipping v1.0 built-in granular sources** — not
placeholders. That was a deliberate decision on 2026-06-24 ("procedural as final v1.0").
They are embedded into the distributed plugin binary via `juce_add_binary_data`, which is
precisely why their provenance needs to be checkable rather than assumed.

**Reproducibility.** Re-running the generator regenerates all four files **bit-identically**:

```bash
python3 plugins/O-simpleGrain/tools/generate_samples.py
md5 plugins/O-simpleGrain/Source/samples/*.wav      # must match the table above
```

Verified 2026-08-01 — MD5 match on every file. The seed is what makes this hold; changing
`SEED` produces different (still self-authored) audio and would invalidate the hashes above.

**No third-party content.** Nothing here was sampled, recorded, or derived from any
commercial sample library, any third-party recording, or any external dataset. Every file
is synthesized from scratch by the script named above — noise shaping, filtering, and
envelope work over a seeded NumPy PRNG. There is no license obligation attached to these
files beyond the repository's own AGPL-3.0-or-later terms; this document is provenance
evidence, not a third-party notice.
