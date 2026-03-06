# MCP23S17 16-Input Reader with OLED Display

## Hardware
- ESP32-S3 DevKitC-1
- MCP23S17 SPI I/O Expander (16 inputs)
- HiLetgo 0.96" OLED (SSD1306, 128x64)

## Pin Connections
| MCP23S17 | ESP32-S3 |
|----------|----------|
| CS       | GPIO10 (FSPICS0) |
| MOSI     | GPIO11 |
| MISO     | GPIO13 |
| SCK      | GPIO12 |
| VCC      | 3.3V |
| GND      | GND |

| OLED | ESP32-S3 |
|------|----------|
| SDA  | GPIO21 |
| SCL  | GPIO22 |
| VCC  | 3.3V |
| GND  | GND |

## Dependencies
- robtillaart/MCP23S17
- adafruit/Adafruit SSD1306
- adafruit/Adafruit GFX Library

## Build & Upload
```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor