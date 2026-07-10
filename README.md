# P4-DIGITAL-MIXING

8-channel digital audio mixer for **ESP32-P4** with WebSocket web UI. 15-band EQ, compressor, reverb, 5-bus routing via WebSocket + Serial.

Uses Waveshare ESP32-P4-NANO board with C6 coprocessor for WiFi (SDIO).

## Features

- 8-channel mixing with 5 buses (L/R, Group 1/2, Aux 1/2)
- 15-band graphic EQ per channel (31Hz–18kHz)
- Compressor per channel (ratio, attack, release, threshold, gain)
- Reverb per channel
- 2 physical I2S inputs (PCM1808 ADC), 2 physical I2S outputs (PCM5102 DAC)
- Multiband DSP processor on main bus (1–10 band)
- WiFi AP + STA via ESP32-C6 coprocessor
- WebSocket web UI + Serial terminal
- SPIFFS configuration storage

## Hardware Requirements

- Waveshare ESP32-P4-NANO (or any ESP32-P4 with C6 SDIO WiFi + I2S1 header)
- PCM1808 ADC (I2S input), PCM5102 DAC (I2S output), CS8416 (S/PDIF input)
- Power via USB-C

### I2S Pin Connections (ESP32-P4 external I2S1 header)

| Signal | GPIO |
|--------|------|
| I2S_BCK   | GPIO22 |
| I2S_LRCLK | GPIO23 |
| I2S_DOUT  | GPIO20 |
| I2S_DIN   | GPIO21 |
| CODEC_EN  | 2     |
| MCLK      | NC    |

### C6 SDIO Pins (internal, not on header)

| Signal | GPIO |
|--------|------|
| CLK  | GPIO18 |
| CMD  | GPIO19 |
| D0   | GPIO14 |
| D1   | GPIO15 |
| D2   | GPIO16 |
| D3   | GPIO17 |
| RST  | GPIO54 |

## Compilation

```bash
# Build
pio run -e esp32-p4

# Flash
pio run -t upload -e esp32-p4 --upload-port /dev/ttyACM0

# Monitor
pio device monitor -p /dev/ttyACM0 -b 115200
```

Requires the pioarduino PlatformIO fork for ESP32-P4 support (configured in `platformio.ini`).

## Usage

1. Power the board via USB-C
2. Connect to WiFi AP `AudioMixer-8ch` (password: `mixer1234`)
3. Open `http://192.168.4.1` in a browser
4. Control channel EQ, routing, compressor, reverb, and master levels

For STA mode, write WiFi credentials to SPIFFS (`/wifi.txt`) via serial or API.

## Project Structure

```
├── main/
│   ├── audio_mixer_p4.cpp    # Main entry, tasks, I2S init
│   ├── settings.h             # Pin definitions, constants
│   ├── global_vars.h/cpp      # Global state, audio DSP task
│   ├── DualCompressor.h       # Stereo compressor
│   ├── serial_cmd.h           # Serial command interface
│   ├── web_server.h/cpp       # HTTP server, WebSocket, WiFi
├── libraries/YummyDSP/        # YummyDSP Arduino library
├── platformio.ini             # PlatformIO configuration
├── partitions_p4.csv          # 16MB partition table
└── README.md
```

## Credits

- Original ESP32-P4 project by Junon M.: https://github.com/junon10/esp32-multiband-dsp
- DSP: YummyDSP by Junon M.
- ESP32-P4 PlatformIO support: pioarduino
