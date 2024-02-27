#include "common.h"

class UI {
    public:
    UI();

    // is there anything to do here?
    bool actionsPending();

    // user actions
    // TODO

    // incoming notifications
    void newMessageNotification(message_t);
};