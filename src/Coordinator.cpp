#include "Coordinator.h"

Coordinator::Coordinator(DAPNETClient &client, Mailbox &mailbox) : client(client), mailbox(mailbox)
{
    client.unhandledCallback = std::bind(&Coordinator::handleMessage, this, std::placeholders::_1);
};


void Coordinator::run() {

}