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
    async _waitForNative() {
        return new Promise((resolve) => {
            const check = () => {
                if (window.__JUCE__ && window.__JUCE__.backend) {
                    resolve();
                } else {
                    setTimeout(check, 50);
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
     * Helper for safe async calls with error handling.
     */
    async _safeCall(fn, errorMsg, defaultValue = false) {
        try {
            return await fn();
        } catch (e) {
            console.error(`[PresetManager] ${errorMsg}:`, e);
            return defaultValue;
        }
    }

    /**
     * Load a preset by name.
     */
    async loadPreset(name) {
        return this._safeCall(async () => {
            const success = await this._loadPreset(name);
            if (success) {
                this.currentPreset = name;
                this._updateDisplay();
                this.onPresetChanged(name);
            }
            return success;
        }, 'Load failed');
    }

    /**
     * Select the next preset in the list.
     */
    async selectNext() {
        return this._safeCall(async () => {
            const nextName = await this._selectNextPreset();
            return nextName ? this.loadPreset(nextName) : false;
        }, 'selectNext failed');
    }

    /**
     * Select the previous preset in the list.
     */
    async selectPrevious() {
        return this._safeCall(async () => {
            const prevName = await this._selectPreviousPreset();
            return prevName ? this.loadPreset(prevName) : false;
        }, 'selectPrevious failed');
    }

    /**
     * Save current state as a preset with given name.
     */
    async savePreset(name) {
        if (!name?.trim()) {
            console.warn('[PresetManager] Empty preset name');
            return false;
        }

        return this._safeCall(async () => {
            const trimmedName = name.trim();
            const success = await this._savePreset(trimmedName);
            if (success) {
                this.currentPreset = trimmedName;
                await this.refresh();
                this.onPresetChanged(this.currentPreset);
            }
            return success;
        }, 'Save failed');
    }

    /**
     * Save preset using native system file dialog.
     */
    async saveWithDialog() {
        return this._safeCall(async () => {
            const result = await this._savePresetWithDialog();
            if (result?.success) {
                this.currentPreset = result.name;
                this._updateDisplay();
                await this.refresh();
                this.onPresetChanged(this.currentPreset);
                return result;
            }
            return { success: false, name: '' };
        }, 'saveWithDialog failed', { success: false, name: '' });
    }

    /**
     * Load a preset from file using native system file dialog.
     */
    async loadFromFile() {
        return this._safeCall(async () => {
            const result = await this._loadPresetFromFile();
            if (result?.success) {
                this.currentPreset = result.name || 'Loaded Preset';
                this._updateDisplay();
                await this.refresh();
                this.onPresetChanged(this.currentPreset);
                return result;
            }
            return { success: false, name: '' };
        }, 'loadFromFile failed', { success: false, name: '' });
    }

    /**
     * Delete a preset by name. Factory presets cannot be deleted.
     */
    async deletePreset(name) {
        return this._safeCall(async () => {
            if (await this._isFactoryPreset(name)) {
                console.warn('[PresetManager] Cannot delete factory presets');
                return false;
            }
            const success = await this._deletePreset(name);
            if (success) await this.refresh();
            return success;
        }, 'Delete failed');
    }

    /**
     * Check if a preset is a factory preset (read-only).
     */
    async isFactoryPreset(name) {
        return this._safeCall(() => this._isFactoryPreset(name), 'isFactoryPreset failed');
    }

    /**
     * Prompt user to delete current preset.
     * Note: confirm() may not work in all JUCE WebView contexts.
     */
    promptDelete() {
        if (confirm(`Delete preset "${this.currentPreset}"?`)) {
            this.deletePreset(this.currentPreset);
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
