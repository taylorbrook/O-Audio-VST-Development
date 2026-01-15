# Ouaricon Polystutter

**Plugin Type:** Multi-Lane Beat Repeater / Stutter Effect
**Target DAWs:** Logic Pro, Ableton Live, FL Studio
**Format:** AU, VST3, Standalone

---

## Core Concept

A polyrhythmic beat repeater with 4 independent lanes, each running at different subdivisions to create complex, evolving rhythmic patterns. Unlike single-lane beat repeaters (Ableton Beat Repeat, etc.), this enables layered polyrhythmic stutters from a single audio input.

**Tagline:** *"Polyrhythmic Beat Repeater"*

---

## Unique Value Proposition

No existing beat repeater offers this combination of capabilities:

1. **4 Independent Repeat Lanes** - Different subdivisions running simultaneously (1/8 + 1/16 + 1/8T + 1/32)
2. **16-Step Pattern Sequencer** - Per-lane step sequencer with pan per step
3. **Tape Degradation Simulation** - Repeats degrade like worn tape loops
4. **Reverse & Freeze Modes** - Play captured audio backward or hold indefinitely
5. **MIDI Trigger** - Specific MIDI notes trigger specific lanes
6. **Multiple Trigger Modes** - Beat-sync, envelope follower, sidechain, or manual (ms)
7. **Ping-Pong Pan** - Repeats alternate left/right in stereo field
8. **Swing Control** - Add groove to rigid subdivisions

---

## Target User

- Electronic music producers (EDM, techno, experimental)
- Sound designers seeking glitchy, rhythmic textures
- Live performers wanting dynamic stutter effects
- Anyone bored of single-lane beat repeaters

---

## Feature Specification

### Per-Lane Controls (x4 lanes)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Enabled | On/Off | Lane 1 On | Enable/disable lane |
| Subdivision | 1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T | 1/16 | Repeat timing (synced) |
| Manual Time | 10-2000ms | Off | Manual repeat time (overrides subdivision) |
| Repeats | 1-16 | 4 | Number of repeats per trigger |
| Decay | 0-100% | 90% | Volume reduction per repeat |
| Pitch | -12 to +12 st | 0 | Pitch shift per repeat |
| Filter | -100 to +100 | 0 | LP/HP sweep direction |
| Probability | 0-100% | 100% | Chance of triggering |
| Volume | 0-100% | 100% | Lane output level |
| Pan | -100 to +100 | 0 | Lane stereo position |
| Ping-Pong | On/Off | Off | Alternate pan L/R per repeat |
| Ping-Pong Width | 0-100% | 50% | Stereo spread of ping-pong |
| Reverse | On/Off | Off | Play captured segment backward |
| Swing | -50 to +50% | 0% | Timing offset for groove |

### Pattern Sequencer (per lane)

Each lane has a 16-step pattern sequencer:

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Step On/Off | On/Off | All On | Enable step |
| Step Pan | -100 to +100 | 0 | Per-step pan position |
| Pattern Length | 1-16 | 16 | Active steps in pattern |

### Global Controls

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Mix | 0-100% | 50% | Dry/wet balance |
| Master Swing | -50 to +50% | 0% | Global swing amount |
| Freeze | On/Off (momentary) | Off | Hold current capture indefinitely |

### Trigger Modes

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Trigger Mode | Sync/Envelope/Sidechain/Manual | Sync | How repeats are triggered |
| Envelope Trigger | On/Off | Off | Enable envelope follower |
| Threshold | 0-100% | 50% | Envelope trigger sensitivity |
| Attack | 0.1-50ms | 1ms | Envelope attack time |
| Release | 10-500ms | 100ms | Envelope release time |
| Sidechain | On/Off | Off | Use sidechain for triggering |
| Manual Trigger | Button | - | Manual one-shot trigger |

### MIDI Trigger

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| MIDI Enable | On/Off | Off | Enable MIDI triggering |
| Lane 1 Note | C-1 to G9 | C3 | MIDI note to trigger Lane 1 |
| Lane 2 Note | C-1 to G9 | D3 | MIDI note to trigger Lane 2 |
| Lane 3 Note | C-1 to G9 | E3 | MIDI note to trigger Lane 3 |
| Lane 4 Note | C-1 to G9 | F3 | MIDI note to trigger Lane 4 |
| All Lanes Note | C-1 to G9 | G3 | MIDI note to trigger all lanes |
| Freeze Note | C-1 to G9 | A3 | MIDI note to toggle freeze |

### Tape Degradation

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Saturation | 0-100% | 0% | Tape saturation amount |
| Wow | 0-100% | 0% | Slow pitch modulation |
| Flutter | 0-100% | 0% | Fast pitch modulation |
| Hiss | 0-100% | 0% | Tape hiss level |
| HF Rolloff | 0-100% | 0% | High frequency loss per repeat |
| Dropout | 0-100% | 0% | Random dropout probability |

---

## Signal Flow

```
Input ─────────────┬─────────────────────────────────────────────▶ Dry Path
                   │
                   ▼
         ┌─────────────────────────────────────────────────┐
         │              4-Lane Repeat Engine               │
         │                                                 │
         │  ┌──────────────────────────────────────────┐  │
         │  │         Pattern Sequencer (16-step)       │  │
         │  └──────────────────────────────────────────┘  │
         │                      │                         │
         │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐│
         │  │ Lane 1  │ │ Lane 2  │ │ Lane 3  │ │ Lane 4  ││
         │  │Subdiv/ms│ │Subdiv/ms│ │Subdiv/ms│ │Subdiv/ms││
         │  │ + Pan   │ │ + Pan   │ │ + Pan   │ │ + Pan   ││
         │  │+PingPong│ │+PingPong│ │+PingPong│ │+PingPong││
         │  │+Reverse │ │+Reverse │ │+Reverse │ │+Reverse ││
         │  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘│
         │       └──────┬────┴──────┬────┴──────┬────┘     │
         │              ▼           ▼           ▼          │
         │         ┌─────────────────────────────────┐     │
         │         │         Lane Mixer              │     │
         │         └─────────────────────────────────┘     │
         └─────────────────────────┬───────────────────────┘
                                   │
                   ┌───────────────┴───────────────┐
                   ▼                               ▼
          ┌─────────────────┐             ┌──────────────┐
          │  Tape Degrader  │             │    Freeze    │
          └────────┬────────┘             │    Buffer    │
                   │                      └──────────────┘
                   ▼
          ┌─────────────────┐
          │   Dry/Wet Mix   │───▶ Output
          └─────────────────┘
                   ▲
                   │
              Dry Path
```

---

## Trigger Sources

```
                    ┌─────────────────────────────────────┐
                    │          Trigger Router             │
                    │                                     │
   Beat Sync ──────▶│  Subdivision timing from host       │
                    │                                     │
   Envelope ───────▶│  Transient detection on input       │
                    │                                     │
   Sidechain ──────▶│  External audio triggers            │
                    │                                     │
   Manual (ms) ────▶│  Free-running timer                 │
                    │                                     │
   MIDI ───────────▶│  Note-on triggers specific lanes    │
                    │                                     │
                    └──────────────┬──────────────────────┘
                                   │
                                   ▼
                           Lane Triggers
```

---

## UI Vision

**Layout:** (Finalized v5)
- Header: Plugin title + botanical overlay
- 4 vertical lane strips side-by-side (320px total)
  - Each lane: progress bar + 3×3 knob grid + toggle buttons (PING/REV/MAN/FRZ)
- Pattern sequencer: Always visible 16×4 step grid (130px)
- Tape degradation: 6 knobs horizontal row (130px)
- Footer: ENV, SC, MIDI, TRIG toggles (50px)

**Aesthetic:** Ouaricon Audio Naturalist
- Background: Paper texture (`img/paper-background.jpg`)
- Botanical overlay: Winged insect (`img/botanical-bug.png`, 32% opacity, right side)
- Colors: Browns (#8B7355, #5C4033), greens (#8BA870, #6B8E4E), cream paper tones
- Typography: Garamond serif, uppercase labels, wide letter-spacing
- Knobs: 42px botanical seed cross-section pattern (10-segment conic gradient)

**Knob Layout:**
- Labels ABOVE knobs (9px uppercase)
- Values BELOW knobs (8px)
- 3×3 grid per lane: SUBDIV/REPS/DECAY, PITCH/FILTR/PROB, VOL/PAN/SWING

**Interactive Elements:**
- Progress bar per lane (shows stutter playback position)
- Per-lane Freeze button (not global)
- Pattern sequencer step toggles
- Lane enable/disable dims entire lane

**Dimensions:** 1000 × 750px (fixed, non-resizable)

---

## Preset Ideas

1. **Default** - Single lane, 1/16, 4 repeats, clean
2. **Polyrhythmic** - All 4 lanes at different subdivisions
3. **Tape Loop** - Heavy tape degradation, slow decay
4. **Glitch Machine** - Fast subdivisions, random probability, reverse
5. **Sidechain Pump** - Sidechain triggered, rhythmic gating
6. **Triplet Swing** - Triplet lanes, swing enabled
7. **Deteriorating Echo** - Long repeats with HF rolloff
8. **Stutter Build** - Probability ramps up over time
9. **Ping-Pong Madness** - All lanes with ping-pong pan
10. **MIDI Performer** - MIDI trigger mode, lanes on C-D-E-F
11. **Pattern Groove** - 16-step patterns with swing
12. **Frozen in Time** - Freeze-focused, atmospheric
13. **Reverse Tape** - Reverse mode with tape degradation
14. **Manual Glitch** - Manual ms timing, off-grid feel

---

## Technical Considerations

### DSP Challenges

1. **Click-Free Looping** - 5ms crossfade at repeat boundaries
2. **Tempo Sync** - PPQ position tracking for tight sync
3. **Manual Timing** - Free-running timer independent of host tempo
4. **Multi-Lane CPU** - Efficient buffer management for 4 lanes
5. **Pitch Shifting** - Quality pitch algorithm for repeat pitch
6. **Reverse Playback** - Buffer read in reverse with crossfades
7. **Freeze Buffer** - Separate buffer for indefinite hold
8. **Pattern Sequencer** - Step tracking synced to host
9. **Swing Implementation** - Timing offset calculation
10. **MIDI Processing** - Note parsing and lane routing

### Edge Cases

- Very fast subdivisions (1/32 at 180 BPM)
- Very slow subdivisions (1/4 at 60 BPM)
- Manual timing vs synced timing conflicts
- Tempo changes mid-playback
- Freeze during pattern playback
- Reverse + ping-pong combination
- MIDI trigger during envelope trigger
- Offline vs real-time rendering parity

---

## Differentiation from Competitors

| Feature | Ableton Beat Repeat | Effectrix | Stutter Edit | **Polystutter** |
|---------|---------------------|-----------|--------------|-------------------|
| Multi-lane | No | Sequenced | Limited | Yes (4 simultaneous) |
| Polyrhythms | No | Limited | No | Yes |
| Pattern sequencer | No | Yes | Yes | Yes (with pan) |
| Tape degradation | No | No | No | Yes |
| Envelope trigger | Limited | No | No | Yes |
| Sidechain input | No | No | No | Yes |
| Manual ms timing | No | No | No | Yes |
| MIDI trigger | No | No | Yes | Yes |
| Reverse mode | No | Yes | Yes | Yes |
| Freeze | No | No | Yes | Yes |
| Ping-pong pan | No | No | No | Yes |
| Per-step pan | No | No | No | Yes |
| Swing | No | No | No | Yes |

---

## Success Criteria

- [ ] All 4 lanes run simultaneously without clicks
- [ ] Tempo sync is rock-solid at all BPMs
- [ ] Manual ms timing works independently of host tempo
- [ ] Pattern sequencer syncs perfectly with host
- [ ] Reverse mode sounds clean (no clicks)
- [ ] Freeze holds audio indefinitely without artifacts
- [ ] MIDI triggers lanes reliably with low latency
- [ ] Ping-pong pan sounds smooth
- [ ] Swing feels musical
- [ ] Tape degradation sounds authentic
- [ ] CPU usage < 8% with all features active
- [ ] Passes pluginval validation
- [ ] Works in Logic, Ableton, FL Studio

---

## Complexity Assessment

**Rating: High**

This plugin has significant complexity due to:
- 4 simultaneous processing lanes
- Pattern sequencer with per-step controls
- Multiple trigger modes (sync, envelope, sidechain, manual, MIDI)
- Reverse and freeze buffer management
- Swing and ping-pong timing calculations
- Tape degradation DSP chain

Recommend breaking implementation into phases:
1. Core single-lane repeat engine
2. Multi-lane infrastructure
3. Pattern sequencer
4. Advanced triggers (envelope, sidechain, MIDI)
5. Special modes (reverse, freeze, ping-pong)
6. Tape degradation
7. UI integration

---

*Created from research: research/stutter-effects/path-b-beat-repeater.md*
*Refined with additional features: reverse, freeze, MIDI trigger, pattern sequencer, ping-pong pan, swing, manual ms timing*
