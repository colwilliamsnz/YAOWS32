#include <Arduino.h>


/**
 * @brief Convert mm to inches (for rain)
 */
float mm_to_inches(float mm)
{
  return (mm / 25.4);
}


/**
 * @brief Convert temperature degrees C to F
 */
float C_to_F(float tempC)
{
  return (tempC * 9.0) / 5.0 + 32.0;
}


/**
 * @brief Convert temperature degrees F to C
 */
float F_to_C(float tempF)
{
  return (tempF - 32.0) * 5.0 / 9.0;
}


/**
 * @brief Convert pressure hPa to inHg
 */
float hPa_to_inHg(float hPa)
{
  return hPa * 0.02953;
}


/**
 * @brief Convert speed in kph to ms (meters per second)
 * 
 * @param speedKpH
 * @return float 
 */
float kph_to_ms(float kph)
{
  return kph / 3.6;
}

/**
 * @brief Convert speed in kph to mph (miles per hour)
 * 
 * @param speed in kilometers per hour (kph)
 * @return float 
 */
float kph_to_mph(float kph)
{
  return kph / 1.60934;
}


/**
 * @brief Calculate the Dewpoint in degrees C from TempC and Humidity
 */
float calcDewPointC(float tempC, float humidity)
{
/*
Dewpoint is the temperature to which air must be cooled for saturation (100% relative
humidity) to occur, providing there is no change in water content. The dewpoint is an
important measurement used to predict the formation of dew, frost, and fog.

This equation is an approximation of the Goff & Gratch equation, which is extremely
complex. This equation is one recommended by the World Meteorological Organization
for saturation of air with respect to water.
*/  
  float k = log(humidity / 100) + (17.62 * tempC) / (243.12 + tempC);

  return 243.12 * k / (17.62 - k);
} // End calcDewPointC


/**
 * @brief   Calculates Wind Chill in Temp C
 * 
 * @param   tempC
 * @param   windKpH
 * 
 * @return  float 
 */
float calcWindChillC(float tempC, float windKpH)
{
  /*
  Wind chill takes into account how the speed of the wind affects our perception of the air
  temperature. Our bodies warm the surrounding air molecules by transferring heat from the
  skin. If there’s no air movement, this insulating layer of warm air molecules stays next
  to the body and offers some protection from cooler air molecules. However, wind sweeps
  that comfy warm air surrounding the body away. The faster the wind blows, the faster heat
  is carried away and the colder the environment feels.
  */
  float windChillC;

  if (windKpH > 0) // we only calculate wind chill if there is wind
  {
    windChillC = 13.12 + (0.6215 * tempC) - (11.37 * pow(windKpH, 0.16)) + (0.3965 * tempC * pow(windKpH, 0.16));
  }
  else
  {
    windChillC = tempC;
  }

  // in all cases if calculated windchill > air temp, we return the air temp (this always
  // occurs at wind speeds of 0 kph OR temperatures > 24 deg C (76 deg F))
  if (windChillC > tempC)
  {
    windChillC = tempC;
  }

  return windChillC;

} // End calcWindChillC


/**
 * @brief   Calculate Heat Index in deg C
 * 
 * @param   temperature temperature in C
 * @param   percentHumidity humidity in percent
 * 
 * @return  float heat index
 */
float calcHeatIndexC(float temperature, float percentHumidity)
 {
/*
  Heat Index uses temperature and relative humidity to determine how hot the air actually
  “feels.” When humidity is low, the apparent temperature will be lower than the air
  temperature, since perspiration evaporates rapidly to cool the body. However, when
  humidity is high (i.e., the air is saturated with water vapor) the apparent temperature
  “feels” higher than the actual air temperature, because perspiration evaporates more slowly.

  Using both Rothfusz and Steadman's equations:
  http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
 */ 
  float hi;

  temperature = C_to_F(temperature);   // formula requires F native temp

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79) 
  {
    hi = -42.379 + 2.04901523 * temperature + 10.14333127 * percentHumidity +
         -0.22475541 * temperature * percentHumidity +
         -0.00683783 * pow(temperature, 2) +
         -0.05481717 * pow(percentHumidity, 2) +
         0.00122874 * pow(temperature, 2) * percentHumidity +
         0.00085282 * temperature * pow(percentHumidity, 2) +
         -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if ((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if ((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  return (F_to_C(hi));
} // end calcHeatIndexC


float calcWindSpeedHeightComp(float height, float windKpH)
{
/* Recalculate wind speed taking into account height compensation factor (for
    situations where the anemometer can't be located at the regulation 10m AGL
    (above ground level). Compensates for "air friction over land"
    Ref https://cumuluswiki.org/a/Wind_measurement
  
  Compensation factor = 1/(0.233 + 0.656 * log10(h + 4.75))
    where h = height of anemometer in meters above ground level
*/
 
float factor = 1/(0.233 + 0.656 * log10(height + 4.75));

return windKpH * factor;
}