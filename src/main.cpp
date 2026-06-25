// ---------------------------------------------------------------------------
//  ospager - open-hardware POCSAG pager firmware
//
//  Hardware (current target): LilyGo T-Beam v1.1 (ESP32 + SX1278 433 MHz +
//  AXP192) with an optional SSD1306 OLED.
//
//  Architecture: a headless core (radio/POCSAG + RTC + message store) feeds a
//  UI layer built on a character-grid FrameBuffer. Screens render once into the
//  grid; each attached Surface (serial terminal, OLED, future LCD) draws it.
//  The serial terminal is the primary UI now; the OLED renders the same screens
//  to prove the abstraction on real hardware.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <time.h>

#include "board.h"
#include "message_store.h"
#include "pocsag_service.h"
#include "power.h"
#include "time_sync.h"
#include "ui/input.h"
#include "ui/oled_surface.h"
#include "ui/screens.h"
#include "ui/serial_surface.h"
#include "ui/ui.h"

// ----- Headless core --------------------------------------------------------
static Clock         g_clock;
static MessageStore  g_messages;   // traffic addressed to this pager
static MessageStore  g_rubrics;    // all other capcodes (broadcast rubrics)
static PocsagService g_pocsag(g_clock, g_messages, g_rubrics);

// ----- UI -------------------------------------------------------------------
static SerialSurface g_serialSurface(Serial);
static OledSurface   g_oledSurface;
static SerialInput   g_input(Serial);
static UiManager     g_ui;

static HomeScreen     g_home(g_clock, g_messages);
static MenuScreen     g_mainMenu("Menu");
static MessagesScreen g_messagesScreen(g_messages, "Messages");
static DetailScreen   g_messagesDetail(g_messages);
static MessagesScreen g_rubricsScreen(g_rubrics, "Rubrics");
static DetailScreen   g_rubricsDetail(g_rubrics);
static MenuScreen     g_diagMenu("Diagnostics");
static PowerScreen    g_power;

static void fatal(const char* msg) {
  Serial.println(msg);
  Serial.flush();
  for (;;) delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\nospager booting..."));

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  if (!powerInit()) fatal("AXP192 not found - check board. Halting.");
  g_clock.begin();

  bool haveOled = g_oledSurface.begin();
  Serial.printf("OLED: %s\n", haveOled ? "yes" : "no");

  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_CS);
  if (!g_pocsag.begin()) fatal("radio init failed. Halting.");

  // Navigation wiring:
  //   Home -> Menu -> { Messages, Rubrics, Diagnostics -> Power }
  g_home.setMenuScreen(&g_mainMenu);
  g_mainMenu.addItem("Messages", &g_messagesScreen);
  g_mainMenu.addItem("Rubrics", &g_rubricsScreen);
  g_mainMenu.addItem("Diagnostics", &g_diagMenu);
  g_diagMenu.addItem("Power", &g_power);
  g_messagesScreen.setDetailScreen(&g_messagesDetail);
  g_rubricsScreen.setDetailScreen(&g_rubricsDetail);

  // Attach surfaces and start at the home screen.
  g_ui.addSurface(&g_serialSurface);
  if (haveOled) g_ui.addSurface(&g_oledSurface);
  g_ui.push(&g_home);

  g_serialSurface.begin();   // clears the terminal and takes it over
  g_ui.markDirty();
}

void loop() {
  g_pocsag.poll();

  g_input.update();
  InputEvent ev;
  while (g_input.pop(ev)) g_ui.onInput(ev);

  // Drive a once-per-second tick and repaint on new traffic.
  static int      lastSec = -1;
  static uint32_t lastRev = 0;
  time_t now = g_clock.now();
  struct tm lt;
  localtime_r(&now, &lt);
  if (lt.tm_sec != lastSec) {
    lastSec = lt.tm_sec;
    g_ui.onTick(millis());
    g_ui.markDirty();
  }
  uint32_t rev = g_messages.revision() + g_rubrics.revision();
  if (rev != lastRev) {
    lastRev = rev;
    g_ui.markDirty();
  }

  if (g_ui.dirty()) g_ui.render();
}
