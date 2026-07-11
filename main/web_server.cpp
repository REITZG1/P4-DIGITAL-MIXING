#include "web_server.h"
#include "global_vars.h"

#include <WiFi.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <cstring>
#include <cstdio>

static const char *TAG = "web";
static httpd_handle_t server = NULL;
static int ws_client_fd = -1;
static int wifiRetryCount = 0;
static unsigned long wifiReconnectDelay = 0;
static unsigned long lastReconnectAttempt = 0;

// --- Embedded web UI ---
static const char* HTML_INDEX = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, viewport-fit=cover, user-scalable=no">
  <meta name="color-scheme" content="dark">
  <meta name="theme-color" content="#0d0e12">
  <title>Digital Mixer ESP32-P4</title>
  <style>
    :root {
      /* Palet Warna Modern Audio Software */
      --bg-base: #090a0f;
      --bg-surface: #12141c;
      --panel-bg-1: #161821;
      --panel-bg-2: #101218;
      --border: rgba(255, 255, 255, 0.05);
      --border-highlight: rgba(255, 255, 255, 0.1);

      --text-main: #f1f3f5;
      --text-dim: #9ca3af;

      /* Aksen Modul */
      --accent-eq: #fb923c;
      --accent-eq-glow: rgba(251,146,60,.35);
      --accent-route: #38bdf8;
      --accent-route-glow: rgba(56,189,248,.35);
      --accent-reverb: #c084fc;
      --accent-reverb-glow: rgba(192,132,252,.35);
      --accent-comp: #34d399;
      --accent-comp-glow: rgba(52,211,153,.35);
      --accent-master: #fbbf24;
      --accent-master-glow: rgba(251,191,36,.35);

      --danger: #ef4444;
      --success: #10b981;

      --font-body: 'Inter', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
      --font-mono: 'JetBrains Mono', "SFMono-Regular", Consolas, monospace;
    }

    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; margin: 0; padding: 0; }

    html { scroll-behavior: smooth; }

    body {
      font-family: var(--font-body);
      background-color: var(--bg-base);
      background-image:
        radial-gradient(circle at 15% 0%, rgba(56,189,248,0.04), transparent 40%),
        radial-gradient(circle at 85% 100%, rgba(192,132,252,0.04), transparent 40%);
      color: var(--text-main);
      min-height: 100vh;
      font-size: 14px;
      display: flex;
      flex-direction: column;
    }

    /* ---------- Scrollbars ---------- */
    ::-webkit-scrollbar { width: 8px; height: 8px; }
    ::-webkit-scrollbar-track { background: var(--bg-base); }
    ::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.1); border-radius: 4px; }
    ::-webkit-scrollbar-thumb:hover { background: rgba(255,255,255,0.2); }

    /* ---------- Header ---------- */
    header {
      position: sticky; top: 0; z-index: 50;
      padding: 12px 20px;
      background: rgba(13, 14, 18, 0.85);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border-bottom: 1px solid var(--border);
      display: flex; justify-content: space-between; align-items: center; gap: 15px;
      box-shadow: 0 4px 30px rgba(0, 0, 0, 0.5);
    }

    .brand { display: flex; align-items: center; gap: 12px; }
    .brand-icon {
      width: 36px; height: 36px; border-radius: 10px;
      background: linear-gradient(135deg, var(--accent-route), var(--accent-reverb));
      display: flex; align-items: center; justify-content: center;
      box-shadow: 0 0 15px rgba(56,189,248,0.3);
    }
    .brand-icon svg { width: 20px; height: 20px; stroke: #000; }

    .brand-text { line-height: 1.2; }
    .brand-text .title { font-weight: 700; font-size: 1rem; letter-spacing: 0.5px; }
    .brand-text .subtitle {
      font-size: 0.65rem; color: var(--accent-route); letter-spacing: 1px;
      text-transform: uppercase; font-family: var(--font-mono); font-weight: 600;
    }

    .status {
      display: inline-flex; align-items: center; gap: 8px;
      padding: 6px 14px; border-radius: 20px;
      background: rgba(0,0,0,0.4);
      border: 1px solid var(--border);
      font-size: 0.75rem; font-family: var(--font-mono); font-weight: 600;
    }
    .status::before { content: ''; width: 8px; height: 8px; border-radius: 50%; flex-shrink: 0; }
    .status.ws-connected { color: var(--success); border-color: rgba(16,185,129,0.3); }
    .status.ws-connected::before { background: var(--success); box-shadow: 0 0 8px var(--success); animation: pulse 2s infinite; }
    .status.ws-disconnected { color: var(--danger); border-color: rgba(239,68,68,0.3); }
    .status.ws-disconnected::before { background: var(--danger); box-shadow: 0 0 8px var(--danger); }

    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }

    /* ---------- Channel Quick-Nav ---------- */
    .channel-nav {
      display: flex; gap: 8px; padding: 12px 20px;
      background: var(--bg-surface);
      border-bottom: 1px solid var(--border);
      overflow-x: auto; scroll-snap-type: x mandatory;
    }
    .ch-pill {
      flex: 0 0 auto; scroll-snap-align: start;
      width: 36px; height: 36px; border-radius: 50%;
      border: 2px solid var(--ch-accent, rgba(255,255,255,0.1));
      background: var(--bg-base); color: var(--text-main);
      font-size: 0.8rem; font-weight: 700; font-family: var(--font-mono);
      cursor: pointer; display: flex; align-items: center; justify-content: center;
      transition: all 0.2s ease;
    }
    .ch-pill:hover, .ch-pill:active { background: var(--ch-accent); color: #000; transform: translateY(-2px); }
    .ch-pill.master-pill {
      width: auto; padding: 0 16px; border-radius: 20px;
      font-size: 0.7rem; letter-spacing: 1px; text-transform: uppercase;
      border-color: var(--accent-master); color: var(--accent-master);
    }
    .ch-pill.master-pill:hover { background: var(--accent-master); color: #000; }

    /* ---------- Layout Main ---------- */
    main {
      padding: 20px;
      display: flex; flex-direction: row; gap: 20px;
      overflow-x: auto; align-items: flex-start;
      scroll-snap-type: x proximity;
      flex-grow: 1;
    }

    #channelsContainer { display: flex; gap: 20px; }

    /* ---------- Panels ---------- */
    .panel {
      background:
        radial-gradient(circle at 13px 13px, #3a3e48 0 2.4px, transparent 3px),
        radial-gradient(circle at calc(100% - 13px) 13px, #3a3e48 0 2.4px, transparent 3px),
        radial-gradient(circle at 13px calc(100% - 13px), #3a3e48 0 2.4px, transparent 3px),
        radial-gradient(circle at calc(100% - 13px) calc(100% - 13px), #3a3e48 0 2.4px, transparent 3px),
        linear-gradient(145deg, var(--panel-bg-1), var(--panel-bg-2));
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 20px 16px;
      box-shadow: 0 10px 30px -10px rgba(0,0,0,0.8);
      position: relative;
      scroll-snap-align: start;
      display: flex; flex-direction: column;
    }

    #masterPanel {
      min-width: 88vw; max-width: 400px;
      border-color: rgba(251, 191, 36, 0.2);
    }

    .channel-strip {
      min-width: 88vw; max-width: 400px;
      scroll-margin-left: 20px;
    }

    /* * PERBAIKAN DESKTOP: Mencegah overlapping element (CH 1 tersembunyi)
     * dengan menghitung width absolut pada Master panel & scroll-margin.
     */
    @media (min-width: 768px) {
      main { padding: 24px; gap: 24px; }

      #masterPanel {
        min-width: 300px; max-width: 300px; flex: 0 0 300px; /* Lock width agar tidak berubah-ubah */
        position: sticky; left: 24px; z-index: 10;
      }

      .channel-strip {
        min-width: 300px; max-width: 320px;
        /* Offset: 24px (padding kiri) + 300px (Lebar Master) + 24px (gap) = 348px */
        scroll-margin-left: 348px;
      }
    }

    .panel h2, .panel h3 { text-align: center; font-weight: 700; margin: 0 0 15px 0; }

    #masterPanel h2 { color: var(--accent-master); letter-spacing: 1px; }
    #masterPanel h3, .module-title {
      font-size: 0.72rem; text-transform: uppercase; letter-spacing: 1.5px;
      color: var(--text-dim); margin-top: 24px; padding-top: 12px;
      border-top: 1px dashed var(--border); font-family: var(--font-mono);
      display: flex; align-items: center; justify-content: center; gap: 6px;
    }
    #masterPanel h3:first-of-type, .module-title:first-child { margin-top: 10px; }
    .module-title svg { flex: 0 0 auto; }
    .module-title.eq-title svg     { stroke: var(--accent-eq); }
    .module-title.route-title svg  { stroke: var(--accent-route); }
    .module-title.reverb-title svg { stroke: var(--accent-reverb); }
    .module-title.comp-title svg   { stroke: var(--accent-comp); }

    .ch-badge {
      display: inline-flex; align-items: center; justify-content: center;
      width: 28px; height: 28px; border-radius: 8px;
      background: var(--ch-accent, var(--accent-route));
      color: #000; font-weight: 800; font-size: 0.8rem; font-family: var(--font-mono);
      margin-right: 10px;
      box-shadow: 0 0 10px var(--ch-accent);
    }
    .channel-strip h2 { display: flex; align-items: center; justify-content: center; font-size: 1.1rem; }
    .channel-strip::after {
      content: ''; position: absolute; top: 0; left: 0; right: 0; height: 4px;
      background: var(--ch-accent, var(--accent-route)); border-radius: 16px 16px 0 0;
      box-shadow: 0 0 15px var(--ch-accent); opacity: 0.8;
    }

    /* ---------- Collapsible Sections (Dynamics only) ---------- */
    details.ctrl-section { margin-top: 15px; background: rgba(0,0,0,0.15); border-radius: 12px; overflow: hidden; border: 1px solid var(--border); }
    details.ctrl-section > summary {
      list-style: none; cursor: pointer; display: flex; align-items: center; gap: 10px;
      font-size: 0.8rem; font-weight: 600; color: var(--text-main);
      padding: 12px 14px; user-select: none; transition: background 0.2s;
    }
    details.ctrl-section > summary::-webkit-details-marker { display: none; }
    details.ctrl-section > summary:hover { background: rgba(255,255,255,0.05); }
    details.ctrl-section > summary .chev {
      margin-left: auto; width: 8px; height: 8px;
      border-right: 2px solid var(--text-dim); border-bottom: 2px solid var(--text-dim);
      transform: rotate(-45deg); transition: transform 0.3s ease;
    }
    details[open].ctrl-section > summary .chev { transform: rotate(45deg); }
    .section-body { padding: 6px 12px 14px; }
    .comp-section summary svg { stroke: var(--accent-comp); }

    /* ==================================================================
       GRAPHIC EQ — 15-band vertical mini-faders
       ================================================================== */
    .eq-grid {
      display: flex; gap: 7px; overflow-x: auto;
      padding: 6px 4px 8px;
      -webkit-overflow-scrolling: touch;
    }
    .eq-hint { font-size: 0.62rem; color: var(--text-dim); text-align: center; margin-top: -4px; margin-bottom: 6px; opacity: .7; font-family: var(--font-mono); }

    .fader-unit {
      display: flex; flex-direction: column; align-items: center; gap: 6px;
      flex: 0 0 auto;
    }
    .fader-unit .value {
      font-size: 0.65rem; font-family: var(--font-mono); color: var(--text-main);
      background: #000; border-radius: 5px; padding: 2px 5px; min-width: 30px; text-align: center;
      box-shadow: inset 0 2px 4px rgba(0,0,0,0.5);
    }
    .fader-unit .fader-label {
      font-size: 0.6rem; color: var(--text-dim); font-family: var(--font-mono);
      white-space: nowrap;
    }

    .fader-track-wrap { position: relative; width: 30px; }
    .eq-grid .fader-track-wrap { height: 98px; }
    .main-fader-wrap .fader-track-wrap { height: 170px; width: 40px; }
    .master-fader-wrap .fader-track-wrap { height: 170px; width: 40px; }

    .v-fader {
      -webkit-appearance: none; appearance: none;
      position: absolute; top: 50%; left: 50%;
      margin: 0; background: transparent; cursor: pointer;
      touch-action: none;
    }
    .eq-grid .v-fader { width: 98px; height: 26px; transform: translate(-50%, -50%) rotate(-90deg); }
    .main-fader-wrap .v-fader, .master-fader-wrap .v-fader { width: 170px; height: 32px; transform: translate(-50%, -50%) rotate(-90deg); }

    .v-fader::-webkit-slider-runnable-track {
      height: 6px; border-radius: 3px;
      background: #000;
      box-shadow: inset 0 1px 3px rgba(0,0,0,.8);
    }
    .v-fader.eq-fader::-webkit-slider-runnable-track {
      background:
        linear-gradient(to right, transparent calc(50% - 1px), rgba(255,255,255,.5) calc(50% - 1px), rgba(255,255,255,.5) calc(50% + 1px), transparent calc(50% + 1px)),
        #000;
    }
    .v-fader::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 14px; height: 28px; border-radius: 3px;
      background: linear-gradient(180deg,#e5e7eb,#9ca3af);
      border: 1px solid #000;
      box-shadow: 0 2px 4px rgba(0,0,0,.6);
      margin-top: -11px;
      cursor: grab;
      position: relative;
    }
    .v-fader::-webkit-slider-thumb:active { cursor: grabbing; }
    .main-fader-wrap .v-fader::-webkit-slider-thumb, .master-fader-wrap .v-fader::-webkit-slider-thumb {
      width: 18px; height: 34px; margin-top: -14px; border-radius: 4px;
    }

    .v-fader::-moz-range-track { height: 6px; border-radius: 3px; background: #000; }
    .v-fader.eq-fader::-moz-range-track {
      background:
        linear-gradient(to right, transparent calc(50% - 1px), rgba(255,255,255,.5) calc(50% - 1px), rgba(255,255,255,.5) calc(50% + 1px), transparent calc(50% + 1px)),
        #000;
    }
    .v-fader::-moz-range-thumb {
      width: 14px; height: 28px; border-radius: 3px;
      background: linear-gradient(180deg,#e5e7eb,#9ca3af);
      border: 1px solid #000; box-shadow: 0 2px 4px rgba(0,0,0,.6); cursor: grab;
    }
    .main-fader-wrap .v-fader::-moz-range-thumb, .master-fader-wrap .v-fader::-moz-range-thumb {
      width: 18px; height: 34px; border-radius: 4px;
    }

    .master-row { --thumb: var(--accent-master); --thumb-glow: var(--accent-master-glow); }
    .eq-row     { --thumb: var(--accent-eq);     --thumb-glow: var(--accent-eq-glow); }
    .route-row  { --thumb: var(--accent-route);  --thumb-glow: var(--accent-route-glow); }
    .reverb-row { --thumb: var(--accent-reverb); --thumb-glow: var(--accent-reverb-glow); }
    .comp-row   { --thumb: var(--accent-comp);   --thumb-glow: var(--accent-comp-glow); }

    .fader-unit.route-row .v-fader::-webkit-slider-thumb,
    .fader-unit.master-row .v-fader::-webkit-slider-thumb {
      border-color: var(--thumb);
    }

    /* ==================================================================
       MAIN / MASTER FADER blocks
       ================================================================== */
    .fader-block {
      display: flex; justify-content: center; align-items: flex-end;
      gap: 22px; margin-top: 6px;
    }
    .main-fader-wrap, .master-fader-wrap { display: flex; flex-direction: column; align-items: center; gap: 8px; }
    .fader-scale {
      position: absolute; left: -9px; top: 3px; bottom: 3px; width: 5px;
      background-image: repeating-linear-gradient(to bottom, rgba(255,255,255,.28) 0, rgba(255,255,255,.28) 1px, transparent 1px, transparent 25%);
    }

    /* ==================================================================
       KNOBS (rotary controls for sends / reverb / dynamics)
       ================================================================== */
    .knob-grid {
      display: grid; grid-template-columns: repeat(auto-fit, minmax(52px, 1fr));
      gap: 14px 6px; padding: 4px 2px 2px;
    }
    .knob-unit { display: flex; flex-direction: column; align-items: center; gap: 6px; }
    .knob-label {
      font-size: 0.62rem; color: var(--text-dim); font-family: var(--font-mono);
      text-transform: uppercase; letter-spacing: .4px; text-align: center;
    }
    .knob {
      position: relative; width: 44px; height: 44px;
      cursor: grab; touch-action: none; outline: none;
    }
    .knob:active { cursor: grabbing; }
    .knob:focus-visible .knob-body { box-shadow: 0 0 0 3px var(--thumb-glow, rgba(56,189,248,.35)), 0 2px 6px rgba(0,0,0,.6); }
    .knob.active .knob-body { box-shadow: 0 0 0 4px var(--thumb-glow, rgba(56,189,248,.4)), 0 2px 6px rgba(0,0,0,.6); }

    .knob-ring {
      position: absolute; inset: 0; border-radius: 50%;
      background: conic-gradient(from -135deg,
        var(--thumb) 0deg, var(--thumb) calc(var(--pct, 0) * 270deg),
        rgba(255,255,255,.1) calc(var(--pct, 0) * 270deg) 270deg,
        transparent 270deg 360deg);
      -webkit-mask: radial-gradient(farthest-side, transparent calc(100% - 5px), #000 calc(100% - 5px));
      mask: radial-gradient(farthest-side, transparent calc(100% - 5px), #000 calc(100% - 5px));
    }
    .knob-body {
      position: absolute; inset: 7px; border-radius: 50%;
      background: radial-gradient(circle at 35% 28%, #3d414c, #191b21 72%);
      box-shadow: 0 2px 6px rgba(0,0,0,.6), inset 0 1px 1px rgba(255,255,255,.08);
      transform: rotate(calc(-135deg + var(--pct, 0) * 270deg));
      transition: box-shadow .15s ease;
    }
    .knob-pointer {
      position: absolute; top: 3px; left: 50%; width: 2px; height: 38%;
      background: var(--thumb); border-radius: 1px; transform: translateX(-50%);
      box-shadow: 0 0 4px var(--thumb);
    }
    .knob-unit .value {
      font-size: 0.65rem; font-family: var(--font-mono); color: var(--text-main);
      background: #000; border-radius: 5px; padding: 2px 5px; min-width: 34px; text-align: center;
      box-shadow: inset 0 2px 4px rgba(0,0,0,0.5);
    }

    /* ---------- VU Meters ---------- */
    .vu-container { display: flex; justify-content: center; gap: 20px; margin: 15px 0; }
    .vu-wrapper { display: flex; flex-direction: column; align-items: center; gap: 8px; }

    .vu {
      width: 14px; height: 160px;
      background: #000; border-radius: 4px;
      position: relative; overflow: hidden;
      box-shadow: inset 0 2px 10px rgba(0,0,0,0.8), 0 0 0 1px var(--border);
    }
    .vu-fill {
      position: absolute; bottom: 0; left: 0; width: 100%; height: 0%;
      background: linear-gradient(to top,
        #10b981 0%, #10b981 55%,
        #fbbf24 65%, #fbbf24 85%,
        #ef4444 92%, #ef4444 100%);
      transition: height 0.08s ease-out; z-index: 1;
      box-shadow: 0 0 10px rgba(16, 185, 129, 0.5);
    }
    .vu::after {
      content: ''; position: absolute; inset: 0;
      background-image: repeating-linear-gradient(to bottom, transparent 0, transparent 4px, #000 4px, #000 6px);
      z-index: 2; pointer-events: none;
    }
    .vu-caption { font-size: 0.7rem; color: var(--text-dim); font-family: var(--font-mono); font-weight: 600; }

    .info-box {
      font-size: 0.7rem; line-height: 1.5; color: var(--text-dim);
      background: rgba(251, 191, 36, 0.08); padding: 12px; border-radius: 10px;
      border: 1px solid rgba(251, 191, 36, 0.2); margin-top: 20px; text-align: center;
    }

    @media (prefers-reduced-motion: reduce) {
      html { scroll-behavior: auto; }
      .vu-fill { transition: none; }
      .status.ws-connected::before { animation: none; }
    }
  </style>
</head>
<body>
<header>
  <div class="brand">
    <div class="brand-icon">
      <svg viewBox="0 0 24 24" fill="none" stroke-width="2.5" stroke-linecap="round">
        <line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><circle cx="4" cy="12" r="2.5" fill="currentColor" stroke="none"/>
        <line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><circle cx="12" cy="10" r="2.5" fill="currentColor" stroke="none"/>
        <line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/><circle cx="20" cy="14" r="2.5" fill="currentColor" stroke="none"/>
      </svg>
    </div>
    <div class="brand-text">
      <div class="title">ESP32-P4 DSP Console</div>
      <div class="subtitle">8 Channel Digital Mixer</div>
    </div>
  </div>
  <div class="status" id="wsStatus">WS: Wait...</div>
</header>

<nav class="channel-nav" id="channelNav" aria-label="Navigasi channel"></nav>

<main>
  <div class="panel" id="masterPanel">
    <h2>MASTER BUS</h2>

    <div class="vu-container">
      <div class="vu-wrapper">
        <div class="vu" data-vu="masterL"><div class="vu-fill"></div></div>
        <div class="vu-caption">MAIN L</div>
      </div>
      <div class="vu-wrapper">
        <div class="vu" data-vu="masterR"><div class="vu-fill"></div></div>
        <div class="vu-caption">MAIN R</div>
      </div>
    </div>

    <h3>Main Output Fader</h3>
    <div class="fader-block">
      <div class="master-fader-wrap fader-unit master-row">
        <span class="value">0.80</span>
        <div class="fader-track-wrap">
          <div class="fader-scale"></div>
          <input type="range" class="v-fader" min="0" max="1" step="0.01" value="0.8" data-master="lrL" aria-label="Master Out L">
        </div>
        <span class="fader-label">OUT&nbsp;L</span>
      </div>
      <div class="master-fader-wrap fader-unit master-row">
        <span class="value">0.80</span>
        <div class="fader-track-wrap">
          <div class="fader-scale"></div>
          <input type="range" class="v-fader" min="0" max="1" step="0.01" value="0.8" data-master="lrR" aria-label="Master Out R">
        </div>
        <span class="fader-label">OUT&nbsp;R</span>
      </div>
    </div>

    <h3>Sub Groups &amp; Aux</h3>
    <div class="knob-grid" id="masterSendsGrid"></div>

    <h3>Bus Meters</h3>
    <div class="vu-container" style="gap:15px; margin-top:10px;">
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="group1"><div class="vu-fill"></div></div><div class="vu-caption">GRP1</div></div>
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="group2"><div class="vu-fill"></div></div><div class="vu-caption">GRP2</div></div>
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="aux1"><div class="vu-fill"></div></div><div class="vu-caption">AUX1</div></div>
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="aux2"><div class="vu-fill"></div></div><div class="vu-caption">AUX2</div></div>
    </div>

    <div class="info-box">
      <b>Info Routing:</b> Slider UI mengontrol pemrosesan DSP internal di ESP. Jalur audio analog dikonfigurasi melalui PCM1808/PCM5102.
    </div>
  </div>

  <div id="channelsContainer"></div>
</main>

<script>
  const NUM_CHANNELS = 8;
  const NUM_EQ_BANDS = 15;

  const ICONS = {
    eq: '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M4 21v-7m0-4V3m8 18v-9m0-4V3m8 18v-5m0-4V3M1 14h6m2-4h6m2-4h6"/></svg>',
    route: '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12h6l3-7 3 14 3-7h3"/></svg>',
    reverb: '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M4 12v.01M8 12v.01M12 12v.01M16 12v.01M20 12v.01M4 8a16 16 0 0 1 16 0"/></svg>',
    comp: '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/><path d="M8 12h8m-4-4v8"/></svg>'
  };

  const state = {
    master: { lrL: 0.8, lrR: 0.8, group1: 0.8, group2: 0.8, aux1: 0.8, aux2: 0.8 },
    channels: Array.from({ length: NUM_CHANNELS }, () => ({
      eq: new Array(NUM_EQ_BANDS).fill(0.0),
      send: { lr: 1.0, group1: 0.0, group2: 0.0, aux1: 0.0, aux2: 0.0 },
      reverb: 0.0,
      comp: { ratio: 2.0, attack: 10.0, release: 200.0, threshold: -20.0, gain: 0.0 }
    }))
  };

  const channelsContainer = document.getElementById('channelsContainer');
  const channelNav = document.getElementById('channelNav');

  // ================== Widget: Vertical Fader ==================
  // Native <input type=range>, rotated -90deg -> min at bottom, max at top,
  // exactly like a real mixing-console channel fader.
  function createFaderUnit(labelText, min, max, step, initValue, onChange, rowClass, opts) {
    opts = opts || {};
    const wrap = document.createElement('div');
    wrap.className = 'fader-unit ' + (rowClass || '') + (opts.big ? ' ' + (opts.wrapClass || 'main-fader-wrap') : '');

    const valueEl = document.createElement('span');
    valueEl.className = 'value';
    valueEl.textContent = initValue.toFixed((max - min > 2) ? 0 : 2);

    const trackWrap = document.createElement('div');
    trackWrap.className = 'fader-track-wrap';
    if (opts.scale) {
      const scale = document.createElement('div');
      scale.className = 'fader-scale';
      trackWrap.appendChild(scale);
    }

    const input = document.createElement('input');
    input.type = 'range';
    input.className = 'v-fader' + (opts.eq ? ' eq-fader' : '');
    input.min = min; input.max = max; input.step = step; input.value = initValue;
    input.setAttribute('aria-label', labelText);
    trackWrap.appendChild(input);

    const label = document.createElement('span');
    label.className = 'fader-label';
    label.textContent = labelText;

    input.addEventListener('input', () => {
      const v = parseFloat(input.value);
      valueEl.textContent = (max - min > 2) ? v.toFixed(0) : v.toFixed(2);
      onChange(v);
      scheduleSend();
    });

    wrap.append(valueEl, trackWrap, label);
    return wrap;
  }

  // ================== Widget: Rotary Knob ==================
  // Custom knob control: drag vertically (mouse/touch/pen via Pointer Events)
  // or use arrow keys / mouse wheel. Mirrors the aux-send / dynamics knobs
  // found on real mixer channel strips.
  function createKnob(labelText, min, max, step, initValue, onChange, rowClass) {
    const wrap = document.createElement('div');
    wrap.className = 'knob-unit ' + (rowClass || '');

    const label = document.createElement('span');
    label.className = 'knob-label';
    label.textContent = labelText;

    const knob = document.createElement('div');
    knob.className = 'knob';
    knob.tabIndex = 0;
    knob.setAttribute('role', 'slider');
    knob.setAttribute('aria-label', labelText);
    knob.setAttribute('aria-valuemin', min);
    knob.setAttribute('aria-valuemax', max);

    const ring = document.createElement('div'); ring.className = 'knob-ring';
    const body = document.createElement('div'); body.className = 'knob-body';
    const pointer = document.createElement('div'); pointer.className = 'knob-pointer';
    body.appendChild(pointer);
    knob.append(ring, body);

    const valueEl = document.createElement('span');
    valueEl.className = 'value';

    let value = initValue;
    const decimals = (max - min > 2) ? 0 : 2;
    const range = max - min;
    const sensitivity = 130; // px of vertical drag for full sweep

    function render() {
      const pct = (value - min) / range;
      knob.style.setProperty('--pct', pct.toFixed(4));
      valueEl.textContent = value.toFixed(decimals);
      knob.setAttribute('aria-valuenow', value.toFixed(decimals));
    }
    render();

    function setValue(v, fire) {
      v = Math.min(max, Math.max(min, v));
      v = Math.round(v / step) * step;
      v = parseFloat(v.toFixed(6));
      if (v === value) return;
      value = v;
      render();
      if (fire !== false) { onChange(value); scheduleSend(); }
    }

    let dragging = false, startY = 0, startVal = 0;

    knob.addEventListener('pointerdown', (e) => {
      dragging = true;
      startY = e.clientY;
      startVal = value;
      knob.setPointerCapture(e.pointerId);
      knob.classList.add('active');
      knob.focus();
    });
    knob.addEventListener('pointermove', (e) => {
      if (!dragging) return;
      const dy = startY - e.clientY;
      setValue(startVal + (dy / sensitivity) * range);
    });
    function endDrag() { dragging = false; knob.classList.remove('active'); }
    knob.addEventListener('pointerup', endDrag);
    knob.addEventListener('pointercancel', endDrag);

    knob.addEventListener('keydown', (e) => {
      if (e.key === 'ArrowUp' || e.key === 'ArrowRight') { setValue(value + step); e.preventDefault(); }
      else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') { setValue(value - step); e.preventDefault(); }
    });
    knob.addEventListener('wheel', (e) => {
      e.preventDefault();
      setValue(value + (e.deltaY < 0 ? step : -step));
    }, { passive: false });

    wrap.append(label, knob, valueEl);
    return wrap;
  }

  function createModuleTitle(text, iconKey, titleClass) {
    const h = document.createElement('h3');
    h.className = 'module-title ' + (titleClass || '');
    h.innerHTML = (ICONS[iconKey] || '') + '<span>' + text + '</span>';
    return h;
  }

  function createChannelStrip(chIndex) {
    const panel = document.createElement('div');
    panel.className = 'panel channel-strip';
    panel.id = 'channel-' + chIndex;
    const hue = Math.round((360 / NUM_CHANNELS) * chIndex);
    panel.style.setProperty('--ch-accent', `hsl(${hue}, 85%, 60%)`);

    const title = document.createElement('h2');
    title.innerHTML = `<span class="ch-badge">${chIndex + 1}</span> CH ${chIndex + 1}`;
    panel.appendChild(title);

    // ---- Meter ----
    const vuContainer = document.createElement('div');
    vuContainer.className = 'vu-container';
    vuContainer.innerHTML = `<div class="vu-wrapper"><div class="vu" data-vu="ch${chIndex}"><div class="vu-fill"></div></div><div class="vu-caption">LEVEL</div></div>`;
    panel.appendChild(vuContainer);

    // ---- Graphic EQ (always visible - this IS the mixer's signature look) ----
    panel.appendChild(createModuleTitle('Graphic EQ 15-Band', 'eq', 'eq-title'));
    const eqGrid = document.createElement('div');
    eqGrid.className = 'eq-grid';
    const freqs = [31, 63, 125, 250, 500, 1000, 2000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 18000];
    freqs.forEach((f, idx) => {
      eqGrid.appendChild(createFaderUnit(
        f < 1000 ? `${f}` : `${f / 1000}k`,
        -12, 12, 0.5, 0.0,
        (val) => state.channels[chIndex].eq[idx] = val,
        'eq-row',
        { eq: true }
      ));
    });
    panel.appendChild(eqGrid);
    const hint = document.createElement('div');
    hint.className = 'eq-hint';
    hint.textContent = '\u2190 geser untuk lihat semua band \u2192';
    panel.appendChild(hint);

    // ---- Sends & FX (knobs) ----
    panel.appendChild(createModuleTitle('Sends &amp; FX', 'route', 'route-title'));
    const sendGrid = document.createElement('div');
    sendGrid.className = 'knob-grid';
    const sendDefs = [
      { lbl: 'Grp 1', key: 'group1', init: 0.0 },
      { lbl: 'Grp 2', key: 'group2', init: 0.0 },
      { lbl: 'Aux 1', key: 'aux1', init: 0.0 },
      { lbl: 'Aux 2', key: 'aux2', init: 0.0 }
    ];
    sendDefs.forEach(d => {
      sendGrid.appendChild(createKnob(d.lbl, 0, 1, 0.01, d.init,
        v => state.channels[chIndex].send[d.key] = v, 'route-row'));
    });
    sendGrid.appendChild(createKnob('Reverb', 0, 1, 0.01, 0.0,
      v => state.channels[chIndex].reverb = v, 'reverb-row'));
    panel.appendChild(sendGrid);

    // ---- Dynamics (collapsible - advanced processing, tucked away like an insert module) ----
    const compDetails = document.createElement('details');
    compDetails.className = 'ctrl-section comp-section';
    const compSummary = document.createElement('summary');
    compSummary.innerHTML = ICONS.comp + '<span>Dynamics (Compressor)</span><span class="chev"></span>';
    const compBody = document.createElement('div');
    compBody.className = 'section-body';
    const compGrid = document.createElement('div');
    compGrid.className = 'knob-grid';
    const compParams = [
      { lbl: 'Ratio', min: 1, max: 10, step: 0.1, init: 2.0, key: 'ratio' },
      { lbl: 'Attack', min: 0, max: 100, step: 1, init: 10, key: 'attack' },
      { lbl: 'Release', min: 10, max: 1000, step: 10, init: 200, key: 'release' },
      { lbl: 'Thresh', min: -60, max: 0, step: 1, init: -20, key: 'threshold' },
      { lbl: 'Makeup', min: -12, max: 12, step: 0.5, init: 0, key: 'gain' }
    ];
    compParams.forEach(p => {
      compGrid.appendChild(createKnob(p.lbl, p.min, p.max, p.step, p.init,
        v => state.channels[chIndex].comp[p.key] = v, 'comp-row'));
    });
    compBody.appendChild(compGrid);
    compDetails.append(compSummary, compBody);
    panel.appendChild(compDetails);

    // ---- Channel fader (the hero control, always visible at the bottom) ----
    panel.appendChild(createModuleTitle('Channel Fader', 'route', 'route-title'));
    const faderBlock = document.createElement('div');
    faderBlock.className = 'fader-block';
    faderBlock.appendChild(createFaderUnit('TO L/R', 0, 1, 0.01, 1.0,
      v => state.channels[chIndex].send.lr = v, 'route-row',
      { big: true, wrapClass: 'main-fader-wrap', scale: true }));
    panel.appendChild(faderBlock);

    return panel;
  }

  // Navigasi
  const masterPill = document.createElement('button');
  masterPill.className = 'ch-pill master-pill';
  masterPill.textContent = 'Master';
  masterPill.onclick = () => document.getElementById('masterPanel').scrollIntoView({ behavior: 'smooth', inline: 'start' });
  channelNav.appendChild(masterPill);

  for (let ch = 0; ch < NUM_CHANNELS; ch++) {
    channelsContainer.appendChild(createChannelStrip(ch));
    const pill = document.createElement('button');
    pill.className = 'ch-pill';
    pill.textContent = ch + 1;
    pill.style.setProperty('--ch-accent', `hsl(${Math.round(360 / NUM_CHANNELS * ch)}, 85%, 60%)`);
    pill.onclick = () => document.getElementById(`channel-${ch}`).scrollIntoView({ behavior: 'smooth', inline: 'start' });
    channelNav.appendChild(pill);
  }

  // Master Out L/R faders (native range inputs, matched via data-master)
  document.querySelectorAll('#masterPanel input[type=range]').forEach(input => {
    const key = input.dataset.master;
    input.value = state.master[key];
    const span = input.closest('.fader-unit').querySelector('.value');

    input.addEventListener('input', () => {
      const v = parseFloat(input.value);
      state.master[key] = v;
      span.textContent = v.toFixed(2);
      scheduleSend();
    });
  });

  // Master Group/Aux send knobs
  const masterSendsGrid = document.getElementById('masterSendsGrid');
  [
    { lbl: 'Group 1', key: 'group1' },
    { lbl: 'Group 2', key: 'group2' },
    { lbl: 'Aux 1', key: 'aux1' },
    { lbl: 'Aux 2', key: 'aux2' }
  ].forEach(d => {
    masterSendsGrid.appendChild(createKnob(d.lbl, 0, 1, 0.01, state.master[d.key],
      v => state.master[d.key] = v, 'master-row'));
  });

  let ws = null, sendTimer = null;

  function connectWS() {
    const statusEl = document.getElementById('wsStatus');
    ws = new WebSocket('ws://192.168.4.1/ws'); // Ganti dengan IP lokal ESP32

    ws.onopen = () => {
      statusEl.textContent = 'WS: Connected';
      statusEl.className = 'status ws-connected';
      sendParams();
    };

    ws.onclose = () => {
      statusEl.textContent = 'WS: Disconnected';
      statusEl.className = 'status ws-disconnected';
      setTimeout(connectWS, 2000);
    };

    ws.onerror = () => {
      statusEl.textContent = 'WS: Error';
      statusEl.className = 'status ws-disconnected';
    };

    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data);
        if (msg.type === 'vu') updateVuFromMsg(msg);
      } catch (e) {
        console.warn('WS Parsing Error:', e);
      }
    };
  }

  function scheduleSend() {
    if (sendTimer) clearTimeout(sendTimer);
    sendTimer = setTimeout(() => {
      if (ws?.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: 'set', master: state.master, channels: state.channels }));
      }
    }, 50);
  }

  function sendParams() { scheduleSend(); }

  function updateVuFromMsg(msg) {
    msg.channels?.forEach((val, idx) => {
      const vu = document.querySelector(`.vu[data-vu="ch${idx}"] .vu-fill`);
      if (vu) {
        let height = val * 100;
        vu.style.height = `${Math.min(Math.max(height, 0), 100).toFixed(1)}%`;
        if (height > 90) vu.style.boxShadow = "0 0 15px rgba(239, 68, 68, 0.8)";
        else vu.style.boxShadow = "0 0 10px rgba(16, 185, 129, 0.5)";
      }
    });

    const bus = msg.bus || {};
    ['masterL','masterR','group1','group2','aux1','aux2'].forEach(key => {
      const vu = document.querySelector(`.vu[data-vu="${key}"] .vu-fill`);
      if (vu && typeof bus[key] === 'number') {
        vu.style.height = `${(bus[key] * 100).toFixed(1)}%`;
      }
    });
  }

  connectWS();
</script>
</body>
</html>

)rawliteral";

static uint32_t lastVusend = 0;

static esp_err_t ws_send_text(const char *text) {
  if (ws_client_fd < 0) return ESP_FAIL;
  httpd_ws_frame_t frame = {};
  frame.type = HTTPD_WS_TYPE_TEXT;
  frame.payload = (uint8_t*)text;
  frame.len = strlen(text);
  esp_err_t ret = httpd_ws_send_frame_async(server, ws_client_fd, &frame);
  if (ret != ESP_OK) {
    ws_client_fd = -1;
  }
  return ret;
}

void webSocketBroadcastVu() {
  if (ws_client_fd < 0) return;
  uint32_t now = millis();
  if (now - lastVusend < 40) return;
  lastVusend = now;
  taskENTER_CRITICAL(&wsMux);
  if (!wsStateChanged) { taskEXIT_CRITICAL(&wsMux); return; }
  wsStateChanged = false;
  DynamicJsonDocument doc(2048);
  doc["type"] = "vu";
  JsonArray chArr = doc.createNestedArray("channels");
  for (int ch = 0; ch < NUM_MIX_CH; ch++) chArr.add(channels[ch].vu);
  JsonObject bus = doc.createNestedObject("bus");
  bus["masterL"] = master.vuMasterL;
  bus["masterR"] = master.vuMasterR;
  bus["group1"] = master.vuGroup1;
  bus["group2"] = master.vuGroup2;
  bus["aux1"] = master.vuAux1;
  bus["aux2"] = master.vuAux2;
  taskEXIT_CRITICAL(&wsMux);
  String output;
  serializeJson(doc, output);
  ws_send_text(output.c_str());
}

static esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ws_client_fd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WS client connected, fd=%d", ws_client_fd);
    return ESP_OK;
  }
  httpd_ws_frame_t frame = {};
  uint8_t buf[4096] = {0};
  frame.payload = buf;
  frame.len = sizeof(buf) - 1;
  esp_err_t ret = httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1);
  if (ret != ESP_OK) {
    if (ret == ESP_ERR_HTTPD_INVALID_REQ) ws_client_fd = -1;
    return ret;
  }
  if (frame.type == HTTPD_WS_TYPE_CLOSE) {
    ws_client_fd = -1;
    return ESP_OK;
  }
  if (frame.type == HTTPD_WS_TYPE_TEXT && frame.len > 0) {
    buf[frame.len] = 0;
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, (const char*)buf);
    if (err) return ESP_OK;
    const char *type = doc["type"];
    if (type && strcmp(type, "set") == 0) {
      JsonObject mst = doc["master"];
      if (!mst.isNull()) {
        master.lrL = mst["lrL"] | master.lrL;
        master.lrR = mst["lrR"] | master.lrR;
        master.group1 = mst["group1"] | master.group1;
        master.group2 = mst["group2"] | master.group2;
        master.aux1 = mst["aux1"] | master.aux1;
        master.aux2 = mst["aux2"] | master.aux2;
      }
      JsonArray chArr = doc["channels"];
      if (!chArr.isNull()) {
        for (int ch = 0; ch < min((int)chArr.size(), NUM_MIX_CH); ch++) {
          JsonObject c = chArr[ch];
          if (c.isNull()) continue;
          JsonArray eqArr = c["eq"];
          if (!eqArr.isNull()) {
            for (int b = 0; b < min((int)eqArr.size(), NUM_EQ_BANDS); b++) {
              channels[ch].eq[b] = eqArr[b] | channels[ch].eq[b];
            }
          }
          JsonObject snd = c["send"];
          if (!snd.isNull()) {
            channels[ch].send.lr = snd["lr"] | channels[ch].send.lr;
            channels[ch].send.group1 = snd["group1"] | channels[ch].send.group1;
            channels[ch].send.group2 = snd["group2"] | channels[ch].send.group2;
            channels[ch].send.aux1 = snd["aux1"] | channels[ch].send.aux1;
            channels[ch].send.aux2 = snd["aux2"] | channels[ch].send.aux2;
          }
          channels[ch].reverb = c["reverb"] | channels[ch].reverb;
          JsonObject comp = c["comp"];
          if (!comp.isNull()) {
            channels[ch].comp.ratio = comp["ratio"] | channels[ch].comp.ratio;
            channels[ch].comp.attack = comp["attack"] | channels[ch].comp.attack;
            channels[ch].comp.release = comp["release"] | channels[ch].comp.release;
            channels[ch].comp.threshold = comp["threshold"] | channels[ch].comp.threshold;
            channels[ch].comp.gain = comp["gain"] | channels[ch].comp.gain;
          }
        }
      }
    }
  }
  return ESP_OK;
}

static esp_err_t api_status_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  DynamicJsonDocument doc(1024);
  doc["sr"] = SampleRateFreq;
  doc["ch"] = NUM_MIX_CH;
  doc["ws"] = (ws_client_fd >= 0);
  doc["mute"] = Mute;
  doc["comp"] = Compressor;
  bool sta_connected = (WiFi.status() == WL_CONNECTED);
  doc["sta_connected"] = sta_connected;
  if (sta_connected) {
    doc["sta_ssid"] = WiFi.SSID();
    doc["sta_ip"] = WiFi.localIP().toString();
  }
  String output;
  serializeJson(doc, output);
  httpd_resp_sendstr(req, output.c_str());
  return ESP_OK;
}

static esp_err_t api_wifi_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  char buf[512] = {0};
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"no data\"}");
    return ESP_OK;
  }
  buf[ret] = 0;
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, buf);
  if (err) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"bad json\"}");
    return ESP_OK;
  }
  const char *ssid = doc["ssid"];
  const char *password = doc["password"] | "";
  if (!ssid || strlen(ssid) == 0) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"ssid required\"}");
    return ESP_OK;
  }
  String content = String(ssid) + "\n" + String(password);
  if (!writeFile("/wifi.txt", content)) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"write failed\"}");
    return ESP_OK;
  }
  WiFi.begin(ssid, password);
  httpd_resp_sendstr(req, "{\"ok\":true}");
  ESP_LOGI(TAG, "WiFi config saved, connecting to '%s'", ssid);
  return ESP_OK;
}

static esp_err_t api_save_handler(httpd_req_t *req) {
  saveConfig();
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true}");
  return ESP_OK;
}

static esp_err_t api_reboot_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true}");
  esp_restart();
  return ESP_OK;
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, HTML_INDEX);
  return ESP_OK;
}

static const httpd_uri_t uri_index = {
  .uri = "/", .method = HTTP_GET, .handler = index_handler
};
static const httpd_uri_t uri_ws = {
  .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .is_websocket = true
};
static const httpd_uri_t uri_api_status = {
  .uri = "/api/status", .method = HTTP_GET, .handler = api_status_handler
};
static const httpd_uri_t uri_api_wifi = {
  .uri = "/api/wifi", .method = HTTP_POST, .handler = api_wifi_handler
};
static const httpd_uri_t uri_api_save = {
  .uri = "/api/save", .method = HTTP_POST, .handler = api_save_handler
};
static const httpd_uri_t uri_api_reboot = {
  .uri = "/api/reboot", .method = HTTP_POST, .handler = api_reboot_handler
};

esp_err_t startWebServer() {
  if (server) return ESP_OK;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 12;
  config.stack_size = 8192;
  config.lru_purge_enable = true;
  if (httpd_start(&server, &config) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return ESP_FAIL;
  }
  httpd_register_uri_handler(server, &uri_index);
  httpd_register_uri_handler(server, &uri_ws);
  httpd_register_uri_handler(server, &uri_api_status);
  httpd_register_uri_handler(server, &uri_api_wifi);
  httpd_register_uri_handler(server, &uri_api_save);
  httpd_register_uri_handler(server, &uri_api_reboot);
  ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
  return ESP_OK;
}

esp_err_t stopWebServer() {
  if (server) { httpd_stop(server); server = NULL; }
  ws_client_fd = -1;
  return ESP_OK;
}

esp_err_t initWiFi() {
  ESP_LOGI(TAG, "Initializing WiFi (Arduino)...");

  String ssid = readFile("/wifi.txt");
  String pass = "";
  if (ssid.isEmpty()) { ssid = "mixer"; pass = "12345678"; }
  int newline = ssid.indexOf('\n');
  if (newline > 0) {
    pass = ssid.substring(newline + 1); pass.trim();
    ssid = ssid.substring(0, newline); ssid.trim();
  }

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("AudioMixer-8ch", "mixer1234");
  ESP_LOGI(TAG, "AP mode: SSID 'AudioMixer-8ch'");

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
      case ARDUINO_EVENT_WIFI_STA_START:
        WiFi.reconnect();
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
        ESP_LOGI(TAG, "STA disconnected (reason: %d)", info.wifi_sta_disconnected.reason);
        unsigned long now = millis();
        if (now - lastReconnectAttempt > wifiReconnectDelay) {
          lastReconnectAttempt = now;
          wifiRetryCount++;
          wifiReconnectDelay = min((unsigned long)(wifiRetryCount * 2000), 60000UL);
          WiFi.reconnect();
        }
        break;
      }
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        wifiRetryCount = 0;
        wifiReconnectDelay = 0;
        ESP_LOGI(TAG, "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
        break;
    }
  });

  if (ssid.length() > 0 && ssid != "mixer") {
    WiFi.begin(ssid.c_str(), pass.c_str());
    ESP_LOGI(TAG, "STA mode: SSID '%s'", ssid.c_str());
  }

  WiFi.setSleep(false);
  ESP_LOGI(TAG, "WiFi init complete");
  return ESP_OK;
}
