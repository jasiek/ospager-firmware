#include <Arduino.h>
#include <RadioLib.h>

#include "pocsag.h"

SX1276 radio = new Module(10, 2, 9, 3);
PagerClient client(&radio);

const uint32_t DIO2_PIN = 0;
const uint32_t PAGER_ADDRESS = 0;
const float f = 439.9875;

void setup() {
  pager.begin(f, 1200);
  int state = pager.startReceive(DIO2_PIN, PAGER_ADDRESS);
  if (state != RADIOLIB_ERR_NONE) {
    // error
  }
}

void loop() {
  if (pager.available() >= 2) {
    String str;
    int state = page.readData(str);
    if (state != RADIOLIB_ERR_NONE) {
      // error
    }
    // print(str)
  }
}

