/*
  =========================== Maintenance window ==============================
  On power-on / EN button reset, and on the first boot after a firmware
  upload, the station stays awake with WiFi on for a few minutes and serves:

    http://<hostname>.local/          status page (countdown, battery, buttons)
    http://<hostname>.local/update    ElegantOTA firmware upload
    http://<hostname>.local/log       console log (everything Log printed)

  All pages use the g_otaUser / g_otaPass login (passwords.cpp).

  Normal measure/upload cycles keep running on the usual schedule while the
  window is open. The window closes when its countdown reaches zero (it is
  extended while the status page is open or an upload is running) or when
  "Sleep now" is pressed, after which normal low power operation resumes.

  Naming: s_ = private to this file (static), g_ = shared project global.
  ============================================================================
*/
#include "maintenance.h"

#include "buildprofile.h"   // station vs test rig build profile
#include "debug.h"          // debug compiler directives
#include "version.h"        // version number

#include "functions.h"      // project specific functions
#include "globals.h"        // shared globals (sensor readings etc)
#include "passwords.h"      // OTA / web login
#include "station.h"        // maintenance window timings
#include "log.h"            // Serial + web log

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>        // http://<hostname>.local name
#include <esp_ota_ops.h>    // firmware rollback state
#include <Preferences.h>    // NVS (flash) storage for the "just updated" flag

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>


// *** NVS (flash) flag set just before the post-OTA reboot ********************
static const char         PREFS_NS[]        = "maint";
static const char         PREFS_OTA_FLAG[]  = "otaJustDone";


// *** Web server **************************************************************
static AsyncWebServer     s_server(80);


// *** State shared between the async web task and the main task ***************
// The web server runs in its own task. Its callbacks ONLY set these flags and
// values - all printing and acting on them happens in the main task (in
// maintenanceRun) so nothing is done from two tasks at once.

// Firmware upload (set by ElegantOTA callbacks)
static volatile bool      s_otaActive       = false;  // upload in progress
static volatile bool      s_otaEnded        = false;  // upload just finished
static volatile bool      s_otaSuccess      = false;  // ...and whether it worked
static volatile size_t    s_otaCurrent      = 0;      // bytes received so far
static volatile size_t    s_otaTotal        = 0;      // total bytes expected

// Button requests from the status page (/api/cmd)
static volatile bool      s_reqExtend       = false;
static volatile bool      s_reqSleep        = false;
static volatile bool      s_reqCycle        = false;
static volatile bool      s_reqReboot       = false;

// millis() of the last status/log page request - used to keep the window open
// while someone has the status page (or auto-refreshing log page) open
static volatile unsigned long s_lastClientMs = 0;


// *** Window timing ***********************************************************
static unsigned long          s_windowStart = 0;  // millis() when window opened
static volatile unsigned long s_deadline    = 0;  // millis() when window closes
static volatile unsigned long s_nextCycle   = 0;  // millis() of next scheduled cycle
static bool                   s_started     = false;

static const unsigned long    EXTEND_MS       = 5UL * 60UL * 1000UL;  // "Extend" button adds 5 min
static const unsigned long    CLIENT_HOLD_MS  = 30UL * 1000UL;        // extend by this...
static const unsigned long    CLIENT_SEEN_MS  = 10UL * 1000UL;        // ...while a page polled this recently


// *** Helper functions ********************************************************

/**
 * @brief millis() value at which the window must close regardless of
 * extensions (g_maintMaxSecs after it opened). An upload in progress is the
 * only thing allowed to go past it.
 */
static unsigned long hardStopMillis()
{
  return s_windowStart + (unsigned long)g_maintMaxSecs * 1000UL;
} // end function hardStopMillis ***********************************************


/**
 * @brief Seconds until the window closes (0 if already due)
 */
static long secondsRemaining()
{
  long remainingMs = (long)(s_deadline - millis());

  return remainingMs > 0 ? remainingMs / 1000 : 0;
} // end function secondsRemaining *********************************************


/**
 * @brief Seconds until the next scheduled measure/upload cycle (0 if due)
 */
static long secondsToNextCycle()
{
  long remainingMs = (long)(s_nextCycle - millis());

  return remainingMs > 0 ? remainingMs / 1000 : 0;
} // end function secondsToNextCycle *******************************************


/**
 * @brief Schedule the next cycle on the same clock-aligned boundary the station
 * would wake at from sleep (eg 8:10, 8:20...) so the window doesn't upset the
 * normal reporting schedule.
 */
static void scheduleNextCycle()
{
  struct tm timeWakeup;

  double offsetSeconds = secondsUntilNextWake(&timeWakeup);

  if (offsetSeconds < 1) offsetSeconds = 1;

  s_nextCycle = millis() + (unsigned long)(offsetSeconds * 1000.0);

  #if DEBUG
    Log.println(&timeWakeup, "[maint] Next scheduled cycle at %H:%M:%S");
  #endif
} // end function scheduleNextCycle ********************************************


/**
 * @brief Move the window's closing time out to "target" - never past the hard
 * stop (g_maintMaxSecs) and never earlier than it already is.
 */
static void extendDeadlineTo(unsigned long target)
{
  unsigned long hardStop = hardStopMillis();

  // (signed compares so millis() rollover is handled)
  if ((long)(target - hardStop) > 0) target = hardStop;
  if ((long)(target - s_deadline) > 0) s_deadline = target;
} // end function extendDeadlineTo *********************************************


/**
 * @brief Human readable ESP32 reset reason for the log and status page
 */
static const char *resetReasonStr(esp_reset_reason_t resetReason)
{
  switch (resetReason)
  {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_EXT:       return "external pin";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "panic/exception";
    case ESP_RST_INT_WDT:   return "interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "task watchdog";
    case ESP_RST_WDT:       return "other watchdog";
    case ESP_RST_DEEPSLEEP: return "deep sleep";
    case ESP_RST_BROWNOUT:  return "brown-out";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "unknown";
  }
} // end function resetReasonStr ***********************************************


/**
 * @brief Format a reading as a JSON number. Sensor readings can be NaN (eg a
 * sensor not fitted) and "nan" is not valid JSON, so NaN becomes null (shown
 * as "—" on the status page).
 */
static void jsonNum(char *out, size_t len, float value, int decimals)
{
  if (isfinite(value)) snprintf(out, len, "%.*f", decimals, value);
  else                 snprintf(out, len, "null");
} // end function jsonNum ******************************************************


/**
 * @brief Copy a string for use inside JSON quotes - escapes " and \ and drops
 * control characters (a WiFi SSID can contain almost anything).
 */
static void jsonStr(char *out, size_t len, const char *in)
{
  size_t outLen = 0;

  for (; *in && outLen + 2 < len; in++)
  {
    if (*in == '"' || *in == '\\') out[outLen++] = '\\';
    if ((unsigned char)*in >= 0x20) out[outLen++] = *in;
  }

  out[outLen] = 0;
} // end function jsonStr ******************************************************


/**
 * @brief Firmware build date/time as "YY-MM-DD HH:MM" for the status page
 * header. Built from the compiler's __DATE__ ("Nov 14 2026") and __TIME__
 * ("15:11:22") the first time it's called.
 */
static const char *buildStamp()
{
  static char stamp[16] = "";

  if (stamp[0]) return stamp; // already built

  static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

  const char *date  = __DATE__;
  const char *found = strstr(months, String(date).substring(0, 3).c_str());

  int month = found ? (int)(found - months) / 3 + 1 : 0;
  int day   = atoi(date + 4);
  int year  = atoi(date + 7) % 100;

  snprintf(stamp, sizeof(stamp), "%02d-%02d-%02d %.5s", year, month, day, __TIME__);

  return stamp;
} // end function buildStamp ***************************************************


/**
 * @brief Check the browser's login (g_otaUser / g_otaPass). If it's missing or
 * wrong, ask the browser for it and return false.
 */
static bool checkAuth(AsyncWebServerRequest *request)
{
  if (request->authenticate(g_otaUser, g_otaPass)) return true;

  request->requestAuthentication();

  return false;
} // end function checkAuth ****************************************************


// *** Status page (http://<hostname>.local/) **********************************
// Countdown, battery/WiFi/temperature tiles and the Log / Firmware update /
// Extend / Run cycle now / Sleep now buttons. Polls /api/status every 2 secs.
static const char STATUS_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>YAOWS32</title>
<style>
:root{--bg:#f4f6f8;--card:#fff;--fg:#1d2733;--mut:#6b7785;--line:#e3e8ee;--acc:#2f6fed;--ok:#1f9d55;--warn:#d97706;--bad:#dc2626}
@media (prefers-color-scheme:dark){:root{--bg:#0f141a;--card:#18202a;--fg:#e6edf3;--mut:#8b98a7;--line:#2a3542;--acc:#5b8dff}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
.wrap{max-width:720px;margin:0 auto;padding:24px 16px}
header{display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-bottom:16px}
h1{font-size:22px;margin:0}.badge{font-size:12px;font-weight:600;padding:3px 8px;border-radius:999px;background:var(--acc);color:#fff}
.badge.test{background:var(--warn)}.mut{color:var(--mut)}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:16px}
.count{font-size:40px;font-weight:700;font-variant-numeric:tabular-nums}
.bar{height:6px;background:var(--line);border-radius:3px;overflow:hidden;margin-top:8px}.bar>div{height:100%;background:var(--acc);transition:width 1s linear}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px}
.k{font-size:12px;color:var(--mut);text-transform:uppercase;letter-spacing:.04em}.v{font-size:17px;font-weight:600;font-variant-numeric:tabular-nums}
.act{display:grid;grid-template-columns:170px 1fr;gap:14px;align-items:center;padding:10px 0;border-top:1px solid var(--line)}
.act:first-child{border-top:0;padding-top:0}.act:last-child{padding-bottom:0}
.act p{margin:0;color:var(--mut);font-size:14px}.act .btn,.act button{width:100%;text-align:center}
code{font-size:13px;background:var(--bg);padding:1px 5px;border-radius:4px}
@media (max-width:520px){.act{grid-template-columns:1fr;gap:6px}}
a.btn,button{appearance:none;border:1px solid var(--line);background:var(--card);color:var(--fg);padding:10px 16px;border-radius:8px;font:inherit;font-weight:600;cursor:pointer;text-decoration:none}
a.btn.pri{background:var(--acc);border-color:var(--acc);color:#fff}button.danger{color:var(--bad)}
#ota{display:none;color:var(--warn);font-weight:600}
</style></head><body><div class="wrap">
<header><h1>YAOWS32</h1><span id="prof" class="badge"></span><span class="mut" id="ver"></span></header>
<div class="card"><div class="k">Maintenance window closes in</div>
<div class="count" id="cd">--:--</div><div class="bar"><div id="bar" style="width:100%"></div></div>
<div id="ota">Firmware upload in progress - window held open</div></div>
<div class="card acts">
<div class="act"><a class="btn pri" href="/log" target="_blank">Log</a>
<p>Opens the station's console output in a new tab: everything it has printed since boot (most recent 16&nbsp;KB), exactly as on a USB serial monitor. Refresh it to see new output, or turn on auto-refresh. It won't jump while you're scrolled up.</p></div>
<div class="act"><a class="btn pri" href="/update" target="_blank">Firmware update</a>
<p>Upload a compiled <code>firmware.bin</code> (<code>.pio/build/&lt;env&gt;/</code>). The station reboots into it and this window reopens, so you can watch the new firmware start. Refresh this page after about 30 seconds. If the new firmware fails before completing a cycle with WiFi, it rolls back to the current version automatically.</p></div>
<div class="act"><button onclick="cmd('extend')">Extend +5 min</button>
<p>Adds 5 minutes to the countdown (up to the configured maximum).</p></div>
<div class="act"><button onclick="cmd('cycle')">Run cycle now</button>
<p>Runs an extra cycle immediately: reads all sensors and posts to the cloud services (a dry-run on the test rig). Cycles also run automatically every <span id="per">N</span> minutes on the normal schedule while this window is open. Note: an extra cycle resets the wind and rain counters for the current period.</p></div>
<div class="act"><button class="danger" onclick="if(confirm('Close the window and go to sleep?'))cmd('sleep')">Sleep now</button>
<p>Closes the window now. WiFi is switched off and the station returns to its normal wake, measure, upload and sleep schedule. This page will stop responding.</p></div>
</div>
<div class="card grid" id="g"></div>
<p class="mut">While this page (or the log page with auto-refresh) is open, the window is extended automatically, up to the configured maximum.</p>
</div><script>
let max=1;const f=s=>String(Math.floor(s/60)).padStart(2,'0')+':'+String(s%60).padStart(2,'0');
function cell(k,v){return '<div><div class="k">'+k+'</div><div class="v">'+v+'</div></div>'}
const esc=s=>String(s||'—').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const n=(v,d,u)=>v==null?'—':v.toFixed(d)+' '+u;
async function poll(){let r;try{r=await fetch('/api/status')}catch(e){cd.textContent='offline';return}
if(!r.ok){cd.textContent='error '+r.status;return}
try{const j=await r.json();
prof.textContent=j.profile;prof.className='badge'+(j.testrig?' test':'');ver.textContent='v'+j.version+' · '+j.built+' · '+j.hostname;
cd.textContent=f(j.remaining);max=Math.max(max,j.remaining);bar.style.width=(100*j.remaining/max)+'%';
ota.style.display=j.ota?'block':'none';
per.textContent=j.period;
g.innerHTML=cell('Battery',n(j.batt,2,'V'))+cell('Current',n(j.curr,1,'mA'))+cell('WiFi network',esc(j.ssid))+cell('WiFi RSSI',j.rssi+' dBm')+
cell('IP',j.ip)+cell('Uptime',f(j.uptime))+cell('Reset reason',j.reset)+cell('Air temp',n(j.temp,1,'°C'));
}catch(e){cd.textContent='bad status data';console.error(e)}}
async function cmd(c){await fetch('/api/cmd?c='+c,{method:'POST'});poll()}
poll();setInterval(poll,2000);
</script></body></html>)HTML";


// *** Log page (http://<hostname>.local/log) **********************************
// Shows the Log history (/api/log) as plain text. Updates on Refresh or the
// optional 5 sec auto-refresh, and only scrolls to the end if already there.
static const char LOG_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>YAOWS32 log</title>
<style>
:root{--bg:#f4f6f8;--card:#fff;--fg:#1d2733;--mut:#6b7785;--line:#e3e8ee;--acc:#2f6fed}
@media (prefers-color-scheme:dark){:root{--bg:#0f141a;--card:#18202a;--fg:#e6edf3;--mut:#8b98a7;--line:#2a3542;--acc:#5b8dff}}
*{box-sizing:border-box}html,body{height:100%}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;display:flex;flex-direction:column}
.bar{display:flex;align-items:center;gap:10px;flex-wrap:wrap;padding:12px 16px;border-bottom:1px solid var(--line);background:var(--card)}
h1{font-size:18px;margin:0 8px 0 0}.mut{color:var(--mut);font-size:13px}.sp{flex:1}
a.btn,button{appearance:none;border:1px solid var(--line);background:var(--card);color:var(--fg);padding:7px 12px;border-radius:8px;font:inherit;font-size:14px;font-weight:600;cursor:pointer;text-decoration:none}
button.pri{background:var(--acc);border-color:var(--acc);color:#fff}
label{display:flex;align-items:center;gap:6px;font-size:14px;cursor:pointer}
pre{flex:1;margin:0;padding:12px 16px;overflow:auto;white-space:pre;tab-size:8;-moz-tab-size:8;font:12.5px/1.45 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}
</style></head><body>
<div class="bar"><h1>YAOWS32 log</h1>
<button class="pri" onclick="load()">Refresh</button>
<label><input type="checkbox" id="auto" onchange="setAuto()">Auto-refresh (5 s)</label>
<button onclick="log.scrollTop=log.scrollHeight">Jump to end</button>
<span class="sp"></span><span class="mut" id="st">loading…</span>
<a class="btn" href="/">← Status</a></div>
<pre id="log"></pre>
<script>
let first=true,timer=null;
async function load(){
const atEnd=log.scrollHeight-log.scrollTop-log.clientHeight<40,top=log.scrollTop;
let r;try{r=await fetch('/api/log')}catch(e){st.textContent='offline';return}
if(!r.ok){st.textContent='error '+r.status;return}
const t=await r.text();log.textContent=t;
log.scrollTop=(first||atEnd)?log.scrollHeight:top;first=false;
st.textContent='updated '+new Date().toLocaleTimeString([],{hour12:false})+' · '+(t.split('\n').length-1)+' lines';}
function setAuto(){clearInterval(timer);timer=auto.checked?setInterval(load,5000):null}
load();
</script></body></html>)HTML";


// *** Status page buttons *****************************************************

/**
 * @brief Turn a button command from the status page (/api/cmd?c=...) into a
 * request flag for the main task to act on. Runs in the web server task, so
 * it only sets a flag.
 */
static void queueCommand(const char *cmd, size_t len)
{
  if      (len == 6 && !strncmp(cmd, "extend", 6))  s_reqExtend  = true;
  else if (len == 5 && !strncmp(cmd, "sleep", 5))   s_reqSleep   = true;
  else if (len == 5 && !strncmp(cmd, "cycle", 5))   s_reqCycle   = true;
  else if (len == 6 && !strncmp(cmd, "reboot", 6))  s_reqReboot  = true;
} // end function queueCommand *************************************************


// *** Public functions (see maintenance.h) ************************************

/**
 * @brief Decide whether this boot should open the maintenance window (web
 * status page, firmware upload and log). The window keeps WiFi on for a few
 * minutes (~120-170mA) so it is only opened when someone is likely to be there
 * to use it:
 *   - power-on or EN button reset
 *   - first boot after a firmware upload (so the new firmware can be watched)
 * Other restarts (WiFi failure path, brown-out, crash) skip the window so a
 * flat battery can't get stuck in a WiFi-on reboot loop.
 *
 * @return true if the maintenance window should be opened
 */
bool maintenanceShouldEnter()
{
  esp_reset_reason_t resetReason = esp_reset_reason();

  Log.println();
  Log.println(F("Maintenance window check..."));
  Log.print(F("\tReset reason:\t\t"));
  Log.println(resetReasonStr(resetReason));

  // *** Is this the first boot after a firmware upload? ***********************
  // Checked two ways, either one counts:
  //   1. flag saved to flash (NVS) just before ElegantOTA reboots
  //   2. the bootloader's "new firmware, not yet verified" (rollback) state
  Preferences prefs;
  prefs.begin(PREFS_NS, false);
  bool otaFlag = prefs.getBool(PREFS_OTA_FLAG, false);
  if (otaFlag) prefs.remove(PREFS_OTA_FLAG); // one-shot - clear it for next boot
  prefs.end();

  bool pendingVerify = false;
  esp_ota_img_states_t otaState;

  if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &otaState) == ESP_OK)
  {
    pendingVerify = (otaState == ESP_OTA_IMG_PENDING_VERIFY);
  }

  bool justUpdated = otaFlag || pendingVerify;

  // *** Only open on power-on/EN reset or after a firmware update *************
  if (justUpdated)
  {
    Log.println(F("\tFirst boot after firmware update - opening window"));
  }
  else if (resetReason != ESP_RST_POWERON && resetReason != ESP_RST_EXT)
  {
    Log.println(F("\tSkipping (not a power-on/EN reset or firmware update)"));
    return false;
  }

  // *** WiFi is needed to serve the web pages *********************************
  if (WiFi.status() != WL_CONNECTED)
  {
    Log.println(F("\tSkipping (no WiFi)"));
    return false;
  }

  // *** Low battery check - avoids a brown-out loop on a flat battery *********
  // (test rig has no battery so the check is skipped)
  #if !TEST_RIG
    if (g_maintMinBattV > 0)
    {
      getSensorINA219();
      float battVoltage = g_stationBatteryVoltage;

      // < 1V means no INA219 fitted - can't judge, so allow the window
      if (battVoltage > 1.0F && battVoltage < g_maintMinBattV)
      {
        Log.print(F("\tSkipping (battery low: "));
        Log.print(battVoltage);
        Log.println(F(" V)"));
        return false;
      }
    }
  #endif

  return true;
} // end function maintenanceShouldEnter ***************************************


/**
 * @brief Open the maintenance window: start mDNS (<hostname>.local), register
 * the web pages and APIs, start ElegantOTA and the web server, and print the
 * page addresses to the log. WiFi must already be connected.
 */
void maintenanceBegin()
{
  s_windowStart = millis();
  s_deadline    = s_windowStart + (unsigned long)g_maintWindowSecs * 1000UL;

  // *** mDNS so the station can be found as http://<hostname>.local ***********
  if (MDNS.begin(DEVICE_HOSTNAME))
  {
    MDNS.addService("http", "tcp", 80);
  }

  // *** Web pages *************************************************************
  // Status page
  s_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (!checkAuth(request)) return;

    request->send(request->beginResponse(200, "text/html", (const uint8_t *)STATUS_PAGE, strlen(STATUS_PAGE)));
  });

  // Log page
  s_server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (!checkAuth(request)) return;

    request->send(request->beginResponse(200, "text/html", (const uint8_t *)LOG_PAGE, strlen(LOG_PAGE)));
  });

  // *** APIs used by the pages ************************************************
  // Log history as plain text (log page). Also counts as "someone is looking"
  // to keep the window open.
  s_server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (!checkAuth(request)) return;

    s_lastClientMs = millis();

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain; charset=utf-8", Log.snapshot());
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
  });

  // Station status as JSON (status page, polled every 2 secs). Also counts as
  // "someone is looking" to keep the window open.
  s_server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (!checkAuth(request)) return;

    s_lastClientMs = millis();

    // readings that may be NaN are sent as null (see jsonNum)
    char battJson[16], currJson[16], tempJson[16];
    jsonNum(battJson, sizeof(battJson), g_stationBatteryVoltage, 2);
    jsonNum(currJson, sizeof(currJson), g_stationBatteryCurrent, 1);
    jsonNum(tempJson, sizeof(tempJson), g_weatherTempAirC, 1);

    char ssidJson[72];
    jsonStr(ssidJson, sizeof(ssidJson), WiFi.SSID().c_str());

    char json[600];
    snprintf(json, sizeof(json),
             "{\"version\":\"%s\",\"profile\":\"%s\",\"testrig\":%s,\"hostname\":\"%s.local\","
             "\"remaining\":%ld,\"nextCycle\":%ld,\"period\":%d,\"uptime\":%lu,\"ota\":%s,\"batt\":%s,\"curr\":%s,"
             "\"ssid\":\"%s\",\"rssi\":%d,\"ip\":\"%s\",\"reset\":\"%s\",\"temp\":%s,\"built\":\"%s\"}",
             VERSION, DEVICE_PROFILE, TEST_RIG ? "true" : "false", DEVICE_HOSTNAME,
             secondsRemaining(), secondsToNextCycle(), g_stationUpdatePeriod, millis() / 1000UL, s_otaActive ? "true" : "false",
             battJson, currJson,
             ssidJson, WiFi.RSSI(), WiFi.localIP().toString().c_str(),
             resetReasonStr(esp_reset_reason()), tempJson, buildStamp());

    request->send(200, "application/json", json);
  });

  // Button commands (status page): extend / cycle / sleep / reboot
  s_server.on("/api/cmd", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (!checkAuth(request)) return;

    if (!request->hasParam("c"))
    {
      request->send(400, "text/plain", "missing c");
      return;
    }

    String cmd = request->getParam("c")->value();
    queueCommand(cmd.c_str(), cmd.length());

    request->send(200, "text/plain", "ok");
  });

  // *** ElegantOTA firmware upload page (/update) *****************************
  // Callbacks run in the web server task so they only record state - progress
  // and results are printed by maintenanceRun. After a successful upload
  // ElegantOTA reboots the ESP32 itself (2 secs later, from ElegantOTA.loop).
  ElegantOTA.begin(&s_server, g_otaUser, g_otaPass);
  ElegantOTA.setAutoReboot(true);

  ElegantOTA.onStart([]()
  {
    s_otaCurrent = 0;
    s_otaTotal   = 0;
    s_otaEnded   = false;
    s_otaActive  = true;
  });

  ElegantOTA.onProgress([](size_t current, size_t final)
  {
    s_otaCurrent = current;
    s_otaTotal   = final;
  });

  ElegantOTA.onEnd([](bool success)
  {
    s_otaSuccess = success;
    s_otaEnded   = true;
    s_otaActive  = false;
  });

  // *** Start serving *********************************************************
  s_server.begin();

  s_started = true;

  IPAddress ip = WiFi.localIP();

  Log.println();
  Log.println(F("=== Maintenance window OPEN ============================================"));
  Log.printf("\tStatus page:\t\thttp://%s.local/  (http://%s/)\n", DEVICE_HOSTNAME, ip.toString().c_str());
  Log.printf("\tFirmware update:\thttp://%s.local/update\n", DEVICE_HOSTNAME);
  Log.printf("\tLog:\t\t\thttp://%s.local/log\n", DEVICE_HOSTNAME);
  Log.printf("\tCloses in:\t\t%d s (extended while status page open, max %d s)\n",
             g_maintWindowSecs, g_maintMaxSecs);

  #if CLOUD_DRY_RUN
    Log.println(F("\tCloud updates:\t\tDRY-RUN (nothing is sent)"));
  #endif

  Log.println(F("========================================================================"));
} // end function maintenanceBegin *********************************************


/**
 * @brief Run the maintenance window until it closes. Each pass of the loop:
 *   - keeps the window open while the status/log page is being viewed
 *   - reports firmware upload progress/result (and holds the window open)
 *   - acts on button requests (extend, run cycle, reboot, sleep)
 *   - runs the normal measure/upload cycle on its usual schedule
 * Returns when the countdown reaches zero or "Sleep now" is pressed (never
 * during an upload).
 *
 * @param cycleFn the normal measure/upload cycle (runCycle in main.cpp)
 */
void maintenanceRun(void (*cycleFn)())
{
  int lastOtaPct = -1; // last upload % printed (prints every 10%)

  scheduleNextCycle();

  while (true)
  {
    unsigned long timeNow = millis();

    // *** Keep the window open while the status/log page is being viewed ******
    if (s_lastClientMs != 0 && timeNow - s_lastClientMs < CLIENT_SEEN_MS)
    {
      extendDeadlineTo(timeNow + CLIENT_HOLD_MS);
    }

    // *** Firmware upload progress ********************************************
    if (s_otaActive)
    {
      // never close the window mid-upload (ignores the hard stop)
      if ((long)(timeNow + 60000UL - s_deadline) > 0) s_deadline = timeNow + 60000UL;

      if (s_otaTotal > 0)
      {
        int pct = (int)((uint64_t)s_otaCurrent * 100 / s_otaTotal);

        if (pct / 10 != lastOtaPct / 10)
        {
          lastOtaPct = pct;
          Log.printf("[ota] %d%% (%u / %u bytes)\n", pct, (unsigned)s_otaCurrent, (unsigned)s_otaTotal);
        }
      }
    }

    // *** Firmware upload finished ********************************************
    if (s_otaEnded)
    {
      s_otaEnded = false;
      lastOtaPct = -1;

      if (s_otaSuccess)
      {
        // Tell the next boot to reopen the maintenance window. ElegantOTA
        // waits 2 secs before rebooting, so this is written in time.
        Preferences prefs;
        prefs.begin(PREFS_NS, false);
        prefs.putBool(PREFS_OTA_FLAG, true);
        prefs.end();

        Log.println(F("[ota] Upload complete - rebooting into new firmware..."));
        Log.println(F("[ota] The maintenance window will reopen after the reboot"));
        Log.println(F("[ota] (it will roll back automatically if it fails before completing a cycle)"));
      }
      else
      {
        Log.println(F("[ota] Upload FAILED - still running current firmware"));
      }
    }

    // *** Button requests from the status page ********************************
    if (s_reqExtend)
    {
      s_reqExtend = false;
      extendDeadlineTo(s_deadline + EXTEND_MS);

      Log.printf("[maint] Window extended - closes in %ld s\n", secondsRemaining());
    }

    if (s_reqCycle)
    {
      s_reqCycle = false;

      Log.println(F("[maint] Running measurement/upload cycle..."));
      cycleFn();
      Log.println(F("[maint] Cycle complete"));
    }

    // *** Scheduled cycle (same clock-aligned schedule as sleep mode) *********
    // (waits until any upload has finished)
    if ((long)(millis() - s_nextCycle) >= 0 && !s_otaActive)
    {
      Log.println();
      Log.printf("[maint] Scheduled cycle (every %d min)\n", g_stationUpdatePeriod);

      cycleFn();
      resetDay();
      scheduleNextCycle();
    }

    if (s_reqReboot)
    {
      Log.println(F("[maint] Rebooting (no maintenance window on software reset)..."));
      delay(500);
      ESP.restart();
    }

    // *** Time to close the window? *******************************************
    if (s_reqSleep && !s_otaActive)
    {
      s_reqSleep = false;
      Log.println(F("[maint] Sleep requested"));
      break;
    }

    if ((long)(timeNow - s_deadline) >= 0 && !s_otaActive) break;

    ElegantOTA.loop();  // performs the reboot after a successful upload
    delay(20);
  }
} // end function maintenanceRun ***********************************************


/**
 * @brief Close the maintenance window: stop the web server and mDNS. Call
 * before killWiFi() / initSleep().
 */
void maintenanceEnd()
{
  if (!s_started) return; // window was never opened

  Log.println(F("=== Maintenance window CLOSED - entering normal low power operation ==="));

  s_server.end();
  MDNS.end();

  s_started = false;
} // end function maintenanceEnd ***********************************************


/**
 * @brief Mark freshly uploaded firmware as good, cancelling the automatic
 * rollback. Called after each successful cycle (see runCycle in main.cpp) -
 * only does anything when the running firmware is still "pending verify", ie
 * the first good cycle after an upload.
 */
void markAppValidIfPending()
{
  const esp_partition_t *runningPartition = esp_ota_get_running_partition();
  esp_ota_img_states_t   otaState;

  if (esp_ota_get_state_partition(runningPartition, &otaState) == ESP_OK &&
      otaState == ESP_OTA_IMG_PENDING_VERIFY)
  {
    esp_ota_mark_app_valid_cancel_rollback();

    Log.println(F("[ota] New firmware verified (WiFi + full cycle OK) - rollback cancelled"));
  }
} // end function markAppValidIfPending ****************************************
