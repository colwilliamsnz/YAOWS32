#ifndef MATH_H
#define MATH_H

#include <Arduino.h>

float   mm_to_inches(float mm);

float   C_to_F(float tempC);

float   F_to_C(float tempF);

float   hPa_to_inHg(float hPa);

float   kph_to_ms(float kph);

float   kph_to_mph(float kph);

float   calcDewPointC(float tempC, float humidity);

float   calcHeatIndexC(float tempC, float humidity);

float   calcWindChillC(float tempC, float windKpH);

float   calcWindSpeedHeightComp(float height, float windKpH);

#endif