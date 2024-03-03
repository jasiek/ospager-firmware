#include "UI.h"

UI::UI() {
    Serial.begin(115200);
    Serial.println("UI initialized");
}

void UI::newMessageNotification(message_t message) {
    Serial.print("New message received at ");
    Serial.println(message.timestamp);

    Serial.print("From address: ");
    Serial.println(message.address);

    // print message contents
    Serial.print("Message: ");
    Serial.print((char *)message.message);
    Serial.println();
}

bool UI::actionsPending() {
    // TODO
    return false;
}

void UI::displayErroMessage(const char *message) {
    Serial.println(message);
}