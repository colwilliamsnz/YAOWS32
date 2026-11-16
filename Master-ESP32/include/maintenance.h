#ifndef MAINTENANCE_H
#define MAINTENANCE_H

// *** Maintenance window (OTA firmware upload + web log) **********************
// A short period after power-on / EN button reset (or the first boot after a
// firmware upload) during which the station keeps WiFi up and serves:
//
//   http://<hostname>.local/          status page (countdown, battery, links)
//   http://<hostname>.local/update    ElegantOTA firmware upload
//   http://<hostname>.local/log       console log (everything Log printed)
//
// All pages use the g_otaUser / g_otaPass login (passwords.cpp).
//
// Used from setup() in main.cpp:
//
//   if (maintenanceShouldEnter())
//   {
//     maintenanceBegin();
//     runCycle();
//     maintenanceRun(runCycle);
//     maintenanceEnd();
//   }
// *****************************************************************************

bool    maintenanceShouldEnter();             // should this boot open the window?
void    maintenanceBegin();                   // start web server etc (WiFi must be up)
void    maintenanceRun(void (*cycleFn)());    // serve until the window closes
void    maintenanceEnd();                     // stop web server (before killWiFi)

void    markAppValidIfPending();              // new firmware is good - cancel rollback

#endif
