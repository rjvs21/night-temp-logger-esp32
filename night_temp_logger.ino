/*
 * Overnight Temperature Logger - ESP32 + DHT11 + Blynk IoT
 *
 * Tracks LOW / HIGH / AVERAGE temperature during a nightly window.
 * Stats accumulate from NIGHT_START to NIGHT_END, then hold until
 * the next night begins so you can read last night's results in the morning.
 *
 * Blynk Virtual Pins:
 *   V0 = current temp        V3 = night average
 *   V1 = night low           V4 = reading count
 *   V2 = night high          V5 = status string (optional Label widget)
 */

#include "secrets.h"   // BLYNK_TEMPLATE_ID, BLYNK_AUTH_TOKEN, WiFi, SHEETS_URL

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define DHTPIN  18
#define DHTTYPE DHT11   // DHT11: ~1C resolution, +/-2C accuracy
DHT dht(DHTPIN, DHTTYPE);

int        NIGHT_START_HOUR = 21;   // 9 PM (changeable from app via V6)
int        NIGHT_END_HOUR   = 8;    // 8 AM (changeable from app via V7)
const long GMT_OFFSET_SEC   = -7 * 3600;  // Edmonton MST; adjust if needed
const int  DST_OFFSET_SEC   = 3600;       // DST handling

// Read sensor every 2 min
const unsigned long READ_INTERVAL_MS = 120000UL;

// ---- State ----
float nightLow  = NAN;
float nightHigh = NAN;
double tempSum  = 0;
long   readings = 0;
bool   inNight  = false;   // are we currently within the night window?

BlynkTimer timer;

// App sets the night start hour (V6) and end hour (V7)
BLYNK_WRITE(V6) {
  int h = param.asInt();
  if (h >= 0 && h <= 23) NIGHT_START_HOUR = h;
  Serial.printf("Start hour set to %d\n", NIGHT_START_HOUR);
}
BLYNK_WRITE(V7) {
  int h = param.asInt();
  if (h >= 0 && h <= 23) NIGHT_END_HOUR = h;
  Serial.printf("End hour set to %d\n", NIGHT_END_HOUR);
}

// On (re)connect, pull the latest stored values from the app
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V6, V7);
}

bool isNightTime(struct tm &t) {
  int h = t.tm_hour;
  // Window wraps past midnight
  if (NIGHT_START_HOUR > NIGHT_END_HOUR)
    return (h >= NIGHT_START_HOUR || h < NIGHT_END_HOUR);
  else
    return (h >= NIGHT_START_HOUR && h < NIGHT_END_HOUR);
}

void resetStats() {
  nightLow = NAN; nightHigh = NAN;
  tempSum = 0; readings = 0;
}

void logToSheets(float temp, float low, float high, float avg, long n) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();                 // skip cert check (fine for this)
  HTTPClient http;
  http.begin(client, SHEETS_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Apps Script redirects
  http.addHeader("Content-Type", "application/json");
  String body = "{\"temp\":" + String(temp,1) +
                ",\"low\":"  + String(low,1)  +
                ",\"high\":" + String(high,1) +
                ",\"avg\":"  + String(avg,1)  +
                ",\"readings\":" + String(n) + "}";
  int code = http.POST(body);
  Serial.printf("Sheets POST -> %d\n", code);
  http.end();
}

void sampleTemp() {
  struct tm t;
  if (!getLocalTime(&t)) {
    Blynk.virtualWrite(V5, "Time sync failed");
    return;
  }

  float c = dht.readTemperature();   // use readTemperature(true) for Fahrenheit
  if (isnan(c)) {
    Blynk.virtualWrite(V5, "Sensor read error");
    return;
  }

  Blynk.virtualWrite(V0, c);

  Serial.printf("Current: %.1f C\n", c);

  bool nightNow = isNightTime(t);

  // Night just ended (window closed) -> push morning summary once
  if (!nightNow && inNight && readings > 0) {
    float avg = tempSum / readings;
    char msg[120];
    snprintf(msg, sizeof(msg),
      "Last night: low %.1fC, high %.1fC, avg %.1fC (%ld readings)",
      nightLow, nightHigh, avg, readings);
    Blynk.logEvent("night_summary", msg);
    Serial.printf("MORNING SUMMARY -> %s\n", msg);
  }

  // Just entered the night window -> start fresh
  if (nightNow && !inNight) resetStats();
  inNight = nightNow;

  if (nightNow) {
    if (isnan(nightLow)  || c < nightLow)  nightLow  = c;
    if (isnan(nightHigh) || c > nightHigh) nightHigh = c;
    tempSum += c;
    readings++;

    float avg = tempSum / readings;
    Blynk.virtualWrite(V1, nightLow);
    Blynk.virtualWrite(V2, nightHigh);
    Blynk.virtualWrite(V3, avg);
    Blynk.virtualWrite(V4, readings);
    Blynk.virtualWrite(V5,
      String("Logging... ") + readings + " readings");

    Serial.printf("  Night -> low %.1f  high %.1f  avg %.1f  (n=%ld)\n",
                  nightLow, nightHigh, avg, readings);

    logToSheets(c, nightLow, nightHigh, avg, readings);
  } else {
    // Outside window: leave last night's stats on display
    Blynk.virtualWrite(V5, "Idle (showing last night)");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org");
  timer.setInterval(READ_INTERVAL_MS, sampleTemp);
}

void loop() {
  Blynk.run();
  timer.run();
}
