#include "Coordinator.h"
#include <stdlib.h>

Coordinator::Coordinator(DAPNETClient &client, Mailbox &mailbox, UI &ui) : client(client), mailbox(mailbox), ui(ui)
{
    client.unhandledCallback = std::bind(&Coordinator::handleMessage, this, std::placeholders::_1);
};

int16_t Coordinator::begin() {
    int16_t state = client.begin();
    if (state != RADIOLIB_ERR_NONE) {
        char *message = "Error: %d";
        char buffer[20];
        sprintf(buffer, message, state);
        ui.displayErroMessage(buffer);
    }
    return state;
}

void Coordinator::run() {
    // wait on action from UI
    if (ui.actionsPending()) {
        // process ui actions
    }
    // if no action pending then attempt to receive messages
    while (client.run()) {
        message_t lastMessage = mailbox.getMessage(mailbox.getMessageCount() - 1);
        ui.newMessageNotification(lastMessage);
    }
}

void Coordinator::handleMessage(message_t message) {
    this->mailbox.storeMessage(message);
}