#include <RadioLib.h>
#include "common.h"
#include "config.h"
#include <functional>

#define DAPNET_FREQUENCY 439.9875

typedef std::function<void(message_t)> unhandledMessageCallback_t;

class DAPNETClient {
public:
  DAPNETClient(PagerClient &receiver, uint32_t ric);

  int16_t begin();
  bool run();
  bool handleMessage(uint8_t *message, uint32_t address);
  void setNodename(uint8_t *message);
  void setTime(uint8_t *message);

  PagerClient &receiver;
  char nodename[16];
  uint32_t ric;
  uint32_t addresses[DAPNET_count];
  uint32_t masks[DAPNET_count];
  unhandledMessageCallback_t unhandledCallback;
};
