#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

const uint8_t MESSAGE_LENGTH = 80;
const uint32_t MAILBOX_SIZE = 100;
const uint8_t CALLSIGN_LENGTH = 16;

const uint32_t DAPNET_addresses[] = {
    0,   // this pager's RIC
    8,   // current node
    208, // local time
};
#endif