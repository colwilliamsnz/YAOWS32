#include "station.h"

// === Weather station specifics ===============================================

// Time in minutes between wakeup/update events (= time ESP32 sleeps). Should be
// an even divisor of 60 minutes (eg 5, 10, 15 minutes) however the more regular
// the wake update period, the quicker a battery powered system will drain.
constexpr   int             g_stationUpdatePeriod       = 1;

// time in 24 hour format that weather stats needs to be reset (default = 
// midnight = "00:00")
constexpr   char            g_stationDailyResetTime[]   = "00:00"; 

// Altitude of the station in metres. Used to calculate relative pressure from 
// pressure sensor which gives ABS
constexpr   float           g_stationAltitude           = 43.37;

// If the altitude is not known, enter the nearest stations relative pressure in
// hPa and view the debug console to get  an estimate of the station altitude to
// enter into g_stationAltitude above
constexpr   float           g_stationPressureCal        = 1015.60;

// Wind speed factor - see below anemometer details for an explanation. The
// station is native metric so this value should be set to a factor represending
// kilometers per hour.
constexpr   float           g_stationWindSpeedFactor    = 3.621;

// Rain bucket volume - the amount of rain that has fallen when the rain sensor
// bucket tips once. Measured in mm.
constexpr   float           g_stationRainTipVolume      = 0.2;

/* Anemometer "WindSpeedFactor" explainer...
For Davis 6410
ref https://www.davisinstruments.com/product_documents/weather/spec_sheets/6410_SS.pdf
V = P(2.250/T) where:
    V = Velocity in mph
    P = number of pulses per sample period
    T = sample period in seconds.
V = P(3.621/T) where
    V = Velocity in kph
    P = number of pulses per sample period
    T = sample period in seconds.
    OR put more succinctly, one revolution of the wind cups per second = 2.25
    mph (3.621 kph) of wind velocity.

For Fine Offset
ref http://mile-end.co.uk/blog/bits/Weather-Sensor-Assembly.pdf
V = P(1.492/T) where:
    V = Velocity in mph
    P = number of pulses per sample period
    T = sample period in seconds.
V = P(2.500/T) where:
    V = Velocity in kph
    P = number of pulses per sample period
    T = sample period in seconds.
    or put more succinctly, one revolution of the wind cups per second = 1.492
    mph (2.5 kph) of wind velocity.
*/

// Wind gust period in seconds. This is the sample time for measuring wind gusts
// Note that if this value is changed, the SLAVE ATTiny84 code must be updated!
// Whole values are best and sample time of 6s found to be ideal in testing with
// a signal generator to simulate the pulses at various frequencies.
constexpr   float           g_stationWindGustPeriod     = 6.0F;

// Wind vane offset in +- degrees from true north - use where the wind vane
// cannot be installed/adjusted physically to point true north
constexpr   int             g_stationWindVaneOffset     = 0;


// === NTP server settings for internet time ===================================
constexpr   char            g_ntpServer1[]              = "nz.pool.ntp.org";
constexpr   char            g_ntpServer2[]              = "pool.ntp.org";

// timezone offset in += seconds (43200 = +12 hours)
constexpr   long            g_ntpOffset                 = 43200;

// daylight savings time offset in +- seconds (3600 = +1 hour)
constexpr   int             g_ntpOffsetDST              = 0;

// minutes between RTC updates from NTP time server
constexpr   int             g_ntpCheckInterval          = 100;