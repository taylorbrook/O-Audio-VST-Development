#pragma once

// research-3.3 draw-bench: draw() timing with analyzer data injected through
// the real backend event path (window.__JUCE__.backend.emitByBackend, the
// string form JUCE's C++ emitEventIfBrowserIsVisible uses). Injected after
// kBenchScript + kBenchScript32 (reuses window.__bench, __benchQ, the rAF /
// clearRect draw() timer).
static const char* kBenchScript33 = R"JS(
(function () {
  const BENCH_ID = -987654321;
  const be = window.__JUCE__.backend;
  const P = (type, extra) => {
    try {
      const o = Object.assign({ type: type, pnow: performance.now() }, extra || {});
      window.__benchRawPost(JSON.stringify({ eventId: '__juce__invoke',
        payload: { name: 'benchReport', params: [o], resultId: BENCH_ID } }));
    } catch (e) {}
  };
  const raf = () => new Promise((r) => requestAnimationFrame(r));
  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
  const Q = window.__benchQ;

  // Keep the last packet per band (the drag simulation mutates a copy).
  const S = { pk: {}, rate: 48000, stereo: true, rateKnown: true, eqIn: 0 };
  const prev = be.emitByBackend;
  be.emitByBackend = function (eventId, str) {
    prev.call(be, eventId, str);
    if (eventId !== 'eqSections') return;
    S.eqIn++;
    try {
      const d = JSON.parse(str);
      if (d.sampleRate) S.rate = d.sampleRate;
      if ('stereo' in d) S.stereo = d.stereo;
      if ('rateKnown' in d) S.rateKnown = d.rateKnown;
      for (const b of d.bands || []) S.pk[b.band] = b;
    } catch (e) {}
  };

  // Seeded noise so every run sees the same spectra.
  let seed = 12345;
  const rnd = () => { seed = (seed * 1664525 + 1013904223) >>> 0; return seed / 4294967296; };
  // Pink-ish: -30 dB at 10 Hz, -3 dB / oct, rolled off above 16 kHz, +-4 dB noise, floor -90.
  const spectrum = (offset) => {
    const a = new Array(256);
    for (let i = 0; i < 256; i++) {
      const f = 10 * Math.pow(3000, i / 255);
      let db = -30 + offset - 3 * Math.log2(f / 10);
      if (f > 16000) db -= 24 * Math.log2(f / 16000);
      db += (rnd() - 0.5) * 8;
      a[i] = Math.max(-90, Math.min(-6, Math.round(db * 100) / 100));
    }
    return JSON.stringify(a);
  };

  const stats = (arr) => {
    const d = arr.slice().sort((a, b) => a - b);
    if (!d.length) return { n: 0 };
    const mean = d.reduce((a, b) => a + b, 0) / d.length;
    return { n: d.length, median: d[d.length >> 1], p95: d[Math.min(d.length - 1, Math.floor(d.length * 0.95))],
             p99: d[Math.min(d.length - 1, Math.floor(d.length * 0.99))], max: d[d.length - 1], min: d[0], mean: mean };
  };

  const setAnalyzer = async (mode) => {
    const btn = document.querySelector('#seg-analyzer button[data-v="' + mode + '"]');
    if (btn) btn.click();
    await raf(); await raf();
    return !!btn;
  };

  const mutatePacket = (b, k) => {
    const src = S.pk[b];
    if (!src) return null;
    const pk = JSON.parse(JSON.stringify(src));
    const m = 1 + 0.05 * Math.sin(k * 0.21);
    for (const s of pk.sections || []) s[0] *= m;          // g: shifts the band frequency
    for (const o of pk.onePoles || []) o[0] *= m;
    return JSON.stringify({ sampleRate: S.rate, stereo: S.stereo, rateKnown: S.rateKnown, bands: [pk] });
  };

  const secCount = () => Object.keys(Q.secBy).reduce((a, b) => a + (Q.actBy[b] ? Q.secBy[b] : 0), 0);
  const geomNow = () => {
    const c = document.getElementById('curve'), r = c.getBoundingClientRect();
    return { w: Math.round(r.width), h: Math.round(r.height), cw: c.width, ch: c.height, dpr: window.devicePixelRatio || 1,
             vw: window.innerWidth, vh: window.innerHeight };
  };

  Object.assign(window.__bench, {
    // opts: n, analyzer (0..3), inject ('none' | 'post' | 'both'), injectEvery (frames),
    // framesPerDraw (1 | 2), cache (bool; the scratch page reads window.__peqCache),
    // dragBand (0 | b: mutated packet for band b each draw), warm (untimed draws)
    drawCase: async (tag, opts) => {
      const o = Object.assign({ n: 150, analyzer: 0, inject: 'none', injectEvery: 1, framesPerDraw: 2, cache: false, dragBand: 0, warm: 15, phases: false, gradCache: false }, opts || {});
      window.__peqCache = !!o.cache;
      window.__peqCacheStats = { hit: 0, miss: 0, gridResets: 0 };
      const found = await setAnalyzer(o.analyzer);
      const intervals = [];
      let frame = 0, last = -1;
      const one = async (timed) => {
        if (o.inject !== 'none' && frame % o.injectEvery === 0) {
          if (o.inject === 'both') be.emitByBackend('analyzerPre', spectrum(+4));
          be.emitByBackend('analyzerPost', spectrum(0));
        }
        if (o.dragBand) { const s = mutatePacket(o.dragBand, frame); if (s) be.emitByBackend('eqSections', s); }
        window.dispatchEvent(new Event('resize'));
        for (let f = 0; f < o.framesPerDraw; f++) {
          const ts = await raf();
          if (timed) { const t = performance.now(); if (last >= 0) intervals.push(t - last); last = t; }
        }
        frame++;
      };
      for (let i = 0; i < o.warm; i++) await one(false);
      window.__peqCacheStats = { hit: 0, miss: 0, gridResets: 0 };
      window.__peqGradCache = !!o.gradCache;
      window.__peqPhases = o.phases ? {} : null;
      Q.draws = []; Q.timing = true;
      const t0 = performance.now();
      for (let i = 0; i < o.n; i++) await one(true);
      Q.timing = false;
      const wall = performance.now() - t0;
      const ph = window.__peqPhases; window.__peqPhases = null; window.__peqGradCache = false;
      const phases = {};
      if (ph && ph.draws) for (const k of Object.keys(ph)) if (k !== 'draws') phases[k] = ph[k] / ph.draws;
      P('drawCase', { id: tag, opts: o, analyzerButtonFound: found, draw: stats(Q.draws), raw: Q.draws.slice(),
                      frameInterval: stats(intervals), wallMs: wall, phasesMeanMs: phases, phaseDraws: ph ? ph.draws : 0, sections: secCount(), geom: geomNow(),
                      cacheStats: window.__peqCacheStats, listenPressed: (document.getElementById('btn-listen') || {}).getAttribute
                        ? document.getElementById('btn-listen').getAttribute('aria-pressed') : null });
    },

    // A real pointer drag on the plot (relay writes -> C++ -> eqSections at
    // 60 Hz) with analyzer injection every frame; draw() timed throughout.
    dragCase: async (tag, x0, y0, x1, y1, steps, opts) => {
      const o = Object.assign({ analyzer: 2, inject: 'post', cache: false }, opts || {});
      window.__peqCache = !!o.cache;
      const found = await setAnalyzer(o.analyzer);
      const c = document.getElementById('curve');
      const ptr = (type, x, y) => {
        const r = c.getBoundingClientRect();
        c.dispatchEvent(new PointerEvent(type, { bubbles: true, cancelable: true, composed: true, button: 0, buttons: type === 'pointerup' ? 0 : 1,
          pointerId: 7, pointerType: 'mouse', isPrimary: true, clientX: r.left + x, clientY: r.top + y }));
      };
      const eq0 = S.eqIn;
      window.__peqCacheStats = { hit: 0, miss: 0, gridResets: 0 };
      Q.draws = []; Q.timing = true;
      ptr('pointerdown', x0, y0);
      for (let i = 1; i <= steps; i++) {
        if (o.inject !== 'none') {
          if (o.inject === 'both') be.emitByBackend('analyzerPre', spectrum(+4));
          be.emitByBackend('analyzerPost', spectrum(0));
        }
        // back and forth so the node stays in the plot
        const u = 0.5 - 0.5 * Math.cos(Math.PI * 2 * i / steps * 2);
        ptr('pointermove', x0 + (x1 - x0) * u, y0 + (y1 - y0) * u);
        await raf();
      }
      ptr('pointerup', x0, y0);
      await raf(); await raf();
      Q.timing = false;
      P('dragCase', { id: tag, opts: o, analyzerButtonFound: found, draw: stats(Q.draws), raw: Q.draws.slice(),
                      eqSectionsDuringDrag: S.eqIn - eq0, sections: secCount(), geom: geomNow(), cacheStats: window.__peqCacheStats });
    },

    setListen: async (tag, band) => {
      // select via chip, then the strip's Listen button
      const chip = document.querySelector('#chips [data-b="' + band + '"]') || document.querySelector('[data-b="' + band + '"]');
      if (chip) chip.click();
      await raf(); await raf();
      const btn = document.getElementById('btn-listen');
      if (btn) btn.click();
      await sleep(100);
      P('done', { id: tag, chip: !!chip, pressed: btn ? btn.getAttribute('aria-pressed') : null });
    },
    clearListen: async (tag) => {
      const btn = document.getElementById('btn-listen');
      if (btn && btn.getAttribute('aria-pressed') === 'true') btn.click();
      await sleep(100);
      P('done', { id: tag, pressed: btn ? btn.getAttribute('aria-pressed') : null });
    },
  });
})();
)JS";
