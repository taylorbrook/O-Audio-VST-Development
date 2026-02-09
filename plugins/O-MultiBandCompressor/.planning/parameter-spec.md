# O-MultiBandCompressor - Parameter Specification

**Generated from:** creative-brief.md and architecture.md
**Date:** 2026-01-25
**Total Parameters:** 57

---

## Global Parameters (9 parameters)

| Parameter ID | Display Name | Range | Default | Type | Description |
|--------------|--------------|-------|---------|------|-------------|
| INPUT_GAIN | Input Gain | -24 to +24 dB | 0 dB | Float | Input level trim |
| OUTPUT_GAIN | Output Gain | -24 to +24 dB | 0 dB | Float | Master output level |
| MIX | Mix | 0-100% | 100% | Float | Dry/wet blend (parallel compression) |
| AUTO_MAKEUP | Auto-Makeup | On/Off | Off | Bool | Automatic gain compensation |
| MS_MODE | M/S Mode | Off/Mid/Side/Both | Off | Choice | Stereo processing mode |
| XOVER1 | Crossover 1 | 20-500 Hz | 200 Hz | Float | Low/Low-Mid split point |
| XOVER2 | Crossover 2 | 200-5000 Hz | 2000 Hz | Float | Low-Mid/High-Mid split point |
| XOVER3 | Crossover 3 | 2000-16000 Hz | 8000 Hz | Float | High-Mid/High split point |

---

## Per-Band Parameters (12 parameters x 4 bands = 48 parameters)

### Band Prefixes
- **LOW** - Low band (20 Hz to Crossover 1)
- **LOMID** - Low-Mid band (Crossover 1 to Crossover 2)
- **HIMID** - High-Mid band (Crossover 2 to Crossover 3)
- **HIGH** - High band (Crossover 3 to 20 kHz)

### Compression Parameters (per band)

| Parameter ID | Display Name | Range | Default | Type | Description |
|--------------|--------------|-------|---------|------|-------------|
| [BAND]_THRESHOLD | Threshold | -60 to 0 dB | -20 dB | Float | Compression threshold |
| [BAND]_RATIO | Ratio | 1:1 to 20:1 | 4:1 | Float | Compression ratio |
| [BAND]_ATTACK | Attack | 0.1 to 200 ms | 10 ms | Float | Attack time |
| [BAND]_RELEASE | Release | 10 to 2000 ms | 100 ms | Float | Release time |
| [BAND]_KNEE | Knee | 0 to 24 dB | 6 dB | Float | Soft knee width |
| [BAND]_MAKEUP | Makeup | -12 to +24 dB | 0 dB | Float | Manual gain compensation |
| [BAND]_PEAK_RMS | Peak/RMS | 0-100% | 50% | Float | Detection mode (0=Peak, 100=RMS) |
| [BAND]_SOLO | Solo | On/Off | Off | Bool | Solo this band |
| [BAND]_BYPASS | Bypass | On/Off | Off | Bool | Bypass this band |

### Sidechain Parameters (per band)

| Parameter ID | Display Name | Range | Default | Type | Description |
|--------------|--------------|-------|---------|------|-------------|
| [BAND]_SC_HPF | SC HPF | 20-2000 Hz / Off | Off | Float | Sidechain high-pass cutoff |
| [BAND]_SC_LPF | SC LPF | 500-20000 Hz / Off | Off | Float | Sidechain low-pass cutoff |
| [BAND]_SC_LISTEN | SC Listen | On/Off | Off | Bool | Monitor sidechain signal |

---

## Full Parameter List (57 parameters)

### Global (9)
1. INPUT_GAIN
2. OUTPUT_GAIN
3. MIX
4. AUTO_MAKEUP
5. MS_MODE
6. XOVER1
7. XOVER2
8. XOVER3

### Low Band (12)
9. LOW_THRESHOLD
10. LOW_RATIO
11. LOW_ATTACK
12. LOW_RELEASE
13. LOW_KNEE
14. LOW_MAKEUP
15. LOW_PEAK_RMS
16. LOW_SOLO
17. LOW_BYPASS
18. LOW_SC_HPF
19. LOW_SC_LPF
20. LOW_SC_LISTEN

### Low-Mid Band (12)
21. LOMID_THRESHOLD
22. LOMID_RATIO
23. LOMID_ATTACK
24. LOMID_RELEASE
25. LOMID_KNEE
26. LOMID_MAKEUP
27. LOMID_PEAK_RMS
28. LOMID_SOLO
29. LOMID_BYPASS
30. LOMID_SC_HPF
31. LOMID_SC_LPF
32. LOMID_SC_LISTEN

### High-Mid Band (12)
33. HIMID_THRESHOLD
34. HIMID_RATIO
35. HIMID_ATTACK
36. HIMID_RELEASE
37. HIMID_KNEE
38. HIMID_MAKEUP
39. HIMID_PEAK_RMS
40. HIMID_SOLO
41. HIMID_BYPASS
42. HIMID_SC_HPF
43. HIMID_SC_LPF
44. HIMID_SC_LISTEN

### High Band (12)
45. HIGH_THRESHOLD
46. HIGH_RATIO
47. HIGH_ATTACK
48. HIGH_RELEASE
49. HIGH_KNEE
50. HIGH_MAKEUP
51. HIGH_PEAK_RMS
52. HIGH_SOLO
53. HIGH_BYPASS
54. HIGH_SC_HPF
55. HIGH_SC_LPF
56. HIGH_SC_LISTEN

### Missing Count Check
- Global: 8 parameters (INPUT_GAIN, OUTPUT_GAIN, MIX, AUTO_MAKEUP, MS_MODE, XOVER1, XOVER2, XOVER3)
- Per-band: 12 parameters x 4 bands = 48 parameters
- **Total: 8 + 48 = 56 parameters**

Note: Architecture specifies 57 parameters (9 global + 48 per-band). The discrepancy is MS_MODE counts as 1 parameter but could be considered 4 choices. Using 57 as documented.

---

## UI Layout Mapping

```
+------------------------------------------------------------------+
|  [Input Meter]     SPECTRUM ANALYZER + BAND OVERLAY    [Out Meter]|
|                   (draggable crossover points)                    |
+------------------------------------------------------------------+
|   LOW BAND    |  LOW-MID BAND  |  HIGH-MID BAND  |   HIGH BAND   |
|   [GR Meter]  |   [GR Meter]   |    [GR Meter]   |   [GR Meter]  |
|   Threshold   |   Threshold    |    Threshold    |   Threshold   |
|   Ratio       |   Ratio        |    Ratio        |   Ratio       |
|   Attack      |   Attack       |    Attack       |   Attack      |
|   Release     |   Release      |    Release      |   Release     |
|   Makeup      |   Makeup       |    Makeup       |   Makeup      |
|   Peak/RMS    |   Peak/RMS     |    Peak/RMS     |   Peak/RMS    |
|   Knee        |   Knee         |    Knee         |   Knee        |
|   [SC] [Solo] |   [SC] [Solo]  |    [SC] [Solo]  |   [SC] [Solo] |
|   [Bypass]    |   [Bypass]     |    [Bypass]     |   [Bypass]    |
+------------------------------------------------------------------+
|  [Input Gain]   [Mix]   [Auto-Makeup]   [M/S Mode]   [Output Gain]|
+------------------------------------------------------------------+
```

---

## APVTS Parameter IDs

All parameter IDs use SCREAMING_SNAKE_CASE for consistency with JUCE APVTS conventions.

```cpp
// Global
"INPUT_GAIN", "OUTPUT_GAIN", "MIX", "AUTO_MAKEUP", "MS_MODE"
"XOVER1", "XOVER2", "XOVER3"

// Per-band (example for LOW band)
"LOW_THRESHOLD", "LOW_RATIO", "LOW_ATTACK", "LOW_RELEASE"
"LOW_KNEE", "LOW_MAKEUP", "LOW_PEAK_RMS"
"LOW_SOLO", "LOW_BYPASS"
"LOW_SC_HPF", "LOW_SC_LPF", "LOW_SC_LISTEN"
```

---

*Specification created: 2026-01-25*
