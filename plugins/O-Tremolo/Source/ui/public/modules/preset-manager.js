/*
   This file is part of the Ouaricon Audio preset-manager module.
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
/**
 * Ouaricon Preset Manager - JavaScript UI Module
 *
 * Provides preset selection, save, load, and navigation functionality.
 * Works with the C++ OuariconPresetManager via JUCE 8 native functions.
 *
 * Required C++ Native Functions (register in PluginEditor.cpp):
 *   - savePreset(name)           : Save preset with given name
 *   - savePresetWithDialog()     : Open native save dialog, returns {success, name}
 *   - loadPreset(name)           : Load preset by name
 *   - loadPresetFromFile()       : Open native load dialog, returns {success, name}
 *   - getPresetList()            : Get array of preset names
 *   - getCurrentPreset()         : Get current preset name
 *   - selectNextPreset()         : Get next preset name
 *   - selectPreviousPreset()     : Get previous preset name
 *   - deletePreset(name)         : Delete a user preset
 *   - isFactoryPreset(name)      : Check if preset is factory (read-only)
 *
 * Usage:
 *   import { PresetManager } from './modules/preset-manager.js';
 *   import { getNativeFunction } from './js/juce/index.js';
 *
 *   const presets = new PresetManager({
 *     displayElement: document.getElementById('preset-name'),
 *     prevButton: document.getElementById('prev-preset'),
 *     nextButton: document.getElementById('next-preset'),
 *     saveButton: document.getElementById('save-preset'),   // Uses native save dialog
 *     loadButton: document.getElementById('load-preset'),   // Uses native load dialog
 *     getNativeFunction: getNativeFunction,  // JUCE 8 native function factory
 *     onPresetChanged: (name) => console.log('Loaded:', name)
 *   });
 *
 *   presets.initialize();
 */

export class PresetManager {
    constructor(options = {}) {
        this.displayElement = options.displayElement;
        this.prevButton = options.prevButton;
        this.nextButton = options.nextButton;
        this.saveButton = options.saveButton;
        this.loadButton = options.loadButton;
        this.deleteButton = options.deleteButton;
        this.menuButton = options.menuButton;

        // Optional confirmation hook for destructive actions. window.confirm() is
        // unreliable (silent no-op or throw) in some JUCE WebView backends, so callers
        // can supply a reliable native / in-DOM dialog here:
        //   onConfirmDelete: (presetName, message) => Promise<boolean> | boolean
        this.onConfirmDelete = options.onConfirmDelete || null;

        // JUCE 8 native function factory (required)
        this.getNativeFunction = options.getNativeFunction;

        // Callbacks
        this.onPresetChanged = options.onPresetChanged || (() => {});
        this.onPresetListUpdated = options.onPresetListUpdated || (() => {});

        // State
        this.currentPreset = 'Default';
        this.presetList = [];
        this.isInitialized = false;

        // Native function references (populated during initialize)
        this._savePreset = null;
        this._savePresetWithDialog = null;
        this._loadPreset = null;
        this._loadPresetFromFile = null;
        this._getPresetList = null;
        this._getCurrentPreset = null;
        this._selectNextPreset = null;
        this._selectPreviousPreset = null;
        this._deletePreset = null;
        this._isFactoryPreset = null;
    }

    /**
     * Initialize the preset manager.
     * Fetches current preset and list from C++.
     */
    async initialize() {
        if (this.isInitialized) return;

        // Wait for JUCE native integration
        await this._waitForNative();

        // Get native function references using JUCE 8 pattern
        if (this.getNativeFunction) {
            this._savePreset = this.getNativeFunction("savePreset");
            this._savePresetWithDialog = this.getNativeFunction("savePresetWithDialog");
            this._loadPreset = this.getNativeFunction("loadPreset");
            this._loadPresetFromFile = this.getNativeFunction("loadPresetFromFile");
            this._getPresetList = this.getNativeFunction("getPresetList");
            this._getCurrentPreset = this.getNativeFunction("getCurrentPreset");
            this._selectNextPreset = this.getNativeFunction("selectNextPreset");
            this._selectPreviousPreset = this.getNativeFunction("selectPreviousPreset");
            this._deletePreset = this.getNativeFunction("deletePreset");
            this._isFactoryPreset = this.getNativeFunction("isFactoryPreset");
        } else {
            console.error('[PresetManager] getNativeFunction not provided - native functions unavailable');
            return;
        }

        // Bind navigation buttons
        if (this.prevButton) {
            this.prevButton.addEventListener('click', () => this.selectPrevious());
        }
        if (this.nextButton) {
            this.nextButton.addEventListener('click', () => this.selectNext());
        }
        if (this.saveButton) {
            this.saveButton.addEventListener('click', () => this.saveWithDialog());
        }
        if (this.loadButton) {
            this.loadButton.addEventListener('click', () => this.loadFromFile());
        }
        if (this.deleteButton) {
            this.deleteButton.addEventListener('click', () => this.promptDelete());
        }

        // Load initial state
        await this.refresh();

        this.isInitialized = true;
    }

    /**
     * Wait for JUCE native integration to be available.
     */
    async _waitForNative(maxAttempts = 100, intervalMs = 50) {
        return new Promise((resolve) => {
            let attempts = 0;
            const check = () => {
                if (window.__JUCE__ && window.__JUCE__.backend) {
                    resolve();
                } else if (++attempts >= maxAttempts) {
                    // Bound the poll so a missing backend surfaces instead of hanging
                    // initialize() forever. Resolve anyway — downstream native calls are
                    // already guarded by try/catch and will report their own failures.
                    console.error(`[PresetManager] JUCE backend unavailable after ` +
                        `${(maxAttempts * intervalMs) / 1000}s — preset UI may be non-functional`);
                    resolve();
                } else {
                    setTimeout(check, intervalMs);
                }
            };
            check();
        });
    }

    /**
     * Refresh preset list and current preset from C++.
     */
    async refresh() {
        try {
            // Get current preset name
            const current = await this._getCurrentPreset();
            this.currentPreset = current || 'Default';

            // Get preset list
            const list = await this._getPresetList();
            this.presetList = list || [];

            // Update display
            this._updateDisplay();

            this.onPresetListUpdated(this.presetList);
        } catch (e) {
            console.error('[PresetManager] Refresh failed:', e);
        }
    }

    /**
     * Load a preset by name.
     */
    async loadPreset(name) {
        try {
            const success = await this._loadPreset(name);
            if (success) {
                this.currentPreset = name;
                this._updateDisplay();
                this.onPresetChanged(name);
            }
            return success;
        } catch (e) {
            console.error('[PresetManager] Load failed:', e);
            return false;
        }
    }

    /**
     * Select the next preset in the list.
     */
    async selectNext() {
        try {
            const nextName = await this._selectNextPreset();
            if (nextName) {
                const success = await this.loadPreset(nextName);
                return success;
            }
        } catch (e) {
            console.error('[PresetManager] selectNext failed:', e);
        }
        return false;
    }

    /**
     * Select the previous preset in the list.
     */
    async selectPrevious() {
        try {
            const prevName = await this._selectPreviousPreset();
            if (prevName) {
                const success = await this.loadPreset(prevName);
                return success;
            }
        } catch (e) {
            console.error('[PresetManager] selectPrevious failed:', e);
        }
        return false;
    }

    /**
     * Save current state as a preset with given name.
     * For programmatic saves (e.g., auto-save, overwrite).
     */
    async savePreset(name) {
        if (!name || name.trim() === '') {
            console.warn('[PresetManager] Empty preset name');
            return false;
        }

        try {
            const success = await this._savePreset(name.trim());
            if (success) {
                this.currentPreset = name.trim();
                await this.refresh();
                this.onPresetChanged(this.currentPreset);
            }
            return success;
        } catch (e) {
            console.error('[PresetManager] Save failed:', e);
            return false;
        }
    }

    /**
     * Save preset using native system file dialog.
     * Opens macOS/Windows save dialog in User presets folder.
     * Returns {success: boolean, name: string}.
     */
    async saveWithDialog() {
        try {
            const result = await this._savePresetWithDialog();
            if (result && result.success) {
                this.currentPreset = result.name;
                this._updateDisplay();
                await this.refresh();
                this.onPresetChanged(this.currentPreset);
                return result;
            }
            return { success: false, name: '' };
        } catch (e) {
            console.error('[PresetManager] saveWithDialog failed:', e);
            return { success: false, name: '' };
        }
    }

    /**
     * Load a preset from file using native system file dialog.
     * Opens macOS/Windows open dialog for JSON preset files.
     * Returns {success: boolean, name: string}.
     */
    async loadFromFile() {
        try {
            const result = await this._loadPresetFromFile();
            if (result && result.success) {
                this.currentPreset = result.name || 'Loaded Preset';
                this._updateDisplay();
                await this.refresh();
                this.onPresetChanged(this.currentPreset);
                return result;
            }
            return { success: false, name: '' };
        } catch (e) {
            console.error('[PresetManager] loadFromFile failed:', e);
            return { success: false, name: '' };
        }
    }

    /**
     * Delete a preset by name.
     * Factory presets cannot be deleted.
     */
    async deletePreset(name) {
        try {
            const isFactory = await this._isFactoryPreset(name);
            if (isFactory) {
                console.warn('[PresetManager] Cannot delete factory presets');
                return false;
            }

            const success = await this._deletePreset(name);
            if (success) {
                await this.refresh();
            }
            return success;
        } catch (e) {
            console.error('[PresetManager] Delete failed:', e);
            return false;
        }
    }

    /**
     * Check if a preset is a factory preset (read-only).
     */
    async isFactoryPreset(name) {
        try {
            return await this._isFactoryPreset(name);
        } catch (e) {
            console.error('[PresetManager] isFactoryPreset failed:', e);
            return false;
        }
    }

    /**
     * Prompt the user to delete the current preset, then delete it if confirmed.
     * Prefers options.onConfirmDelete (a reliable native / in-DOM dialog); window.confirm()
     * is unreliable — a silent no-op or throw — in some JUCE WebView backends. Falls back to
     * a guarded window.confirm(); if no confirmation mechanism is available the delete is
     * safely aborted (fail-safe) and the reason is logged, rather than silently swallowed.
     */
    async promptDelete() {
        const message = `Delete preset "${this.currentPreset}"?`;
        let confirmed = false;

        if (typeof this.onConfirmDelete === 'function') {
            try {
                confirmed = await this.onConfirmDelete(this.currentPreset, message);
            } catch (e) {
                console.error('[PresetManager] onConfirmDelete failed:', e);
                return;
            }
        } else if (typeof window !== 'undefined' && typeof window.confirm === 'function') {
            try {
                confirmed = window.confirm(message);
            } catch (e) {
                console.warn('[PresetManager] window.confirm unavailable in this WebView; ' +
                    'delete aborted. Supply options.onConfirmDelete for a reliable dialog.');
                return;
            }
        } else {
            console.warn('[PresetManager] No confirmation mechanism available; delete aborted. ' +
                'Supply options.onConfirmDelete.');
            return;
        }

        if (confirmed) {
            await this.deletePreset(this.currentPreset);
        }
    }

    /**
     * Update the display element with current preset name.
     */
    _updateDisplay() {
        if (this.displayElement) {
            this.displayElement.textContent = this.currentPreset;
        }
    }

    /**
     * Get current preset name.
     */
    getCurrentPreset() {
        return this.currentPreset;
    }

    /**
     * Get list of all presets.
     */
    getPresetList() {
        return [...this.presetList];
    }
}


/**
 * Factory function for common preset UI pattern.
 * Creates a complete preset bar with navigation, load, and save buttons.
 */
export function createPresetBar(containerId, options = {}) {
    const container = document.getElementById(containerId);
    if (!container) {
        console.error(`[PresetManager] Container not found: ${containerId}`);
        return null;
    }

    // Create UI elements
    container.innerHTML = `
        <div class="preset-bar" style="display: flex; align-items: center; gap: 8px;">
            <button class="preset-prev" title="Previous preset">&lt;</button>
            <span class="preset-name" style="min-width: 120px; text-align: center;">Default</span>
            <button class="preset-next" title="Next preset">&gt;</button>
            <button class="preset-load" title="Load preset from file">Load</button>
            <button class="preset-save" title="Save preset">Save</button>
        </div>
    `;

    const manager = new PresetManager({
        displayElement: container.querySelector('.preset-name'),
        prevButton: container.querySelector('.preset-prev'),
        nextButton: container.querySelector('.preset-next'),
        loadButton: container.querySelector('.preset-load'),
        saveButton: container.querySelector('.preset-save'),
        ...options
    });

    manager.initialize();
    return manager;
}


// Make available globally for non-module usage
if (typeof window !== 'undefined') {
    window.OuariconPresetManager = PresetManager;
    window.createPresetBar = createPresetBar;
}
