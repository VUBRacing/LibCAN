#include "CAN_Adafruit.h"

bool CANAdafruit::init() {
    #if defined(ARDUINO_FEATHER_M4_CAN)
        pinMode(PIN_CAN_STANDBY, OUTPUT);
        digitalWrite(PIN_CAN_STANDBY, false);
        pinMode(PIN_CAN_BOOSTEN, OUTPUT);
        digitalWrite(PIN_CAN_BOOSTEN, true);
    #endif

    for (int i = 0; i < 10; i++) {
        if (CAN.begin(500000)) {
            return true;
        }
        delay(200);
    }
    return false;
}

void CANAdafruit::send(Message message) {
    CAN.beginPacket(message.id);
    for (size_t i = 0; i < message.data_field.size(); i++) {
        CAN.write(message.data_field[i]);
    }
    CAN.endPacket();
}

Message CANAdafruit::read() {
    Message result;
    Message noting = {0, 1, {0}};
    result.packet_size = CAN.parsePacket();

    if (!result.packet_size) {
        return noting;
    }

    result.id = CAN.packetId();
    result.data_field.resize(result.packet_size);
    CAN.readBytes(result.data_field.data(), result.packet_size);

    return result;
}
