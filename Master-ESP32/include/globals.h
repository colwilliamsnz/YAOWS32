#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

#include <WiFi.h>
#include <time.h>

#include <Wire.h>
#include <OneWire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_AM2315.h>

#include <dht.h>
#include <Adafruit_INA219.h>
#include <DallasTemperature.h>

// *** Global classes *********************************************************

extern DHT sensor_dht22;

extern OneWire oneWire2;

extern DallasTemperature sensor_ds18b20;

extern Adafruit_BME280 sensor_bme280;
extern Adafruit_BMP085 sensor_bmp180;
extern Adafruit_INA219 sensor_ina219;
extern Adafruit_AM2315 sensor_am2315;


// *** Global variables *******************************************************

extern byte  g_slaveRecord[];

extern float g_weatherTempAirC;
extern float g_weatherTempSoilC;
extern float g_weatherTempWindChillC;

extern float g_weatherHumidity;
extern float g_weatherPressureRel;
extern float g_weatherPressureAbs;
extern float g_weatherDewPointC;
extern float g_weatherHeatIndexC;

extern float g_weatherRainFall;
extern float g_weatherRainFallDaily;
extern float g_weatherRainRate;

extern float g_weatherWindAvg;
extern float g_weatherWindGust;
extern float g_weatherWindDir;

extern float g_stationBatteryVoltage;
extern float g_stationBatteryCurrent;
extern int   g_stationWiFiRSSI;

extern int   g_ntpCheckIntervalTimer;

extern bool  g_sensorStatusBME280;
extern bool  g_sensorStatusBMP180;

#endif