#!/usr/bin/env python3
"""
O-Wind MIDI Expression Test
Generates a short MIDI file demonstrating CC2 (breath), CC74 (embouchure),
CC1 (mod wheel), and pitch bend — the hidden expression controls in O-Wind.
"""

from midiutil import MIDIFile

midi = MIDIFile(1)
midi.addTempo(0, 0, 90)
midi.addTrackName(0, 0, "O-Wind Expression Test")

ch = 0

# --- Section 1: Breath Controller Sweep (CC2) ---
# Sustained note with breath pressure ramping from strong to full and back
midi.addNote(0, ch, 72, 0, 4, 100)  # C5, 4 beats
for i in range(64):
    t = i * (4.0 / 64)
    # Triangle: ramp 80->127 over 2 beats, back down to 80
    if i < 32:
        val = 80 + int((i / 32) * 47)
    else:
        val = 80 + int(((64 - i) / 32) * 47)
    midi.addControllerEvent(0, ch, t, 2, val)
# Reset CC2
midi.addControllerEvent(0, ch, 4.0, 2, 0)

# --- Section 2: Embouchure / Register Transitions (CC74) ---
# Hold a note with high breath, sweep embouchure through register breaks in steps
midi.addNote(0, ch, 60, 5, 6, 100)  # C4, 6 beats (longer to hear transitions)
midi.addControllerEvent(0, ch, 5.0, 2, 110)  # high breath so registers are audible
# Step through embouchure positions to hit register plateaus clearly
steps = [20, 20, 50, 50, 80, 80, 100, 100, 127, 127, 80, 50, 20]
beat_per_step = 6.0 / len(steps)
for i, val in enumerate(steps):
    midi.addControllerEvent(0, ch, 5.0 + i * beat_per_step, 74, val)
# Reset
midi.addControllerEvent(0, ch, 11.0, 74, 0)
midi.addControllerEvent(0, ch, 11.0, 2, 0)

# --- Section 3: Mod Wheel Vibrato (CC1) ---
# Melody with increasing vibrato depth
notes = [(67, 12, 1.5), (69, 13.5, 1.5), (72, 15, 2)]  # G4, A4, C5
for note, start, dur in notes:
    midi.addNote(0, ch, note, start, dur, 90)

# Vibrato ramps up gently across the phrase
for i in range(48):
    t = 12.0 + i * (5.0 / 48)
    val = int((i / 47) * 50)  # 0 -> 50 (subtle, musical range)
    midi.addControllerEvent(0, ch, t, 1, val)
midi.addControllerEvent(0, ch, 17.0, 1, 0)

# --- Section 4: Pitch Bend Expression ---
# Note with expressive pitch bends (scoops and falls)
midi.addNote(0, ch, 65, 18, 3, 95)  # F4
# Scoop up into the note
midi.addPitchWheelEvent(0, ch, 18.0, -4096)
for i in range(16):
    t = 18.0 + i * (0.5 / 16)
    val = int(-4096 + (i / 15) * 4096)
    midi.addPitchWheelEvent(0, ch, t, val)
# Slight vibrato via pitch bend
import math
for i in range(32):
    t = 18.5 + i * (1.5 / 32)
    val = int(1200 * math.sin(i * 0.5))
    midi.addPitchWheelEvent(0, ch, t, val)
# Fall off at end
for i in range(8):
    t = 20.0 + i * (1.0 / 8)
    val = int(-(i / 7) * 6000)
    midi.addPitchWheelEvent(0, ch, t, val)
midi.addPitchWheelEvent(0, ch, 21.0, 0)

# --- Section 5: Combined Expression ---
# Musical phrase using all CCs together
phrase = [(60, 22, 1, 85), (64, 23, 1, 90), (67, 24, 1.5, 95), (72, 25.5, 2, 100)]
for note, start, dur, vel in phrase:
    midi.addNote(0, ch, note, start, dur, vel)

# Breath swells with each note (louder range)
for note, start, dur, vel in phrase:
    midi.addControllerEvent(0, ch, start, 2, 70)
    midi.addControllerEvent(0, ch, start + dur * 0.3, 2, 115)
    midi.addControllerEvent(0, ch, start + dur * 0.8, 2, 85)

# Gradual vibrato entrance (tighter max)
for i in range(24):
    t = 24.0 + i * (3.5 / 24)
    midi.addControllerEvent(0, ch, t, 1, min(45, int(i * 2)))
midi.addControllerEvent(0, ch, 27.5, 1, 0)
midi.addControllerEvent(0, ch, 27.5, 2, 0)

# Write file
output = "/Users/taylorbrook/Dev/VST-development/plugins/O-Wind/test-midi/O-Wind-Expression-Test.mid"
with open(output, "wb") as f:
    midi.writeFile(f)

print(f"Written: {output}")
print("Sections:")
print("  Bars 1-4:   CC2 breath sweep (single note)")
print("  Bars 5-8:   CC74 embouchure register walk")
print("  Bars 9-14:  CC1 mod wheel vibrato ramp")
print("  Bars 15-18: Pitch bend scoops & falls")
print("  Bars 19-24: All CCs combined phrase")
