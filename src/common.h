#include "config.h"
#ifndef COMMON_H
#define COMMON_H

typedef struct {
  uint32_t timestamp;
  uint32_t address;
  uint8_t message[MESSAGE_LENGTH];
} message_t;

#endif