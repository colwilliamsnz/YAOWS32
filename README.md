# YAOWS32

## Summary
Yet Another Open Weather Station is an open weather station shield for ESP32 dev board. It is designed to be powered by a small 5w (or similar) 20v solar panel. It features battery and current monitoring as well as an optional DS3231 real time clock for accurate wake up events for pushing updates to various cloud based weather services such as Weather Underground, PWS Weather, WeatherCloud, and Thingverse.

Power consumption is kept relatively low and solar compatible by sleeping the ESP32. Wind speed, direction and rain are sensed full time utilising an ATTiny85 "slave". The ESP32 wakes from sleep briefly at pre-determined intervals to collect sensor readings including slave data, then upload to the specified cloud services.

The design is intended to be open and extensible with 3 x 1-Wire headers, 3 x I2C headers, and 2 x Analog headers for connecting various peripheral sensors.

![YAOWS32 Prototype](Images/yaows32_prototype.jpeg)

The original prototype board installed in its weather proof housing and [Stevenson screen](https://en.wikipedia.org/wiki/Stevenson_screen). A 5 watt 20v solar panel and weather proof box containing a 9000mAh SLA battery and PWM solar controller sit alongside. A 1-Wire DHT22 temp/humidity sensor in a barrel type weather proof housing is installed on the right. On the left is an I2C BMP180 pressure sensor in a custom made weather proof housing.

![YAOWS32 Prototype](Images/yaows32_initial_install.jpeg)
