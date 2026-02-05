#include "CAN_RP2040"

bool CANRP2040::init() {
    for (int i = 0; i < 10; i++) {
        if (mcp.begin(500000)) {
            return true;
        }
        delay(200);
    }
    return false;
}

void CANRP2040::send(Message message) {
    mcp.beginPacket(message.id);
    for (size_t i = 0; i < message.data_field.size(); i++) {
        mcp.write(message.data_field[i]);
    }
    mcp.endPacket();
}

Message CANRP2040::read() {
    Message result;
    Message noting = {0, 1, {0}};
    result.packet_size = mpc.parsePacket();

    if (!result.packet_size) {
        return noting;
    }

    result.id = mpc.packetId();
    result.data_field.resize(result.packet_size);
    mpc.readBytes(result.data_field.data(), result.packet_size);

    return result;
}
