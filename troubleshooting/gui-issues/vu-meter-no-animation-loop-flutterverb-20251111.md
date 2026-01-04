---
plugin: FlutterVerb
date: 2025-11-11
problem_type: ui_layout
component: webview
symptoms:
  - VU meter needle doesn't move smoothly
  - Needle jumps abruptly between positions instead of animating
  - No ballistic motion (fast attack, slow decay)
root_cause: logic_error
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [webview, vu-meter, animation, canvas, requestAnimationFrame, ballistics]
---

# Troubleshooting: VU Meter Missing Animation Loop (No Ballistic Motion)

## Problem
VU meter needle in FlutterVerb v1.0.1 updated position but didn't animate smoothly. The needle jumped directly to target positions instead of moving with realistic ballistic motion (fast attack when signal increases, slow decay when signal decreases). This made the meter hard to read and visually jarring.

## Environment
- Plugin: FlutterVerb
- JUCE Version: 8.0.9
- Affected: WebView UI canvas animation (index.html), Stage 5 (GUI integration)
- Date: 2025-11-11

## Symptoms
- VU meter needle visible and responds to audio levels
- Needle position updates but with abrupt jumps (no smooth motion)
- No ballistic behavior (should rise fast, fall slowly like analog VU meters)
- Visually jarring and hard to read peak levels
- Missing the "organic" feel of real hardware meters

## What Didn't Work

**v1.0.0 (broken completely):** Needle didn't move at all
- **Why it failed:** Storing linear gain instead of dB, wrong atomic pattern

**v1.0.1 (moved but not smoothly):** Direct CSS transform on each event
- **Why it partially worked:** Needle position updated to correct angle
- **Why unsatisfactory:** Each audio event directly set needle angle, causing instant jumps. No interpolation between values. No ballistic physics.

## Solution

Added `requestAnimationFrame` loop with separate current/target angle tracking and ballistic speeds (fast attack, slow decay).

**Code changes (v1.0.1 → v1.0.2):**

```javascript
// v1.0.1 (worked but jerky):
function updateVUMeter(dbLevel) {
    const minDB = -60, maxDB = 0;
    const clampedDB = Math.max(minDB, Math.min(maxDB, dbLevel));
    const normalizedLevel = (clampedDB - minDB) / (maxDB - minDB);

    // Direct update - instant jump, no smoothing
    const angle = -45 + (normalizedLevel * 90);
    vuNeedle.style.transform = `translateX(-50%) rotate(${angle}deg)`;
}

// v1.0.2 (smooth ballistic motion):
let currentNeedleAngle = -45;  // Where needle currently is (visual state)
let targetNeedleAngle = -45;   // Where needle should move to (from audio)

function updateVUMeter(dbLevel) {
    const minDB = -60, maxDB = 0;
    const clampedDB = Math.max(minDB, Math.min(maxDB, dbLevel));
    const normalizedLevel = (clampedDB - minDB) / (maxDB - minDB);

    // Only update target - don't draw yet
    targetNeedleAngle = -45 + (normalizedLevel * 90);
}

function animateVUMeter() {
    // Ballistic motion: fast attack, slow decay
    const attackSpeed = 0.4;   // Fast rise (responsive to peaks)
    const decaySpeed = 0.15;   // Slow fall (readable, smooth fallback)

    if (currentNeedleAngle < targetNeedleAngle) {
        // Attack: needle moving up (louder signal)
        currentNeedleAngle += (targetNeedleAngle - currentNeedleAngle) * attackSpeed;
    } else {
        // Decay: needle falling back (quieter signal)
        currentNeedleAngle += (targetNeedleAngle - currentNeedleAngle) * decaySpeed;
    }

    // Apply transform with current position
    vuNeedle.style.transform = `translateX(-50%) rotate(${currentNeedleAngle}deg)`;

    // Update color based on level
    const normalizedLevel = (currentNeedleAngle + 45) / 90;
    if (normalizedLevel > 0.9) {
        vuNeedle.style.background = 'linear-gradient(180deg, #ff6666 0%, #ff4444 100%)';
    } else if (normalizedLevel > 0.75) {
        vuNeedle.style.background = 'linear-gradient(180deg, #ffcc66 0%, #ffaa44 100%)';
    } else {
        vuNeedle.style.background = 'linear-gradient(180deg, #d4a574 0%, #c49564 100%)';
    }

    // Continue loop
    requestAnimationFrame(animateVUMeter);
}

// Start animation loop (runs continuously at ~60fps)
animateVUMeter();
```

**Key architecture changes:**
1. **Separated state:** `currentNeedleAngle` (visual) vs `targetNeedleAngle` (audio data)
2. **Event handler only updates target:** Decouples audio event rate from display rate
3. **Animation loop interpolates:** Smoothly moves current toward target
4. **Ballistic speeds:** Different attack (0.4) vs decay (0.15) for realistic physics
5. **requestAnimationFrame:** Browser-optimized ~60fps loop

## Why This Works

**Root cause:** Canvas/DOM animations require continuous redraw loops. Simply updating a value in an event callback doesn't create smooth motion. The architecture was conflating "what value should be displayed" (target) with "what is currently displayed" (current visual state).

**Why the solution works:**

1. **Separation of data and view:**
   - Audio events arrive at ~30-60Hz (variable rate from C++)
   - Display updates at stable 60fps (requestAnimationFrame)
   - Target value can update independently from visual interpolation

2. **Exponential interpolation formula:**
   ```javascript
   current += (target - current) * speed
   ```
   - Creates smooth easing (fast initially, slows near target)
   - Never overshoots target
   - Naturally converges without needing threshold checks
   - Speed parameter controls responsiveness (0.0 = no movement, 1.0 = instant)

3. **Ballistic motion physics:**
   - **Attack = 0.4:** Needle catches peaks quickly (responsive)
   - **Decay = 0.15:** Needle falls slowly (easier to read, less jittery)
   - Mimics physical VU meter with mass/spring/damper system
   - Industry standard behavior users expect

4. **requestAnimationFrame benefits:**
   - Synced to display refresh (no tearing/judder)
   - Automatically pauses when tab not visible (saves CPU)
   - Better than setInterval/setTimeout for animation
   - Provides consistent ~60fps on all browsers

**Pattern from TapeAge:** This follows the exact VU meter animation pattern validated in TapeAge plugin. The combination of target/current split + RAF loop + ballistic speeds is the established pattern for WebView meters.

## Prevention

**When implementing animated meters/visualizations in JUCE WebView:**

1. ✅ **Always separate target from current state:**
   ```javascript
   let currentValue = 0;  // What's displayed NOW
   let targetValue = 0;   // What we're moving TOWARD

   // Events only update target
   Juce.addEventListener('LEVEL', (v) => { targetValue = v; });
   ```

2. ✅ **Use requestAnimationFrame for continuous animation:**
   ```javascript
   function animate() {
       currentValue += (targetValue - currentValue) * speed;
       draw(currentValue);
       requestAnimationFrame(animate);
   }
   animate();  // Start once, runs forever
   ```

3. ✅ **Use ballistic speeds for realistic meter behavior:**
   ```javascript
   const ATTACK = 0.3 - 0.5;   // Fast rise (responsive to peaks)
   const DECAY = 0.1 - 0.2;    // Slow fall (readable)

   const speed = current < target ? ATTACK : DECAY;
   current += (target - current) * speed;
   ```

4. ✅ **Start animation loop once at initialization:**
   ```javascript
   // At end of init code:
   animateVUMeter();  // Starts loop, never call again
   ```

5. ❌ **Don't draw directly in event callbacks:**
   ```javascript
   // BAD: Jerky motion tied to event rate
   Juce.addEventListener('LEVEL', (value) => {
       needle.style.transform = `rotate(${value}deg)`;  // Don't do this
   });

   // GOOD: Event updates target, loop animates
   Juce.addEventListener('LEVEL', (value) => {
       targetAngle = value;  // Just update target
   });
   ```

6. ❌ **Don't use CSS transitions for continuous animation:**
   ```javascript
   // BAD: CSS transition creates lag and can't do ballistic motion
   needle.style.transition = 'transform 0.05s';
   needle.style.transform = `rotate(${angle}deg)`;

   // GOOD: RAF loop with ballistic interpolation
   function animate() {
       current += (target - current) * speed;
       needle.style.transform = `rotate(${current}deg)`;
       requestAnimationFrame(animate);
   }
   ```

7. 🔍 **Test with varying signal types:**
   - Transient peaks (drums) - should catch fast (attack)
   - Sustained tones (synth pads) - should be smooth
   - Silence - should return to rest slowly (decay)
   - No jitter, flickering, or oscillation

**Standard meter animation pattern:**
```javascript
// Setup (run once)
let current = restPosition;
let target = restPosition;
const ATTACK = 0.4, DECAY = 0.15;

// Data input (event listener)
Juce.addEventListener('LEVEL', (db) => {
    target = mapDBToAngle(db);
});

// Animation loop (runs continuously)
function animate() {
    const speed = current < target ? ATTACK : DECAY;
    current += (target - current) * speed;
    needle.style.transform = `rotate(${current}deg)`;
    requestAnimationFrame(animate);
}
animate();
```

**Speed tuning guide:**
- 0.1 = Very smooth, sluggish (too slow for peaks)
- 0.2 = Smooth decay (good for release)
- 0.4 = Responsive attack (good for peaks)
- 0.6 = Very fast (may overshoot feel)
- 1.0 = Instant (defeats purpose of smoothing)

## Related Issues

- See also: [toggle-switch-wrong-api-getsliderstate-flutterverb-20251111.md](toggle-switch-wrong-api-getsliderstate-flutterverb-20251111.md) - Other FlutterVerb v1.0.2 WebView fix
- Similar pattern: TapeAge VU meter implementation (working reference with same RAF + ballistic pattern)

**Promoted to Required Reading:** This pattern is now Pattern #20 in `troubleshooting/patterns/juce8-critical-patterns.md` - all subagents will see this before code generation.
