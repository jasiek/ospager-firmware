#include "DAPNETClient.h"
#include "Mailbox.h"

class Coordinator {
    public:
    Coordinator(DAPNETClient &client, Mailbox &mailbox); // TODO add UI
    void handleMessage(message_t);
    void run();

    DAPNETClient &client;
    Mailbox &mailbox;
}