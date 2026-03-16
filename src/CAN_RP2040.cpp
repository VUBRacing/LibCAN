#include "Adafruit_MCP2515.h"
#include "CAN_RP2040.h"

CANRP2040::CANRP2040() // contruction
    : CAN(19u, &SPI1)     
{
}


bool CANRP2040::init() {
    SPI1.setSCK(14);
    SPI1.setTX(15);
    SPI1.setRX(8);


    SPI1.begin();
    delay(10);

    for (int i = 0; i < 10; i++) {
        if (CAN.begin(500000)) {
            pinMode(interruptPin, INPUT);
            attachInterrupt(digitalPinToInterrupt(interruptPin), ISR, FALLING);
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

CANRP2040* CANRP2040::instance = nullptr;

void CANRP2040::ISR() {
    if (instance) {
        instance->handleInterrupt();
    }
}

void CANRP2040::handleInterrupt() {

    Message msg;

    msg.packet_size = CAN.parsePacket();

    if (!msg.packet_size)
        return;

    msg.id = CAN.packetId();
    msg.data_field.resize(msg.packet_size);

    CAN.readBytes(msg.data_field.data(), msg.packet_size);

    uint8_t next = (rxBuffer.head + 1) % CAN_BUFFER_SIZE;

    if (next != rxBuffer.tail) { // prevent overflow
        rxBuffer.messages[rxBuffer.head] = msg;
        rxBuffer.head = next;
    }
}

bool CANRP2040::available() {
    return rxBuffer.head != rxBuffer.tail;
}

Message CANRP2040::receive() {

    Message msg;

    if (rxBuffer.head == rxBuffer.tail)
        return {0,0,{}};

    msg = rxBuffer.messages[rxBuffer.tail];

    rxBuffer.tail = (rxBuffer.tail + 1) % CAN_BUFFER_SIZE;

    return msg;
}