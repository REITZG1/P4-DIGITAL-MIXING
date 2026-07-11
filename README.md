# P4-DIGITAL-MIXING

8-channel digital audio mixer for **ESP32-P4** with WebSocket web UI. 15-band EQ, compressor, reverb, 5-bus routing via WebSocket + Serial.

Uses Waveshare ESP32-P4-NANO board with C6 coprocessor for WiFi (SDIO).

## Features

- 8-channel mixing with 5 buses (L/R, Group 1/2, Aux 1/2)
- 15-band graphic EQ per channel (31Hz–18kHz)
- Compressor per channel (ratio, attack, release, threshold, gain)
- Reverb per channel
- 8-channel TDM I2S input (4× PCM1808 ADC, 4 data lines)
- 6-channel TDM I2S output (3× PCM5102 DAC, 3 data lines)
- Multiband DSP processor on main bus (1–10 band)
- WiFi AP + STA via ESP32-C6 coprocessor (SDIO)
- WebSocket web UI + Serial terminal
- SPIFFS configuration storage

## Hardware Requirements

- Waveshare ESP32-P4-NANO (or any ESP32-P4 with C6 SDIO WiFi + I2S1 header)
- 4× PCM1808 ADC (I2S input), 3× PCM5102 DAC (I2S output)
- 3× 74LVC1G125 clock buffer for BCK/LRCK/MCLK distribution
- Power via USB-C

### I2S Pin Connections (ESP32-P4 external I2S1 header)

| Signal       | GPIO  | Function |
|--------------|-------|----------|
| MCLK         | 21    | Master clock to clock buffer |
| BCK          | 22    | Bit clock to clock buffer / PCM1808/PCM5102 |
| LRCK         | 25    | Frame sync (LR clock) to PCM1808/PCM5102 |
| ADC_DIN0     | 45    | PCM1808 #1 DOUT (ch 1-2) |
| ADC_DIN1     | 32    | PCM1808 #2 DOUT (ch 3-4) |
| ADC_DIN2     | 36    | PCM1808 #3 DOUT (ch 5-6) |
| ADC_DIN3     | 33    | PCM1808 #4 DOUT (ch 7-8) |
| DAC_DOUT0    | 26    | PCM5102 #1 DIN (ch 1-2) |
| DAC_DOUT1    | 24    | PCM5102 #2 DIN (ch 3-4) |
| DAC_DOUT2    | 27    | PCM5102 #3 DIN (ch 5-6) |

GPIO20 and GPIO23 are unused (I2S standard pins replaced by clock buffer + multi-data-line TDM).

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
