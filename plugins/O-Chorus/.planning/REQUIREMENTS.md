# O-Chorus - Requirements

## Functional Requirements

### FR-1: Multi-Voice Chorus Engine
- Implement 1-8 chorus voices with independent delay lines
- Each voice uses phase-offset LFO modulation
- Per-voice depth variation for organic, non-mechanical character
- Smooth voice count transitions (no clicks/pops when changing)

### FR-2: Analog Character
- Bucket-brigade delay emulation via interpolated delay lines
- Subtle soft-clipping saturation on wet signal path
- High-frequency rolloff to simulate analog bandwidth limitations
- Slight per-voice modulation randomization

### FR-3: Stereo Imaging
- Voices distributed across stereo field based on Width parameter
- Mono-compatible at Width = 0%
- Full panoramic spread at Width = 100%

### FR-4: Parameter Controls
- Rate: 0.05-5.0 Hz, logarithmic scaling
- Depth: 0-100%, linear
- Voices: 1-8, integer steps
- Width: 0-100%, linear
- Tone: -100% to +100%, bipolar (cut/boost high frequencies)
- Mix: 0-100%, linear (dry/wet)

### FR-5: Audio Quality
- Sample-rate independent operation
- Click-free parameter changes (smoothed)
- No aliasing artifacts from modulation
- Cubic or allpass interpolation on delay lines

## Non-Functional Requirements

### NFR-1: Performance
- CPU usage under 5% on modern hardware at 8 voices
- Support 44.1kHz - 192kHz sample rates
- Buffer sizes 32 - 2048 samples

### NFR-2: Compatibility
- VST3 and AU formats
- macOS and Windows
- WebView-based UI

### NFR-3: Latency
- Zero added latency (no lookahead required)
- Delay lines are internal processing only
