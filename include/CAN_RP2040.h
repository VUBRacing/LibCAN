#ifndef CAN_RP2040_H
#define CAN_RP2040_H

#include <Arduino.h>
#include "CAN_Library.h"
#include <Adafruit_MCP2515.h>

class CANRP2040 : public CANLibrary {
public:
    CANRP2040(); // constructor needed
    bool init() override;
    void send(Message message) override;
    Message read() override;

    bool available() override;
    Message receive() override;

private:
    Adafruit_MCP2515 CAN;
    uint8_t interruptPin;

    static CANRP2040* instance;

    struct Buffer {
        Message messages[CAN_BUFFER_SIZE];
        volatile uint8_t head = 0;
        volatile uint8_t tail = 0;
    };

    Buffer rxBuffer;

    static void ISR();
    void handleInterrupt();
};

#endif
