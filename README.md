# ESP32 Digital Audio Mixer (8-Ch WebSocket)

8-channel digital audio mixer untuk **ESP32** (bukan P4), porting dari versi ESP32-P4. Fitur: 15-band EQ, compressor, reverb, 5-bus routing, kontrol via WebSocket dan Serial.

## Fitur

- 8-channel mixing dengan 5 bus (L/R, Group 1/2, Aux 1/2)
- 15-band graphic equalizer per channel (31Hz-18kHz)
- Compressor per channel (ratio, attack, release, threshold, gain)
- Reverb per channel
- 2 physical input (I2S ADC), 2 physical output (I2S DAC)
- Multiband DSP processor pada main bus (1-10 band)
- Kontrol via WebSocket (web UI) dan Serial terminal
- WiFi Access Point + Station mode
- Konfigurasi tersimpan di SPIFFS

## Hardware Requirements

- ESP32 DevKit board (ESP32-WROOM, ESP32-WROVER, dll)
- PCM5102A DAC (I2S output)
- PCM1808 ADC (I2S input)
- Power supply via USB

### Pin Connections (ESP32 DevKit)

| Signal | GPIO Pin |
|--------|----------|
| I2S_BCK    | GPIO26 |
| I2S_LRCLK  | GPIO27 |
| I2S_DOUT   | GPIO25 |
| I2S_DIN    | GPIO33 |
| CODEC_EN   | NC     |
| MCLK       | NC     |

*Sesuaikan pin dengan board Anda.*

### ESP32 vs ESP32-P4

| Fitur | ESP32 (ini) | ESP32-P4 (original) |
|-------|-------------|---------------------|
| CPU | Xtensa dual-core 240MHz | RISC-V dual-core 400MHz |
| WiFi/BT | **Ya** (built-in) | Tidak (via C6 coprocessor) |
| I2S ports | 1 | 2 |
| Phys I/O | 2 in / 2 out | 4 in / 4 out |
| PSRAM | Opsional (8MB max) | Hingga 32MB |

## Cara Kompilasi

### PlatformIO (Recommended)

```bash
# Build
pio run -e esp32dev

# Flash ke board
pio run -t upload -e esp32dev

# Monitor serial
pio device monitor -b 115200
```

### Arduino IDE

1. Install ESP32 Arduino Core dari Boards Manager
2. Install library: ArduinoJson (via Library Manager)
3. Board: ESP32 Dev Module atau Generic ESP32
4. Partition: Minimal SPIFFS (1.9MB APP / 1.5MB SPIFFS)
5. Flash Size: 4MB
6. Upload Speed: 115200

## Kontrol via Web

1. Colok ESP32 ke USB
2. Cari WiFi `AudioMixer-8ch` (password: `mixer1234`)
3. Buka browser ke `http://192.168.4.1`
4. Kontrol channel EQ, routing, compressor, reverb, dan master level

## Kontrol via Serial

Konek via serial monitor (115200 baud), ketik `menu` untuk daftar perintah.

## Struktur Folder

```
├── main/
│   ├── audio_mixer_p4.cpp    # Main program (setup, tasks)
│   ├── settings.h             # Constants, pin definitions
│   ├── global_vars.h/cpp      # Global state, audio DSP task
│   ├── DualCompressor.h       # Stereo compressor implementation
│   ├── serial_cmd.h           # Serial command interface
│   ├── web_server.h/cpp       # HTTP server, WebSocket, WiFi
├── components/yummydsp/       # YummyDSP library (ESP-IDF)
├── libraries/YummyDSP/        # YummyDSP library (Arduino)
├── platformio.ini             # PlatformIO config
├── partitions.csv             # Partition table
├── sdkconfig.defaults         # Default sdkconfig
└── README.md
```

## Credits

- Project asli oleh Junon M. (ESP32-P4): https://github.com/junon10/esp32-multiband-dsp
- DSP Library: YummyDSP by Junon M.
- Port ke ESP32: REITZG1
