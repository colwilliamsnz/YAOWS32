#include <Arduino.h>

#include "version.h"      // version number and serial DEBUG flag are set here

#include "globals.h"      // all other globals (vars and objects) are init'd here
#include "passwords.h"    // all password and cloud API key globals are init'd here
#include "station.h"      // all weather station configuration globals are init'd here

#include "functions.h"    // project specific functions
#include "weathermath.h"  // math functions - conversions and calculations
#include "cloud.h"        // cloud functions - updates to cloud services

#include "gpio.h"         // #define entries for GPIO pin assignments

void setup()
{
  // Start serial console for debugging (refer "version.h" header file for where
  // to set DEBUG on/off)
  Serial.begin(115200);
  
  // This is the default but as it needs to be set to 240Mhz for the DHT11 sensor
  // to function, it is specified here. If a different sensor is used for temp/hum,
  // some additional power savings are to be made by running the station at 80Mhz.
  setCpuFrequencyMhz(240);

  // Setup initial state of input/output pins
  setupGPIOPins();

  // Flash onboard LEDs and display startup info to serial console (if DEBUG set)
  initStartup();

  // Setup I2C interfaces (default + a second one for onboard devices)
  setupI2C();

  // Setup onboard sensors
  setupSensorsOnboard();

  // Setup external sensors
  setupSensorsExternal();

  // Setup Real time clock module
  initRTC();

  // We need WiFi for NTP sync (any other setup that requires WiFi should go in here)
  initWiFi();

  // Sync ESP32 (and external RTC if applicable) with NTP
  getNTPTime();
  
  // Disconnect and power down WiFi chipset
  killWiFi();

  // Go to sleep until next wake event at which point regular weather reporting will begin
  initSleep();

  // Reset slave ATTiny85 measurements
  resetSlave();
}

void loop()
{
  displayDS3231DateTime();

  displayESP32DateTime();

  getSlave();

  getSensors();

  getDerivedMeasurements();

  initWiFi();

  // **** functions that require WiFi start here ****

  //sendCloudDataWeatherUnderground();

  //sendCloudDataWeatherCloud();

  //sendCloudDataPWSWeather();

  //sendCloudDataThingSpeak();

  getNTPTime();

  // **** functions that require WiFi end here ****

  killWiFi();

  resetDay();

  initSleep();
}