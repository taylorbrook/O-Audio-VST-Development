# Parameter Specification (Draft)

**Status:** Draft - Awaiting UI mockup for full specification
**Created:** 2026-08-20
**Source:** Derived from BRIEF.md parameters table

This is a lightweight specification to enable parallel DSP research.
Full specification will be generated from finalized UI mockup.

## Parameters

### console
- **Type:** Choice
- **Choices:** SNES, PS1, NES, Game Boy, Genesis
- **Default:** SNES
- **DSP Purpose:** Selects the emulated system — codec (BRR / SPU-ADPCM / DPCM / 4-bit wave / 8-bit DAC), fixed internal sample rate, interpolation mode, and output-stage model.

### crush
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 50
- **DSP Purpose:** Codec intensity — how hard the console's compression/quantization artifacts hit; blend/drive into the codec domain from subtle color (0%) to fully degraded (100%).

### age
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 20
- **DSP Purpose:** Hardware-condition model — noise floor, electrical hum, filter dulling, and drift increase with age.

### reverb
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 0
- **DSP Purpose:** PS1/N64-style SPU reverb send — murky short game reverb, routable in every console mode.

### mix
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 100
- **DSP Purpose:** Parallel dry/wet blend of processed vs. clean signal.

## Next Steps

- [ ] Complete UI mockup workflow (/start → option 3)
- [ ] Finalize design and generate full parameter-spec.md
- [ ] Validate consistency between draft and final spec
