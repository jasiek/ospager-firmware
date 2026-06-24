// ---------------------------------------------------------------------------
//  T-Beam POCSAG clock
//
//  Hardware : LilyGo T-Beam v1.1  (ESP32 + SX1278 433 MHz + AXP192 PMU)
//             + SSD1306 128x64 OLED on the shared I2C bus (addr 0x3C).
//  Function : Receive POCSAG pager messages on 439.9875 MHz, sync the ESP32
//             internal RTC from the broadcast time messages, and show a live
//             HH:MM:SS clock (second precision) on the OLED.
//
//  POCSAG is direct-FSK demodulated by RadioLib's PagerClient. On the SX127x
//  the radio runs in continuous (direct) mode where:
//     * DIO1 outputs the recovered bit clock (DCLK)  -> drives the read ISR
//     * DIO2 outputs the demodulated data            -> sampled on each clock
//
//  Time source: time broadcasts use the label "YYYYMMDDHHMMSS" followed by
//  YYMMDDHHMMSS, e.g. "YYYYMMDDHHMMSS260624230600". The SAME label is sent by
//  several capcodes in different timezones, so the label alone can't tell them
//  apart - we key off the capcode:
//     * capcode 224 -> fields are LOCAL wall time (second precision, but rare)
//     * capcode 216 -> fields are UTC          (second precision, ~every 4 min)
//  We store the RTC in UTC and render LOCAL time via DISPLAY_TZ (a POSIX TZ
//  string with DST rules). Syncing primarily from the frequent UTC sender
//  keeps the clock fresh; the rare local sender is used too when it appears.
//  Other broadcasts (e.g. capcode 208's minute-only "XTIME=") are ignored.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <time.h>
#include <sys/time.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Select the AXP192 PMU before including XPowersLib so that the unified
// XPowersPMU type is defined for this chip (T-Beam v1.1 uses the AXP192).
#define XPOWERS_CHIP_AXP192
#include <XPowersLib.h>

// ----- T-Beam v1.1 LoRa (SX1278) pin map -----------------------------------
#define LORA_SCK    5
#define LORA_MISO   19
#define LORA_MOSI   27
#define LORA_CS     18
#define LORA_RST    23
#define LORA_DIO0   26   // IRQ
#define LORA_DIO1   33   // direct-mode bit clock (DCLK)
#define LORA_DIO2   32   // direct-mode data        (DATA)

// ----- I2C bus (shared by AXP192 PMU and the SSD1306 OLED) ------------------
#define I2C_SDA     21
#define I2C_SCL     22
#define OLED_ADDR   0x3C
#define OLED_W      128
#define OLED_H      64

// ----- POCSAG receive settings ---------------------------------------------
static const float    POCSAG_FREQ_MHZ = 439.9875f;
static const uint16_t POCSAG_BAUD     = 1200;   // 512 / 1200 / 2400
static const bool     POCSAG_INVERT   = false;

// Label that prefixes the time broadcasts.
static const char     TIME_LABEL[]    = "YYYYMMDDHHMMSS";
// Capcodes carrying second-precision time, and how to interpret their fields.
static const uint32_t SYNC_CAPCODE_LOCAL = 224;  // fields are local wall time
static const uint32_t SYNC_CAPCODE_UTC   = 216;  // fields are UTC
// POSIX timezone for display. Default: Central European Time with EU DST
// (covers Poland/Germany/...). Change this if you are in another zone.
static const char     DISPLAY_TZ[]    = "CET-1CEST,M3.5.0,M10.5.0/3";

// SX1278 module wiring: Module(cs, irq = DIO0, rst, gpio = DIO1)
SX1278 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);
PagerClient pager(&radio);

// AXP192 power-management unit (T-Beam v1.1)
XPowersPMU PMU;

// SSD1306 OLED (no dedicated reset pin on the add-on -> -1)
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

// ----- RTC sync state ------------------------------------------------------
static bool          haveDisplay   = false;
static bool          timeSynced    = false;
static uint32_t      lastSyncMillis = 0;
static uint32_t      lastSyncCapcode = 0;

static void halt(const char *msg) {
  Serial.println(msg);
  Serial.flush();
  while (true) {
    delay(1000);
  }
}

// Bring up the AXP192 and power the LoRa radio rail (LDO2 on the T-Beam).
static bool setupPower() {
  if (!PMU.begin(Wire, AXP192_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
    return false;
  }
  // LDO2 -> SX1278 VDD.  (LDO3 -> GPS, left off; DCDC1/3 -> ESP32, untouched.)
  PMU.setLDO2Voltage(3300);
  PMU.enableLDO2();
  delay(50);   // let the radio regulator settle before SPI access
  return true;
}

// Probe for the SSD1306 and initialise it. Returns false if not present so the
// firmware can still run as a serial-only clock.
static bool setupDisplay() {
  // begin(vcc, addr, reset=false, periphBegin=false): we manage Wire ourselves.
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, false, false)) {
    return false;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("T-Beam POCSAG clock"));
  display.println(F("waiting for sync..."));
  display.display();
  return true;
}

// Parse the local-time broadcast into a struct tm. Returns false if the
// message is not a recognised time message.
static bool parsePocsagTime(const String &msg, struct tm &out) {
  int idx = msg.indexOf(TIME_LABEL);
  if (idx < 0) {
    return false;
  }

  // Collect the digits that follow the label.
  char d[16];
  size_t n = 0;
  for (size_t i = idx + strlen(TIME_LABEL); i < msg.length() && n < 12; i++) {
    char c = msg[i];
    if (c >= '0' && c <= '9') {
      d[n++] = c;
    }
  }
  if (n < 12) {
    return false;
  }

  auto two = [&](size_t pos) { return (d[pos] - '0') * 10 + (d[pos + 1] - '0'); };
  int yy = two(0), mo = two(2), dd = two(4);
  int hh = two(6), mi = two(8), ss = two(10);

  if (mo < 1 || mo > 12 || dd < 1 || dd > 31 ||
      hh > 23 || mi > 59 || ss > 59) {
    return false;
  }

  memset(&out, 0, sizeof(out));
  out.tm_year  = (2000 + yy) - 1900;
  out.tm_mon   = mo - 1;
  out.tm_mday  = dd;
  out.tm_hour  = hh;
  out.tm_min   = mi;
  out.tm_sec   = ss;
  out.tm_isdst = -1;   // let mktime resolve DST for the local-time sender
  return true;
}

// Convert a UTC calendar time to a Unix epoch, independent of the current TZ
// (newlib here has no timegm()). Howard Hinnant's days_from_civil algorithm.
static time_t timegmUtc(const struct tm &t) {
  int  year  = t.tm_year + 1900;
  int  month = t.tm_mon + 1;            // 1..12
  year -= (month <= 2);
  int  era   = (year >= 0 ? year : year - 399) / 400;
  unsigned yoe = (unsigned)(year - era * 400);
  unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + t.tm_mday - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = (long)era * 146097 + (long)doe - 719468;   // days since 1970-01-01
  long long secs = (long long)days * 86400LL +
                   t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
  return (time_t)secs;
}

// Push a parsed broadcast time into the ESP32 RTC. The RTC is kept in UTC:
// UTC fields convert with timegmUtc(); local fields convert with mktime()
// under DISPLAY_TZ (which also resolves DST).
static void applyRtc(struct tm &t, uint32_t capcode, bool fieldsAreUtc) {
  time_t epoch = fieldsAreUtc ? timegmUtc(t) : mktime(&t);
  struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);

  timeSynced      = true;
  lastSyncMillis  = millis();
  lastSyncCapcode = capcode;

  struct tm lt;
  localtime_r(&epoch, &lt);
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
  Serial.printf("[RTC] synced from capcode %lu (%s) -> local %s\n",
                (unsigned long)capcode, fieldsAreUtc ? "UTC" : "local", buf);
}

// Render the live clock to the OLED.
static void drawClock(time_t now) {
  if (!haveDisplay) {
    return;
  }
  struct tm lt;
  localtime_r(&now, &lt);
  char dateBuf[16], timeBuf[16];
  strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", &lt);
  strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &lt);

  display.clearDisplay();

  // Date, small, centred (10 chars * 6px = 60px -> x=(128-60)/2).
  display.setTextSize(1);
  display.setCursor(34, 2);
  display.print(timeSynced ? dateBuf : "----------");

  // Time, big, centred (8 chars * 12px = 96px -> x=(128-96)/2).
  display.setTextSize(2);
  display.setCursor(16, 24);
  display.print(timeSynced ? timeBuf : "--:--:--");

  // Sync status, small, bottom.
  display.setTextSize(1);
  display.setCursor(0, 54);
  if (timeSynced) {
    unsigned long ago = (millis() - lastSyncMillis) / 1000UL;
    display.printf("synced %lus ago  cc%lu", ago, (unsigned long)lastSyncCapcode);
  } else {
    display.print("waiting for POCSAG");
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== T-Beam POCSAG clock ==="));

  // RTC is stored in UTC; display in local time per DISPLAY_TZ (with DST).
  setenv("TZ", DISPLAY_TZ, 1);
  tzset();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);   // 400 kHz: AXP192 + SSD1306 both support it

  if (!setupPower()) {
    halt("AXP192 not found over I2C - is this a v1.1 board? Halting.");
  }
  Serial.println(F("AXP192 OK, LoRa rail (LDO2) enabled."));

  haveDisplay = setupDisplay();
  Serial.printf("SSD1306 OLED %s\n",
                haveDisplay ? "OK" : "not found - serial-only clock");

  // Bring up the SPI bus the radio lives on.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  Serial.print(F("Init SX1278 (FSK) ... "));
  int state = radio.beginFSK();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("failed (code %d)\n", state);
    halt("Radio init failed. Check wiring/power. Halting.");
  }
  Serial.println(F("OK"));

  Serial.print(F("Init POCSAG decoder ... "));
  state = pager.begin(POCSAG_FREQ_MHZ, POCSAG_BAUD, POCSAG_INVERT);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("failed (code %d)\n", state);
    halt("Pager init failed. Halting.");
  }
  Serial.println(F("OK"));

  Serial.print(F("Start receive ... "));
  // addr = 0, mask = 0  -> match every capcode, i.e. see all traffic.
  state = pager.startReceive(LORA_DIO2, (uint32_t)0, (uint32_t)0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("failed (code %d)\n", state);
    halt("startReceive failed. Halting.");
  }
  Serial.printf("listening on %.4f MHz @ %u baud%s\n",
                POCSAG_FREQ_MHZ, POCSAG_BAUD,
                POCSAG_INVERT ? " (inverted)" : "");
  Serial.println(F("------------------------------------------"));
}

void loop() {
  // 1) Drain any decoded POCSAG frames and sync the RTC from time messages.
  if (pager.available() >= 2) {
    String msg;
    uint32_t addr = 0;
    int state = pager.readData(msg, 0, &addr);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.printf("[POCSAG] capcode=%lu  msg=", (unsigned long)addr);
      Serial.println(msg);

      // Sync the RTC only from the known second-precision time senders.
      struct tm t;
      if (parsePocsagTime(msg, t)) {
        if (addr == SYNC_CAPCODE_UTC) {
          applyRtc(t, addr, /*fieldsAreUtc=*/true);
        } else if (addr == SYNC_CAPCODE_LOCAL) {
          applyRtc(t, addr, /*fieldsAreUtc=*/false);
        }
      }
    } else {
      Serial.printf("[POCSAG] decode error (code %d)\n", state);
    }
  }

  // 2) Refresh the clock once per second (second precision).
  static int lastSec = -1;
  time_t now = time(nullptr);
  struct tm lt;
  localtime_r(&now, &lt);
  if (lt.tm_sec != lastSec) {
    lastSec = lt.tm_sec;
    drawClock(now);
    if (!haveDisplay && timeSynced) {
      // Serial-only fallback clock: overwrite a single line.
      char buf[24];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
      Serial.printf("\r[CLOCK] %s ", buf);
    }
  }
}
