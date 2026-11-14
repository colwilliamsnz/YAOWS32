#ifndef MAINTENANCE_H
#define MAINTENANCE_H

/*
 * Maintenance window: a short period after power-on / EN-button reset during
 * which the station keeps WiFi up and serves:
 *
 *   http://<hostname>.local/            status page (countdown, battery, links)
 *   http://<hostname>.local/update      ElegantOTA firmware upload
 *   http://<hostname>.local/log         console log (everything Serial printed)
 *
 * All pages use the g_otaUser / g_otaPass login (passwords.cpp).
 */

// True if this boot should open the window (reset reason + battery check)
bool maintenanceShouldEnter();

// Start web server, mDNS and ElegantOTA. WiFi must be connected.
void maintenanceBegin();

// Serve until the window expires (or "sleep" command). cycleFn is run when the
// "cycle" console command / button is used.
void maintenanceRun(void (*cycleFn)());

// Stop web server and mDNS (call before killWiFi / sleep)
void maintenanceEnd();

// Mark a freshly OTA'd firmware as good (cancels automatic rollback). Safe to
// call every cycle - only acts when the running image is pending verification.
void markAppValidIfPending();

#endif
