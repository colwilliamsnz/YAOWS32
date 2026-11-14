# YAOWS32 – ATTiny85 slave firmware

Firmware for the ATTiny85 "slave" of the YAOWS32 weather station. It runs full time on very little power (~600 µA). It counts anemometer pulses and rain gauge tips and tracks wind direction while the ESP32 master sleeps. The master ([../Master-ESP32](../Master-ESP32)) collects the totals over I2C each time it wakes.

Counting wind and rain needs to happen continuously, but keeping the ESP32 awake for that would drain the battery. The ATTiny85 does the continuous work so the ESP32 can sleep between reports.

For the hardware and background, see the [project README](../README.md). The approach is based on [PaulRB/Wemos-Weather-Station](https://github.com/PaulRB/Wemos-Weather-Station).

## How it works

The main loop runs roughly once a millisecond. Between passes the chip sits in idle sleep, which saves about 200 µA compared with running flat out. On each pass it:

1. **Checks the anemometer.** The switch is debounced in hardware. Each pulse increments:
   - a **wind speed** count for the whole reporting period
   - a **gust** count for the current 6-second window
2. **Samples wind direction**, only while the anemometer is turning. The ADC is powered up briefly, reads the vane, and powered down again (saving about 150 µA). The reading is matched against a lookup table and adds one "vote" to that compass direction.
3. **Tracks the highest gust.** Every 6 seconds the gust count is compared with the highest so far, then reset.
4. **Checks the rain gauge** and counts each bucket tip. This switch is also debounced in hardware.

The pull-ups on the sensor pins are switched on only for each read, to save power.

**Wind direction** is reported as the direction with the **most votes** over the period, not the last reading. That smooths out gusts that briefly swing or stall the vane.

The ATTiny85 reports raw counts only. The master converts them to km/h, mm and mm/h, using the anemometer factor, rain tip volume and update period set in `Master-ESP32/src/station.cpp`.

## I2C interface

The ATTiny85 is an I2C slave at address **`0x04`**, on the master's second I2C bus (`Wire1`, GPIO13 SDA / GPIO14 SCL).

**Read 8 bytes:** the totals for the current period, each a 16-bit value, high byte first:

| Bytes | Value |
|---|---|
| 0–1 | Wind direction, degrees (majority direction for the period) |
| 2–3 | Wind speed pulse count for the period |
| 4–5 | Highest gust pulse count in any 6-second window |
| 6–7 | Rain gauge tip count for the period |

Reading also clears the wind direction votes.

**Write `'R'`:** resets all counters, starting a new period. The master sends this straight after each read.

## Pins

| ATTiny85 pin | Port | Use |
|---|---|---|
| 2 | PB3 / A3 | Wind direction (analog, from the vane) |
| 3 | PB4 | Rain gauge (switch) |
| 6 | PB1 | Wind speed (anemometer switch) |
| 5 | PB0 | I2C SDA |
| 7 | PB2 | I2C SCL |
| 4 / 8 | | GND / VCC |

## Configuration

At the top of `ATTiny85-Slave.ino`:

- **`I2C_SLAVE_ADDRESS`** (`0x04`): must match the address the master reads from.
- **`WIND_GUST_PERIOD`** (6000 ms): must match `g_stationWindGustPeriod` in the master's `station.cpp`, or gust speeds will be wrong. 6 seconds was found to be the most accurate in testing.
- **Wind vane lookup table.**
  - **Davis 6410** is active by default: 8 directions, decoded from its 20 kΩ potentiometer.
  - **Fine Offset** vanes: a 16-direction table is included, commented out. Swap the two blocks to use it.
  - **Other vanes:** read the ADC value at each compass point and fill in the `reading[]` / `compass[]` arrays.

## Building and flashing

This is a plain Arduino sketch, not PlatformIO. It needs [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore) with these board settings:

| Setting | Value |
|---|---|
| Chip | ATTiny85 |
| Clock | 1 MHz internal |
| BOD level | BOD disabled |
| Save EEPROM | EEPROM not retained |
| Timer 1 clock | CPU |
| LTO | Enabled |
| millis()/micros() | Enabled |

Flash it with an ISP programmer (for example a USBasp, or an Arduino running "Arduino as ISP"). On a new chip, run **Burn Bootloader** once first; with ATTinyCore this sets the clock and BOD fuses. Then use **Upload Using Programmer**.
