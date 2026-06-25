#include "pocsag_service.h"

#include <Arduino.h>
#include <RadioLib.h>

#include "board.h"
#include "message_store.h"
#include "time_sync.h"

// ----- Time-broadcast identification ---------------------------------------
// Several capcodes send the same "YYYYMMDDHHMMSS" label in different zones, so
// we tell them apart by capcode (see README for the survey of senders).
static const char     TIME_LABEL[]       = "YYYYMMDDHHMMSS";
static const uint32_t SYNC_CAPCODE_LOCAL = 224;   // fields are local wall time
static const uint32_t SYNC_CAPCODE_UTC   = 216;   // fields are UTC

// SX1278 module wiring: Module(cs, irq = DIO0, rst, gpio = DIO1).
static SX1278 radio =
    new Module(PIN_LORA_CS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1);
static PagerClient pager(&radio);

// Parse the "YYYYMMDDHHMMSS" + YYMMDDHHMMSS time message into fields.
static bool parsePocsagTime(const String& msg, struct tm& out) {
  int idx = msg.indexOf(TIME_LABEL);
  if (idx < 0) return false;

  char d[12];
  size_t n = 0;
  for (size_t i = idx + strlen(TIME_LABEL); i < msg.length() && n < 12; i++) {
    char c = msg[i];
    if (c >= '0' && c <= '9') d[n++] = c;
  }
  if (n < 12) return false;

  auto two = [&](size_t p) { return (d[p] - '0') * 10 + (d[p + 1] - '0'); };
  int yy = two(0), mo = two(2), dd = two(4);
  int hh = two(6), mi = two(8), ss = two(10);
  if (mo < 1 || mo > 12 || dd < 1 || dd > 31 || hh > 23 || mi > 59 || ss > 59) {
    return false;
  }

  memset(&out, 0, sizeof(out));
  out.tm_year = (2000 + yy) - 1900;
  out.tm_mon  = mo - 1;
  out.tm_mday = dd;
  out.tm_hour = hh;
  out.tm_min  = mi;
  out.tm_sec  = ss;
  return true;
}

PocsagService::PocsagService(Clock& clock, MessageStore& messages,
                             MessageStore& rubrics)
    : clock_(clock), messages_(messages), rubrics_(rubrics) {}

bool PocsagService::begin() {
  int state = radio.beginFSK();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] beginFSK failed (%d)\n", state);
    return false;
  }
  state = pager.begin(POCSAG_FREQ_MHZ, POCSAG_BAUD, POCSAG_INVERT);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] pager.begin failed (%d)\n", state);
    return false;
  }
  // addr = 0, mask = 0  -> match every capcode.
  state = pager.startReceive(PIN_LORA_DIO2, (uint32_t)0, (uint32_t)0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] startReceive failed (%d)\n", state);
    return false;
  }
  return true;
}

void PocsagService::poll() {
  if (pager.available() < 2) return;

  String msg;
  uint32_t addr = 0;
  int state = pager.readData(msg, 0, &addr);
  if (state != RADIOLIB_ERR_NONE) return;

  // Sync the RTC first so the message below (and especially the time broadcast
  // that carries the sync) is stamped with the corrected time, not the stale
  // pre-sync clock.
  struct tm fields;
  if (parsePocsagTime(msg, fields)) {
    if (addr == SYNC_CAPCODE_UTC) {
      clock_.syncFromUtcFields(fields, addr);
    } else if (addr == SYNC_CAPCODE_LOCAL) {
      clock_.syncFromLocalFields(fields, addr);
    }
  }

  // Route by capcode: this pager's own traffic is a "message"; everything else
  // is a "rubric". The two stores are otherwise identical.
  MessageStore& dest = (addr == PAGER_CAPCODE) ? messages_ : rubrics_;
  dest.add(addr, clock_.now(), msg);
}
