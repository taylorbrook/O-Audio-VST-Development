// ===== Footer Keyboard =====
// Add this code AFTER your existing parameter binding code.
// Requires: <div class="footer-keyboard-viz" id="keyboard-viz"></div> in HTML
// Requires: sendMidiNote native function registered in PluginEditor.cpp

(function initFooterKeyboard() {
    const keyboardViz = document.getElementById('keyboard-viz');
    if (!keyboardViz) return;

    // ===== CONFIGURATION =====
    const octaves = 2;       // Number of octaves (1-3)
    const startNote = 48;    // First MIDI note (48 = C3, 60 = C4)
    const velocity = 0.8;    // Default note velocity (0-1)
    // =========================

    const playingKeys = new Set();
    const qwertyPressed = new Set();

    // QWERTY keyboard mapping (standard piano layout)
    const qwertyMap = {
        'z': 48, 's': 49, 'x': 50, 'd': 51, 'c': 52, 'v': 53, 'g': 54,
        'b': 55, 'h': 56, 'n': 57, 'j': 58, 'm': 59,
        'q': 60, '2': 61, 'w': 62, '3': 63, 'e': 64, 'r': 65, '5': 66,
        't': 67, '6': 68, 'y': 69, '7': 70, 'u': 71,
        'i': 72, '9': 73, 'o': 74, '0': 75, 'p': 76
    };

    const whiteSemitones = [0, 2, 4, 5, 7, 9, 11];
    const blackPositions = [0, 1, 3, 4, 5];  // Which white key they follow
    const blackSemitones = [1, 3, 6, 8, 10];
    const noteNames = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];

    // Get native function for MIDI communication
    let sendMidiNote = null;
    if (window.Juce && window.Juce.getNativeFunction) {
        sendMidiNote = window.Juce.getNativeFunction('sendMidiNote');
    }

    // Build white keys
    const totalWhiteKeys = octaves * 7;
    for (let oct = 0; oct < octaves; oct++) {
        for (let i = 0; i < 7; i++) {
            const midiNote = startNote + oct * 12 + whiteSemitones[i];
            const key = document.createElement('div');
            key.className = 'white-key';
            key.dataset.note = midiNote;

            // Add note label
            const label = document.createElement('span');
            label.textContent = i === 0 ? `C${Math.floor(midiNote / 12) - 1}` : noteNames[i];
            key.appendChild(label);

            keyboardViz.appendChild(key);
        }
    }

    // Build black keys (positioned absolutely)
    for (let oct = 0; oct < octaves; oct++) {
        for (let i = 0; i < blackPositions.length; i++) {
            const whiteIdx = oct * 7 + blackPositions[i];
            const midiNote = startNote + oct * 12 + blackSemitones[i];
            const key = document.createElement('div');
            key.className = 'black-key';
            key.dataset.note = midiNote;
            key.style.left = `calc(100% / ${totalWhiteKeys} * ${whiteIdx + 1} - 7px)`;
            keyboardViz.appendChild(key);
        }
    }

    // Note handlers
    function noteOn(note) {
        if (playingKeys.has(note)) return;
        playingKeys.add(note);
        const el = keyboardViz.querySelector(`[data-note="${note}"]`);
        if (el) el.classList.add('pressed');
        if (sendMidiNote) sendMidiNote(note, velocity, true);
    }

    function noteOff(note) {
        if (!playingKeys.has(note)) return;
        playingKeys.delete(note);
        const el = keyboardViz.querySelector(`[data-note="${note}"]`);
        if (el) el.classList.remove('pressed');
        if (sendMidiNote) sendMidiNote(note, 0, false);
    }

    // Mouse/touch events for all keys
    keyboardViz.querySelectorAll('.white-key, .black-key').forEach(key => {
        const note = parseInt(key.dataset.note);
        key.addEventListener('mousedown', e => { e.preventDefault(); noteOn(note); });
        key.addEventListener('mouseup', () => noteOff(note));
        key.addEventListener('mouseleave', () => { if (playingKeys.has(note)) noteOff(note); });
        key.addEventListener('touchstart', e => { e.preventDefault(); noteOn(note); });
        key.addEventListener('touchend', e => { e.preventDefault(); noteOff(note); });
    });

    // Global mouseup to catch releases outside keyboard
    document.addEventListener('mouseup', () => {
        playingKeys.forEach(n => noteOff(n));
    });

    // QWERTY keyboard input
    document.addEventListener('keydown', e => {
        if (e.repeat || e.target.tagName === 'INPUT') return;
        const baseNote = qwertyMap[e.key.toLowerCase()];
        if (baseNote !== undefined && !qwertyPressed.has(e.key)) {
            const note = baseNote + (startNote - 48);  // Adjust for startNote offset
            const maxNote = startNote + octaves * 12 - 1;
            if (note >= startNote && note <= maxNote) {
                qwertyPressed.add(e.key);
                noteOn(note);
            }
        }
    });

    document.addEventListener('keyup', e => {
        const baseNote = qwertyMap[e.key.toLowerCase()];
        if (baseNote !== undefined) {
            qwertyPressed.delete(e.key);
            const note = baseNote + (startNote - 48);
            noteOff(note);
        }
    });
})();
