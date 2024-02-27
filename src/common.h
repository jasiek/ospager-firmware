#include "config.h"

typedef struct message_t {
  uint32_t timestamp;
  uint32_t address;
  uint8_t message[MESSAGE_LENGTH];
};
