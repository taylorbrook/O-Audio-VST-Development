/**
 * Ouaricon Preset Manager - JavaScript UI Module
 *
 * Provides preset selection, save, and navigation functionality.
 * Works with the C++ OuariconPresetManager via native functions.
 *
 * Usage:
 *   import { PresetManager } from './modules/preset-manager.js';
 *
 *   const presets = new PresetManager({
 *     displayElement: document.getElementById('preset-name'),
 *     prevButton: document.getElementById('prev-preset'),
 *     nextButton: document.getElementById('next-preset'),
 *     saveButton: document.getElementById('save-preset'),
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
        this.deleteButton = options.deleteButton;
        this.menuButton = options.menuButton;

        this.onPresetChanged = options.onPresetChanged || (() => {});
        this.onPresetListUpdated = options.onPresetListUpdated || (() => {});

        this.currentPreset = 'Default';
        this.presetList = [];
        this.isInitialized = false;
    }

    /**
     * Initialize the preset manager.
     * Fetches current preset and list from C++.
     */
    async initialize() {
        if (this.isInitialized) return;

        // Wait for JUCE native integration
        await this._waitForNative();

        // Bind navigation buttons
        if (this.prevButton) {
            this.prevButton.addEventListener('click', () => this.selectPrevious());
        }
        if (this.nextButton) {
            this.nextButton.addEventListener('click', () => this.selectNext());
        }
        if (this.saveButton) {
            this.saveButton.addEventListener('click', () => this.promptSave());
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
            const current = await window.__JUCE__.backend.getCurrentPreset();
            this.currentPreset = current || 'Default';

            // Get preset list
            const list = await window.__JUCE__.backend.getPresetList();
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
            const success = await window.__JUCE__.backend.loadPreset(name);
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
            const nextName = await window.__JUCE__.backend.selectNextPreset();
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
            const prevName = await window.__JUCE__.backend.selectPreviousPreset();
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
     * Save current state as a new preset.
     */
    async savePreset(name) {
        if (!name || name.trim() === '') {
            console.warn('[PresetManager] Empty preset name');
            return false;
        }

        try {
            const success = await window.__JUCE__.backend.savePreset(name.trim());
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
     * Delete a preset by name.
     */
    async deletePreset(name) {
        try {
            const isFactory = await window.__JUCE__.backend.isFactoryPreset(name);
            if (isFactory) {
                alert('Cannot delete factory presets.');
                return false;
            }

            const success = await window.__JUCE__.backend.deletePreset(name);
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
     * Prompt user to save current preset.
     */
    promptSave() {
        const name = prompt('Save preset as:', this.currentPreset);
        if (name !== null && name.trim() !== '') {
            this.savePreset(name.trim());
        }
    }

    /**
     * Prompt user to delete current preset.
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
 * Creates a complete preset bar with navigation.
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
            <button class="preset-save" title="Save preset">Save</button>
        </div>
    `;

    const manager = new PresetManager({
        displayElement: container.querySelector('.preset-name'),
        prevButton: container.querySelector('.preset-prev'),
        nextButton: container.querySelector('.preset-next'),
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
