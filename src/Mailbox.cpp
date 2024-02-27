#include "Mailbox.h"
#include <stdlib.h>

int compareMessagesByTimestamp(const void *a, const void *b) {
    message_t *am = (message_t*)a;
    message_t *bm = (message_t*)b;
    return am->timestamp > bm->timestamp;
}

bool Mailbox::storeMessage(message_t message) {
    messages[message_count] = message;
    message_count++;
    sortMailbox();
    return true;
    // TODO error handling when too many messages
}

void Mailbox::sortMailbox() {
    // sorts mailbox by received timestamp descending
    qsort(messages, message_count, sizeof(message_t), compareMessagesByTimestamp);
}

uint8_t Mailbox::getMessageCount() {
    return message_count;
}

message_t Mailbox::getMessage(uint8_t index) {
    return messages[index];
}