// *** Echo data to serial port - set to 0 to stop all serial ******************
#define DEBUG 1

#define VERSION "v0.9.2"

/*** Version log ***************************************************************

0.9.2 Update Cloud posting routine for WeatherCloud.
- Added DEBUG_CLOUD_WEATHERCLOUD compiler flag so an alternative ID and Key can
    be configured for a secondary "debug" station. Stops live weather data from
    being polluted during testing.
    
- Added serial debug/display of response codes from WeatherCloud.

0.9.1 - Initial release to GitHub.

*/