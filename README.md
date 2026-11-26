# P4 Digital Mixer

Professional 8-channel digital loudspeaker management system firmware for the **ESP32-P4**, with a browser console served over HTTP/WebSocket. The signal chain provides 15-band EQ, per-channel compressor and reverb, and five output buses (Main L/R, Group 1–2, Aux 1–2).

> **Build profile:** the firmware currently runs the simplified **2 physical input / 8 mix channel** configuration defined in `main/app_config.h`. The original 8-channel TDM pin definitions and the multiband main-bus processor are retained as legacy code but are not active in this build.

---

## 1. Feature set

### Audio
- 8 mixer channels; channels 1–2 are physical inputs, channels 3–8 are mix channels
- 15-band graphic EQ per channel (31 Hz … 18 kHz)
- Per-channel compressor (ratio, attack, release, threshold, makeup gain)
- Per-channel reverb send
- 5-bus routing: `lr`, `group1`, `group2`, `aux1`, `aux2`
- Master faders for Main L/R; master mute
- Real-time level metering for every channel and bus

### Connectivity
- HTTP server and WebSocket endpoint on the device
- WiFi AP `AudioMixer-8ch` / `mixer1234` at `192.168.4.1`
- Optional WiFi STA credentials from SPIFFS `/wifi.txt`
- Ethernet initialized alongside WiFi (`initEthernet()`)
- Serial command console for diagnostics and configuration

### Storage
- SPIFFS `/config.json` holds the persisted mixer configuration
- SPIFFS is auto-formatted on mount failure
- `/wifi.txt` holds STA credentials (line 1 SSID, line 2 password)

---

## 2. Hardware

### Board
Waveshare ESP32-P4-NANO (or any ESP32-P4 with an ESP32-C6 SDIO WiFi coprocessor and the I2S1 header). The C6 provides WiFi over SDIO; the P4 runs the audio DSP.

### Active I2S pins (`main/app_config.h`)

| Signal | GPIO | Direction / function |
|--------|------|----------------------|
| `I2S0_MCLK_PIN` | 20 | Master clock out (no buffer) |
| `I2S_LRCLK_PIN` | 21 | Word select / LR clock |
| `I2S_BCK_PIN` | 22 | Bit clock |
| `I2S_DIN_PIN` | 23 | ADC data in — PCM1808 #1 (channels 1–2) |
| `I2S_DIN2_PIN` | 24 | ADC data in — PCM1808 #2 (channels 3–4) |
| `I2S_DOUT_PIN` | -1 | Unused in this build (no DAC output) |
| `CODEC_ENABLE_PIN` | -1 | Unused |

- Sample rate: `48000` Hz
- `channelCount = 2`, `NUM_PHYS_IN = 2`, `NUM_MIX_CH = 8`
- I2S port: `I2S_NUM_1`
- Watchdog timeout: `WDT_TIMEOUT = 10` s

### ESP32-C6 SDIO pins (internal)

| Signal | GPIO |
|--------|------|
| CLK | 18 |
| CMD | 19 |
| D0 | 14 |
| D1 | 15 |
| D2 | 16 |
| D3 | 17 |
| RST | 54 |

### Legacy 8-channel TDM pin map (not active)

The original build used MCLK 21, BCK 22, LRCK 25, four ADC data lines (45, 32, 36, 33) and three DAC lines (26, 24, 27). These values are **not** used by the current simplified build; do not wire hardware from the legacy table.

---

## 3. Source layout and naming convention

```
main/
├── main.cpp              # Firmware entry: setup(), loop(), FreeRTOS tasks, I2S init
├── app_config.h          # Board pins, channel/bus counts, DSP limits, defaults
├── mixer_state.h/.cpp    # Global mixer state, DSP/audio task, persistence
├── dsp_compressor.h      # Stereo compressor / limiter class
├── serial_console.h      # Serial command interface
├── web_server.h/.cpp     # HTTP endpoints, WebSocket, WiFi/Ethernet setup
├── CMakeLists.txt        # ESP-IDF component registration
└── idf_component.yml     # Managed component dependencies
libraries/YummyDSP/       # Bundled DSP library (PlatformIO build)
components/yummydsp/      # Same library for the ESP-IDF build (keep in sync)
web-ui/                   # Next.js static console (see web-ui/README.md)
platformio.ini            # PlatformIO environment and SPIFFS data_dir
partitions_p4.csv         # 16 MB flash partition table (SPIFFS @ 0x390000, 0xC70000)
```

### Naming rules

| Element | Convention | Example |
|---------|-----------|---------|
| Source files | `snake_case`, role-based name | `mixer_state.cpp`, `dsp_compressor.h` |
| Header guard | `#pragma once` | — |
| Types / classes | `PascalCase` | `MixChannel`, `DualCompressor` |
| Functions / methods | `lower_snake_case` | `init_config`, `webSocketBroadcastVu` (legacy exception) |
| Variables / members | `lower_snake_case` | `channels`, `master_level` |
| Constants / macros | `UPPER_SNAKE_CASE` | `NUM_MIX_CH`, `WDT_TIMEOUT` |
| Structs | `PascalCase`, singular noun | `ChannelSends`, `MasterState` |
| Modules | one responsibility per file; declaration in `.h`, definition in `.cpp` | `web_server.h/.cpp` |

Legacy symbols such as `DualCompressor` and `webSocketBroadcastVu` are preserved for compatibility; new code follows the table above.

### Runtime tasks

| Task | Core | Purpose |
|------|------|---------|
| `audioTask` | 1 | I2S read/write, DSP graph, level metering |
| `webTask` | 0 | HTTP/WebSocket service |
| `serialCommTask` | 0 | Serial command interpreter |

---

## 4. Build and flash

### Firmware (PlatformIO)

Requires the pioarduino PlatformIO fork for ESP32-P4 support.

```bash
pio run -e esp32-p4
pio run -t upload -e esp32-p4 --upload-port /dev/ttyACM0
pio device monitor -p /dev/ttyACM0 -b 115200
```

On this machine use `C:\Users\Bratas\.platformio\penv\Scripts\pio.exe` if `pio` is not on PATH.

### Firmware (ESP-IDF / CI — authoritative release build)

```bash
idf.py set-target esp32p4
idf.py build
```

CI clones `arduino-esp32` (`idf-release/v5.5`) and `ArduinoJson` (`7.x`) into `components/` before building; those directories are build-time only and must not be committed.

The CMake project is `p4_digital_mixer`; release artifacts are `p4_digital_mixer.bin` and `p4_digital_mixer_merged.bin`.

### Web UI and SPIFFS image

```bash
cd web-ui
npm ci
npm run build:fs     # static export → web-ui/spiffs/
```

`platformio.ini` sets `data_dir = web-ui/spiffs`, so the exported UI can be flashed to the SPIFFS partition:

```bash
pio run -t uploadfs -e esp32-p4 --upload-port /dev/ttyACM0
```

The exported SPIFFS image is about 1.8 MB and fits the `0xC70000` (≈12.7 MB) SPIFFS partition.

> **Current limitation:** `main/web_server.cpp` still serves the embedded `HTML_INDEX` string at `/`. A wildcard SPIFFS file-serving handler has not been added yet, so uploading `spiffs/` alone does not replace the embedded page. See `web-ui/docs/PERFORMANCE-RESEARCH.md` for the UI/performance research record.

---

## 5. Web / API contract

### HTTP

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/` | Console page (currently the embedded `HTML_INDEX`) |
| GET | `/api/status` | Device status JSON |
| POST | `/api/wifi` | Write STA credentials to `/wifi.txt` |
| POST | `/api/save` | Persist mixer configuration |
| POST | `/api/reboot` | Restart the device |

`/api/status` returns:

```json
{
  "sr": 48000,
  "ch": 8,
  "ws": true,
  "mute": false,
  "comp": true,
  "sta_connected": true,
  "sta_ssid": "lab",
  "sta_ip": "192.168.1.50"
}
```

### WebSocket

Endpoint: `ws://<device-host>/ws` (same origin as the page).

Device → UI (level meters, sent roughly every 40 ms):

```json
{
  "type": "vu",
  "channels": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
  "bus": {
    "masterL": 0.0, "masterR": 0.0,
    "group1": 0.0, "group2": 0.0, "aux1": 0.0, "aux2": 0.0
  }
}
```

UI → device (control changes):

```json
{
  "type": "set",
  "master": { "lrL": 0.8, "lrR": 0.8, "group1": 0.8, "group2": 0.8, "aux1": 0.8, "aux2": 0.8 },
  "mute": false,
  "channels": [
    {
      "eq": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
      "send": { "lr": 1, "group1": 0, "group2": 0, "aux1": 0, "aux2": 0 },
      "reverb": 0,
      "comp": { "ratio": 2, "attack": 10, "release": 200, "threshold": -20, "gain": 0 },
      "mute": false
    }
  ]
}
```

> **Known gap:** the firmware `set` handler parses `master`, `eq`, `send`, `reverb` and `comp`, but currently **ignores the `mute` fields**. Mute is UI-local until the firmware handler is extended.

---

## 6. Runtime behavior

- Configuration is loaded from `/config.json`; the filesystem is formatted automatically if mounting fails.
- STA credentials come from `/wifi.txt`; if absent or set to `mixer`, the device stays in AP-only mode.
- WiFi is `WIFI_AP_STA`; Ethernet is initialized in parallel.
- WebSocket reconnects with capped exponential backoff (2 s → 15 s) and keeps a stable status label while reconnecting.
- Meter rendering in the web UI runs outside React via `lib/mixer/meter-bridge.ts` at 30 fps; incoming VU frames update a plain store, so meters never trigger a React render.
- Control drags are painted directly to the DOM and coalesced into one state commit per animation frame.

---

## 7. Project status

| Area | State |
|------|-------|
| Firmware audio path | Active (2 physical inputs, 8 mix channels) |
| Legacy 8-ch TDM / multiband DSP | Present but inactive |
| WebSocket VU streaming | Active |
| WebSocket control (`set`) | Active except `mute` |
| Web UI static export | Active (`web-ui/build`, `web-ui/spiffs`) |
| SPIFFS static file serving on device | Pending |
| Component tests for web UI | Pending |

---

## 8. Credits

- Original ESP32-P4 project: Junon M. — <https://github.com/junon10/esp32-multiband-dsp>
- DSP: YummyDSP by Junon M.
- ESP32-P4 PlatformIO support: pioarduino
