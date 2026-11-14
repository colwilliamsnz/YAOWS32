#include "debug.h"
#include "version.h"

#include "functions.h"
#include "globals.h"

#include "station.h"
#include "passwords.h"
#include "weathermath.h"
#include "cloud.h"

#include "gpio.h"
#include "log.h"
#include "buildprofile.h"

#include <Arduino.h>

#include <WiFi.h>
#include <time.h>

#include <Wire.h>
#include <OneWire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP085.h>
#include <dht.h>
#include <Adafruit_INA219.h>
#include <DallasTemperature.h>

void getDerivedMeasurements()
{
  g_weatherDewPointC = calcDewPointC(g_weatherTempAirC, g_weatherHumidity);
  g_weatherHeatIndexC = calcHeatIndexC(g_weatherTempAirC, g_weatherHumidity);
  g_weatherTempWindChillC = calcWindChillC(g_weatherTempAirC, g_weatherWindAvg);

  #if DEBUG
    Log.println();
    Log.print(F("\tDew Point: \t\t"));
    Log.print(g_weatherDewPointC);
    Log.println(" °C");
    Log.print(F("\tHeat Index: \t\t"));
    Log.print(g_weatherHeatIndexC);
    Log.println(" °C");
    Log.print(F("\tWind Chill: \t\t"));
    Log.print(g_weatherTempWindChillC);
    Log.println(" °C");
    Log.print(F("\tRainfall: \t\t"));
    Log.print(g_weatherRainFall);
    Log.println(" mm");
    Log.print("\tRainfall (daily): \t");
    Log.print(g_weatherRainFallDaily);
    Log.println(" mm");
    Log.print(F("\tRain Rate: \t\t"));
    Log.print(g_weatherRainRate);
    Log.println(" mm/hr");
  #endif
} // end function getDerivedMeasurements ***************************************


/**
 * @brief Send i2c command to reset ATTiny85 Slave stats
 */
void resetSlave()
{
  Wire1.beginTransmission(0x4);
  Wire1.write("R");
  Wire1.endTransmission();
} // end function resetSlave ***************************************************


/**
 * @brief Init wire and wire1 (second ESP32 I2C bus) objects
 */
void setupI2C()
{
  // init default i2c bus for peripheral (external) sensors
  Wire.begin();

  // init 2nd i2c bus for onboard sensors
  Wire1.begin(INPUT_SENSOR_I2C2_SDA, INPUT_SENSOR_I2C2_SCL, 100000);
} // end function setupI2C *****************************************************


/**
 * @brief Init onboard sensors
 */
void setupSensorsOnboard()
{
  sensor_ina219.begin(&Wire1); // i2c2 INA219 voltage/current sensor
} // end setupI2C *************************************************************


/**
 * @brief Setup peripheral (external) sensors
 */
void setupSensorsExternal()
{
  //g_sensorStatusBME280 = sensor_bme280.begin(); // i2c1 BME280 sensor pressure, temp & hum sensor (not fitted)
  g_sensorStatusBMP180 = sensor_bmp180.begin(); // i2c1 BMP180 sensor pressure
  //sensor_dht22.begin();                       // 1Wire dht22 temp & hum sensor (not fitted)

  // i2c1 AM2315 temp & hum sensor (the station's air temp/humidity sensor)
  if (!sensor_am2315.begin())
  {
    Log.println(F("\tAM2315 not found on I2C bus, check wiring!"));
  }

  sensor_ds18b20.begin();                       // 1Wire ds18b20 waterproof temp sensor
} // end function setupSensorsExternal *****************************************


/**
 * @brief Disconnect from WiFi network AND power off WiFi module
 */
void killWiFi()
{
  // if we're already disconnected, exit early
  if (getWiFiStatus() == 255) return; 

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  digitalWrite(OUTPUT_LED_WIFI, LOW);

  #if DEBUG
    Log.println();
    Log.print(F("Killing WiFi connection"));
    Log.println();
  #endif
} // end function killWiFi *****************************************************


/**
 * @brief Seconds from now until the next scheduled update (aligned to the clock,
 * eg 8:10, 8:20...). Used by initSleep() and by the maintenance window so both
 * keep the same schedule.
 * @param wakeOut (optional) receives the wake up date/time
 */
double secondsUntilNextWake(struct tm *wakeOut)
{
  struct tm timeNow; // time structure for current time
  getLocalTime(&timeNow);

  struct tm timeWakeup = timeNow; // time stucture for next wake up time

  // create new wakeup time based on update interval
  int nextWakeupMinute = getNextWakeupMinute(timeWakeup.tm_min, g_stationUpdatePeriod);

  // we've hit the next hour so increment forwardtime by 1 hour!
  if (nextWakeupMinute == 0) timeWakeup.tm_hour += 1;
  timeWakeup.tm_min = nextWakeupMinute;
  timeWakeup.tm_sec = 0;

  // calculate the difference between time as an offset in seconds
  double offsetSeconds = difftime(mktime(&timeWakeup), mktime(&timeNow));

  if (wakeOut) *wakeOut = timeWakeup;

  return offsetSeconds;
} // end function secondsUntilNextWake *****************************************


/**
 * @brief Put ESP32 into light sleep mode to save battery power between updates
 */
void initSleep()
{
  struct tm timeWakeup; // time stucture for next wake up time

  // sleep for this amount of time
  double offsetSeconds = secondsUntilNextWake(&timeWakeup);

  #if DEBUG
    Log.println();
    Log.println(F("Entering power save light sleep mode..."));
    Log.print(F("\tWakeup interval: \t"));
    Log.print(g_stationUpdatePeriod);
    Log.println(F(" minutes"));
    Log.println(&timeWakeup, "\tNext wakeup datetime:\t%Y/%m/%d %H:%M:%S");
    Log.flush();
  #endif

  // Light sleep for number of seconds required to hit the next update period in
  // exact minutes (eg, 8:10, 8:20 etc)
  esp_sleep_enable_timer_wakeup(offsetSeconds * 1000000);
  esp_light_sleep_start();
  
  #if DEBUG
    Log.println("");
    Log.println("Waking up...");
  #endif
} // end function initSleep ****************************************************


/**
 * @brief return the next wakeup time (minutes only) based on the current time
 * and the update interval. Used to keep station wake ups and cloud updates at a
 * strict interval from "o'clock". Eg, 8:10, 8:20, 8:30, 8:40...
 */
int getNextWakeupMinute(int currentMinutes, int updateInterval)
{
  int wakesPerHour; // number of wakes per hour based on the update interval
  int nextWake;     // calculate next time the alarm should be set to

  wakesPerHour = currentMinutes / updateInterval;

  if (currentMinutes % updateInterval > 0) wakesPerHour++;

  nextWake = wakesPerHour * updateInterval;

  // calculated interval is the same as the current minute so set to next
  // interval
  if (nextWake == currentMinutes) nextWake += (updateInterval);

  if (nextWake > 59) nextWake = 0; // we can't go past 0 o'clock

  return nextWake;
}


/**
 * @brief Connect to WiFi network specified in passwords.cpp
 */
void initWiFi()
{
  int _statusWiFi = getWiFiStatus();

   // If we're not connected, connect
  if (_statusWiFi != 3)
  {
    WiFi.setHostname(DEVICE_HOSTNAME); // must be set before WiFi.begin()
    WiFi.begin(g_wiFiSSID, g_wiFiPass);

    #if DEBUG
      Log.println();
      Log.print(F("Attempting WiFi connection to SSID "));
      Log.print(g_wiFiSSID);
    #endif

    int WLanCount = 0;
    while (WiFi.status() != WL_CONNECTED && WLanCount < 20)
    {
      digitalWrite(OUTPUT_LED_WIFI, HIGH); // Flash WiFi LED
      delay(500);
      digitalWrite(OUTPUT_LED_WIFI, LOW);
      delay(500);

      #if DEBUG
            Log.printf(".");
      #endif

      ++WLanCount;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      // Connection succesful - turn on WiFi indicator LED
      digitalWrite(OUTPUT_LED_WIFI, HIGH);

      // Get WiFi RSSI
      g_stationWiFiRSSI = WiFi.RSSI();

      // Display IP address etc to serial console 
      getWiFiInfo();
    }
    
    else
    {
      // Put ESP32 into deep sleep mode for the update interval period
      #if DEBUG
        Log.println();
        Log.print(F("\tFailed to connect to WiFi. Sleeping for "));
        Log.print(g_stationUpdatePeriod);
        Log.println(F(" minutes to conserve battery."));
      #endif

      initSleep();
      ESP.restart();
    }
  }
} // end function initWiFi ****************************************************


/**
 * @brief Parent function for reading all peripheral sensors
 */
void getSensors()
{
  #if DEBUG
    Log.println();
    Log.println(F("Reading sensors...onboard..."));
  #endif

  getSensorINA219(); // Voltage/current sensor

  #if DEBUG
    Log.println();
    Log.println(F("Reading sensors...external..."));
  #endif

  getSensorBMP180();  // Pressure sensor
  //getSensorBME280(); // Pressure, Temp & Hum sensor (not fitted)
  //getSensorDHT22();  // 1-wire Temp/Hum sensor (not fitted)
  getSensorAM2315();    // I2C Temp/Hum sensor - air temp & humidity
  //getSensorDS18B20(); // Soil temp sensor
}



void getWiFiInfo() //////////////////// TODO TODO should be named "printWiFiInfo"
{
  #if DEBUG
    Log.println();
    Log.printf("\tSSID:\t\t\t%s, ", WiFi.SSID().c_str());
    Log.print(WiFi.RSSI());
    Log.println(" dBm");
    Log.print("\tIP address:\t\t");
    Log.print(WiFi.localIP());
    Log.print(" / ");
    Log.println(WiFi.subnetMask());
    Log.print("\tGateway:\t\t");
    Log.println(WiFi.gatewayIP());
    Log.print("\tDNS:\t\t\t");
    Log.println(WiFi.dnsIP());
  #endif
}


int getWiFiStatus()
{
  int WiFiStatus = WiFi.status();
  
  #if DEBUG
    Log.println();
    Log.println(F("Checking WiFi..."));
    Log.printf("\tStatus:\t\t\t%d", WiFiStatus);

    switch (WiFiStatus)
    {
    case WL_IDLE_STATUS:      // WL_IDLE_STATUS = 0
      Log.printf(", WiFi IDLE\n");
      break;
    case WL_NO_SSID_AVAIL:    // WL_NO_SSID_AVAIL = 1
      Log.printf(", NO SSID AVAIL\n");
      break;
    case WL_SCAN_COMPLETED:   // WL_SCAN_COMPLETED = 2
      Log.printf(", WiFi SCAN_COMPLETED\n");
      break;
    case WL_CONNECTED:        // WL_CONNECTED = 3
      Log.printf(", WiFi CONNECTED\n");
      break;
    case WL_CONNECT_FAILED:   // WL_CONNECT_FAILED = 4
      Log.printf(", WiFi WL_CONNECT FAILED\n");
      break;
    case WL_CONNECTION_LOST:  // WL_CONNECTION_LOST = 5
      Log.printf(", WiFi CONNECTION LOST\n");
      break;
    case WL_DISCONNECTED:     // WL_DISCONNECTED = 6
      Log.printf(", WiFi DISCONNECTED\n");
      break;
    case WL_NO_SHIELD:        // WL_DISCONNECTED = 255
      Log.printf(", WiFi NOT INITALIZED\n");
      break;
    }
  #endif

  return WiFiStatus;
}


/**
* @brief Setup GPIO pins and set initial state to OFF
*/
void setupGPIOPins()
{
  pinMode(OUTPUT_LED_WIFI,    OUTPUT);
  pinMode(OUTPUT_LED_MISC_1,  OUTPUT);
  pinMode(OUTPUT_LED_MISC_2,  OUTPUT);

  // add analog pins in here too as needed

} // end function setupGPIOPins ***********************************************


/**
* @brief Startup: flash onboard LEDs and send version info to the serial console
*/
void initStartup()
{
  for (int i = 0; i < 20; i++)
  {
    digitalWrite(OUTPUT_LED_MISC_1, !digitalRead(OUTPUT_LED_MISC_1));
    delay(100);
    digitalWrite(OUTPUT_LED_MISC_2, !digitalRead(OUTPUT_LED_MISC_2));

    #if DEBUG
      Log.printf(".");
    #endif
  }

  #if DEBUG
    Log.println("");
    Log.print("YAOWS32 Weather Station | ");
    Log.print(VERSION);
    Log.print(" | ");
    Log.print(DEVICE_PROFILE);
    Log.print(" | Compiled on ");
    Log.print(__DATE__);
    Log.print(" @ ");
    Log.println(__TIME__);
  #endif
} // end function displayStartup **********************************************


/**
* @brief Connect to ATTiny85 "Slave" and get Rain and Wind measurements then reset
* the slave, ready for the next wake-up cycle and collection of measurements.
*/
void getSlave()
{
  #if DEBUG
    Log.println("");
    Log.println("Reading ATTiny85 Slave...");
  #endif

  // request 8 bytes from slave device
  uint8_t bytesReceived = Wire1.requestFrom((uint8_t)0x4, (uint8_t)8);

  if (bytesReceived != 8)
  {
    // Slave missing (eg desktop test rig) or bus error - don't interpret junk
    while (Wire1.available()) Wire1.read();

    g_weatherRainRate = 0;
    g_weatherRainFall = 0;
    g_weatherWindAvg  = 0;
    g_weatherWindGust = 0;

    Log.printf("\tSlave not responding (%u of 8 bytes) - wind/rain set to 0\n", bytesReceived);
    return;
  }

  int i = 0; // counter for each byte as it arrives
  while (Wire1.available() && i < 8)
  {
    g_slaveRecord[i] = Wire1.read();
    i = i + 1;
  }

  int rawWindDir = word(g_slaveRecord[0], g_slaveRecord[1]);
  int rawWindSpd = word(g_slaveRecord[2], g_slaveRecord[3]);
  int rawWindGst = word(g_slaveRecord[4], g_slaveRecord[5]);
  int rawRainCnt = word(g_slaveRecord[6], g_slaveRecord[7]);

  resetSlave(); // we have our data, so reset the slave ready for next wakeup period

  // Calcualate average wind speed, in Km/hr, over reporting period
  // Vel = Pulse Count * (Anemometer Speed Factor / Reporting period in seconds)
  float windSpeed = rawWindSpd * (g_stationWindSpeedFactor / (g_stationUpdatePeriod * 60));

  // Calculate wind gust speed, in Km/hr, over period of 6s
  // Vel = Pulse Count * (g_stationWindSpeedFactor / 6 seconds)
  float windGustSpeed = rawWindGst * (g_stationWindSpeedFactor / g_stationWindGustPeriod);

  // Calculate rain rate, in mm/hr
  float rainRate = (g_stationRainTipVolume * rawRainCnt) * (60 / g_stationUpdatePeriod);

  g_weatherRainRate = rainRate;

  // Add rain tip count to the overall volume for the entire day
  g_weatherRainFallDaily = g_weatherRainFallDaily + (g_stationRainTipVolume * rawRainCnt);

  // Assign rain tip count to the overall volume for the reporting period
  g_weatherRainFall = g_stationRainTipVolume * rawRainCnt;

  g_weatherWindAvg = calcWindSpeedHeightComp(g_stationWindVaneHeight, windSpeed);
  g_weatherWindGust = calcWindSpeedHeightComp(g_stationWindVaneHeight, windGustSpeed);
  g_weatherWindDir = rawWindDir;

  #if DEBUG
    Log.print("\tWind Dir (avg): \t");
    Log.print(rawWindDir);
    Log.println(" deg");
    Log.print("\tWind Speed (raw): \t");
    Log.println(rawWindSpd);
    Log.print("\tWind Speed (calc): \t");
    Log.print(windSpeed);
    Log.println(" km/h");
    Log.print("\tWind Gust (raw): \t");
    Log.println(rawWindGst);
    Log.print("\tWind Gust (calc): \t");
    Log.print(windGustSpeed);
    Log.println(" km/h");
    Log.print("\tRain Tip Count: \t");
    Log.println(rawRainCnt);
  #endif
} // end function getSlave ****************************************************


/**
* @brief Get internet time from NTP server and set the RTC if successful
*
*/
void getNTPTime()
{
  // check if update interval has passed? 0 means either first run OR the countdown timer is done
  if (g_ntpCheckIntervalTimer > 0)
  {
    g_ntpCheckIntervalTimer -= g_stationUpdatePeriod;

    #if DEBUG
        Log.println();
        Log.print(F("Skipping NTP time sync...next update in "));
        Log.print(g_ntpCheckIntervalTimer + g_stationUpdatePeriod);
        Log.println(F(" minutes"));
    #endif
    
    return;
  }

  // perform the NTP check...
  #if DEBUG
    Log.println();
    Log.println(F("Updating time from NTP server..."));
  #endif

  IPAddress ip;
  Log.print("DNS server: ");
  Log.println(WiFi.dnsIP());

  if (WiFi.hostByName(g_ntpServer1, ip) == 1) {
    Log.print("Resolved to: ");
    Log.println(ip);
  } else {
    Log.println("DNS lookup failed");
  }

  configTzTime(g_ntpTimeZone, g_ntpServer1, g_ntpServer2, g_ntpServer3);

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
  {
    #if DEBUG
        Log.println("\tFailed");
    #endif

    return;
  }

  #if DEBUG
    Log.println(&timeinfo, "\tGot datetime: \t\t%Y/%m/%d %H:%M:%S");
  #endif

  g_ntpCheckIntervalTimer = g_ntpCheckInterval; // reset the interval timer now that we got a valid time from NTP
} // end getNTPTime ***********************************************************


/**
 * @brief Get the temperature and humidty from DHT22 1-Wire sensor
 * 
 */
void getSensorDHT22()
{
  float _temp = sensor_dht22.readTemperature();
  float _hum = sensor_dht22.readHumidity();

  g_weatherTempAirC = _temp;
  g_weatherHumidity = _hum;

#if DEBUG
  Log.print("\tDHT22 Temp:\t\t");
  Log.print(_temp);
  Log.print(" °C");
  Log.println();
  Log.print("\tDHT22 Hum:\t\t");
  Log.print(_hum);
  Log.print(" %");
  Log.println();
#endif
} // end function getSensorDHT22 **********************************************


/**
 * @brief Get the temperature and humidty from AM2315 I2C sensor
 * 
 */
void getSensorAM2315()
{
  float _temp = NAN;
  float _hum = NAN;

  // One I2C transaction for both values. The library refuses reads less than
  // 2s apart (and begin() takes a test reading), so retry once after 2.1s.
  bool ok = sensor_am2315.readTemperatureAndHumidity(&_temp, &_hum);
  if (!ok)
  {
    delay(2100);
    ok = sensor_am2315.readTemperatureAndHumidity(&_temp, &_hum);
  }

  if (!ok)
  {
    _temp = NAN;
    _hum = NAN;
    Log.println(F("\tAM2315 read FAILED"));
  }

  g_weatherTempAirC = _temp;
  g_weatherHumidity = _hum;

#if DEBUG
  Log.print("\tAM2315 Temp:\t\t");
  Log.print(_temp);
  Log.print(" °C");
  Log.println();
  Log.print("\tAM2315 Hum:\t\t");
  Log.print(_hum);
  Log.print(" %");
  Log.println();
#endif
} // end function getSensorAM2315 **********************************************


/**
 * @brief Get the temperature from DS18B20 1-Wire sensor
 * 
 */
void getSensorDS18B20()
{
  sensor_ds18b20.requestTemperatures();

  float _temp = sensor_ds18b20.getTempCByIndex(0);

  g_weatherTempSoilC = _temp;

#if DEBUG
  Log.print("\tDS18B20 Temp:\t\t");
  Log.print(_temp);
  Log.print(" °C");
  Log.println();
#endif
} // end function getSensorDS18B20 ********************************************


/**
 * @brief Get temp, humidity & pressure from BME280 I2C sensor
 * 
 */
void getSensorBME280()
{
  if (!g_sensorStatusBME280)
  {
    #ifdef DEBUG
      Log.println("\tCould not find a valid BME280 sensor, check wiring!");
    #endif
    
    return;
  }

  float _temp = sensor_bme280.readTemperature();
  float _hum = sensor_bme280.readHumidity();
  float _press = sensor_bme280.readPressure() / 100.0F;

  g_weatherPressureAbs = _press;
  g_weatherPressureRel = _press + g_stationAltitude / 8.3; // formula to convert abs to rel (altitude / 8.3)

  #if DEBUG
    Log.print(F("\tBME280 Temp: \t\t"));
    Log.print(_temp);
    Log.print(" °C");
    Log.println();
    Log.print(F("\tBME280 Hum: \t\t"));
    Log.print(_hum);
    Log.print(" %");
    Log.println();
    Log.print(F("\tBME280 Press: \t\t"));
    Log.print(_press);
    Log.print(" hPa (Abs)");
    Log.println();
    Log.print(F("\tBME280 Press: \t\t"));
    Log.print(g_weatherPressureRel);
    Log.print(" hPa (Rel)");
    Log.println();
  #endif
} // end function getSensorBME280 *********************************************


/**
 * @brief Get atmospheric pressure (rel and abs) from BMP180 sensor
 */
void getSensorBMP180()
{
  if (!g_sensorStatusBMP180)
  {
    #if DEBUG
      Log.println("\tCould not find a valid BMP180 sensor, check wiring!");
    #endif

    return; // exit early - sensor is likely not connected
  }

  float _press = sensor_bmp180.readPressure() / 100.0F;

  g_weatherPressureAbs = _press;
  g_weatherPressureRel = _press + g_stationAltitude / 8.3; // formula to convert abs to rel (altitude / 8.3)

  float _altitude = sensor_bmp180.readAltitude(g_stationPressureCal * 100.0F); // hPa -> Pa (library expects Pa). Estimates altitude from nearest observation station's sea-level pressure (for a new install location)

  #if DEBUG
    Log.print(F("\tBMP180 Press: \t\t"));
    Log.print(_press);
    Log.print(" hPa (Abs)");
    Log.println();
    Log.print(F("\tBMP180 Press: \t\t"));
    Log.print(g_weatherPressureRel);
    Log.print(" hPa (Rel)");
    Log.println();
    Log.print(F("\tBMP180 Alt: \t\t"));
    Log.print(_altitude);
    Log.print(" m");
    Log.println();
  #endif
} // end getSensorBMP180 ******************************************************


/**
 * @brief Get input voltage (battery) and current draw from INA219 sensor
 */
void getSensorINA219()
{
  g_stationBatteryVoltage = sensor_ina219.getBusVoltage_V();
  g_stationBatteryCurrent = sensor_ina219.getCurrent_mA();

  #if DEBUG
    Log.print(F("\tINA219 Batt: \t\t"));
    Log.print(g_stationBatteryVoltage);
    Log.print(" V");
    Log.println();
    Log.print(F("\tINA219 Curr: \t\t"));
    Log.print(g_stationBatteryCurrent);
    Log.print(" mA");
    Log.println();
  #endif
} // end getSensorINA219 ******************************************************


/**
* @brief Print ESP32 RTC Time and Date to the serial monitor
*/
void displayESP32DateTime()
{
  #if DEBUG
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    Log.println(&timeinfo, "\tESP32 RTC datetime:\t%Y/%m/%d %H:%M:%S");
  #endif
} // end function displayESP32Time ********************************************


/**
 * @brief Reset weather stats that need to be reset daily by comparing current
 * time with config'd daily reset time.
 */
void resetDay()
{
  #if DEBUG
    Log.println();
    Log.println(F("Checking reset of daily stats..."));
  #endif
  
  struct tm timeNow;
  getLocalTime(&timeNow);

  char timeCheck[6]; // "HH:MM" + null. char array we will assemble tm struct for comparison with configured reset time
  snprintf(timeCheck, sizeof(timeCheck), "%02d:%02d", timeNow.tm_hour, timeNow.tm_min);

  if (strcmp(timeCheck, g_stationDailyResetTime) == 0) // char arrays match, its time to reset the daily stats
  {
    g_weatherRainFallDaily = 0; // reset daily accumulated rainfall
      #if DEBUG
      Log.print(F("\tCurrent time matches configured time ("));
      Log.print(g_stationDailyResetTime);
      Log.println(F(")--resetting"));
    #endif
  }
  else
  {
    #if DEBUG
      Log.print(F("\tCurrent time doesn't match configured time ("));
      Log.print(g_stationDailyResetTime);
      Log.println(F(")--skipping"));
    #endif
  }
} // end function resetDay ****************************************************