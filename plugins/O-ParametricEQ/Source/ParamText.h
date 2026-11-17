/*
   This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
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
    ParamText.h — host text for the float parameters (Stage 4, PLAN-4.1 P2).

    JUCE's default text for an interval-0 float is seven decimals and no unit
    (a host showed "B3 Freq 999.9998779"). These are the strings a host shows
    and the parsers for what a user types into it. Free functions on
    juce::String only, so the render harness gates them directly (HT1).

    The unit is IN the string, so no parameter carries withLabel (a host would
    print the unit twice). Text never changes a value: the layout golden
    (--dump-params) has no text column and stays byte-identical (HT1g).

    Parsers: leading / trailing space trimmed; a leading '+' or minus (ASCII or
    U+2212); a decimal point or comma; then an optional unit, any case:
    k / kHz (x 1000), Hz, dB, %. A string with no leading number returns the
    parameter's DEFAULT, never 0 or the minimum. JUCE clamps the result to
    the range (convertTo0to1).
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace opeq::paramtext
{
    /** Round to `dp` decimals, as the text will show it. */
    inline double roundTo (double v, int dp)
    {
        const double s = std::pow (10.0, dp);
        return std::round (v * s) / s;
    }

    /** "42.5 Hz" below 100 Hz, "440 Hz" below 1 kHz, "1.20 kHz" below 10 kHz,
        "12.0 kHz" above. Each threshold is taken on the value AS SHOWN, so
        999.7 reads "1.00 kHz", never "1000 Hz". */
    inline juce::String freqToText (float value)
    {
        const double v = (double) value;

        if (roundTo (v, 1) < 100.0)
            return juce::String (v, 1) + " Hz";

        // Integer formatting: String (double, 0) is the shortest repr, not 0 dp.
        if (roundTo (v, 0) < 1000.0)
            return juce::String ((juce::int64) std::llround (v)) + " Hz";

        const double k = v / 1000.0;

        if (roundTo (k, 2) < 10.0)
            return juce::String (k, 2) + " kHz";

        return juce::String (k, 1) + " kHz";
    }

    /** Signed, one decimal: "+3.0 dB", "-2.5 dB", "0.0 dB" (never "-0.0"). */
    inline juce::String dbToText (float value)
    {
        const double r = roundTo ((double) value, 1);

        if (std::abs (r) < 0.05)
            return "0.0 dB";

        return (r > 0.0 ? "+" : "-") + juce::String (std::abs (r), 1) + " dB";
    }

    /** Two decimals: "0.71", "30.00". */
    inline juce::String qToText (float value)
    {
        return juce::String ((double) value, 2);
    }

    /** No decimals: "100 %". */
    inline juce::String percentToText (float value)
    {
        return juce::String ((juce::int64) std::llround ((double) value)) + " %";
    }

    /** The leading number of `text` and what follows it. False when there is
        no digit before the unit. */
    inline bool splitNumber (const juce::String& text, double& number, juce::String& unit)
    {
        const auto s = text.trim().replace (juce::String::charToString ((juce::juce_wchar) 0x2212), "-");
        int i = 0;
        const int n = s.length();
        juce::String digits;

        if (i < n && (s[i] == '+' || s[i] == '-'))
        {
            if (s[i] == '-')
                digits << "-";
            ++i;
        }

        while (i < n && s[i] == ' ')
            ++i;

        bool anyDigit = false, seenPoint = false;

        for (; i < n; ++i)
        {
            const auto c = s[i];

            if (c >= '0' && c <= '9')
            {
                digits << juce::String::charToString (c);
                anyDigit = true;
            }
            else if ((c == '.' || c == ',') && ! seenPoint)
            {
                digits << ".";
                seenPoint = true;
            }
            else
            {
                break;
            }
        }

        if (! anyDigit)
            return false;

        number = digits.getDoubleValue();
        unit = s.substring (i).trim().toLowerCase();
        return true;
    }

    /** "440", "440 Hz", "1k", "1.2 kHz", "2.5K". */
    inline float textToFreq (const juce::String& text, float defaultValue)
    {
        double v = 0.0;
        juce::String unit;

        if (! splitNumber (text, v, unit))
            return defaultValue;

        if (unit.startsWith ("k"))
            v *= 1000.0;

        return (float) v;
    }

    /** "-3", "-3 dB", "+3", "+3dB". */
    inline float textToDb (const juce::String& text, float defaultValue)
    {
        double v = 0.0;
        juce::String unit;
        return splitNumber (text, v, unit) ? (float) v : defaultValue;
    }

    /** "0.7", "12". */
    inline float textToQ (const juce::String& text, float defaultValue)
    {
        double v = 0.0;
        juce::String unit;
        return splitNumber (text, v, unit) ? (float) v : defaultValue;
    }

    /** "150", "150%", "150 %". */
    inline float textToPercent (const juce::String& text, float defaultValue)
    {
        double v = 0.0;
        juce::String unit;
        return splitNumber (text, v, unit) ? (float) v : defaultValue;
    }

    /** Which text a float parameter uses, by its ID suffix. */
    enum class Kind { freq, db, q, percent };

    inline Kind kindForId (const juce::String& idOrSuffix)
    {
        if (idOrSuffix.endsWith ("freq"))       return Kind::freq;
        if (idOrSuffix.endsWith ("_q") || idOrSuffix == "q") return Kind::q;
        if (idOrSuffix.endsWith ("gain_scale")) return Kind::percent;
        return Kind::db;   // gain, output_gain
    }

    inline juce::String toText (Kind kind, float value)
    {
        switch (kind)
        {
            case Kind::freq:    return freqToText (value);
            case Kind::q:       return qToText (value);
            case Kind::percent: return percentToText (value);
            case Kind::db:      break;
        }

        return dbToText (value);
    }

    inline float fromText (Kind kind, const juce::String& text, float defaultValue)
    {
        switch (kind)
        {
            case Kind::freq:    return textToFreq (text, defaultValue);
            case Kind::q:       return textToQ (text, defaultValue);
            case Kind::percent: return textToPercent (text, defaultValue);
            case Kind::db:      break;
        }

        return textToDb (text, defaultValue);
    }

    /** The attributes a float parameter is built with: text both ways, no label. */
    inline juce::AudioParameterFloatAttributes attributesFor (Kind kind, float defaultValue)
    {
        return juce::AudioParameterFloatAttributes()
                   .withStringFromValueFunction ([kind] (float v, int) { return toText (kind, v); })
                   .withValueFromStringFunction ([kind, defaultValue] (const juce::String& t)
                                                 { return fromText (kind, t, defaultValue); });
    }
} // namespace opeq::paramtext
