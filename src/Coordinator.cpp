#include "Coordinator.h"

Coordinator::Coordinator(DAPNETClient &client, Mailbox &mailbox, UI &ui) : client(client), mailbox(mailbox), ui(ui)
{
    client.unhandledCallback = std::bind(&Coordinator::handleMessage, this, std::placeholders::_1);
};

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