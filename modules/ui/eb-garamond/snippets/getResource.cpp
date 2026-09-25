/*
   This file is part of the Ouaricon Audio eb-garamond module.
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
// COPY-PASTE SNIPPET — not compiled (OuariconModules.cmake globs cpp/ only).
//
// Paste these four branches into the plugin editor's getResource(), just
// before its final `return std::nullopt;`. It assumes:
//   - the plugin's own helper
//       makeBinaryResource (const char* data, int size, const char* mimeType)
//     returning std::optional<juce::WebBrowserComponent::Resource>;
//   - the plugin's single UI binary-data target uses NAMESPACE UIBinaryData
//     (rename the namespace if the plugin's target uses another one).
//
// Keep the literal one-branch-per-URL shape: scripts/serve-ui.js pairs each
// URL literal with the first binary-data symbol within ~400 characters, and a
// table or loop breaks the headless gates' tree placement.
//
// Symbols: juce_add_binary_data STRIPS hyphens (never converts them to
// underscores), so EBGaramond-Regular.woff2 is EBGaramondRegular_woff2.

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
