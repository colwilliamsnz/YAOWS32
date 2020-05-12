#include "functions.h"
#include "globals.h"
#include "station.h"
#include "passwords.h"
#include "weathermath.h"
#include "cloud.h"

#include "gpio.h"

#include "version.h"

#include <Arduino.h>


void sendCloudDataWeatherCloud()
{
  char cloudServer[] = "api.weathercloud.net";

  #if DEBUG
    Serial.println();
    Serial.println(F("Posting data to WeatherCloud..."));
  #endif

  WiFiClient client;

  if (!client.connect(cloudServer, 80))
  {
    #if DEBUG
      Serial.println(F("\tConnection failed"));
    #endif

    return;
  }

  #if DEBUG
    Serial.println("\tConnected successfully");
  #endif

  String postStr = "";
  postStr += "/set/wid/";

  #if DEBUG_CLOUD_WEATHERCLOUD
    postStr += g_cloudServiceWeatherCloudId_DEBUG;
    postStr += "/key/";
    postStr += g_cloudServiceWeatherCloudKey_DEBUG;
  #else
    postStr += g_cloudServiceWeatherCloudId;
    postStr += "/key/";
    postStr += g_cloudServiceWeatherCloudKey;
  #endif

  postStr += "/temp/";
  postStr += g_weatherTempAirC * 10;    // 21.5deg reported as 215
  //postStr += "/tempin/";              // Inside temp
  postStr += "/hum/";
  postStr += g_weatherHumidity;
  //postStr += "/humin/";               // Inside humidity
  postStr += "/bar/";
  postStr += g_weatherPressureRel * 10; // 1017hPa reported as 10170
  postStr += "/rain/";
  postStr += g_weatherRainFall * 10;    // Rainfall 2.0mm reported as 20
  postStr += "/rainrate/";
  postStr += g_weatherRainRate * 10;
  postStr += "/heat/";
  postStr += g_weatherHeatIndexC * 10;
  postStr += "/dew/";
  postStr += g_weatherDewPointC * 10;
  postStr += "/wspd/";                  // Windspeed reported in m/s * 10
  postStr += convertKpHtoMpS(g_weatherWindAvg * 10);
  postStr += "/wspdavg/";
  postStr += convertKpHtoMpS(g_weatherWindAvg * 10);
  postStr += "/wspdhi/";
  postStr += convertKpHtoMpS(g_weatherWindGust * 10);
  postStr += "/wdir/";
  postStr += g_weatherWindDir;
  postStr += "/wdiravg/";
  postStr += g_weatherWindDir;
  //postStr += "/uvi/";                 // UVI of 3 reported as 30
  //postStr += "/solarrad/";            // Solarrad of 600 reported as 6000
  postStr += "/chill/";               
  postStr += g_weatherTempWindChillC * 10; // Windchill of 21.5deg reported as 215

  #if DEBUG
    Serial.print("\tGET ");
    Serial.println(postStr);
  #endif

  client.print(String("GET ") + postStr + " HTTP/1.1\r\n" + 
      "Host: " + cloudServer + "\r\n" +
      "User-Agent: YAOWS32\r\n" +
      "Connection: close\r\n\r\n");

  while (client.connected())
  {
    while (client.available())
    {
      String line = client.readStringUntil('\n');

      // 400 = Bad request?
      // 401 = Invalid WID or KEY?
      // 404 = Invalid request action?
      // 500 = Too frequent data upload?
      // 200 = Success!
      if (line == "200\r") line = "Success posting weather data!";
      if (line == "500\r") line = "Update period too frequent, ignoring data!";

      #if DEBUG
        Serial.println("\t" + line);
      #endif
    }   
  }
  Serial.println(F("\tDisconnected from WeatherCloud"));
}

void sendCloudDataThingSpeak()
{
  char _server[] = "api.thingspeak.com";

#if DEBUG
  Serial.println();
  Serial.println(F("Posting data to Thingspeak..."));
#endif

  WiFiClient client;

  if (!client.connect(_server, 80))
  {
    Serial.println("\tConnection failed");
    return;
  }
  Serial.println("\tConnected successfully");

  String postStr = "";
  postStr += "/update?api_key=";
  postStr += g_cloudServiceThingSpeakAPIKey;
  postStr += "&field1=";
  postStr += String(g_stationTempC);
  postStr += "&field2=";
  postStr += String(g_stationBatteryVoltage);
  postStr += "&field3=";
  postStr += String(g_stationBatteryCurrent);

  client.print(String("GET ") + postStr + " HTTP/1.1\r\n" + "Host: " + _server + "\r\n" + "Connection: close\r\n\r\n");

#if DEBUG
  Serial.print("\tGET ");
  Serial.println(postStr);
#endif

  while (client.available())
  {
    char line = client.read();

    Serial.println(line);
  }
}


void sendCloudDataWeatherUnderground()
{
  char _server[] = "rtupdate.wunderground.com";

  #if DEBUG
    Serial.println();
    Serial.println(F("Posting data to Wunderground..."));
  #endif

  WiFiClient client;

  // Convert sensor data to suit Wunderground requirements (temp/dewpoint=f, pressure=inHg)
  float _tempF = convertTempCtoF(g_weatherTempAirC);                 // Convert native C to F
  float _baromInHg = convertPressureHPAtoINHG(g_weatherPressureRel); // Convert native hPa to inHg
  float _dewPtF = convertTempCtoF(g_weatherDewPointC);               // Convert native C to F

  if (!client.connect(_server, 80))
  {
    Serial.println(F("\tConnection failed"));
    return;
  }

  #if DEBUG
    Serial.println("\tConnected successfully");
  #endif

  String postStr = "";
  postStr += "/weatherstation/updateweatherstation.php?ID=";
  postStr += g_cloudServiceWeatherUndergroundLogin;
  postStr += "&PASSWORD=";
  postStr += g_cloudServiceWeatherUndergroundPass;
  postStr += "&dateutc=now";
  postStr += "&tempf=";
  postStr += _tempF;
  postStr += "&humidity=";
  postStr += g_weatherHumidity;
  postStr += "&dewptf=";
  postStr += _dewPtF;
  postStr += "&baromin=";
  postStr += _baromInHg;
  postStr += "&softwaretype=DynamicAlgorithmsESP32&action=updateraw";

  client.print(String("GET ") + postStr + " HTTP/1.1\r\n" + "Host: " + _server + "\r\n" + "Connection: close\r\n\r\n");

  #if DEBUG
    Serial.print("\tGET ");
    Serial.println(postStr);
  #endif

  while (client.available())
  {
    char line = client.read();
    Serial.println(line);
  }
}


void sendCloudDataPWSWeather()
{
  char _server[] = "www.pwsweather.com";

#if DEBUG
  Serial.println();
  Serial.println(F("Posting data to PWSWeather..."));
#endif

  WiFiClient client;

  // Convert sensor data to suit PWSWeather requirements (temp/dewpoint=f, pressure=inHg)
  float _tempF = convertTempCtoF(g_weatherTempAirC);                 // Convert native C to F
  float _baromInHg = convertPressureHPAtoINHG(g_weatherPressureRel); // Convert native hPa to inHg
  float _dewPtF = convertTempCtoF(g_weatherDewPointC);               // Convert native C to F

  if (!client.connect(_server, 80))
  {
    Serial.println(F("\tConnection failed"));
    return;
  }

#if DEBUG
  Serial.println("\tConnected successfully");
#endif

  String postStr = "";
  postStr += "/pwsupdate/pwsupdate.php?ID=";
  postStr += g_cloudServicePwsWeatherId;
  postStr += "&PASSWORD=";
  postStr += g_cloudServicePwsWeatherPassword;
  postStr += "&dateutc=now";
  postStr += "&tempf=";
  postStr += _tempF;
  postStr += "&humidity=";
  postStr += g_weatherHumidity;
  postStr += "&dewptf=";
  postStr += _dewPtF;
  postStr += "&baromin=";
  postStr += _baromInHg;
  postStr += "&action=updateraw";

  client.print(String("GET ") + postStr + " HTTP/1.1\r\n" + "Host: " + _server + "\r\n" + "Connection: close\r\n\r\n");

#if DEBUG
  Serial.print("\tGET ");
  Serial.println(postStr);
#endif

  while (client.available())
  {
    //String line = client.readStringUntil('\r');
    char line = client.read();
    Serial.println(line);
  }
}