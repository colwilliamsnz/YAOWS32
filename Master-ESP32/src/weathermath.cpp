#include <Arduino.h>

/*
Convert temperature degrees C to F
*/
float convertTempCtoF(float tempC)
{
  return (tempC * 9.0) / 5.0 + 32.0;
}


/*
Convert temperature degrees F to C
*/
float convertTempFtoC(float tempF)
{
  return (tempF - 32.0) * 5.0 / 9.0;
}


/*
Convert pressure hPa to inHg
*/
float convertPressureHPAtoINHG(float pressure_hPa)
{
  return pressure_hPa * 0.02953;
}


/**
 * @brief Convert speed in kph to mps (meters per second)
 * 
 * @param speedKpH 
 * @return float 
 */
float convertKpHtoMpS(float speedKpH)
{
  return speedKpH / 3.6;
}

/**
 * @brief Convert speed in kph to mph (miles per hour)
 * 
 * @param speedKpH 
 * @return float 
 */
float convertKpHtoMpH(float speedKpH)
{
  return speedKpH / 1.60934;
}


/*
Calculate the Dewpoint in degrees C from TempC and Humidity
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

Ref Davis Instruments support document "Derived variables in Davis Weather Products"
https://www.davisinstruments.com/support/derived-variables-in-davis-weather-products/
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
  
  Ref Davis Instruments support document "Derived variables in Davis Weather Products"
  https://www.davisinstruments.com/support/derived-variables-in-davis-weather-products/
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
 * @brief   Compute Heat Index in deg C
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

  temperature = convertTempCtoF(temperature);   // formula requires F native temp

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

  return (convertTempFtoC(hi));
} // end calcHeatIndexC