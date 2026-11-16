#include <Arduino.h>

#include "debug.h"        // debug compiler directives are set here
#include "buildprofile.h" // station vs test rig build profile (platformio env)
#include "version.h"      // version number and serial DEBUG flag are set here

#include "globals.h"      // all other globals (vars and objects) are init'd here
#include "passwords.h"    // all password and cloud API key globals are init'd here
#include "station.h"      // all weather station configuration globals are init'd here

#include "functions.h"    // project specific functions
#include "weathermath.h"  // math functions - conversions and calculations
#include "cloud.h"        // cloud functions - updates to cloud services
#include "maintenance.h"  // maintenance window - OTA firmware upload + web log
#include "log.h"          // Log - Serial + history for the web log page

#include "gpio.h"         // #define entries for GPIO pin assignments

/**
 * @brief Firmware rollback protection. The Arduino core calls this at boot -
 * returning true tells it NOT to mark the running firmware as good straight
 * away. After an OTA update the new firmware stays "pending verify" until
 * markAppValidIfPending() is called at the end of the first successful cycle
 * (see runCycle). If it crashes or resets before then, the bootloader rolls
 * back to the previous firmware automatically.
 */
extern "C" bool verifyRollbackLater()
{
  return true;
} // end function verifyRollbackLater ******************************************


/**
 * @brief One measurement + upload cycle: read the ATTiny85 slave and sensors,
 * calculate derived measurements, connect WiFi and post to the cloud services.
 *
 * Leaves WiFi connected (the caller decides when to kill it) and never sleeps,
 * so it is used both by loop() and from inside the maintenance window.
 */
void runCycle()
{
  displayESP32DateTime();

  getSlave();

  getSensors();

  getDerivedMeasurements();

  initWiFi();

  // **** functions that require WiFi start here ****

  getNTPTime();

  sendCloudDataMQTT();

  //sendCloudDataWeatherUnderground();

  sendCloudDataWeatherCloud();

  //sendCloudDataPWSWeather();

  //sendCloudDataThingSpeak();

  // **** functions that require WiFi end here ****

  // We got here with WiFi up and without crashing - if this is new firmware
  // (first boot after an OTA upload) mark it as good to cancel the rollback
  if (WiFi.status() == WL_CONNECTED) markAppValidIfPending();
} // end function runCycle *****************************************************

void setup()
{
  // Start serial console for debugging (refer "debug.h" header file for where
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

  // We need WiFi for NTP sync (any other setup that requires WiFi should go in here)
  initWiFi();

  // Sync ESP32 RTC with NTP
  getNTPTime();

  // Maintenance window (power-on / EN reset, or first boot after a firmware
  // upload): serve the status page, firmware upload and web log for a few
  // minutes. One normal cycle runs straight away so its output can be watched
  // on the log page, then cycles continue on schedule until the window closes.
  // See maintenance.cpp.
  if (maintenanceShouldEnter())
  {
    maintenanceBegin();
    runCycle();
    maintenanceRun(runCycle);
    maintenanceEnd();
  }

  // Disconnect and power down WiFi chipset
  killWiFi();

  // Go to sleep until next wake event at which point regular weather reporting will begin
  initSleep();

  // Reset slave ATTiny85 measurements
  resetSlave();
}

void loop()
{
  runCycle();

  killWiFi();

  resetDay();

  initSleep();
}
