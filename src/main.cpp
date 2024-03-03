#include <Arduino.h>
#include "Coordinator.h"


const uint32_t PAGER_ADDRESS = 1002374;
const float f = 439.9875;


SX1276 radio = new Module(LORA_SS, LORA_DIO0, LORA_RST, LORA_DIO1);
PagerClient pager(&radio);

DAPNETClient dapnet(pager, PAGER_ADDRESS);
Mailbox mailbox;
UI ui;

Coordinator *coordinator = new Coordinator(dapnet, mailbox, ui);

void setup() {
  Serial.begin(115200);
  if (coordinator->begin() != RADIOLIB_ERR_NONE) {
    while(1) {};
  }
}

void loop() {

  coordinator->run();
}

