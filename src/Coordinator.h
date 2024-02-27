#include "DAPNETClient.h"
#include "Mailbox.h"
#include "UI.h"

class Coordinator {
    public:
    Coordinator(DAPNETClient &client, Mailbox &mailbox, UI &ui); // TODO add UI
    void handleMessage(message_t);
    void run();

    DAPNETClient &client;
    Mailbox &mailbox;
    UI &ui;
};