/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    bend_state_check.cpp — O-Prism v1.26.1 pitch-bend / state / zero-channel gate.

    Drives the REAL processor (createPluginFilter(), no editor) and closes the
    v1.26.0 code-review findings that are host-interaction sequences — the ones
    the v1.26.1 CHANGELOG had to verify by reasoning because no gate covered
    them:

      [A] Centred wheel is inert. A note struck with the wheel at 8192 renders
          at its unbent frequency. startNote() now writes the bend table on
          every strike, so this guards the no-op: applyPitchBend(f, 0.0f) is
          f * pow(2, 0.0), exactly f.

      [B] CR-03 — a released note does not re-strike detuned. Bend C4, release
          it, centre the wheel, strike C4 again: it must sound at 261.63 Hz.
          The bend step is asserted first, so the verdict cannot pass because
          the wheel silently did nothing.

      [C] The startNote seed — a note struck under a HELD wheel plays bent.
          The mirror of [B]: with the wheel parked at +1 semitone and no note
          sounding, striking E4 must render 349.23 Hz, not 329.63 Hz.

          Each test uses a note no earlier test has bent (A/B C4, C E4, D G4).
          They must: the bend table is keyed by note and survives for the life
          of the instance, so re-using C4 here let [C] pass on pre-fix code —
          [B] had left note 60 reading +1 semitone, which is the very bug [B]
          exists to catch. A fresh note has no entry to inherit.

      [D] WR-09 — a zero-channel block does not dereference channel 0. Eight
          zero-channel blocks (one carrying a note-on, which is the path into
          PrismVoice::renderNextBlock's getWritePointer(0)), then a normal
          block that must still render audio. Survival IS the verdict: before
          the fix this crashed.

      [E] WR-03 — setStateInformation clears a user-wavetable override. Bind
          osc A to a user table, then restore a session that has none: the
          override must be gone, both with a "userWavetables" child present
          and empty, and with the child absent entirely (legacy states).

    NOT covered here, deliberately:
      - clearPitchBend()/clearAllPitchBends() in isolation. The startNote seed
        overwrites a stale entry at the next strike, so the clear has no
        separately observable effect on pitch; [B] gates the user-visible
        failure mode that both mechanisms serve. The call sites are verified
        by reading the source.
      - WR-07's LFO-rate dead band. The fix is the removal of a guard, and
        LFO::setRate is a single divide with no phase side effect — there is
        no state left to observe between samples.

    Usage:  O-Prism-bend-state-check
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{

int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  ok    " : "  FAIL  ") << what << "\n";
    if (! ok)
        ++failures;
}

constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;
constexpr double kC4Hz       = 261.6255653;   // MIDI 60, 12-TET, A4 = 440

// Wheel positions. pitchBendRange defaults to 2 semitones and both
// PrismVoice::pitchWheelMoved and the startNote seed normalise the raw 14-bit
// value as (v - 8192) / 8192 — so 12288 is +0.5, i.e. +1.00 semitone.
constexpr int kWheelCentre = 8192;
constexpr int kWheelUp1St  = 12288;

/** 12-TET frequency of a MIDI note, A4 = 440 — the tuning O-Prism defaults to. */
double noteHz (int midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}

/** The same note one semitone sharp — what a +1 st wheel must produce. */
double noteUp1StHz (int midiNote)
{
    return noteHz (midiNote) * std::pow (2.0, 1.0 / 12.0);
}

// ─── Pitch analysis ───────────────────────────────────────────────────────

/** Fundamental period via normalised autocorrelation, parabolic-interpolated,
    searched +/-20 % around the lag of the note under test. That window spans
    -3.9..+3.2 semitones around it, so it brackets both the unbent and the
    +1-semitone answer and cannot bias the verdict toward either. */
double measurePeriodSamples (const std::vector<float>& x, double expectedLag)
{
    const int n = static_cast<int> (x.size());
    const int lagMin = static_cast<int> (expectedLag * 0.8);
    const int lagMax = static_cast<int> (expectedLag * 1.2);
    const int span   = n - lagMax - 1;

    std::vector<double> r (static_cast<size_t> (lagMax + 2), 0.0);
    double bestVal = -2.0;
    int bestLag = lagMin;

    for (int lag = lagMin - 1; lag <= lagMax + 1; ++lag)
    {
        double num = 0.0, d0 = 0.0, d1 = 0.0;
        for (int i = 0; i < span; ++i)
        {
            const double a = x[static_cast<size_t> (i)];
            const double b = x[static_cast<size_t> (i + lag)];
            num += a * b; d0 += a * a; d1 += b * b;
        }
        const double v = (d0 > 0.0 && d1 > 0.0) ? num / std::sqrt (d0 * d1) : 0.0;
        r[static_cast<size_t> (lag)] = v;
        if (lag >= lagMin && lag <= lagMax && v > bestVal) { bestVal = v; bestLag = lag; }
    }

    const double ym = r[static_cast<size_t> (bestLag - 1)];
    const double y0 = r[static_cast<size_t> (bestLag)];
    const double yp = r[static_cast<size_t> (bestLag + 1)];
    const double denom = ym - 2.0 * y0 + yp;
    const double delta = (std::abs (denom) > 1e-12) ? 0.5 * (ym - yp) / denom : 0.0;
    return bestLag + delta;
}

double measureHz (const std::vector<float>& x, int midiNote)
{
    return kSampleRate / measurePeriodSamples (x, kSampleRate / noteHz (midiNote));
}

double centsFrom (double measured, double reference)
{
    return 1200.0 * std::log2 (measured / reference);
}

double rms (const std::vector<float>& x)
{
    double acc = 0.0;
    for (auto v : x) acc += static_cast<double> (v) * v;
    return std::sqrt (acc / static_cast<double> (x.empty() ? 1 : x.size()));
}

// ─── Rendering ────────────────────────────────────────────────────────────

/** Run `seconds` of audio, delivering `midi` in the first block only.
    Returns the left channel. */
std::vector<float> pump (juce::AudioProcessor& proc, juce::MidiBuffer midi, double seconds)
{
    const int numBlocks = static_cast<int> (std::ceil (seconds * kSampleRate / kBlockSize));
    juce::AudioBuffer<float> buf (proc.getTotalNumOutputChannels(), kBlockSize);

    std::vector<float> out;
    out.reserve (static_cast<size_t> (numBlocks) * kBlockSize);
    for (int b = 0; b < numBlocks; ++b)
    {
        buf.clear();
        proc.processBlock (buf, midi);
        midi.clear();
        const float* l = buf.getReadPointer (0);
        out.insert (out.end(), l, l + kBlockSize);
    }
    return out;
}

juce::MidiBuffer msg (const juce::MidiMessage& m)
{
    juce::MidiBuffer b;
    b.addEvent (m, 0);
    return b;
}

/** Strike `midiNote` and return the steady-state tail of the render — the
    first 0.25 s is dropped so the attack and any glide are excluded. */
std::vector<float> strike (juce::AudioProcessor& proc, int midiNote, double seconds = 0.75)
{
    auto y = pump (proc, msg (juce::MidiMessage::noteOn (1, midiNote, static_cast<juce::uint8> (100))), seconds);
    const size_t skip = static_cast<size_t> (0.25 * kSampleRate);
    return { y.begin() + static_cast<long> (skip), y.end() };
}

/** Release `midiNote` and run the tail to completion. ampRelease is set to
    1 ms by setUpParams(), so 0.5 s is ~500 release times — the tail-off branch
    in renderNextBlock() has certainly reached clearCurrentNote(). */
void release (juce::AudioProcessor& proc, int midiNote)
{
    pump (proc, msg (juce::MidiMessage::noteOff (1, midiNote)), 0.5);
}

void moveWheel (juce::AudioProcessor& proc, int position)
{
    pump (proc, msg (juce::MidiMessage::pitchWheel (1, position)), 0.1);
}

void setParam (OPrismAudioProcessor& proc, const char* id, float plainValue)
{
    auto* p = proc.getAPVTS().getParameter (id);
    jassert (p != nullptr);
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

/** A fast, flat-topped envelope so the measured window is a steady tone and
    the release tail ends promptly. Everything else stays at its default —
    all four effect mixes default to 0.0 and glideMode to "Off", so the
    rendered pitch is the oscillator's. */
void setUpParams (OPrismAudioProcessor& proc)
{
    setParam (proc, "ampAttack",  0.001f);
    setParam (proc, "ampDecay",   0.001f);
    setParam (proc, "ampSustain", 1.0f);
    setParam (proc, "ampRelease", 0.001f);
}

// ─── [E] helpers ──────────────────────────────────────────────────────────

/** A session blob from a fresh instance that has never had an override:
    "userWavetables" is present (getStateInformation always creates it) with
    both properties empty. */
juce::MemoryBlock cleanSessionBlob()
{
    std::unique_ptr<juce::AudioProcessor> q (createPluginFilter());
    q->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
    q->prepareToPlay (kSampleRate, kBlockSize);

    juce::MemoryBlock blob;
    q->getStateInformation (blob);
    return blob;
}

/** The same blob with the "userWavetables" child removed — what a pre-1.9
    session looks like. */
juce::MemoryBlock stripUserWavetables (const juce::MemoryBlock& blob)
{
    std::unique_ptr<juce::XmlElement> xml (
        juce::AudioProcessor::getXmlFromBinary (blob.getData(), static_cast<int> (blob.getSize())));

    if (xml != nullptr)
        xml->deleteAllChildElementsWithTagName ("userWavetables");

    juce::MemoryBlock out;
    if (xml != nullptr)
        juce::AudioProcessor::copyXmlToBinary (*xml, out);
    return out;
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::unique_ptr<juce::AudioProcessor> proc (createPluginFilter());
    auto* prism = dynamic_cast<OPrismAudioProcessor*> (proc.get());

    if (prism == nullptr)
    {
        std::cerr << "createPluginFilter() did not return an OPrismAudioProcessor\n";
        return 1;
    }

    proc->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
    proc->prepareToPlay (kSampleRate, kBlockSize);
    setUpParams (*prism);

    std::cout << "O-Prism bend / state / zero-channel gate\n";

    // ─── [A] Centred wheel is inert ───────────────────────────────────────
    std::cout << "\n[A] centred wheel — startNote seeds 0.0f, which must be a no-op\n";
    {
        constexpr int note = 60;   // C4
        auto y = strike (*proc, note);
        const double hz = measureHz (y, note);
        check (rms (y) > 1.0e-4, juce::String ("renders a non-silent tone (rms ")
                                     + juce::String (rms (y), 6) + ")");
        check (std::abs (centsFrom (hz, kC4Hz)) < 1.0,
               juce::String ("C4 at ") + juce::String (hz, 3) + " Hz, "
                   + juce::String (centsFrom (hz, kC4Hz), 3) + " cents from 261.626");
        release (*proc, note);
    }

    // ─── [B] CR-03: a released note does not re-strike detuned ────────────
    std::cout << "\n[B] CR-03 — bend, release, centre the wheel, re-strike\n";
    {
        constexpr int note = 60;   // C4

        pump (*proc, msg (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100))), 0.3);
        auto bent = pump (*proc, msg (juce::MidiMessage::pitchWheel (1, kWheelUp1St)), 0.75);
        bent.erase (bent.begin(), bent.begin() + static_cast<long> (0.25 * kSampleRate));

        const double bentHz = measureHz (bent, note);
        // Stimulus gate: if the wheel did nothing, the re-strike check below
        // would pass vacuously (pattern_gate_stimulus_below_threshold_is_vacuous).
        check (std::abs (centsFrom (bentHz, noteUp1StHz (note))) < 2.0,
               juce::String ("wheel at +1 st actually bends: ") + juce::String (bentHz, 3)
                   + " Hz, " + juce::String (centsFrom (bentHz, noteHz (note)), 2) + " cents from C4");

        release (*proc, note);
        moveWheel (*proc, kWheelCentre);

        auto again = strike (*proc, note);
        const double againHz = measureHz (again, note);
        check (std::abs (centsFrom (againHz, noteHz (note))) < 1.0,
               juce::String ("re-strike with the wheel centred is unbent: ")
                   + juce::String (againHz, 3) + " Hz, "
                   + juce::String (centsFrom (againHz, noteHz (note)), 2) + " cents from C4");
        release (*proc, note);
    }

    // ─── [C] the startNote seed ───────────────────────────────────────────
    std::cout << "\n[C] seed — a note struck under a held wheel plays bent\n";
    {
        constexpr int note = 64;   // E4 — untouched by [A] and [B], so it has
                                   // no stale table entry to pass on.
        moveWheel (*proc, kWheelUp1St);   // no note sounding: no voice hears this
        auto y = strike (*proc, note);
        const double hz = measureHz (y, note);
        check (std::abs (centsFrom (hz, noteUp1StHz (note))) < 1.0,
               juce::String ("strike under a held wheel is bent: ") + juce::String (hz, 3)
                   + " Hz, " + juce::String (centsFrom (hz, noteHz (note)), 2) + " cents from E4 "
                   + "(unseeded would read 0.00)");
        release (*proc, note);
        moveWheel (*proc, kWheelCentre);
    }

    // ─── [D] WR-09: zero-channel blocks ───────────────────────────────────
    std::cout << "\n[D] WR-09 — zero-channel blocks must not touch channel 0\n";
    {
        constexpr int note = 67;   // G4 — again a note no earlier test bent, so
                                   // this section reports on WR-09 alone.
        juce::AudioBuffer<float> zero (0, kBlockSize);
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)), 0);

        for (int i = 0; i < 8; ++i)
        {
            proc->processBlock (zero, m);   // first block carries the note-on
            m.clear();
        }
        check (true, "8 zero-channel blocks (one with a note-on) returned without crashing");

        // The instance must still be usable afterwards.
        pump (*proc, msg (juce::MidiMessage::allNotesOff (1)), 0.2);
        auto y = strike (*proc, note);
        const double hz = measureHz (y, note);
        check (rms (y) > 1.0e-4, "a normal block after them still renders audio");
        check (std::abs (centsFrom (hz, noteHz (note))) < 1.0,
               juce::String ("and still at the right pitch (") + juce::String (hz, 3) + " Hz)");
        release (*proc, note);
    }

    // ─── [E] WR-03: state restore clears a user-wavetable override ────────
    std::cout << "\n[E] WR-03 — setStateInformation clears a user-wavetable override\n";
    {
        const juce::String tmpName ("ZZ-improve-verify-tmp");

        prism->startEditing (0);
        const bool saved = prism->saveEditedWavetable (tmpName);
        prism->stopEditing (0);
        check (saved, "a user wavetable was created to bind to");

        if (saved)
        {
            prism->selectUserWavetable (0, tmpName);
            check (prism->isUserTableActive (0) && prism->getActiveUserTableName (0) == tmpName,
                   "osc A is bound to the user table");

            const auto blob = cleanSessionBlob();

            prism->setStateInformation (blob.getData(), static_cast<int> (blob.getSize()));
            check (! prism->isUserTableActive (0) && prism->getActiveUserTableName (0).isEmpty(),
                   juce::String ("restoring a session with no override clears it (name now \"")
                       + prism->getActiveUserTableName (0) + "\")");

            // Legacy state: no "userWavetables" child at all.
            prism->selectUserWavetable (0, tmpName);
            check (prism->isUserTableActive (0), "re-bound for the legacy-state case");

            const auto legacy = stripUserWavetables (blob);
            check (legacy.getSize() > 0, "legacy blob (child stripped) was built");
            prism->setStateInformation (legacy.getData(), static_cast<int> (legacy.getSize()));
            check (! prism->isUserTableActive (0) && prism->getActiveUserTableName (0).isEmpty(),
                   "restoring a legacy state with no userWavetables child clears it too");

            prism->deleteUserWavetable (tmpName);
            check (prism->getActiveOscTable (0) != nullptr,
                   "osc A resolves to a factory table after cleanup");
        }
    }

    proc->releaseResources();

    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string (failures))
              << "\n";
    return failures;
}
