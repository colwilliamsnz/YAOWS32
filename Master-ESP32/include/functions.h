#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>

void    setupGPIOPins();
void    initStartup();

void    displayDS3231DateTime();
void    displayESP32DateTime();
void    setDS3121RTC();

int     getNextWakeupMinute(int, int);

int     getWiFiStatus();

void    sendCloudDataThingSpeak();
void    sendCloudDataWeatherUnderground();
void    sendCloudDataPWSWeather();
void    sendCloudDataWeatherCloud();

void    getSensors();

void    getSensorBME280();
void    getSensorBMP180();
void    getSensorDHT22();
void    getSensorUV();
void    getSensorDS18B20();

void    getSensorINA219();
void    getSensorDS3231();

void    getDerivedMeasurements();

void    getSlave();
void    resetSlave();

void    initWiFi();
void    getWiFiInfo();
void    killWiFi();

void    initRTC();
void    getNTPTime();

void    initSleep();

void    setupI2C();

void    resetDay();

void    setupSensorsOnboard();
void    setupSensorsExternal();

#endif