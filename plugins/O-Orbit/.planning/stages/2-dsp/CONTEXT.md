# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-02-10
**Participants:** User (Taylor), Claude

## Requirements Confirmed

- All 3 phases of Stage 2 will be implemented in order (2.1 -> 2.2 -> 2.3)
- Phase 2.1: Motion Engine + 2-speaker VBAP (using SAF from the start, no equal-power panning detour)
- Phase 2.2: Full 2D VBAP + multi-channel output (Quad, 5.1, 7.1, Hexaphonic, Octaphonic)
- Phase 2.3: 3D VBAP with elevation + custom speaker layouts + auto-downmix
- Drift path uses Perlin noise (not filtered random walk) for organic, fractal-like movement
- Speed parameter reused for Drift rate (maps to noise octave frequency)
- Motion position updated per-block with linear interpolation of azimuth/elevation per-sample
- No binaural/HRTF rendering for v1.0 (VBAP amplitude panning only, consistent pipeline stereo through 24ch)
- Binaural could be a v1.1 feature if there's demand

## Constraints Identified

- SAF v1.3.4 already integrated as git submodule (Stage 1 complete)
- macOS uses Apple Accelerate (auto-detected, zero config)
- Windows will need Intel MKL or OpenBLAS for SAF BLAS/LAPACK dependency
- VBAP triangulation must run on background thread (100-500ms for large layouts), not audio thread
- Custom path drawing deferred to v1.1 (not MVP)
- Maximum 24 output channels (covers all practical speaker arrays)

## Existing Foundation (from Stage 1)

- 17 APVTS parameters with cached atomic pointers in 3 groups (Motion, Spatial, Mix)
- DSP stubs: MotionEngine (returns 0,0,1), VBAPRenderer (equal gain), DistanceModel (passthrough)
- SpeakerLayout struct with 8 preset layouts (Stereo through Octaphonic)
- Multi-channel bus: mono/stereo input, 2-24 channel discrete output
- SmoothedValue for speed, width, depth, tilt, mix
- SAF v1.3.4 compiling with Apple Accelerate

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Stereo rendering | 2-speaker VBAP via SAF | Consistent pipeline from stereo to 24ch; no throwaway equal-power code |
| Drift algorithm | Perlin noise | Smoother, more organic movement with fractal-like structure |
| Drift speed control | Reuse Speed parameter | Maps to noise octave frequency; no extra parameter needed |
| Motion update rate | Per-block + linear interpolation | Industry standard; smooth at all speeds with negligible CPU |
| Phase scope | All 3 phases (2.1, 2.2, 2.3) | Full DSP as planned in ROADMAP |
| Binaural/HRTF | Deferred to v1.1 | Out of scope for VBAP-based spatializer; different rendering paradigm |
| Custom path | Deferred to v1.1 | Too complex for MVP; 4 built-in paths sufficient |

## Phase Breakdown

### Phase 2.1: Motion Engine + Distance Model + 2-Speaker VBAP
- Implement all 4 path algorithms: Orbit, Pendulum, Linear, Drift (Perlin)
- Per-block position update with per-sample linear interpolation
- Tempo sync (read host BPM, convert divisions to Hz)
- Distance model: attenuation (3 curves) + air absorption (1-pole LPF)
- Source mode: Mono sum / L+R Split with phase offset
- 2-speaker VBAP using SAF for stereo output
- Mix parameter (dry/wet blend)
- **Test:** audible motion in stereo, tempo sync locks to host, distance affects level+brightness

### Phase 2.2: Full 2D VBAP + Multi-Channel Output
- Integrate SAF `saf_vbap` for 2D VBAP (pair-wise panning)
- Support all ear-level presets: Stereo, Quad, 5.1, 7.1, Hexaphonic, Octaphonic
- VBAP gain calculation per-block with per-sample gain smoothing (linear ramp)
- Speaker layout switching at runtime (preset dropdown)
- Multi-channel output routing (match VBAP gains to output channels)
- Center diverge parameter (spread to adjacent speakers)
- **Test:** 5.1/7.1 output in DAW, VBAP gains correct, no clicks on rapid motion

### Phase 2.3: 3D VBAP + Custom Layouts + Auto-Downmix
- 3D VBAP with Delaunay triangulation (SAF `findLsTriplets`)
- Elevation enable/range parameters activate 3D mode
- 5.1.4 and 7.1.4 preset layouts with height speakers
- Custom speaker layout data structure (add/remove/reposition via state)
- Auto-downmix: detect channel mismatch, energy-preserving fold-down
- Stereo downmix fallback using equal-power from VBAP azimuth
- **Test:** height speakers active in 7.1.4, downmix badge displays, custom layout works

## Open Questions

- SAF `saf_vbap` API specifics for 2-speaker (stereo) VBAP -- may need special handling or fallback to pair-wise
- Perlin noise implementation: use a lightweight header-only library or implement from scratch (~30-50 lines)
- Thread safety pattern for VBAP triangle data swap when layout changes (mutex vs lock-free)

## Performance Targets

| Config | Target CPU |
|--------|-----------|
| Stereo, 1 source | <1% |
| 7.1, 2 sources | <3% |
| 7.1.4, 2 sources | <4% |
| 24-channel custom, 2 sources | <5% |

## Next Phase

Ready for: **research** phase (investigate SAF VBAP API, Perlin noise implementation, thread safety patterns)
