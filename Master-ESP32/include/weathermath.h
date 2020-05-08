#ifndef MATH_H
#define MATH_H

#include <Arduino.h>

float   calcDewPointC(float tempC, float humidity);

float   calcHeatIndexC(float tempC, float humidity);

float   calcWindChillC(float tempC, float windKpH);

float   convertTempCtoF(float tempC);
float   convertTempFtoC(float tempF);

float   convertPressureHPAtoINHG(float pressure_hPa);

float   convertKpHtoMpS(float speedKpH);

float   convertKpHtoMpH(float speedKpH);

#endif