/*
  ==============================================================================

    O-MultiBandCompressor - UI Application Logic
    Phase 5.1: Layout and visualization placeholders only

  ==============================================================================
*/

console.log('O-MultiBandCompressor UI initializing (Phase 5.1)...');

// Wait for DOM to load
document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded');
    console.log('JUCE backend:', window.__JUCE__?.backend);

    // Initialize spectrum canvas placeholder
    initializeSpectrumPlaceholder();

    // Initialize meter animations (placeholder)
    initializeMeterPlaceholders();

    console.log('Phase 5.1 UI initialized');
    console.log('Parameter binding will be added in Phase 5.2');
});

// Spectrum analyzer placeholder with grid
function initializeSpectrumPlaceholder() {
    const canvas = document.getElementById('spectrum-canvas');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    canvas.width = canvas.offsetWidth * 2; // Retina resolution
    canvas.height = canvas.offsetHeight * 2;
    ctx.scale(2, 2);

    const width = canvas.offsetWidth;
    const height = canvas.offsetHeight;

    // Draw grid
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.2)';
    ctx.lineWidth = 1;

    // Horizontal lines (dB grid)
    for (let i = 0; i <= 4; i++) {
        const y = (height / 4) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
    }

    // Vertical lines (frequency grid)
    for (let i = 0; i <= 10; i++) {
        const x = (width / 10) * i;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
        ctx.stroke();
    }

    // Draw placeholder spectrum curve
    ctx.strokeStyle = 'rgba(107, 142, 35, 0.5)';
    ctx.lineWidth = 2;
    ctx.beginPath();

    for (let x = 0; x < width; x++) {
        // Simulate frequency response curve (higher in mids)
        const freq = Math.pow(x / width, 1.5);
        const lowBump = Math.sin(freq * Math.PI * 2) * 0.2;
        const midBump = Math.sin(freq * Math.PI * 4) * 0.3;
        const y = height * (0.5 + lowBump + midBump);

        if (x === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    }
    ctx.stroke();

    // Add text
    ctx.fillStyle = 'rgba(60, 47, 47, 0.4)';
    ctx.font = '14px Garamond';
    ctx.textAlign = 'center';
    ctx.fillText('Spectrum Analyzer', width / 2, height / 2 - 10);
    ctx.fillText('(Phase 5.3)', width / 2, height / 2 + 10);
}

// Meter placeholder animations
function initializeMeterPlaceholders() {
    const inputMeter = document.getElementById('inputMeterFill');
    const outputMeter = document.getElementById('outputMeterFill');
    const grMeters = [
        document.getElementById('grLow'),
        document.getElementById('grLomid'),
        document.getElementById('grHimid'),
        document.getElementById('grHigh')
    ];

    // Simulate random meter activity (placeholder for real metering in Phase 5.3)
    function updateMeters() {
        // Input/output meters (vertical)
        const inputLevel = 20 + Math.random() * 60;
        const outputLevel = 20 + Math.random() * 60;

        if (inputMeter) inputMeter.style.height = `${inputLevel}%`;
        if (outputMeter) outputMeter.style.height = `${outputLevel}%`;

        // GR meters (horizontal)
        grMeters.forEach((meter, i) => {
            if (meter) {
                const grAmount = Math.random() * 40; // 0-40% gain reduction
                meter.style.width = `${grAmount}%`;
            }
        });

        requestAnimationFrame(updateMeters);
    }

    // Start animation (just for visual testing, will be replaced with real data)
    // Comment out for production - this is just to show the UI is working
    // updateMeters();

    console.log('Meter placeholders initialized (not animating yet - Phase 5.3)');
}

// Crossover line dragging (Phase 5.2 - implement later)
// For now, just log that they exist
const crossoverLines = document.querySelectorAll('.crossover-line');
console.log(`Found ${crossoverLines.length} crossover lines (dragging in Phase 5.2)`);
