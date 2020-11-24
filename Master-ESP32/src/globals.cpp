#include "globals.h"
#include "gpio.h"

#include <Arduino.h>

// *** Global Objects *********************************************************

Adafruit_BME280 sensor_bme280;
Adafruit_BMP085 sensor_bmp180;
Adafruit_INA219 sensor_ina219;
Adafruit_AM2315 sensor_am2315;

DHT sensor_dht22(INPUT_SENSOR_1WIRE_1, DHT22);

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire2(INPUT_SENSOR_1WIRE_2);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensor_ds18b20(&oneWire2);


// *** Global variables *******************************************************

// *** ATTiny Slave Record for data to be transferred over 12C bus
byte    g_slaveRecord[8];

// *** Weather Station conditions
float   g_weatherTempAirC       {0};
float   g_weatherTempSoilC      {0};
float   g_weatherTempWindChillC {0};
float   g_weatherHumidity       {0};
float   g_weatherPressureRel    {0};
float   g_weatherPressureAbs    {0};
float   g_weatherDewPointC      {0};
float   g_weatherHeatIndexC     {0};

float   g_weatherRainFall       {0};
float   g_weatherRainFallDaily  {0};
float   g_weatherRainRate       {0};

float   g_weatherWindAvg        {0};
float   g_weatherWindGust       {0};
float   g_weatherWindDir        {0};


// *** Weather station onboard sensor telemetry data
float   g_stationBatteryVoltage {0};
float   g_stationBatteryCurrent {0};
int     g_stationWiFiRSSI       {0};


// *** Timer (minutes) to determine if we should check NTP yet?
int     g_ntpCheckIntervalTimer {0};


// *** I2C sensor status
bool    g_sensorStatusBME280;
bool    g_sensorStatusBMP180;