#include <Arduino.h>
#include "Coordinator.h"

SX1276 radio = new Module(10, 2, 9, 3);

const uint32_t PAGER_ADDRESS = 0;
const float f = 439.9875;
Coordinator *coordinator;

void setup() {
  MultiAddressPagerClient pager(&radio);
  DAPNETClient dapnet(pager, PAGER_ADDRESS);
  Mailbox mailbox;
  UI ui;

  coordinator = new Coordinator(dapnet, mailbox, ui);
}

void loop() {
  coordinator->run();
}

