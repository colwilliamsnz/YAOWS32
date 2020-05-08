#ifndef STATION_H
#define STATION_H

// *** Weather station specifics **********************************************
extern  const   int             g_stationUpdatePeriod;

extern  const   float           g_stationAltitude;
extern  const   float           g_stationPressureCal;

extern  const   float           g_stationWindSpeedFactor;
extern  const   int             g_stationWindVaneOffset;
extern  const   float           g_stationWindGustPeriod;
extern  const   float           g_stationRainTipVolume;

extern  const   char            g_stationDailyResetTime[];

extern  const   char            g_ntpServer1[];
extern  const   char            g_ntpServer2[];
extern  const   long            g_ntpOffset;
extern  const   int             g_ntpOffsetDST;
extern  const   int             g_ntpCheckInterval;

#endif