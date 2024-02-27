#include "common.h"

class Mailbox {
    bool storeMessage(message_t message);
    uint8_t getMessageCount();
    message_t getMessage(uint8_t index);
    
    // TODO: mark as read

    // semi private
    void sortMailbox();

    message_t messages[MAILBOX_SIZE];
    uint8_t message_count;
};