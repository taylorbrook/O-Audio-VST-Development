# OuariconAnalogEQ - Parameter Specification (Draft)

> Draft extracted from creative brief. Full specification will be generated during UI mockup finalization.

## Parameters

### Low Frequency Band (LF)

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| LF Frequency | lf_freq | Float | 30.0 - 500.0 | 100.0 | Hz | Shelving filter corner frequency |
| LF Gain | lf_gain | Float | -12.0 - 12.0 | 0.0 | dB | Shelving filter boost/cut |
| LF Enable | lf_on | Bool | Off/On | On | - | Band bypass toggle |

### Low-Mid Band (LMF)

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| LMF Frequency | lmf_freq | Float | 100.0 - 2000.0 | 500.0 | Hz | Bell filter center frequency |
| LMF Gain | lmf_gain | Float | -12.0 - 12.0 | 0.0 | dB | Bell filter boost/cut |
| LMF Q | lmf_q | Choice | Low/Mid/High | Mid | - | Bandwidth selector (Wide/Medium/Tight) |
| LMF Enable | lmf_on | Bool | Off/On | On | - | Band bypass toggle |

### High-Mid Band (HMF)

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| HMF Frequency | hmf_freq | Float | 500.0 - 8000.0 | 2000.0 | Hz | Bell filter center frequency |
| HMF Gain | hmf_gain | Float | -12.0 - 12.0 | 0.0 | dB | Bell filter boost/cut |
| HMF Q | hmf_q | Choice | Low/Mid/High | Mid | - | Bandwidth selector (Wide/Medium/Tight) |
| HMF Enable | hmf_on | Bool | Off/On | On | - | Band bypass toggle |

### High Frequency Band (HF)

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| HF Frequency | hf_freq | Float | 2000.0 - 20000.0 | 8000.0 | Hz | Shelving filter corner frequency |
| HF Gain | hf_gain | Float | -12.0 - 12.0 | 0.0 | dB | Shelving filter boost/cut |
| HF Enable | hf_on | Bool | Off/On | On | - | Band bypass toggle |

### Global

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| Output Gain | output_gain | Float | -12.0 - 12.0 | 0.0 | dB | Master output level |
| Analog | analog | Bool | Off/On | On | - | Analog warmth/saturation circuit |

## Parameter Count Summary

- **Float parameters:** 10 (4x freq, 4x gain, 1x output, 1x analog drive implicit)
- **Choice parameters:** 2 (LMF Q, HMF Q)
- **Bool parameters:** 6 (4x band enable, 1x output, 1x analog)
- **Total:** 18 parameters

## Q Value Mapping

| Choice | Q Value | Character |
|--------|---------|-----------|
| Low (Wide) | ~0.5 | Broad, gentle curves |
| Mid (Medium) | ~1.0 | Balanced, musical |
| High (Tight) | ~2.0 | Focused, surgical |

## UI Control Mapping

### Dual-Layer Knobs
Each band uses a dual-layer knob:
- **Outer ring:** Frequency (logarithmic scaling)
- **Inner knob:** Gain (linear scaling, bipolar)

### Q Toggle Switches
- 3-position physical switch aesthetic
- Click to cycle: Low → Mid → High → Low

---

*Draft Status: Extracted from creative-brief.md*
*Full specification pending UI mockup finalization*
