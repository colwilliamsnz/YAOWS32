#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>

void    setupGPIOPins();
void    initStartup();

void    displayESP32DateTime();

int     getNextWakeupMinute(int, int);

int     getWiFiStatus();

void    getSensors();

void    getSensorBME280();
void    getSensorBMP180();
void    getSensorDHT22();
void    getSensorDS18B20();

void    getSensorINA219();

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