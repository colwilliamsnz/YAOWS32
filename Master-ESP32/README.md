# YAOWS32 – Master ESP32 firmware

Firmware for the ESP32 "master" of the YAOWS32 solar-powered weather station. It reads the sensors, uploads the readings to MQTT and online weather services, then sleeps until the next reading. Wind and rain are counted continuously by a separate ATTiny85 "slave" ([../Slave-ATTiny85](../Slave-ATTiny85)), which the ESP32 polls over I2C.

For the hardware, sensors and background, see the [project README](../README.md).

## How it works

### Normal operation: wake → measure → upload → sleep

The station spends almost all its time in **light sleep** to save battery. RAM is kept, so totals such as daily rainfall survive between wakes. It wakes on a clock-aligned schedule (every 10 minutes by default: :00, :10, :20…) and runs one cycle:

1. **Read the ATTiny85** for wind speed, gust, direction and rain tips since the last cycle, then reset its counters.
2. **Read the sensors**:
   - INA219: battery voltage and current
   - BMP180: pressure
   - AM2315: air temperature and humidity
3. **Calculate derived values**: dew point, heat index, wind chill, rain rate and wind speed corrected for anemometer height.
4. **Connect to WiFi**, sync the clock from NTP (every 2 hours) and **upload**:
   - **MQTT**: one retained topic per value (`yaows32/airtemp`, `yaows32/humidity`, …), for Home Assistant or similar.
   - **WeatherCloud**.
   - Weather Underground, PWSWeather and ThingSpeak are also supported but switched off in `main.cpp`.
5. **Switch WiFi off**, reset the daily totals at midnight, and sleep until the next scheduled time.

If WiFi can't connect, the station sleeps for one period and restarts rather than draining the battery retrying.

### Maintenance window: OTA updates and web log

On **power-on or EN-button reset**, and on the **first boot after a firmware update**, the station stays awake with WiFi on for a few minutes and serves a small website (login set in `passwords.cpp`):

| Page | Purpose |
|---|---|
| `http://yaows32.local/` | Status: countdown, battery, WiFi, temperature, and buttons to extend the window, run a cycle now or go to sleep |
| `/update` | Upload new firmware ([ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)) |
| `/log` | Everything the station has printed since boot, like a USB serial monitor. Refresh to update. |

While the window is open:
- Normal cycles keep running on schedule, so you can watch live readings in the log and no data is missed.
- The window is extended automatically while the status page is open or an upload is in progress, up to a maximum.

Other restarts (WiFi failure, brown-out, crash) skip the window, so a flat battery can't get stuck in a WiFi-on reboot loop.

**Safe updates:** new firmware has to connect to WiFi and finish a full cycle before it is marked good. If it crashes first, the ESP32 rolls back to the previous firmware on its own.

### Two build targets

| PlatformIO env | Use |
|---|---|
| `station` (default) | The real, mast-mounted station |
| `testrig` | A desk board for development. Identical code, except cloud uploads are a **dry run** (payloads printed to the log, nothing sent). It uses its own hostname (`yaows32-test.local`), wakes every 2 minutes and has a longer maintenance window. |

## Getting started

1. **Create your local config files.** Both are git-ignored:
   - copy `include/debug_example` → `include/debug.h` (debug output options)
   - copy `src/passwords_example` → `src/passwords.cpp` (WiFi, MQTT, cloud service keys, and the web login)
2. **Check the station settings** in `src/station.cpp`:
   - update period, altitude, rain gauge tip volume
   - anemometer factor and height, time zone
   - maintenance window length and low-battery cutoff
3. **Build and flash:**
   ```sh
   pio run -e station -t upload     # real station
   pio run -e testrig -t upload     # desk test board
   ```
   The **first** flash must be over USB, because the firmware uses a custom partition table (`min_spiffs.csv`) with room for two firmware images. After that, use the `/update` page with `.pio/build/<env>/firmware.bin`.

## Code layout

| File | What's in it |
|---|---|
| `src/main.cpp` | Startup, the maintenance window, and the wake/measure/upload/sleep loop |
| `src/functions.cpp` | Sensor reads, WiFi, NTP, sleep scheduling, ATTiny85 communication |
| `src/cloud.cpp` | MQTT and weather-service uploads (with dry-run support) |
| `src/weathermath.cpp` | Unit conversions and derived values (dew point, wind chill, …) |
| `src/maintenance.cpp` | Maintenance window: web server, status and log pages, OTA, rollback |
| `src/log.cpp` | `Log`: writes to USB serial and to a RAM buffer shown on the `/log` page |
| `src/station.cpp` | Station configuration (location, sensors, timings) |
| `src/globals.cpp` | Sensor objects and current readings |
| `include/buildprofile.h` | Differences between the `station` and `testrig` builds |
| `include/gpio.h` | Pin assignments |
| `include/version.h` | Version number and change log |

Sensors that aren't fitted are commented out in `setupSensorsExternal()` and `getSensors()` (`src/functions.cpp`). This includes the BME280, DHT22 and DS18B20; re-enable them there if you add one.
