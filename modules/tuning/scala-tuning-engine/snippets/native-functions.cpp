/*
   This file is part of the Ouaricon Audio scala-tuning-engine module.
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
// ===== scala-tuning-engine - Native Function Registration =====
// Add these native functions to your PluginEditor WebView setup.
// These are called by tuning-panel.js to communicate with TuningEngine.

// In PluginEditor.cpp, add to your WebBrowserComponent options chain:

void YourEditor::setupTuningNativeFunctions()
{
    // ═══════════════════════════════════════════════════════════════════
    // TUNING INTERVALS
    // ═══════════════════════════════════════════════════════════════════

    // Get current scale intervals as JSON array
    webView->withNativeFunction("getTuningIntervals",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto intervals = processorRef.tuningEngine.getIntervals();
            juce::String json = "[";
            for (size_t i = 0; i < intervals.size(); ++i) {
                if (i > 0) json += ",";
                json += juce::String(intervals[i], 6);
            }
            json += "]";
            complete(json);
        });

    // Set all intervals from JSON array
    webView->withNativeFunction("setTuningIntervals",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                auto jsonArray = juce::JSON::parse(args[0].toString());
                if (auto* arr = jsonArray.getArray()) {
                    std::vector<double> intervals;
                    for (const auto& val : *arr) {
                        intervals.push_back(static_cast<double>(val));
                    }
                    processorRef.tuningEngine.setCustomIntervals(intervals, "Custom");
                    complete(true);
                    return;
                }
            }
            complete(false);
        });

    // Get tuning name
    webView->withNativeFunction("getTuningName",
        [this](const juce::Array<juce::var>& args, auto complete) {
            complete(processorRef.tuningEngine.getActiveTuningName());
        });

    // Set single interval by index
    webView->withNativeFunction("setSingleInterval",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                int index = static_cast<int>(args[0]);
                double cents = static_cast<double>(args[1]);
                processorRef.tuningEngine.setSingleInterval(index, cents);
                complete(true);
                return;
            }
            complete(false);
        });

    // Set single interval with encoded value (for URL-safe transport)
    webView->withNativeFunction("setSingleIntervalEncoded",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                int index = static_cast<int>(args[0]);
                double cents = static_cast<double>(args[1]);
                processorRef.tuningEngine.setSingleInterval(index, cents);
                complete(true);
                return;
            }
            complete(false);
        });

    // ═══════════════════════════════════════════════════════════════════
    // TONIC / ROTATION
    // ═══════════════════════════════════════════════════════════════════

    // Set tonic note (0-11)
    webView->withNativeFunction("setTonicNote",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                int tonic = static_cast<int>(args[0]);
                processorRef.tuningEngine.setTonicNote(tonic);
                complete(true);
                return;
            }
            complete(false);
        });

    // Get current tonic note
    webView->withNativeFunction("getTonicNote",
        [this](const juce::Array<juce::var>& args, auto complete) {
            complete(processorRef.tuningEngine.getTonicNote());
        });

    // ═══════════════════════════════════════════════════════════════════
    // OCTAVE STRETCH
    // ═══════════════════════════════════════════════════════════════════

    webView->withNativeFunction("getOctaveStretch",
        [this](const juce::Array<juce::var>& args, auto complete) {
            complete(processorRef.tuningEngine.getOctaveStretch());
        });

    webView->withNativeFunction("setOctaveStretch",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                float stretch = static_cast<float>(args[0]);
                processorRef.tuningEngine.setOctaveStretch(stretch);
                complete(true);
                return;
            }
            complete(false);
        });

    // ═══════════════════════════════════════════════════════════════════
    // MASTER TUNE (A4 Reference)
    // ═══════════════════════════════════════════════════════════════════

    webView->withNativeFunction("getMasterTune",
        [this](const juce::Array<juce::var>& args, auto complete) {
            complete(processorRef.tuningEngine.getMasterTune());
        });

    webView->withNativeFunction("setMasterTune",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                double hz = static_cast<double>(args[0]);
                processorRef.tuningEngine.setMasterTune(hz);
                complete(true);
                return;
            }
            complete(false);
        });

    // ═══════════════════════════════════════════════════════════════════
    // TEMPERAMENT PRESETS
    // ═══════════════════════════════════════════════════════════════════

    webView->withNativeFunction("setTemperamentPreset",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                int preset = static_cast<int>(args[0]);
                processorRef.tuningEngine.setBuiltInPreset(
                    static_cast<TuningEngine::BuiltInPreset>(preset));
                complete(true);
                return;
            }
            complete(false);
        });

    webView->withNativeFunction("getTemperamentPreset",
        [this](const juce::Array<juce::var>& args, auto complete) {
            complete(static_cast<int>(processorRef.tuningEngine.getBuiltInPreset()));
        });

    // ═══════════════════════════════════════════════════════════════════
    // SCALA FILE I/O
    // ═══════════════════════════════════════════════════════════════════

    // Load .scl file via file chooser
    webView->withNativeFunction("loadScalaFile",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Load Scala File",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                "*.scl");

            chooser->launchAsync(juce::FileBrowserComponent::openMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file.existsAsFile()) {
                        bool success = processorRef.tuningEngine.loadScalaFile(file);
                        complete(success ? processorRef.tuningEngine.getActiveTuningName()
                                        : juce::var());
                    } else {
                        complete(juce::var());
                    }
                });
        });

    // Save .scl file via file chooser
    webView->withNativeFunction("saveScalaFile",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Save Scala File",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("scale.scl"),
                "*.scl");

            chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File()) {
                        auto content = processorRef.tuningEngine.generateScalaFileContent();
                        file.replaceWithText(content);
                        complete(true);
                    } else {
                        complete(false);
                    }
                });
        });

    // Load .kbm file via file chooser
    webView->withNativeFunction("loadKBMFile",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Load Keyboard Mapping",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                "*.kbm");

            chooser->launchAsync(juce::FileBrowserComponent::openMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file.existsAsFile()) {
                        bool success = processorRef.tuningEngine.loadKBMFile(file);
                        complete(success);
                    } else {
                        complete(false);
                    }
                });
        });

    // Save .kbm file via file chooser
    webView->withNativeFunction("saveKBMFile",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Save Keyboard Mapping",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("mapping.kbm"),
                "*.kbm");

            chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File()) {
                        auto content = processorRef.tuningEngine.generateKBMFileContent();
                        file.replaceWithText(content);
                        complete(true);
                    } else {
                        complete(false);
                    }
                });
        });

    // ═══════════════════════════════════════════════════════════════════
    // SCALE GENERATOR
    // ═══════════════════════════════════════════════════════════════════

    // Generate EDO scale
    webView->withNativeFunction("generateEDO",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                int divisions = static_cast<int>(args[0]);
                double period = static_cast<double>(args[1]);
                auto intervals = processorRef.scaleGenerator.generateEDO(divisions, period);

                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i) {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(json);
                return;
            }
            complete(juce::var());
        });

    // Generate harmonic series scale
    webView->withNativeFunction("generateHarmonicSeries",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                int startHarmonic = static_cast<int>(args[0]);
                int endHarmonic = static_cast<int>(args[1]);
                auto intervals = processorRef.scaleGenerator.generateHarmonicSeries(
                    startHarmonic, endHarmonic);

                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i) {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(json);
                return;
            }
            complete(juce::var());
        });

    // Generate rank-2 temperament
    webView->withNativeFunction("generateRank2",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 3) {
                double generator = static_cast<double>(args[0]);
                double period = static_cast<double>(args[1]);
                int count = static_cast<int>(args[2]);
                auto intervals = processorRef.scaleGenerator.generateRank2(
                    generator, period, count);

                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i) {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(json);
                return;
            }
            complete(juce::var());
        });

    // Apply generated scale
    webView->withNativeFunction("applyGeneratedScale",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                auto jsonArray = juce::JSON::parse(args[0].toString());
                juce::String scaleName = args[1].toString();

                if (auto* arr = jsonArray.getArray()) {
                    std::vector<double> intervals;
                    for (const auto& val : *arr) {
                        intervals.push_back(static_cast<double>(val));
                    }
                    processorRef.tuningEngine.setCustomIntervals(intervals, scaleName);
                    complete(true);
                    return;
                }
            }
            complete(false);
        });

    // ═══════════════════════════════════════════════════════════════════
    // EMBEDDED TUNING LIBRARY
    // ═══════════════════════════════════════════════════════════════════

    // Get list of embedded tunings as JSON
    webView->withNativeFunction("getEmbeddedTuningList",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto tunings = EmbeddedTunings::getTuningList();
            juce::String json = "[";
            for (size_t i = 0; i < tunings.size(); ++i) {
                if (i > 0) json += ",";
                json += "{";
                json += "\"id\":\"" + tunings[i].id + "\",";
                json += "\"name\":\"" + tunings[i].name + "\",";
                json += "\"category\":\"" + tunings[i].category + "\",";
                json += "\"noteCount\":" + juce::String(tunings[i].noteCount);
                json += "}";
            }
            json += "]";
            complete(json);
        });

    // Get tuning categories
    webView->withNativeFunction("getEmbeddedTuningCategories",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto categories = EmbeddedTunings::getCategories();
            juce::String json = "[";
            for (size_t i = 0; i < categories.size(); ++i) {
                if (i > 0) json += ",";
                json += "\"" + categories[i] + "\"";
            }
            json += "]";
            complete(json);
        });

    // Load embedded tuning by ID
    webView->withNativeFunction("loadEmbeddedTuning",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                juce::String tuningId = args[0].toString();
                auto tuningData = EmbeddedTunings::getTuning(tuningId);

                if (tuningData.intervals.size() > 0) {
                    processorRef.tuningEngine.setCustomIntervals(
                        tuningData.intervals, tuningData.name);
                    complete(true);
                    return;
                }
            }
            complete(false);
        });

    // ═══════════════════════════════════════════════════════════════════
    // HTML EXPORT
    // ═══════════════════════════════════════════════════════════════════

    webView->withNativeFunction("exportTuningHTML",
        [this](const juce::Array<juce::var>& args, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Export Tuning Documentation",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("tuning-export.html"),
                "*.html");

            chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File()) {
                        auto intervals = processorRef.tuningEngine.getIntervals();
                        auto name = processorRef.tuningEngine.getActiveTuningName();
                        auto html = processorRef.tuningExporter.generateHTML(intervals, name);
                        file.replaceWithText(html);
                        complete(true);
                    } else {
                        complete(false);
                    }
                });
        });
}
