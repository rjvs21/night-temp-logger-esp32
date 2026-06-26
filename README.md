# Overnight Temperature Logger (ESP32 + DHT11 + Blynk)

Logs temperature through the night, tracks the low / high / average, pushes a
morning summary to your phone via Blynk, and appends each reading to a Google
Sheet.

## Hardware
- ESP32 dev board
- DHT11 temperature/humidity sensor
- 3 jumper wires (3-pin module has the pull-up built in)

### Wiring
| DHT11 | ESP32 |
|-------|-------|
| VCC / + | 3.3V |
| DATA / S | GPIO18 |
| GND / - | GND |

(For a bare 4-pin sensor, add a 10kΩ resistor between DATA and VCC.)

## Software setup
1. Install the **Blynk** and **DHT sensor library** (Adafruit) libraries in the Arduino IDE.
2. Copy `secrets.h.example` to `secrets.h` and fill in your Blynk Template ID,
   Auth Token, WiFi credentials, and Google Apps Script URL.
3. In the Blynk console, create datastreams V0–V7 and a `night_summary` event
   (see comments in the sketch).
4. Flash `night_temp_logger.ino` to the ESP32.

## Blynk datastreams
| Pin | Purpose |
|-----|---------|
| V0 | Current temp |
| V1 | Night low |
| V2 | Night high |
| V3 | Night average |
| V4 | Reading count |
| V5 | Status text |
| V6 | Night start hour (0–23, app-settable) |
| V7 | Night end hour (0–23, app-settable) |

## Google Sheets logging
Deploy the included Apps Script (`appsscript/Code.gs`) as a Web App
("Execute as: Me", "Who has access: Anyone") and paste the `/exec` URL into
`secrets.h`.

## Notes
- ESP32 needs a 2.4GHz network.
- Window hours can be changed live from the app; changes apply at the next boundary.
