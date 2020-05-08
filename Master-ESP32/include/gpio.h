#ifndef GPIO_H
#define GPIO_H

// *** Outputs ****************************************************************
#define OUTPUT_LED_WIFI          2  // ESP32 inbuilt blue LED
#define OUTPUT_LED_MISC_1       18  // Future
#define OUTPUT_LED_MISC_2       19  // Future

// *** Inputs *****************************************************************
#define INPUT_SENSOR_1WIRE_1    32  // AM2305 temp/hum sensor
#define INPUT_SENSOR_1WIRE_2    33  // Soil temp sensor (note error in rev1.0 board -- 33&32 are swapped)
#define INPUT_SENSOR_1WIRE_3    26  // Future

#define INPUT_SENSOR_RTC_ALARM  27  // RTC clock alarm pin (note, built in pull up on DS3231)

#define INPUT_SENSOR_ANALOG_1   39  // Future
#define INPUT_SENSOR_ANALOG_2   36  // Future

// Inbuilt i2c interface - external sensors
#define INPUT_SENSOR_I2C1_SDA   21  // ESP32 defined but included here so I know what pins I have left over ;)
#define INPUT_SENSOR_I2C1_SCL   22

// User defined i2c - onboard sensors
#define INPUT_SENSOR_I2C2_SDA   13
#define INPUT_SENSOR_I2C2_SCL   14

#endif