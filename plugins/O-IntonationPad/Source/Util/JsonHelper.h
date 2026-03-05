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
