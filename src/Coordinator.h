#include "DAPNETClient.h"
#include "Mailbox.h"
#include "UI.h"

class Coordinator {
    public:
    Coordinator(DAPNETClient &client, Mailbox &mailbox, UI &ui); // TODO add UI
    int16_t begin();
    void handleMessage(message_t);
    void run();

    DAPNETClient &client;
    Mailbox &mailbox;
    UI &ui;
};