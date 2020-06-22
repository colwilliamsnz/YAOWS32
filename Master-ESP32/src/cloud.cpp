#include "debug.h"
#include "version.h"

#include "passwords.h"

#include "functions.h"
#include "globals.h"
#include "station.h"
#include "weathermath.h"
#include "cloud.h"

#include "gpio.h"

#include <Arduino.h>

void sendCloudDataWeatherCloud()
{
  /*
    Based on API Documentation 0.7
    Format: http://api.weathercloud.net/v01/set/wid/$WID/key/$KEY/...
    Interval: 10 minutes - less than this results in discarded data
  */
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

  String postStr = "/v01/set";

  #if !DEBUG_CLOUD_WEATHERCLOUD
    postStr += "/wid/";
    postStr += g_cloudServiceWeatherCloudId;
    postStr += "/key/";
    postStr += g_cloudServiceWeatherCloudKey;
  #else
    postStr += "/wid/";
    postStr += g_cloudServiceWeatherCloudId_DEBUG;
    postStr += "/key/";
    postStr += g_cloudServiceWeatherCloudKey_DEBUG;
  #endif

  postStr += "/temp/";
  postStr += g_weatherTempAirC * 10;
  postStr += "/hum/";
  postStr += g_weatherHumidity;
  postStr += "/bar/";
  postStr += g_weatherPressureRel * 10;
  postStr += "/rain/";
  postStr += g_weatherRainFall * 10;
  postStr += "/rainrate/";
  postStr += g_weatherRainRate * 10;
  postStr += "/heat/";
  postStr += g_weatherHeatIndexC * 10;
  postStr += "/dew/";
  postStr += g_weatherDewPointC * 10;
  postStr += "/wspd/";
  postStr += kph_to_ms(g_weatherWindAvg * 10);
  postStr += "/wspdavg/";
  postStr += kph_to_ms(g_weatherWindAvg * 10);
  postStr += "/wspdhi/";
  postStr += kph_to_ms(g_weatherWindGust * 10);
  postStr += "/wdir/";
  postStr += g_weatherWindDir;
  postStr += "/wdiravg/";
  postStr += g_weatherWindDir;
  postStr += "/chill/";               
  postStr += g_weatherTempWindChillC * 10;

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

      // 200 = Success
      // 400 = Bad request
      // 401 = Incorrect WID or KEY
      // 429 = To many requests
      if (line == "200\r") line = "Success posting weather data!";
      if (line == "429\r") line = "Update period too frequent, ignoring data!";

      #if DEBUG_CLOUD_VERBOSE
        Serial.println("\t" + line);
      #endif
    }   
  }
  #if DEBUG
    Serial.println(F("\tDisconnected from WeatherCloud"));
  #endif
}

void sendCloudDataThingSpeak()
{
  char cloudServer[] = "api.thingspeak.com";

  #if DEBUG
    Serial.println();
    Serial.println(F("Posting data to Thingspeak..."));
  #endif

  WiFiClient client;

  if (!client.connect(cloudServer, 80))
  {
    Serial.println("\tConnection failed");
    return;
  }
  Serial.println("\tConnected successfully");

  String postStr = "/update";
  
  #if !DEBUG_CLOUD_THINGSPEAK
    postStr += "?api_key=";
    postStr += g_cloudServiceThingSpeakAPIKey;
  #else
    postStr += "?api_key=";
    postStr += g_cloudServiceThingSpeakAPIKey_DEBUG;
  #endif

  postStr += "&field1=";
  postStr += String(g_stationBatteryVoltage);
  postStr += "&field2=";
  postStr += String(g_stationBatteryCurrent);
  postStr += "&field3=";
  postStr += String(g_stationWiFiRSSI);
  postStr += "&field4=";
  postStr += String(g_weatherTempAirC);
  postStr += "&field5=";
  postStr += String(g_weatherHumidity);
  postStr += "&field6=";
  postStr += String(g_weatherPressureRel);
  postStr += "&field7=";
  postStr += String(g_weatherRainFall);

  client.print(String("GET ") + postStr + " HTTP/1.1\r\n" +
    "Host: " + cloudServer + "\r\n" +
    "User-Agegnt: YAOWS32\r\n" +
    "Connection: close\r\n\r\n");

  #if DEBUG
    Serial.print("\tGET ");
    Serial.println(postStr);
  #endif

  while (client.connected())
  {
    while (client.available())
    {
      String line = client.readStringUntil('\n');

      #if DEBUG_CLOUD_VERBOSE
        Serial.println("\t" + line);
      #endif
    }   
  }
  #if DEBUG
    Serial.println(F("\tDisconnected from ThingSpeak"));
  #endif
}


void sendCloudDataWeatherUnderground()
{
  char cloudServer[] = "rtupdate.wunderground.com";

  #if DEBUG
    Serial.println();
    Serial.println(F("Posting data to Wunderground..."));
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

  String postStr = "/weatherstation/updateweatherstation.php";
  
  #if !DEBUG_CLOUD_WEATHERUNDERGROUND
    postStr += "?ID=";
    postStr += g_cloudServiceWeatherUndergroundID;
    postStr += "&PASSWORD=";
    postStr += g_cloudServiceWeatherUndergroundKey;
  #else
    postStr += "?ID=";
    postStr += g_cloudServiceWeatherUndergroundID_DEBUG;
    postStr += "&PASSWORD=";
    postStr += g_cloudServiceWeatherUndergroundKey_DEBUG;
  #endif

  postStr += "&dateutc=now";
  postStr += "&tempf=";
  postStr += C_to_F(g_weatherTempAirC);         // native C to F
  postStr += "&humidity=";
  postStr += g_weatherHumidity;
  postStr += "&dewptf=";
  postStr += C_to_F(g_weatherDewPointC);        // native C to F
  postStr += "&baromin=";
  postStr += hPa_to_inHg(g_weatherPressureRel); // native hPa to inHg
  postStr += "&winddir=";
  postStr += g_weatherWindDir;
  postStr += "&windspeedmph=";
  postStr += kph_to_mph(g_weatherWindAvg);
  postStr += "&windgustmph=";
  postStr += kph_to_mph(g_weatherWindGust);
  postStr += "&rainin=";
  postStr += mm_to_inches(g_weatherRainFall);
  postStr += "&dailyrainin=";
  postStr += mm_to_inches(g_weatherRainFallDaily);
  postStr += "&softwaretype=YAOWS32&action=updateraw";

  client.print(String("GET ") + postStr + " HTTP/1.1\r\n" +
    "Host: " + cloudServer + "\r\n" +
    "User-Agent: YAOWS32\r\n" +
    "Connection: close\r\n\r\n");

  #if DEBUG
    Serial.print("\tGET ");
    Serial.println(postStr);
  #endif

  while (client.connected())
    {
      while (client.available())
      {
        String line = client.readStringUntil('\n');

        #if DEBUG_CLOUD_VERBOSE
          Serial.println("\t" + line);
        #endif
      }   
    }
    #if DEBUG
      Serial.println(F("\tDisconnected from Wunderground"));
    #endif
}


void sendCloudDataPWSWeather()
{
  char cloudServer[] = "www.pwsweather.com";

  #if DEBUG
    Serial.println();
    Serial.println(F("Posting data to PWSWeather..."));
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

  String postStr = "/pwsupdate/pwsupdate.php?";

  #if !DEBUG_CLOUD_PWSWEATHER
    postStr += "ID=";
    postStr += g_cloudServicePwsWeatherId;
    postStr += "&PASSWORD=";
    postStr += g_cloudServicePwsWeatherPassword;
  #else
    postStr += "ID=";
    postStr += g_cloudServicePwsWeatherId_DEBUG;
    postStr += "&PASSWORD=";
    postStr += g_cloudServicePwsWeatherPassword_DEBUG;
  #endif

  // PWSWeather requirements (temp/dewpoint=f, pressure=inHg, rain=in)
  postStr += "&dateutc=now";
  postStr += "&tempf=";
  postStr += C_to_F(g_weatherTempAirC);         // native C to F
  postStr += "&humidity=";
  postStr += g_weatherHumidity;
  postStr += "&dewptf=";
  postStr += C_to_F(g_weatherDewPointC);        // native C to F
  postStr += "&baromin=";
  postStr += hPa_to_inHg(g_weatherPressureRel); // native hPa to inHg
  postStr += "&winddir=";
  postStr += g_weatherWindDir;
  postStr += "&windspeedmph=";
  postStr += kph_to_mph(g_weatherWindAvg);
  postStr += "&windgustmph=";
  postStr += kph_to_mph(g_weatherWindGust);
  postStr += "&rainin=";
  postStr += mm_to_inches(g_weatherRainFall);
  postStr += "&dailyrainin=";
  postStr += mm_to_inches(g_weatherRainFallDaily);
  postStr += "&softwaretype=YAOWS32&action=updateraw";

  client.print(String("GET ") + postStr + " HTTP/1.1\r\n" +
    "Host: " + cloudServer + "\r\n" +
    "User-Agent: YAOWS32\r\n" +
    "Connection: close\r\n\r\n");

  #if DEBUG
    Serial.print("\tGET ");
    Serial.println(postStr);
  #endif

  while (client.connected())
    {
      while (client.available())
      {
        String line = client.readStringUntil('\n');

        #if DEBUG_CLOUD_VERBOSE
          Serial.println("\t" + line);
        #endif
      }   
    }
    #if DEBUG
      Serial.println(F("\tDisconnected from PWSWeather"));
    #endif
}