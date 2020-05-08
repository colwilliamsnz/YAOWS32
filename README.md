# YAOWS32

## Summary
Yet Another Open Weather Station (YAOWS32) is a weather station shield for ESP32 IoT dev board. Wind and rain sensors are accommodated as well as numerous other peripheral sensors via 3 x 1-Wire, 3 x I2C, and 2 x Analog headers.

Power consumption is kept low and battery/solar compatible by sleeping the ESP32 and waking it just long enough to sense weather conditions and upload to the cloud on a scheduled basis. Wind speed, direction and rain are sensed full time utilising a low power ATTiny85 running as an I2C slave.

It is designed to be powered by 12v SLA battery which is in turn charged by a small 5w (or similar) 20v solar panel (note, the solar controller is not part of the core design).

![YAOWS32 Prototype](Images/yaows32_prototype.jpeg)

## Objectives
Having owned a couple of low cost weather stations that failed after a year or two, I decided to design something that would go the distance whilst providing an ideal project to learn a bit about Arduino, C++, and IoT.

As core Arduino boards are so pervasive and inexpensive, the design started out as an Arduino Nano in a master slave configuration using 433Mhz to transmit data between an indoor master and outdoor units. Eventually I discovered the more powerful and versatile ESP32 platform and so the design morphed into an all-in-one outdoor solution.

Finally a robust and expandable system requiring next to no maintenance was also desirable.

## Circuit
The prototype circuit was designed using EasyEDA. Version 1.1 of the circuit design and PCB board is available [here](https://easyeda.com/colwilliamsnz/YAOWS_copy).

_Note: The 3231 RTC will likely be removed from a future design revision as it draws approx 700uA and adds very little to the overall solution VS the ESP32 onboard RTC and regular NTP time sync to keep any skew in check. In rev 1.1, the RTC can be omitted entirely from the board or disabled by removing the jumper marked “RTC”._

### Components
TBC

## Sensors

Here follows are the sensors tested in my prototype. All are commonly available components that are enabled by adding in the appropriate library.

### Wind & Rain

I opted for Davis Instruments 6410 Anemometer and 6462 Rain gauge which I picked up second hand. The common Fine Offset units sold on Ebay, Amazon, etc are also compatible.

![Davis 6410](Images/sensor_daviswind.png | width=250)

Practically any "tipping" rain gauge or anemometer that generates a pulse as it rotates may be used. Any wind vane that is either a potentiometer OR resister network can be used but will need to be decoded first (ie, ADC readings determined against wind directions).

![Fine Offset](Images/sensor_fineoffset.png)

### Temperature & Humidity

A 1-wire DHT11 was used. The version I have is housed in a very nice weather proof probe case including mounting hardware. There are identical looking AM2315 I2C versions also. These are not waterproof so still require sheltering from rain in some form of Stevenson screen.

![DHT11](Images/sensor_dht11.png)

### Pressure

I housed an I2C BMP180 in a custom weather proof case. These are accurate and inexpensive. Very nice custom housings for such sensors are available from [AliExpress](https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200507192408&SearchText=temperature+and+humidity+housing) and are worthwhile considering for long term installations.

## Prototype

The original prototype board installed in its weather proof housing and [Stevenson screen](https://en.wikipedia.org/wiki/Stevenson_screen). A 5 watt 20v solar panel and weather proof box containing a 9000mAh SLA battery and PWM solar controller sit alongside.

_Note: the solar plant is oversized as I intended it to double as an emergency power supply for charging phones during power outages--we often loose power in bad weather conditions where I live._

![YAOWS32 Prototype](Images/yaows32_initial_install.jpeg)
