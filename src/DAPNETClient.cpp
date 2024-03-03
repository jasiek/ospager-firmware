#include "DAPNETClient.h"
#include "common.h"


DAPNETClient::DAPNETClient(PagerClient &receiver, uint32_t ric) : receiver(receiver),
                                                                              ric(ric),
                                                                              unhandledCallback(NULL)
{
  memcpy(this->addresses, DAPNET_addresses, DAPNET_count);
  memcpy(this->masks, DAPNET_masks, DAPNET_count);
  this->addresses[0] = ric;
}

// TODO: 2024-02-15 (jps): Add error handling
int16_t DAPNETClient::begin() {
  int16_t state;
  state = this->receiver.begin(DAPNET_FREQUENCY, 1200);
  if (state != RADIOLIB_ERR_NONE) return state;

  state = this->receiver.startReceive(LORA_DIO2, this->addresses, this->masks, DAPNET_count);
  if (state != RADIOLIB_ERR_NONE) return state;

  return state;
}

bool DAPNETClient::run() {
  // Runs one cycle, meant to be put in a loop
  if (this->receiver.available() >= 2) {
    uint8_t data[MESSAGE_LENGTH];
    memset(data, 0, MESSAGE_LENGTH);
    uint32_t address;
    int state = this->receiver.readData(data, (size_t*)MESSAGE_LENGTH, &address);
    if (state != RADIOLIB_ERR_NONE) {
      // error
      return false;
    }
    this->handleMessage(data, address);
    return true;
  }
  return false; 
}

bool DAPNETClient::handleMessage(uint8_t *message, uint32_t address) {
  // Return value indicates that there's been an unhandled message
  if (address == 8) {
    // current node
    this->setNodename(message);
    return false;
  }
  if (address == 208) {
    // https://hampager.de/dokuwiki/doku.php?id=blog:uhrzeitsynchronisation_mit_lokaler_zeitzone
    this->setTime(message);
    return false;
  }
  if (unhandledCallback) {
    message_t unhandled;
    unhandled.address = address;
    unhandled.timestamp = millis();
    memcpy(unhandled.message, message, MESSAGE_LENGTH);
    unhandledCallback(unhandled);
  }
  return true;
}

void DAPNETClient::setNodename(uint8_t *message) {
  strncpy(this->nodename, (char *)message, CALLSIGN_LENGTH);
}

void DAPNETClient::setTime(uint8_t *message) {
  // TODO
}
