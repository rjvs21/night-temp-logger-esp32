# Overnight Temperature Logger — Full Build Guide

An ESP32 + DHT11 project that logs temperature through the night, tracks the
**low / high / average**, pushes a **morning summary** to your phone via Blynk,
and appends every reading to a **Google Sheet**. The night window can be changed
live from the Blynk app.

---

## 1. What you need

**Hardware**
- ESP32 dev board
- DHT11 temperature/humidity sensor (3-pin module recommended — has the pull-up built in)
- 3 jumper wires
- USB cable for flashing
- (Bare 4-pin DHT11 only) one 10kΩ resistor

**Software / accounts**
- Arduino IDE 2.x
- A free Blynk account (blynk.cloud)
- A Google account (only needed if you want Google Sheets logging)
- A 2.4GHz WiFi network (ESP32 does not support 5GHz)

---

## 2. Wiring

| DHT11 pin | ESP32 pin |
|-----------|-----------|
| VCC / +   | 3.3V      |
| DATA / S  | GPIO18    |
| GND / -   | GND       |

Notes:
- A 3-pin module already includes the pull-up resistor — just three wires.
- For a bare 4-pin sensor, add a 10kΩ resistor between DATA and VCC.
- To use a different data pin, change `#define DHTPIN 18` in the sketch. Avoid
  GPIO 6–11 (flash) and input-only pins 34–39.

Do the wiring with the board unplugged.

---

## 3. Arduino IDE setup

**A. Add ESP32 board support** (one time)
1. File → Preferences → "Additional Boards Manager URLs", add:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. Tools → Board → Boards Manager → search "esp32" → install
   **esp32 by Espressif Systems**.

**B. Install libraries** (Tools → Manage Libraries)
- **Blynk** by Volodymyr Shymanskyy
- **DHT sensor library** by Adafruit (accept the prompt to also install
  **Adafruit Unified Sensor**)

**C. Select board + port** (Tools menu)
- Board → ESP32 Arduino → **ESP32 Dev Module** (or your specific board)
- Port → the COM/tty port that appears when the board is plugged in
- No port? Install the USB-serial driver for your board's chip
  (**CP2102** or **CH340**).

---

## 4. Fill in the sketch

Open the sketch and set these values near the top:

```cpp
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"
char ssid[] = "YOUR_WIFI";
char pass[] = "YOUR_WIFI_PASSWORD";
const char* SHEETS_URL = "PASTE_YOUR_WEB_APP_URL";  // optional, see section 6
```

You get the Template ID and Auth Token from Blynk (section 5). Leave
`SHEETS_URL` as-is if you are not using Google Sheets.

Optional tuning:
- `READ_INTERVAL_MS` — how often it samples (default 120000 = 2 minutes).
- `NIGHT_START_HOUR` / `NIGHT_END_HOUR` — default window 21 (9 PM) to 8 (8 AM).
  These are just startup defaults; they can be overridden live from the app.
- `GMT_OFFSET_SEC` — your timezone offset in seconds (the number is hours × 3600).
  `DST_OFFSET_SEC` is usually `3600` where daylight saving applies, else `0`.

  | Timezone | Offset | `GMT_OFFSET_SEC` |
  |----------|--------|------------------|
  | UTC / GMT (London winter) | 0   | `0` |
  | Central Europe (Paris, Berlin) | +1 | `1 * 3600` |
  | India (IST) | +5:30 | `5 * 3600 + 1800` |
  | China / Singapore | +8 | `8 * 3600` |
  | Japan (JST) | +9 | `9 * 3600` |
  | US Eastern (New York) | -5 | `-5 * 3600` |
  | US Central (Chicago) | -6 | `-6 * 3600` |
  | US Mountain (Denver, Edmonton) | -7 | `-7 * 3600` |
  | US Pacific (Los Angeles) | -8 | `-8 * 3600` |

  Default is Edmonton/Mountain (`-7 * 3600`). Pick your row and set the value
  to match. (For half-hour zones like IST, add `1800`; for 45-minute zones add `2700`.)

---

## 5. Blynk cloud setup

**A. Create the template**
1. blynk.cloud → Developer Zone → Templates → New Template.
2. Name it (e.g. "Night Temp Logger"), Hardware **ESP32**, Connection **WiFi**.
3. The Info tab shows the **Template ID** — copy it into the sketch.

**B. Create datastreams** (Template → Datastreams → New, one per row)

| Pin | Name         | Type    | Min–Max | Units |
|-----|--------------|---------|---------|-------|
| V0  | Current Temp | Double  | 0–50    | °C    |
| V1  | Night Low    | Double  | 0–50    | °C    |
| V2  | Night High   | Double  | 0–50    | °C    |
| V3  | Night Avg    | Double  | 0–50    | °C    |
| V4  | Readings     | Integer | 0–2000  | —     |
| V5  | Status       | String  | —       | —     |
| V6  | Start Hour   | Integer | 0–23    | —     |
| V7  | End Hour     | Integer | 0–23    | —     |

**C. Create the morning-summary event** (Template → Events → New Event)
- Event code: `night_summary` (must match the sketch exactly)
- Name: "Morning Summary", Type: Information or Warning
- Open the event → Notifications tab → turn **Push Notifications ON** →
  recipients = device owner.
- Save the template.

**D. Create the device**
1. Devices → New Device → From Template → pick your template → Create.
2. The device Info panel shows the **Auth Token** — copy it into the sketch.

---

## 6. Google Sheets logging (optional)

1. Create a Google Sheet. Row 1 headers: `Time | Temp | Low | High | Avg | Readings`.
2. Extensions → Apps Script. Replace the code with:

```javascript
function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var d = JSON.parse(e.postData.contents);
  sheet.appendRow([new Date(), d.temp, d.low, d.high, d.avg, d.readings]);
  return ContentService.createTextOutput("ok");
}
```

3. Deploy → New deployment → type **Web app** →
   Execute as **Me**, Who has access **Anyone** → Deploy → authorize.
4. Copy the Web app URL (ends in `/exec`) into `SHEETS_URL` in the sketch.

Rows are appended during the night window. To log around the clock, move the
`logToSheets(...)` call to just after `Blynk.virtualWrite(V0, c)` in the sketch.

> Note: Proton Sheets and other end-to-end-encrypted spreadsheets cannot receive
> live data this way — there is no server-side endpoint for the device to post to.
> Use Google Sheets here, or export Blynk history and import the CSV into Proton.

---

## 7. Flash the board

1. Save the sketch (the `.ino` must sit in a folder of the same name).
2. **Close the Serial Monitor** (open monitor during upload often crashes IDE 2.x).
3. Click Upload (→). If it hangs on "Connecting...", hold the **BOOT** button
   until writing starts, then release.
4. Open Serial Monitor at **115200 baud**. You should see Blynk connect, then a
   `Current: XX.X C` line each interval.

Quick logging test without waiting for night: temporarily set the night window
to cover the current time (e.g. start 13, end 23 if it is 2 PM), flash, watch a
row appear, then set it back.

---

## 8. Build the Blynk interface

The web dashboard and the phone dashboard are configured **separately** — building
one does not build the other. Each widget must be linked to a datastream; that
link is the step people most often miss.

### Phone app (Blynk IoT — not the legacy app)

1. Open the app → tap your device → tap the **wrench icon** to enter edit mode.
2. Tap the empty canvas → choose a widget → tap the placed widget →
   set its **Datastream**.
3. Add these widgets:

| Widget          | Datastream      | Purpose                     |
|-----------------|-----------------|-----------------------------|
| Gauge           | V0 Current Temp | live temperature            |
| Gauge / Label   | V1 Night Low    | lowest reading              |
| Gauge / Label   | V2 Night High   | highest reading             |
| Gauge / Label   | V3 Night Avg    | average                     |
| Labeled Value   | V4 Readings     | sample count (sanity check) |
| Labeled Value   | V5 Status       | "Logging..." / "Idle"       |
| Chart           | V0 (add V1,V2)  | overnight curve             |
| Step / Slider   | V6 Start Hour   | set window start (0–23)     |
| Step / Slider   | V7 End Hour     | set window end (0–23)       |

4. Tap the **wrench** again to exit edit mode — widgets go live.
5. Make sure the phone has granted Blynk notification permission so the morning
   summary actually buzzes.

### Web dashboard (optional)

1. blynk.cloud → Devices → your device → Dashboard tab → Edit.
2. Drag widgets from the right panel, click each to set its datastream
   (same mapping as the table above).
3. Save.

Tips:
- Use **Labeled Value** instead of Gauge for the count and status — it is more compact.
- Charts only graph data that arrives **after** the widget is created.
- For the overnight curve, the simple **Chart** is enough; **SuperChart** adds
  range tabs (Live / 1h / 1d / 1w) if you want them.

---

## 9. How it behaves

- Samples every 2 minutes (configurable).
- Stats reset at the start hour, accumulate through the night, and **hold through
  the day**, so each morning you see the previous night's results.
- At the end hour it fires the **morning summary** push with low/high/avg.
- Window hours changed in the app take effect at the **next** boundary.
- On reboot the device re-syncs the app's stored window hours, so your last
  setting sticks.

---

## 10. Troubleshooting

| Symptom | Likely cause / fix |
|---------|--------------------|
| "Read failed" in Serial | Wiring — check DATA on GPIO18, VCC=3.3V, GND |
| Garbled Serial text | Baud not set to 115200 |
| Hangs at `Blynk.begin()` | Wrong/empty Auth Token or WiFi creds; 5GHz network |
| `Sheets POST -> -1` | Bad/unreachable URL or no WiFi |
| `Sheets POST -> 302` | Normal — the redirect is followed; rows still append |
| No morning push | `night_summary` event missing, or phone notifications off |
| Widgets blank | Device offline, or widget not linked to a datastream |
