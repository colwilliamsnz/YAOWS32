#ifndef BUILDPROFILE_H
#define BUILDPROFILE_H

// *** Build profile ***********************************************************
// Selected by the PlatformIO environment (see platformio.ini):
//   pio run -e station   -> TEST_RIG undefined/0 (real, mast mounted station)
//   pio run -e testrig   -> -DTEST_RIG=1         (desktop test board)
//
// Everything that differs between the two lives here so main.cpp etc never
// need to be edited (or calls commented out) when switching boards.
// *****************************************************************************

#ifndef TEST_RIG
  #define TEST_RIG 0
#endif


// *** Test rig ****************************************************************
#if TEST_RIG
  // Test rig always gets debug output and the fast wake cycle. These are
  // defined before debug.h is processed so they win over debug.h values.
  #ifndef DEBUG
    #define DEBUG 1
  #endif
  #ifndef DEBUG_FAST
    #define DEBUG_FAST 1
  #endif

  // Cloud "dry-run": payloads are built and printed but never sent
  #ifndef CLOUD_DRY_RUN
    #define CLOUD_DRY_RUN 1
  #endif

  #define DEVICE_HOSTNAME   "yaows32-test"   // http://yaows32-test.local
  #define DEVICE_PROFILE    "TEST RIG"       // badge on the status page


// *** Station *****************************************************************
#else
  #ifndef CLOUD_DRY_RUN
    #define CLOUD_DRY_RUN 0
  #endif

  #define DEVICE_HOSTNAME   "yaows32"        // http://yaows32.local
  #define DEVICE_PROFILE    "STATION"        // badge on the status page
#endif

#endif
