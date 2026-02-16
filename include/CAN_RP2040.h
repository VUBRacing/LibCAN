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

private:
    Adafruit_MCP2515 CAN(19u, &SPI1);
};

#endif
