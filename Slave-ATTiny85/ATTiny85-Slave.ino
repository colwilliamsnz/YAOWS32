/*
  ============== YAOWS32 Weather Station Wind/Rain Sensor SLAVE ==================
  Wind speed, Wind gust, and Rain bucket tips are provided as "pulse counts over a
  period". Wind direction is read by an analog pin and the majority direction for
  the sample period calculated. The ESP32 "Master" queries the "Slave", which runs
  as an I2C slave, at regular intervals to derive weather stats.

  The "Slave" sleeps and wakes every 1ms to save power VS running full time (saves
  approx 200uA running in this mode).

  Based on this project: https://github.com/PaulRB/Wemos-Weather-Station

  Chip:             ATTiny85
  Core:             https://github.com/SpenceKonde/ATTinyCore
  Clock:            1MHz internal
  BOD Level:        BOD Disabled
  Save EEPROM:      EEPROM not retained
  Timer 1 clock:    CPU
  LTO:              Enabled
  mills()/micros(): Enabled
  Current:          ~600uA
  ================================================================================
*/
#include <Wire.h>       // ATTinyCore inbuilt optimised wire library for I2C
#include <avr/sleep.h>

// *** I2C address of "this" slave
#define I2C_SLAVE_ADDRESS      0x4

// *** Setup GPIO pins
#define SENSOR_WIND_DIRECTION    A3   // Pin 2    Pin 5 = SDA
#define SENSOR_RAIN_COUNT        4    // Pin 3    Pin 7 = SCL
#define SENSOR_WIND_SPEED        1    // Pin 6    Pin 4 = GND   Pin 8 = VCC

// *** Period that Wind samples will be collected and compared with the previous
// sample to provide a maximum gust for the sample period. Note that changes here
// must be reflected in the ESP32 Master code. 6.0 secs was found to be an ideal
// sample period (shorter tended to be less accurate)
#define WIND_GUST_PERIOD    6000UL    // 6000 = 6.0 secs

// *** Settings for Davis 6410 Anemometer **************************************
// Array "reading" are the readings from the A-to-D converter and translate into
// array "compass" wind directions. Note that the Davis is a linear 20k pot so
// there are more than 8 positions available however this seemed overkill so 8
// directions only are decoded here.
#define COMPASS_DIRECTIONS     8        // For Davis
const int reading[COMPASS_DIRECTIONS] = {75, 219, 340, 455, 605, 750, 888, 1020};
const int compass[COMPASS_DIRECTIONS] = {0, 45, 90, 135, 180, 225, 270, 315};

// *** Settings for Fine Offset Anemometer *************************************
// Ref http://mile-end.co.uk/blog/bits/Weather-Sensor-Assembly.pdf
// #define COMPASS_DIRECTIONS 16        // For Fine Offset
// const int reading[COMPASS_DIRECTIONS] = {147, 172, 208, 281, 368, 437, 531, 624, 710, 780, 817, 870, 909, 938, 970, 1023};
// const int compass[COMPASS_DIRECTIONS] = {112,  67,  90, 157, 135, 202, 180,  22,  45, 247, 225, 337,   0, 292, 315,  270};

byte          slaveRecord[8];               // Slave record array for passing data back to ESP32 Master

unsigned int  windDir               = 0;    // Wind direction in degrees (0 - 360)
unsigned int  windDirADC            = 0;    // Raw ADC readings for wind direction

unsigned long windDirTot[COMPASS_DIRECTIONS];

unsigned int  windSpeedCount        = 0;    // Wind speed pulse counter

unsigned int  rainCount             = 0;    // Rain gauge tip counter

unsigned int  windGustCount         = 0;    // Wind gust pulse counter (current gust sense period)
unsigned int  windGustCountMax      = 0;    // Wind gust counter (current reporting period)

byte          sensorWindSpeedCurr   = LOW;  // Comparison between sensor readings for counters (current VS previous)
byte          sensorWindSpeedPrev   = LOW;
byte          sensorRainCurr        = LOW;
byte          sensorRainPrev        = LOW;

unsigned long timerWindGustLast     = 0; // Timer for measuring wind gust period
unsigned long timerNow              = 0; // Timer for NOW to compare against wind gust period


void setup()
{
  // *** Enable chip wide power-savings
  bitSet(PRR, PRTIM1);              // power-off Timer 1 (only using Timer 0)
  bitSet(PRR, PRADC);               // power-off ADC
  bitClear(ADCSRA, ADEN);           // disable ADC
  set_sleep_mode(SLEEP_MODE_IDLE);

  // *** Setup I2C Bus for Slave mode
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  // *** Setup GPIO pins
  pinMode(SENSOR_WIND_SPEED, INPUT);
  pinMode(SENSOR_RAIN_COUNT, INPUT);
}

void loop()
{
  timerNow = millis(); // Timer to compare with wind gust time


  // *** Wind speed & gust pulse counter ***************************************
  // Note, the reed switch/hall sensor is debounced by the circuit
  pinMode(SENSOR_WIND_SPEED, INPUT_PULLUP);
  sensorWindSpeedCurr = digitalRead(SENSOR_WIND_SPEED);
  pinMode(SENSOR_WIND_SPEED, INPUT);

  if (sensorWindSpeedCurr != sensorWindSpeedPrev)
  {
    if (sensorWindSpeedPrev == HIGH)
    {
      windSpeedCount++;       // increment speed pulse counter
      windGustCount++;        // increment gust pulse counter
    }

    sensorWindSpeedPrev = sensorWindSpeedCurr;

    // The anemometer is moving so get the wind direction
    // https://www.wmo.int/pages/prog/www/IMOP/publications/CIMO-Guide/Prelim_2018_ed/8_I_5_en_MR_clean.pdf
    bitClear(PRR, PRADC);       // Power-on ADC
    bitSet(ADCSRA, ADEN);       // Enable ADC

    windDirADC = analogRead(SENSOR_WIND_DIRECTION);

    bitClear(ADCSRA, ADEN);     // Disable ADC
    bitSet(PRR, PRADC);         // Power-off ADC (power saving approx 150uA)

    // Iterate through the available reading array and where it "matches" reading,
    // increment windDirTot index value. When stats are collected by the master,
    // the highest array index will reflect the reported wind direction (IE, the
    // majority reading for the period--helps to even out wind gusts stalling the
    // wind vane, common in urban environments)
    byte index;
    for (index = 0; index < COMPASS_DIRECTIONS && windDirADC >= reading[index]; index++);
    
    windDirTot[index]++;
    // end of wind direction reading section

  } // end of wind speed pulse counter


  // *** Check for new highest wind gust ***************************************
  unsigned long windGustPeriod = timerNow - timerWindGustLast;

  if (windGustPeriod >= WIND_GUST_PERIOD) {
    timerWindGustLast += WIND_GUST_PERIOD;

    // New highest wind gust?
    if (windGustCount > windGustCountMax) windGustCountMax = windGustCount;

    // Zero gust count for next wind gust reporting period
    windGustCount = 0;
  } // end of check for new highest wind gust section


  // *** Rain sensor pulse counter *********************************************
  // Note, the reed switch is debounced by the circuit
  pinMode(SENSOR_RAIN_COUNT, INPUT_PULLUP);
  sensorRainCurr = digitalRead(SENSOR_RAIN_COUNT);
  pinMode(SENSOR_RAIN_COUNT, INPUT);

  if (sensorRainCurr != sensorRainPrev)
  {
    if (sensorRainPrev == HIGH)
    {
      rainCount++;
    }

    sensorRainPrev = sensorRainCurr;
  }
  // end of rain sensor pulse counter section

  // *** Go to sleep for 1ms (timer interrupt mode) to save power (~200uA) when
  // put to sleep VS running full time
  sleep_enable();
  sleep_mode();
  sleep_disable();
}


// *** Gets called when the ATTiny85 receives an i2c request for data - sends
// collected data over I2C to the Master ESP32 device as an 8 byte array.
void requestEvent()
{
  // *** Calculate most common wind direction over the reporting period by iterating
  // through each array element to find the array index with the highest number of
  // "hits"
  long maxWindDirTot = 0;   // current max value for comparison with each remaining index value
  byte maxWindDirIndex = 0; // array index with the highest samples over the period

  for (byte index = 0; index < COMPASS_DIRECTIONS; index++)
  {
    // Is this index value > current max value (or 0 for the first entry in the loop)
    if (windDirTot[index] > maxWindDirTot)
    {
      maxWindDirTot = windDirTot[index];  // new highest "hit" value
      maxWindDirIndex = index;            // new highest "hit index"
    }
    // Reset this particular wind direction array index value as we're done with
    // this in current reporting period
    windDirTot[index] = 0;
  }

  // *** Get wind direction value in degrees for the reporting period
  windDir = compass[maxWindDirIndex];

  // *** Load sensor readings into byte array for transfer over I2c
  slaveRecord[0] = highByte (windDir);
  slaveRecord[1] = lowByte  (windDir);
  slaveRecord[2] = highByte (windSpeedCount);
  slaveRecord[3] = lowByte  (windSpeedCount);
  slaveRecord[4] = highByte (windGustCountMax);
  slaveRecord[5] = lowByte  (windGustCountMax);
  slaveRecord[6] = highByte (rainCount);
  slaveRecord[7] = lowByte  (rainCount);

  // *** Send each byte to I2C buffer in turn
  for (int i = 0; i < 8; i++) Wire.write(slaveRecord[i]);
}


// *** Get instruction from ESP32 master **************************************
void receiveEvent()
{
  byte command = Wire.read();

  switch (command)
  {
    case 'R': // Reset slave stats
      slaveCommandReset();
      break;
  }
}


// *** Reset slave record and collected data **********************************
void slaveCommandReset()
{
  memset(slaveRecord, 0x00, 8); // Fill array with 0's

  windDir          = 0;
  windSpeedCount   = 0;
  windGustCount    = 0;
  windGustCountMax = 0;
  rainCount        = 0;
}
