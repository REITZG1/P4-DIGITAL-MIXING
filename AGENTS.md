# P4-DIGITAL-MIXING

ESP32-P4 firmware for an 8-channel digital audio mixer with a WebSocket web UI. C++ (Arduino framework), no tests.

## Build

CI is the authoritative/release build (ESP-IDF, not PlatformIO):

```bash
# inside espressif/idf:release-v5.5 container
idf.py set-target esp32p4
idf.py build
```

CI first clones `arduino-esp32` (branch `idf-release/v5.5`) and `ArduinoJson` (branch `7.x`) into `components/arduino` and `components/arduinojson` — these are build-time only and must not be committed.

Local alternative (PlatformIO, pioarduino fork required). `pio` may not be on PATH in this environment; invoke `C:\Users\Bratas\.platformio\penv\Scripts\pio.exe` (or add that dir to PATH):

```bash
pio run -e esp32-p4
pio run -t upload -e esp32-p4 --upload-port /dev/ttyACM0
pio device monitor -p /dev/ttyACM0 -b 115200
```

- `platformio.ini`: `src_dir = .`, `build_src_filter = +<main/*>`, `lib_extra_dirs = libraries`. Only `main/*` is compiled as the app.
- `CMakeLists.txt` (ESP-IDF) registers `main/` plus `EXTRA_COMPONENT_DIRS components`.

## Duplicated DSP component (keep in sync)

`components/yummydsp` (ESP-IDF) and `libraries/YummyDSP/src` (PlatformIO) are two copies of the same YummyDSP library. Edits must be applied to both or the two builds diverge. They have already drifted:

- `AudioDriver.cpp/.h`: ESP-IDF copy uses `portMAX_DELAY` on legacy `i2s_read`/`i2s_write`; PlatformIO copy uses `pdMS_TO_TICKS(100)`.
- `Nodes/WaveShaperNode.cpp`: PlatformIO copy calls `this->begin(fs, channelCount)` (undefined `fs`); ESP-IDF copy is correct (`begin(sampleRate, channelCount)`).

## Hardware / pins (README is stale)

The README documents the current simplified build. Trust `main/app_config.h` for pin definitions. Current firmware uses two physical inputs:

- `NUM_PHYS_IN = 2`, `SampleRateFreq = 48000`, `channelCount = 2`
- ESP32-P4: `I2S_BCK_PIN=22`, `I2S_LRCLK_PIN=21`, `I2S_DIN_PIN=23`, `I2S_DIN2_PIN=24`, `I2S0_MCLK_PIN=20`, `I2S_DOUT_PIN=-1`

## Runtime / config

- Config persisted in SPIFFS as `/config.json` (`DEFAULT_CONFIG_FILENAME`); `init_config()` auto-formats on mount failure.
- WiFi STA credentials read from `/wifi.txt` (line 1 SSID, line 2 password); absent/`mixer` → AP-only.
- WiFi AP: `AudioMixer-8ch` / `mixer1234` at `192.168.4.1`; `WiFi.mode(WIFI_AP_STA)`. Ethernet also initialized (`initEthernet()`).
- `audioTask` pinned to core 1; `serialCommTask`/`webTask` to core 0. WDT timeout 10s (`WDT_TIMEOUT`).
- Partitions: 16MB, `partitions_p4.csv` (spiffs at `0x390000`, size `0xC70000`).

## Dependencies / constraints

- `main/idf_component.yml`: IDF `>=5.4`, `espressif/esp_wifi_remote`, `espressif/esp_hosted` (C6 coprocessor over SDIO).
- `main/CMakeLists.txt` requires `arduino yummydsp arduinojson nvs_flash esp_wifi esp_event esp_netif esp_http_server`.

## CI / releases

- `.github/workflows/build.yml`: builds on push to `main` and tags `v*`; release job publishes `.bin` artifacts on tag. Do not edit the managed-component `strncpy` sed patches casually.