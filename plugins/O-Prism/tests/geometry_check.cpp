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

    geometry_check.cpp — O-Prism v1.25.0 Geometry bank + preset-migration gate.

    Drives the REAL processor (createPluginFilter(), no editor) and checks:

      1. Catalogue: 36 factory tables; 28..35 are category "Geometry", the
         rest are not; every name is <= 16 characters.
      2. Embed round-trip: each Geometry table's level-0 frames are finite,
         DC-free (|mean| < 1e-3 per frame), globally peak-normalised to 1.0
         (within one int16 step), and their int16 re-encoding hashes to the
         SHA-256 recorded by the baker in manifest.json — the header in the
         binary is the header the baker wrote, byte for byte.
      3. Pitch: a C4 note on each Geometry table renders finite, non-silent,
         at 261.63 Hz ± 1 cent (autocorrelation, parabolic interpolation),
         with harmonic 1 within 6 dB of the strongest partial (FFT). This is
         the gate the O-Strata critique found missing: a centred orbit plays
         a twelfth or an octave up and every other metric looks fine.
      4. Migration: a preset saved under the 0..27 range — version "1.0.0"
         (what the pre-1.25.0 writer stamped) or no version at all — re-decodes
         to the SAME table name; a 1.25.0 preset is left alone. Negative
         control: the un-migrated index is asserted to differ, so a hook that
         silently stops firing cannot pass.
      5. Factory presets: every one FactoryPresets::build() returns (192 at
         v1.25.0) loads with the oscillator tables it was authored with.

    Usage:  O-Prism-geometry-check <path/to/manifest.json>
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include <juce_cryptography/juce_cryptography.h>   // not in the plugin's JuceHeader; linked by ouaricon_add_processor_console
#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "dsp/WavetableFactory.h"

#include <cmath>
#include <iostream>
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

constexpr int    kFirstGeometry  = WavetableFactory::kFirstGeometryTable;   // 28
constexpr int    kNumTables      = WavetableFactory::kNumFactoryTables;     // 36
constexpr double kSampleRate     = 48000.0;
constexpr int    kBlockSize      = 512;
constexpr double kC4Hz           = 261.6255653;   // MIDI 60, 12-TET, A4 = 440

// ─── Pitch analysis ───────────────────────────────────────────────────────

/** Fundamental period via normalised autocorrelation, parabolic-interpolated,
    searched ±20 % around the expected lag. Sub-sample accuracy is what a
    ±1 cent verdict at 261 Hz needs (1 cent = 0.11 samples at 48 kHz). */
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

/** dB of the spectral peak nearest `hz` relative to the strongest peak
    anywhere above 20 Hz (Hann window, 65536-point FFT). */
double harmonicOneRelativeDb (const std::vector<float>& x, double hz)
{
    constexpr int order = 16;
    constexpr int size  = 1 << order;
    jassert (static_cast<int> (x.size()) >= size);

    juce::dsp::FFT fft (order);
    std::vector<float> buf (static_cast<size_t> (size) * 2, 0.0f);
    const int start = static_cast<int> (x.size()) - size;
    for (int i = 0; i < size; ++i)
    {
        const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * static_cast<float> (i) / static_cast<float> (size));
        buf[static_cast<size_t> (i)] = x[static_cast<size_t> (start + i)] * w;
    }
    fft.performFrequencyOnlyForwardTransform (buf.data());

    const double binHz = kSampleRate / size;
    const int minBin = static_cast<int> (20.0 / binHz);
    float maxMag = 0.0f;
    for (int b = minBin; b < size / 2; ++b)
        maxMag = std::max (maxMag, buf[static_cast<size_t> (b)]);

    const int h1Bin = static_cast<int> (std::round (hz / binHz));
    float h1Mag = 0.0f;
    for (int b = h1Bin - 3; b <= h1Bin + 3; ++b)
        h1Mag = std::max (h1Mag, buf[static_cast<size_t> (b)]);

    return 20.0 * std::log10 ((h1Mag + 1e-12f) / (maxMag + 1e-12f));
}

// ─── Rendering ────────────────────────────────────────────────────────────

std::vector<float> renderNote (juce::AudioProcessor& proc, double seconds)
{
    const int numBlocks = static_cast<int> (std::ceil (seconds * kSampleRate / kBlockSize));
    juce::AudioBuffer<float> buf (proc.getTotalNumOutputChannels(), kBlockSize);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);

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

/** Release the note and run the tail out so the next render starts from
    silence. Returns the RMS of the final block — asserted below. */
float silenceNote (juce::AudioProcessor& proc, double seconds)
{
    const int numBlocks = static_cast<int> (std::ceil (seconds * kSampleRate / kBlockSize));
    juce::AudioBuffer<float> buf (proc.getTotalNumOutputChannels(), kBlockSize);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
    midi.addEvent (juce::MidiMessage::allNotesOff (1), 1);
    for (int b = 0; b < numBlocks; ++b)
    {
        buf.clear();
        proc.processBlock (buf, midi);
        midi.clear();
    }
    return buf.getRMSLevel (0, 0, kBlockSize);
}

juce::File writeTempPreset (const juce::String& name, const juce::String& json)
{
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (name + ".json");
    f.replaceWithText (json);
    return f;
}

int rawTable (OPrismAudioProcessor& proc, const char* id)
{
    return juce::roundToInt (proc.getAPVTS().getRawParameterValue (id)->load());
}

} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 2)
    {
        std::cerr << "usage: O-Prism-geometry-check <manifest.json>\n";
        return 99;
    }

    const juce::File manifestFile (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    auto manifest = juce::JSON::parse (manifestFile.loadFileAsString());
    auto* manifestTables = manifest.getProperty ("tables", {}).getArray();
    if (manifestTables == nullptr)
    {
        std::cerr << "geometry-check: could not read tables[] from " << manifestFile.getFullPathName() << "\n";
        return 98;
    }

    std::unique_ptr<juce::AudioProcessor> base { createPluginFilter() };
    auto* proc = dynamic_cast<OPrismAudioProcessor*> (base.get());
    if (proc == nullptr)
    {
        std::cerr << "geometry-check: createPluginFilter() did not return an OPrismAudioProcessor\n";
        return 97;
    }

    auto& apvts = proc->getAPVTS();
    auto& pm    = proc->getPresetManager();

    // ── 1. Catalogue ──────────────────────────────────────────────────────
    std::cout << "\n[1] catalogue\n";
    check (proc->getNumFactoryTables() == kNumTables, "getNumFactoryTables() == 36 (got " + juce::String (proc->getNumFactoryTables()) + ")");
    check (manifestTables->size() == kNumTables - kFirstGeometry, "manifest lists 8 tables");
    for (int i = 0; i < kNumTables; ++i)
    {
        const bool geo = i >= kFirstGeometry;
        check ((proc->getTableCategory (i) == "Geometry") == geo,
               "table " + juce::String (i) + " \"" + proc->getTableName (i) + "\" category "
                   + proc->getTableCategory (i) + (geo ? " (expected Geometry)" : " (expected not Geometry)"));
        if (geo)
            check (proc->getTableName (i).length() <= 16, "  name <= 16 chars");
    }

    // ── 2. Embed round-trip ───────────────────────────────────────────────
    std::cout << "\n[2] embed round-trip vs manifest\n";
    for (int i = kFirstGeometry; i < kNumTables; ++i)
    {
        const auto* table = proc->getFactoryTable (i);
        const auto& entry = (*manifestTables)[i - kFirstGeometry];
        const juce::String name = proc->getTableName (i);
        check (table != nullptr, name + ": table present");
        if (table == nullptr) continue;

        check (entry.getProperty ("name", {}).toString() == name, name + ": manifest name matches");
        check (static_cast<int> (entry.getProperty ("frames", 0)) == table->numFrames,
               name + ": frames " + juce::String (table->numFrames) + " == manifest");

        std::vector<juce::int16> q;
        q.reserve (static_cast<size_t> (table->numFrames) * WavetableData::kTableSize);
        bool finite = true, dcFree = true;
        float peak = 0.0f;
        for (int f = 0; f < table->numFrames; ++f)
        {
            const float* frame = table->getFrameData (0, f);
            double mean = 0.0;
            for (int s = 0; s < WavetableData::kTableSize; ++s)
            {
                const float v = frame[s];
                finite = finite && std::isfinite (v);
                peak = std::max (peak, std::abs (v));
                mean += v;
                q.push_back (static_cast<juce::int16> (juce::roundToInt (v * 32767.0f)));
            }
            dcFree = dcFree && std::abs (mean / WavetableData::kTableSize) < 1e-3;
            // guard sample: WavetableData wraps sample 0 for interpolation
            if (std::abs (frame[WavetableData::kTableSize] - frame[0]) > 1e-9f)
                check (false, name + ": frame " + juce::String (f) + " guard sample != sample 0");
        }
        check (finite, name + ": all level-0 samples finite");
        check (dcFree, name + ": every frame |DC| < 1e-3");
        check (std::abs (peak - 1.0f) <= 1.0f / 32767.0f, name + ": global peak " + juce::String (peak, 6) + " == 1.0");

        // little-endian int16 bytes, as the baker hashed them
        juce::MemoryBlock bytes;
        for (auto v : q)
        {
            const auto u = static_cast<juce::uint16> (v);
            const juce::uint8 le[2] = { static_cast<juce::uint8> (u & 0xff), static_cast<juce::uint8> (u >> 8) };
            bytes.append (le, 2);
        }
        const juce::String sha = juce::SHA256 (bytes.getData(), bytes.getSize()).toHexString();
        const juce::String expected = entry.getProperty ("sha256_int16_le", {}).toString();
        check (sha == expected, name + ": SHA-256 " + sha.substring (0, 16) + " == manifest " + expected.substring (0, 16));
    }

    // ── 3. Pitch on a rendered C4 ─────────────────────────────────────────
    std::cout << "\n[3] rendered C4 per Geometry table\n";
    proc->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
    proc->prepareToPlay (kSampleRate, kBlockSize);
    auto* tableParam = apvts.getParameter ("oscATable");
    check (tableParam != nullptr, "oscATable parameter exists");
    check (tableParam != nullptr && tableParam->getNumSteps() == kNumTables, "oscATable has 36 steps");

    for (int i = kFirstGeometry; i < kNumTables; ++i)
    {
        const juce::String name = proc->getTableName (i);
        tableParam->setValueNotifyingHost (static_cast<float> (i) / static_cast<float> (kNumTables - 1));
        check (rawTable (*proc, "oscATable") == i, name + ": oscATable reads back " + juce::String (i));

        auto y = renderNote (*proc, 3.0);
        const std::vector<float> tail (y.end() - 65536, y.end());

        bool finite = true; double sumSq = 0.0;
        for (float v : tail) { finite = finite && std::isfinite (v); sumSq += static_cast<double> (v) * v; }
        const double rms = std::sqrt (sumSq / static_cast<double> (tail.size()));
        check (finite, name + ": render finite");
        check (rms > 0.01, name + ": render non-silent (RMS " + juce::String (rms, 4) + ")");

        const double period = measurePeriodSamples (tail, kSampleRate / kC4Hz);
        const double hz = kSampleRate / period;
        const double cents = 1200.0 * std::log2 (hz / kC4Hz);
        check (std::abs (cents) <= 1.0, name + ": f0 " + juce::String (hz, 3) + " Hz (" + juce::String (cents, 2) + " cents from C4)");

        const double h1 = harmonicOneRelativeDb (tail, kC4Hz);
        check (h1 >= -6.0, name + ": harmonic 1 at " + juce::String (h1, 1) + " dB re strongest partial (>= -6)");

        const float tailRms = silenceNote (*proc, 2.0);
        check (tailRms < 1e-3f, name + ": silent again after release (RMS " + juce::String (tailRms, 6) + ")");
    }
    proc->releaseResources();

    // ── 4. Preset migration ───────────────────────────────────────────────
    std::cout << "\n[4] preset migration (0..27 -> 0..35)\n";
    const int vowelMorph = 12, filteredNoise = 27, gyroidOrbit = 33;
    auto legacyValue = [] (int idx) { return juce::String (static_cast<double> (idx) / 27.0, 10); };
    auto newValue    = [] (int idx) { return juce::String (static_cast<double> (idx) / 35.0, 10); };

    // The un-migrated re-decode of 12/27 against 0..35 — must NOT be what we read.
    const int unmigrated = juce::roundToInt (static_cast<float> (vowelMorph) / 27.0f * 35.0f);
    check (unmigrated != vowelMorph, "negative control: 12/27 re-decoded raw against 0..35 is " + juce::String (unmigrated) + ", not 12");

    {
        auto f = writeTempPreset ("oprism-geocheck-legacy-1.0.0",
            "{\"parameters\":{\"oscATable\":" + legacyValue (vowelMorph) + ",\"oscBTable\":" + legacyValue (filteredNoise)
            + "},\"version\":\"1.0.0\",\"plugin\":\"O-Prism\"}");
        check (pm.loadPresetFromFile (f), "legacy preset (version \"1.0.0\") loads");
        check (rawTable (*proc, "oscATable") == vowelMorph && proc->getTableName (rawTable (*proc, "oscATable")) == "Vowel Morph",
               "  oscATable -> " + proc->getTableName (rawTable (*proc, "oscATable")) + " (expected Vowel Morph)");
        check (rawTable (*proc, "oscBTable") == filteredNoise && proc->getTableName (rawTable (*proc, "oscBTable")) == "Filtered Noise",
               "  oscBTable -> " + proc->getTableName (rawTable (*proc, "oscBTable")) + " (expected Filtered Noise)");
        f.deleteFile();
    }
    {
        auto f = writeTempPreset ("oprism-geocheck-legacy-noversion",
            "{\"parameters\":{\"oscATable\":" + legacyValue (vowelMorph) + "},\"plugin\":\"O-Prism\"}");
        check (pm.loadPresetFromFile (f), "legacy preset (no version) loads");
        check (proc->getTableName (rawTable (*proc, "oscATable")) == "Vowel Morph",
               "  oscATable -> " + proc->getTableName (rawTable (*proc, "oscATable")) + " (expected Vowel Morph)");
        f.deleteFile();
    }
    {
        auto f = writeTempPreset ("oprism-geocheck-1.25.0",
            "{\"parameters\":{\"oscATable\":" + newValue (gyroidOrbit) + "},\"version\":\"1.25.0\",\"plugin\":\"O-Prism\"}");
        check (pm.loadPresetFromFile (f), "1.25.0 preset loads");
        check (rawTable (*proc, "oscATable") == gyroidOrbit && proc->getTableName (gyroidOrbit) == "Gyroid Orbit",
               "  oscATable -> " + proc->getTableName (rawTable (*proc, "oscATable")) + " (expected Gyroid Orbit, untouched)");
        f.deleteFile();
    }
    {
        // Round trip through the CURRENT writer: what savePreset stamps must
        // not be treated as legacy on the way back in.
        tableParam->setValueNotifyingHost (static_cast<float> (gyroidOrbit) / 35.0f);
        check (pm.savePreset ("oprism-geocheck-roundtrip"), "savePreset under 1.25.0");
        tableParam->setValueNotifyingHost (0.0f);
        check (pm.loadPreset ("oprism-geocheck-roundtrip"), "loadPreset of that save");
        check (proc->getTableName (rawTable (*proc, "oscATable")) == "Gyroid Orbit",
               "  oscATable -> " + proc->getTableName (rawTable (*proc, "oscATable")) + " (expected Gyroid Orbit)");
        pm.deletePreset ("oprism-geocheck-roundtrip");
    }

    // ── 5. Factory presets ────────────────────────────────────────────────
    std::cout << "\n[5] factory presets load with their authored tables\n";
    const auto defs = FactoryPresets::build (apvts);
    // FactoryPresets.cpp's banner says 96 and jasserts it in Debug; the
    // library has grown to 192 since. Report what build() returns; the gate
    // is that EVERY def loads with its authored tables, not the count.
    check (defs.size() >= 96, juce::String (static_cast<int> (defs.size())) + " factory preset defs (>= 96)");
    int nonZero = 0, checked = 0, wrong = 0;
    for (const auto& def : defs)
    {
        if (! pm.loadPresetFromCategory (def.category, def.name))
        {
            check (false, def.category + "/" + def.name + ": loads from disk");
            continue;
        }
        for (const char* id : { "oscATable", "oscBTable" })
        {
            auto it = def.parameters.find (id);
            const int expected = it == def.parameters.end() ? 0 : juce::roundToInt (it->second * 35.0f);
            const int actual = rawTable (*proc, id);
            ++checked;
            if (expected != 0) ++nonZero;
            if (expected != actual)
            {
                ++wrong;
                check (false, def.category + "/" + def.name + " " + id + ": " + proc->getTableName (actual)
                                  + " != authored " + proc->getTableName (expected));
            }
        }
    }
    check (wrong == 0, juce::String (checked) + " table slots across " + juce::String (static_cast<int> (defs.size()))
                           + " presets match (" + juce::String (nonZero) + " non-Saw)");
    check (nonZero >= 3, "at least three presets exercise a non-zero table");

    std::cout << "\n" << (failures == 0 ? "PASS" : "FAIL") << " — " << failures << " failed check(s)\n";
    return failures;
}
