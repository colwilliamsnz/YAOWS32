# Todo

## Master Code

* ~Wind speed conversions are defined by magic number - fix~
* ~Add wind chill to derived measurements~
* Add RSSI to Thingspeak telemetry data
* Add serial debug text to resetDay function
* Refactor getNTP
* Add wind speed compensation factor for Anemometer (in situations where it can't be located at 10m above ground level). Ref https://cumuluswiki.org/a/Wind_measurement
* getSlave - test for number of bytes received before blindly interpreting the data. Eg, bytesReceived = Wire.available(); if (bytesReceived == 8)...
* Implement watchdog timer, ala: https://arduinodiy.wordpress.com/2016/07/10/reset-a-program-from-a-freeze/
* Add checks to DHT22 code to check isnan (if (isnan(h) || isnan(t) || isnan(f))
* Refactor initRTC function (very messy!)
* Consider: Is 3231RTC worth retaining? ESP32 RTC races a little over say 100 minutes ~ 3 seconds however NTP sync keeps in check VS 700uA that 3231RTC uses...hmmmm!
* Move weather condition variables to a "struc" (or object, with methods to extract measurements in various units)
* Consider: Add in MQTT to Mosquito or similar
* Consider: OTA programing web interface?
* Add in fixed IP address for quicker connections to WiFi (every second running at 170mAh counts!)
* Refactor cloud publishing code - 95% is similar so move it to function
* Fix PWSWeather function--missing wind data
* Consider: Add "test harnis" for cloud publishing services

## Slave Code

* ~Add reference for Fine Offset wind direction http://mile-end.co.uk/blog/bits/Weather-Sensor-Assembly.pdf~

## Electronics

* Prototype sensors with MOSFET control of power and pull up resistors
* Add reverse polarity protection to battery input
