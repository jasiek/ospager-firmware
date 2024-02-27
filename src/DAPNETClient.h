#include "MultiAddressPagerClient.h"
#include "roo_time.h"
#include "common.h"
#include "config.h"

#define DAPNET_FREQUENCY 439.9875
#define DIO2_PIN 0

typedef void (*unhandledMessageCallback_t)(message_t);

class DAPNETClient {
public:
  DAPNETClient(MultiAddressPagerClient &receiver, uint32_t ric);

  void begin();
  bool run();
  void handleMessage(uint8_t *message, uint32_t address);
  void setNodename(uint8_t *message);
  void setTime(uint8_t *message);

  MultiAddressPagerClient &receiver;
  char nodename[16];
  uint32_t ric;
  uint32_t addresses[sizeof(DAPNET_addresses)];
  unhandledMessageCallback_t unhandledCallback;
};
