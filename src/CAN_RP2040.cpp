#include "CAN_RP2040.h"

CANRP2040::CANRP2040()
    : CAN(19) 
{
}


bool CANRP2040::init() {
    for (int i = 0; i < 10; i++) {
        if (CAN.begin(500000)) {
            return true;
        }
        delay(200);
    }
    return false;
}

void CANRP2040::send(Message message) {
    CAN.beginPacket(message.id);
    for (size_t i = 0; i < message.data_field.size(); i++) {
        CAN.write(message.data_field[i]);
    }
    CAN.endPacket();
}

Message CANRP2040::read() {
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
