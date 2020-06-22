# Todo

## Master Code

* ~Wind speed conversions are defined by magic number - fix~
* ~Add wind chill to derived measurements~
* ~Add RSSI to Thingspeak telemetry data~
* ~Rain-need a new global that represents rain for period (current variable is for the entire day). Eg, g_weatherRainFall vs g_weatherRainDaily~
* Add serial debug text to resetDay function
* ~Refactor getNTP~
* ~Add wind speed compensation factor for Anemometer (in situations where it can't be located at 10m above ground level). Ref https://cumuluswiki.org/a/Wind_measurement~
* getSlave - test for number of bytes received before blindly interpreting the data. Eg, bytesReceived = Wire.available(); if (bytesReceived == 8)...
* Implement watchdog timer, ala: https://arduinodiy.wordpress.com/2016/07/10/reset-a-program-from-a-freeze/
* Add checks to DHT22 code to check isnan (if (isnan(h) || isnan(t) || isnan(f))
* ~Refactor initRTC function (very messy!)-Removed, not required~
* ~Is 3221RTC worth retaining? ESP32 RTC races a little over say 100 minutes ~ 3 seconds however NTP sync keeps in check VS 700uA that 3231RTC uses...Removed!~
* Move weather condition variables to a "struc" (or object, with methods to extract measurements in various units)
* Add in MQTT (EG, to Mosquito, Node-Red, InfluxDB, & Grafana)
* Consider: OTA programing web interface?
* Add in fixed IP address for quicker connections to WiFi (every second running at 170mAh counts!)
* Consider: Refactor cloud publishing code - 95% is similar so move it to function
* ~Fix PWSWeather function--missing wind data~
* ~Add "test harnis" for cloud publishing services~
*

## Slave Code

* ~Add reference for Fine Offset wind direction http://mile-end.co.uk/blog/bits/Weather-Sensor-Assembly.pdf~
* Add instantaneous wind speed and direction (repurpose wind gust code -- report the last 6s wind gust recorded for the period).

## Electronics

* Prototype sensors with MOSFET control of power and pull up resistors
* Add reverse polarity protection to battery input
* More power filtering caps to try and resolve the issue with the ESP32 locking up during init of WiFi. Eg, 1000uf electrolytic at the buck converter output, 47uf tantalum on the ESP32 board between 3.3v and GND.
