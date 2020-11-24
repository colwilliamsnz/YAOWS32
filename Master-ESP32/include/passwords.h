#ifndef PASSWORDS_H
#define PASSWORDS_H

#include <Arduino.h>

// *** WiFi credentials ********************************************************
extern  char    g_wiFiSSID[];
extern  char    g_wiFiPass[];

// *** Cloud service, MQTT specifics *******************************************
extern  char    g_cloudServiceMQTTServer[];
extern  uint16_t     g_cloudServiceMQTTPort;
extern  char    g_cloudServiceMQTTID[];
extern  char    g_cloudServiceMQTTPassword[];

// *** Cloud service, Wunderground specifics ***********************************
extern  char    g_cloudServiceWeatherUndergroundID[];
extern  char    g_cloudServiceWeatherUndergroundKey[];

extern  char    g_cloudServiceWeatherUndergroundID_DEBUG[];
extern  char    g_cloudServiceWeatherUndergroundKey_DEBUG[];

// *** Cloud service, PWSWeather specifics *************************************
extern  char    g_cloudServicePwsWeatherId[];
extern  char    g_cloudServicePwsWeatherPassword[];

extern  char    g_cloudServicePwsWeatherId_DEBUG[];
extern  char    g_cloudServicePwsWeatherPassword_DEBUG[];

// *** Cloud service, WeatherCloud specifics ***********************************
extern  char    g_cloudServiceWeatherCloudId[];
extern  char    g_cloudServiceWeatherCloudKey[];

extern  char    g_cloudServiceWeatherCloudId_DEBUG[];
extern  char    g_cloudServiceWeatherCloudKey_DEBUG[];


// *** Cloud service, ThingSpeak specifics *************************************
extern  char    g_cloudServiceThingSpeakAPIKey[];

extern  char    g_cloudServiceThingSpeakAPIKey_DEBUG[];

#endif