/*
   This file is part of O-IntonationPad, an Ouaricon Audio plugin.
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

    JsonHelper - Lightweight JSON string builders for WebView communication
    O-IntonationPad / Ouaricon Development

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <string>

namespace JsonHelper
{

// --- String escaping for JSON safety ---

static inline juce::String escape(const juce::String& s)
{
    return s.replace("\\", "\\\\").replace("\"", "\\\"");
}

// --- Array serializers ---

inline juce::String arrayToJSON(const std::vector<double>& v, int precision = 6)
{
    juce::String json = "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) json += ",";
        json += juce::String(v[i], precision);
    }
    json += "]";
    return json;
}

inline juce::String arrayToJSON(const std::vector<bool>& v)
{
    juce::String json = "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) json += ",";
        json += v[i] ? "true" : "false";
    }
    json += "]";
    return json;
}

inline juce::String arrayToJSON(const std::vector<std::string>& v)
{
    juce::String json = "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) json += ",";
        json += "\"" + escape(juce::String(v[i])) + "\"";
    }
    json += "]";
    return json;
}

// --- Object builder with chained .add() ---

class JsonObjectBuilder
{
public:
    JsonObjectBuilder& add(const char* key, int value)
    {
        appendSep();
        json += "\"" + juce::String(key) + "\":" + juce::String(value);
        return *this;
    }

    JsonObjectBuilder& add(const char* key, double value, int precision = 2)
    {
        appendSep();
        json += "\"" + juce::String(key) + "\":" + juce::String(value, precision);
        return *this;
    }

    JsonObjectBuilder& add(const char* key, const juce::String& value)
    {
        appendSep();
        json += "\"" + juce::String(key) + "\":\"" + escape(value) + "\"";
        return *this;
    }

    juce::String build() const { return "{" + json + "}"; }

private:
    void appendSep() { if (json.isNotEmpty()) json += ","; }
    juce::String json;
};

// --- Array-of-objects builder ---

class JsonArrayBuilder
{
public:
    JsonArrayBuilder& add(const JsonObjectBuilder& obj)
    {
        if (json.isNotEmpty()) json += ",";
        json += obj.build();
        return *this;
    }

    juce::String build() const { return "[" + json + "]"; }

private:
    juce::String json;
};

} // namespace JsonHelper
