#pragma once

// PLAN-3.3 T11: bench helpers for the analyzer, the page-drawn meter, the
// fills and listen. Injected after kBenchScript and kBenchScript32 (same
// document-start user script). The page is the real index.html, unmodified.
// Pixel probes read the plot canvas (#curve) and the meter canvas (#meter).
static const char* kBenchScript33 = R"JS(
(function () {
  const BENCH_ID = -987654321;
  const P = (type, extra) => {
    try {
      const o = Object.assign({ type: type, pnow: performance.now() }, extra || {});
      window.__benchRawPost(JSON.stringify({ eventId: '__juce__invoke',
        payload: { name: 'benchReport', params: [o], resultId: BENCH_ID } }));
    } catch (e) {}
  };
  const raf = () => new Promise((r) => requestAnimationFrame(r));
  const raf2 = () => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
  const Q = window.__benchQ;                       // kBenchScript32's draw timing (Q.timing / Q.draws)
  const S = window.__benchS;                       // kBenchScript's native-call counts (S.natives)
  const R = { base: null, meterClears: 0, frames: [] };
  window.__benchR = R;

  // Meter frames: the page's meterDraw() starts with clearRect on #meter.
  const origClear = CanvasRenderingContext2D.prototype.clearRect;
  CanvasRenderingContext2D.prototype.clearRect = function () {
    if (this.canvas && this.canvas.id === 'meter') R.meterClears++;
    return origClear.apply(this, arguments);
  };

  const curve = () => document.getElementById('curve');
  const meter = () => document.getElementById('meter');
  const M = { l: 38, r: 14, t: 12, b: 22 };
  const geomOf = (c) => { const r = c.getBoundingClientRect(); return { left: r.left, top: r.top, w: Math.round(r.width), h: Math.round(r.height) }; };
  const ptr = (type, x, y, o) => {
    const g = geomOf(curve());
    curve().dispatchEvent(new PointerEvent(type, Object.assign({ bubbles: true, cancelable: true, composed: true, button: 0,
      buttons: type === 'pointerup' ? 0 : 1, pointerId: 7, pointerType: 'mouse', isPrimary: true,
      clientX: g.left + x, clientY: g.top + y }, o || {})));
  };
  const snap = (c) => c.getContext('2d').getImageData(0, 0, c.width, c.height);
  const pct = (a, q) => { const s = a.slice().sort((x, y) => x - y); return s.length ? s[Math.min(s.length - 1, Math.floor(q * s.length))] : -1; };

  // Meter geometry, re-stated from the page's meterDraw (top 9, bot h - 9, -60 .. +6).
  const meterGeom = () => {
    const c = meter(), g = geomOf(c), d = c.width / Math.max(1, g.w);
    const top = 9, bot = g.h - 9, barW = Math.max(8, Math.min(13, (g.w - 44) / 2)), bx0 = g.w - 8 - barW * 2 - 3;
    return { c, g, d, top, bot, barW, bx0, my: (db) => top + (6 - Math.max(-60, Math.min(6, db))) / 66 * (bot - top) };
  };
  // Per bar, down its centre column (CSS px): `tops` = the first non-background
  // pixel (alpha > 120: the peak line when it is above the level), `levels` = the
  // first pixel of the zone fill (sap / ochre / madder: G > R + 10 for sap,
  // R > G + 25 and R > 120 for ochre / madder; the ink peak line is neither).
  const barTops = () => {
    const m = meterGeom(), img = snap(m.c), W = m.c.width, tops = [], levels = [];
    for (let k = 0; k < 2; k++) {
      const X = Math.round((m.bx0 + k * (m.barW + 3) + m.barW / 2) * m.d);
      let top = -1, level = -1;
      for (let Y = Math.round((m.top + 1) * m.d); Y < Math.round((m.bot - 1) * m.d); Y++) {
        const i = (Y * W + X) * 4, r = img.data[i], g = img.data[i + 1], a = img.data[i + 3];
        if (a <= 120) continue;
        if (top < 0) top = Y / m.d;
        if ((g > r + 10) || (r > g + 25 && r > 120)) { level = Y / m.d; break; }
      }
      tops.push(top); levels.push(level);
    }
    return { tops, levels, m6: m.my(-6), h: m.g.h, w: m.g.w };
  };

  Object.assign(window.__bench, {
    // Strip Listen pressed only if it is not already (an idempotent latch).
    latchOn: async (tag) => {
      const b = document.getElementById('btn-listen');
      const was = b ? b.getAttribute('aria-pressed') : null;
      if (b && was !== 'true') b.click();
      await raf2();
      P('done', { id: tag, was });
    },
    natives: async (tag) => { P('natives', { id: tag, n: Object.assign({}, S.natives) }); },

    // Analyzer ink by difference: base = the plot with the analyzer Off.
    anBase: async (tag) => { await raf2(); R.base = snap(curve()); P('done', { id: tag }); },
    anDiff: async (tag, xLimitCss) => {
      await raf2();
      const c = curve(), g = geomOf(c), d = c.width / Math.max(1, g.w), img = snap(c), b = R.base, W = c.width, H = c.height;
      let topY = -1, xs = [], right = 0, n = 0; const rb = [1e9, 1e9, -1, -1, 0];
      const plotBottom = (g.h - M.b - 1) * d, plotTop = (M.t + 1) * d;
      if (b && b.width === W && b.height === H) {
        for (let Y = Math.ceil(plotTop); Y < plotBottom; Y++) {
          for (let X = 0; X < W; X++) {
            const i = (Y * W + X) * 4;
            const diff = Math.abs(img.data[i] - b.data[i]) + Math.abs(img.data[i + 1] - b.data[i + 1])
                       + Math.abs(img.data[i + 2] - b.data[i + 2]) + Math.abs(img.data[i + 3] - b.data[i + 3]);
            if (diff < 24) continue;
            n++;
            if (X / d > xLimitCss) { right++; rb[0] = Math.min(rb[0], X / d); rb[1] = Math.min(rb[1], Y / d); rb[2] = Math.max(rb[2], X / d); rb[3] = Math.max(rb[3], Y / d); rb[4] = Math.max(rb[4], diff); }
            if (topY < 0) topY = Y;
            if (Y <= topY + 1) xs.push(X);
          }
        }
      }
      const cx = xs.length ? xs.reduce((a, v) => a + v, 0) / xs.length / d : -1;
      P('anDiff', { id: tag, n, right, rightBox: right ? { x0: rb[0], y0: rb[1], x1: rb[2], y1: rb[3], maxDiff: rb[4] } : null, topY: topY < 0 ? -1 : topY / d, topX: cx, baseOk: !!(b && b.width === W && b.height === H),
                    w: g.w, h: g.h, plotW: g.w - M.l - M.r });
    },

    meterTops: async (tag) => { await raf2(); P('meterTops', Object.assign({ id: tag }, barTops())); },
    // Bar-top range over `ms`, sampled every frame.
    meterTrack: async (tag, ms) => {
      const t0 = performance.now(), a = [], b = [], la = [], lb = [];
      while (performance.now() - t0 < ms) { await raf(); const t = barTops(); a.push(t.tops[0]); b.push(t.tops[1]); la.push(t.levels[0]); lb.push(t.levels[1]); }
      P('meterTrack', { id: tag, n: a.length, min0: Math.min(...a), max0: Math.max(...a), min1: Math.min(...b), max1: Math.max(...b),
                        lmin0: Math.min(...la), lmax0: Math.max(...la), lmin1: Math.min(...lb), lmax1: Math.max(...lb), m6: barTops().m6 });
    },
    meterClears: async (tag, ms) => {
      const c0 = R.meterClears; await sleep(ms);
      P('meterClears', { id: tag, n: R.meterClears - c0, ms });
    },

    // Mean RGBA of a 3 x 3 device-pixel patch at each CSS point of the plot canvas.
    pixels: async (tag, pts) => {
      await raf2();
      const c = curve(), g = geomOf(c), d = c.width / Math.max(1, g.w), img = snap(c), W = c.width;
      const out = pts.map((p) => {
        const X = Math.round(p[0] * d), Y = Math.round(p[1] * d); const s = [0, 0, 0, 0]; let k = 0;
        for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++) {
          const i = ((Y + dy) * W + (X + dx)) * 4; for (let j = 0; j < 4; j++) s[j] += img.data[i + j]; k++;
        }
        return { x: p[0], y: p[1], rgba: s.map((v) => v / k) };
      });
      P('pixels', { id: tag, pts: out });
    },

    // A node drag for `ms`: press at (x0, y0), move along x0 + ax sin(2 pi t / period) at `hz`,
    // release. Records rAF intervals and the page's draw() times during it.
    dragFor: async (tag, x0, y0, ax, ay, ms, hz) => {
      const iv = []; let last = -1, on = true;
      const tick = (ts) => { if (last >= 0) iv.push(ts - last); last = ts; if (on) requestAnimationFrame(tick); };
      requestAnimationFrame(tick);
      Q.draws = []; Q.timing = true;
      ptr('pointerdown', x0, y0);
      const t0 = performance.now(); let k = 0;
      while (performance.now() - t0 < ms) {
        k++;
        const ph = (performance.now() - t0) / 1000 * 2 * Math.PI * 0.5;
        ptr('pointermove', x0 + ax * Math.sin(ph) + 12, y0 + ay * Math.sin(ph * 1.3));
        await sleep(1000 / hz);
      }
      ptr('pointerup', x0, y0);
      on = false; Q.timing = false;
      await raf2();
      const dr = Q.draws.slice();
      P('dragFor', { id: tag, moves: k, n: iv.length, median: pct(iv, 0.5), p99: pct(iv, 0.99), max: Math.max(...iv),
                     drawN: dr.length, drawMedian: pct(dr, 0.5), drawP99: pct(dr, 0.99) });
    },

    // Press, `steps` moves to (x1, y1) every `dt` ms; pause after the 3rd move until resume();
    // then end with 'up' | 'cancel' | 'lost'.
    dragPauseEnd: async (tag, x0, y0, x1, y1, steps, dt, end) => {
      Q.resume = null;
      ptr('pointerdown', x0, y0);
      for (let i = 1; i <= steps; i++) {
        await sleep(dt);
        ptr('pointermove', x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps);
        if (i === 3) { const p = new Promise((r) => { Q.resume = r; }); P('paused', { id: tag }); await p; }
      }
      await sleep(dt);
      if (end === 'cancel') ptr('pointercancel', x1, y1, { buttons: 0 });
      else if (end === 'lost') ptr('lostpointercapture', x1, y1, { buttons: 0 });
      else ptr('pointerup', x1, y1);
      await raf2();
      P('done', { id: tag });
    },
  });
})();
)JS";
