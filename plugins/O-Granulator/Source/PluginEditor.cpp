/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

// Stage 1: the resource map only (see PluginEditor.h). Stage 3 adds the
// GranulatorEditor from .planning/mockups/v2-PluginEditor.cpp and points its
// withResourceProvider() at granulatorUiResource(). Render-harness consoles
// skip every file matching PluginEditor, so this never reaches a headless build.
#include "PluginEditor.h"
#include "UIBinaryData.h"

namespace
{
    juce::WebBrowserComponent::Resource makeBinaryResource (const char* data, int size, const juce::String& mime)
    {
        juce::WebBrowserComponent::Resource r;
        r.data.assign (reinterpret_cast<const std::byte*> (data), reinterpret_cast<const std::byte*> (data) + size);
        r.mimeType = mime;
        return r;
    }
}

std::optional<juce::WebBrowserComponent::Resource> granulatorUiResource (const juce::String& url)
{
    // Explicit map, no generic loop. charset=utf-8 on text: the page uses thin
    // spaces, minus signs and Han text.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    // The hover-help / label table, imported by the inline controller as
    // './js/i18n.js'. This branch, the juce_add_binary_data entry and the import
    // land together (check-i18n [8]).
    if (url == "/js/i18n.js")
        return makeBinaryResource (UIBinaryData::i18n_js, UIBinaryData::i18n_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared preset-manager JS, embedded from modules/ and served under /js/.
    // juce_add_binary_data STRIPS hyphens: presetmanager_js, not preset_manager_js.
    if (url == "/js/preset-manager.js")
        return makeBinaryResource (UIBinaryData::presetmanager_js, UIBinaryData::presetmanager_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared EB Garamond face (modules/ui/eb-garamond): stylesheet under /css/,
    // the three woff2 faces under /fonts/ where its relative font URLs land.
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource (UIBinaryData::ebgaramond_css, UIBinaryData::ebgaramond_cssSize,
                                   "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondRegular_woff2,
                                   UIBinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondItalic_woff2,
                                   UIBinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondBold_woff2,
                                   UIBinaryData::EBGaramondBold_woff2Size, "font/woff2");

    return std::nullopt;   // 404
}
