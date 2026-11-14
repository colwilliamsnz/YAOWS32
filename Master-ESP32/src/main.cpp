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
#include "maintenance.h"  // OTA + web console maintenance window
#include "log.h"          // Serial + web console logger

#include "gpio.h"         // #define entries for GPIO pin assignments

/**
 * @brief Tell the Arduino core NOT to mark the running firmware as valid at
 * boot. After an OTA update the new image stays "pending verify" until
 * markAppValidIfPending() is called (after WiFi + a full cycle succeed). If it
 * crashes/resets before then, the bootloader rolls back to the previous image.
 */
extern "C" bool verifyRollbackLater()
{
  return true;
}

/**
 * @brief One measurement + upload cycle. Leaves WiFi connected (caller decides
 * when to kill it) and never sleeps, so it can also be run from inside the
 * maintenance window.
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

  // We got here with WiFi up and without crashing - new firmware is good
  if (WiFi.status() == WL_CONNECTED) markAppValidIfPending();
}

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

  // On power-on / EN reset: open the OTA + web console window, run one normal
  // cycle so its output can be watched in the browser, then serve until the
  // window closes.
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
