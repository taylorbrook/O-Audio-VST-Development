# Manual DAW Testing Checklist for [PluginName]

**DAW:** [User specifies - Logic Pro, Ableton Live, etc.]
**Date:** [timestamp]

## Setup

- [ ] Open DAW
- [ ] Load plugin in audio track
- [ ] Open plugin UI
- [ ] Verify all controls visible and styled correctly
- [ ] Verify window size correct (not clipped or oversized)

## Parameter Testing

- [ ] Move each parameter slowly, verify audio changes smoothly
- [ ] Move parameters quickly, verify no zipper noise or clicks
- [ ] Verify parameter ranges (min/max values correct)
- [ ] Test extreme values (no crashes, audio doesn't explode)
- [ ] Verify parameter labels and units display correctly
- [ ] Test with different audio sources (drums, vocals, synths, full mix)

### Specific parameters to test:

[Auto-generate from parameter-spec.md]

- [ ] **[PARAMETER_1]** ([range]): [Expected behavior]
- [ ] **[PARAMETER_2]** ([range]): [Expected behavior]
- [ ] **[PARAMETER_3]** ([range]): [Expected behavior]

## Automation

- [ ] Enable automation for key parameter (e.g., threshold)
- [ ] Draw automation curve (gradual sweep)
- [ ] Play back automation, verify plugin follows correctly
- [ ] Verify no lag or jumps in automation response
- [ ] Record parameter changes (twist knob during playback)
- [ ] Verify recorded automation plays back identically
- [ ] Test automation with multiple parameters simultaneously
- [ ] Verify parameter smoothing (no zipper noise during automation)

**Edge cases:**

- [ ] Jump automation from min to max instantly (no crash, reasonable smoothing)
- [ ] Automate rapidly (1ms resolution), verify CPU handles it
- [ ] Automate during buffer size changes (stress test)

## Preset Recall

- [ ] Set parameters to known values
- [ ] Save preset (e.g., "Aggressive Compression")
- [ ] Change all parameters to different values
- [ ] Load saved preset
- [ ] Verify all parameters restored exactly
- [ ] Test with multiple presets (save 3-5, load each, verify correctness)
- [ ] Test DAW's built-in preset system (if different from plugin's)

**Edge cases:**

- [ ] Load preset while audio playing (no clicks, instant parameter changes)
- [ ] Load preset during automation (verify automation overrides preset)
- [ ] Load very old preset (if plugin has legacy versions)

## Project Save/Load

- [ ] Configure plugin with specific parameter values
- [ ] Save DAW project
- [ ] Close DAW completely (not just project)
- [ ] Reopen DAW
- [ ] Open saved project
- [ ] Verify plugin loads with correct state
- [ ] Verify parameter values restored exactly
- [ ] Play audio, verify sonic result unchanged

**Edge cases:**

- [ ] Save project, change plugin version, reopen (backward compatibility)
- [ ] Save project on one machine, open on another (different CPU, OS version)
- [ ] Corrupted state (manually edit project file, verify plugin doesn't crash)

## Sample Rate Changes

- [ ] Test at 44.1 kHz (standard)
- [ ] Test at 48 kHz (video standard)
- [ ] Test at 96 kHz (high-resolution)
- [ ] Verify sonic character unchanged across sample rates
- [ ] Verify no crashes or glitches during sample rate switch
- [ ] Verify parameters remain valid (no scaling issues)

**DSP-specific tests:**

- **Filters**: Verify cutoff frequencies scale correctly
- **Time-based FX**: Verify delay times remain constant in milliseconds
- **Pitch-based**: Verify tuning remains correct (440 Hz = A4)

## Buffer Size Changes

- [ ] Test at 64 samples (low-latency)
- [ ] Test at 256 samples (typical)
- [ ] Test at 1024 samples (high-latency)
- [ ] Verify no crashes during buffer size switch
- [ ] Verify no audio dropouts or glitches
- [ ] Verify CPU usage scales appropriately (larger buffers = more efficient)
- [ ] Test with non-power-of-2 buffer sizes (e.g., 480 samples)

## Channel Configuration

- [ ] Test with mono input (single channel)
- [ ] Test with stereo input (L/R channels)
- [ ] Test with multichannel (if plugin supports 5.1, 7.1, etc.)
- [ ] Verify channel routing correct (no swapped L/R)
- [ ] Verify mono/stereo switch works correctly
- [ ] Test with mismatched I/O (mono in, stereo out)

**Stereo-specific tests:**

- [ ] Verify stereo image preserved (use width meter)
- [ ] Test with out-of-phase signals (L/R opposite polarity)
- [ ] Test with mono source panned hard left/right

## Edge Cases

- [ ] Silence input: Verify no added noise
- [ ] DC offset input: Verify plugin removes or handles correctly
- [ ] Clipped input (0 dBFS+): Verify no internal clipping artifacts
- [ ] Very low-level input (-60 dBFS): Verify no noise floor issues
- [ ] Change parameters rapidly (click spree): Verify no crashes
- [ ] Open/close UI repeatedly: Verify no memory leaks
- [ ] Load plugin in 10+ tracks simultaneously: Verify CPU remains reasonable

## Sonic Quality

- [ ] Load test signal (sine wave 1kHz, -12 dBFS)
- [ ] Process through plugin (with typical settings)
- [ ] Analyze output (spectrum analyzer, oscilloscope)
- [ ] Verify no unexpected harmonics or artifacts
- [ ] Verify output level correct (gain staging)

**Sine wave analysis:**

- [ ] Input: Clean sine wave (single fundamental frequency)
- [ ] Output: Should match expected behavior (e.g., compressor adds harmonics, EQ doesn't)
- [ ] No aliasing (no high-frequency artifacts above Nyquist)
- [ ] No DC offset (waveform centered at 0V)

- [ ] Load real audio (drums, vocal, mix)
- [ ] Process through plugin
- [ ] A/B test (bypass on/off)
- [ ] Verify sonic quality matches expectations
- [ ] Check for artifacts:
  - [ ] Digital clicks/pops
  - [ ] Zipper noise (parameter modulation)
  - [ ] Pre-ringing (linear-phase filters)
  - [ ] Pumping (compressor release too fast)
  - [ ] Comb filtering (phase issues)

## Stress Testing

- [ ] Leave plugin running for 1+ hour (check for memory leaks)
- [ ] Rapidly load/unload plugin 50+ times (check for resource cleanup)
- [ ] Max out all parameters (extreme settings, verify no crash)
- [ ] Process very loud input (+20 dBFS if possible) - verify no internal overflow
- [ ] Run with other CPU-intensive plugins (check for resource conflicts)

## Cross-DAW Testing (if time permits)

Test in 2-3 different DAWs to catch host-specific issues:

- [ ] Logic Pro (AU)
- [ ] Ableton Live (VST3)
- [ ] Reaper (VST3/AU)
- [ ] Pro Tools (AAX - if available)

Verify consistent behavior across all hosts.

## Final Verification

- [ ] All controls functional
- [ ] No crashes encountered
- [ ] No audio artifacts
- [ ] Parameter automation works
- [ ] Preset recall works
- [ ] Project save/load works
- [ ] Performance acceptable (CPU, latency)
- [ ] Sonic quality meets expectations

## Notes / Issues Found

[Free-form text area for user to note any issues]

---

**When complete, mark all items checked and confirm.**
