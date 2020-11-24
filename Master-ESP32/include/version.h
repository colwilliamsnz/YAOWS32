#ifndef VERSION_H
#define VERSION_H

#define VERSION "1.0.0"

/*** Version log ***************************************************************

1.0.0 - Misc updates and improvements

- Add additional serial debug text to resetDay function.

- Add verbose debug mode for cloud posting functions. If set to 1, responses
    from web service REST API are printed to the serial console.

- Removed all functions and references to the DS3121 sensor (power consumption
    too high to include in the design--the ESP32 RTC is good enough when kept
    updated with regular NTP updates).

- Add wind vane height configuration and calculation/factor into wind speed
    measurements (to counter effect of air friction over land--where wind vane
    height < regulation 10 meters).

- Updated WiFi connection function to sleep for the reporting period interval
    then restart if it can't connect.

- Added Time Zone global to drive TZ in getNTP function (replaces original
    simplistic scheme that used offsets in seconds).

- Added new global that represents rain for reporting period (current variable
    is for the entire day). WeatherUnderground and PWSWeather require this
    distinction. IE, g_weatherRainFall VS g_weatherRainFallDaily.

- Renamed conversion functions (temp C to F, etc) to be more clean & concise.

- Move debug compiler directives from "version.h" into "debug.h" so that Github
    doesn't populate changes made during testing (debug.h added to
    .gitignore).

- Update Cloud posting routines variable names for ids/keys etc to match the
    associated website terminology.

- Refactor Cloud posting routines for WeatherCloud, ThingSpeak, PWSWeather &
    WeatherUnderground to include logging to serial console, all return codes.

- Cloud service update functions: Added DEBUG_CLOUD_XXX compiler flags so an
    alternative ID and Key can be configured for a secondary "debug" station.
    Stops live weather data from being polluted during testing.

- Added/updated comments to better document each cloud service REST API/usage.

- Added reporting of WiFi RSSI to ThinkSpeak as well as inclusion of basic
    weather condition measurements (temperature, humidity, rain & pressure).

- Added compiler directive, DEBUG_FAST so that debugging can optionally be done
    at a faster interval than the default wakeup of 10 minutes. Setting to 1
    sets the wake interval to 2 minutes. Defined in "station.cpp"


0.9.1 - Initial release to GitHub.


*** Note on version numbering **************************************************

Major.Minor.Patch

Where:

- Major. Start at 0. When code is considered feature complete, we reach version
    1.0.0. Increment when substantial changes are made to the codebase. When
    incremented, set Minor and Patch to 0 respectively.

- Minor. Increment when a release contains new features (with or without bug
    fixes). Values > 9 are valid. When incremented, set Patch to 0.

- Patch. Increment when a release contains bug fixes. Values > 9 are valid.

*/

#endif