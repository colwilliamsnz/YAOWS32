#ifndef PASSWORDS_H
#define PASSWORDS_H

extern  char    g_wiFiSSID[];
extern  char    g_wiFiPass[];

// *** Cloud service, Wunderground specifics **********************************
extern  char    g_cloudServiceWeatherUndergroundLogin[];
extern  char    g_cloudServiceWeatherUndergroundPass[];

// *** Cloud service, PWSWeather specifics ************************************
extern  char    g_cloudServicePwsWeatherId[];
extern  char    g_cloudServicePwsWeatherPassword[];

extern  char    g_cloudServicePwsWeatherId_DEBUG[];
extern  char    g_cloudServicePwsWeatherPassword_DEBUG[];

// *** Cloud service, WeatherCloud specifics **********************************
extern  char    g_cloudServiceWeatherCloudId[];
extern  char    g_cloudServiceWeatherCloudKey[];

extern  char    g_cloudServiceWeatherCloudId_DEBUG[];
extern  char    g_cloudServiceWeatherCloudKey_DEBUG[];


// *** Cloud service, ThingSpeak specifics ************************************
#define DEBUG_CLOUD_THINGSPEAK      1
extern  char    g_cloudServiceThingSpeakAPIKey[];

#endif