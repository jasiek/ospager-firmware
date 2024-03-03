#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>


const uint8_t MESSAGE_LENGTH = 80;
const uint32_t MAILBOX_SIZE = 100;
const uint8_t CALLSIGN_LENGTH = 16;

#define LORA_SS 18
#define LORA_DIO0 26
#define LORA_DIO1 33
#define LORA_DIO2 32
#define LORA_RST 23

const uint32_t DAPNET_addresses[] = {
    0,   // this pager's RIC
    8,   // current node
    208, // local time
};

const uint32_t DAPNET_masks[] = {
    0xFFFFF,
    0xFFFFF,
    0xFFFFF
};

const size_t DAPNET_count = sizeof(DAPNET_addresses) / sizeof(DAPNET_addresses[0]);

#endif