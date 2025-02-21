#ifndef CAN_ADAFRUIT_H
#define CAN_ADAFRUIT_H

#include <Arduino.h>
#include <CANSAME5x.h>
#include "CAN_Library.h"

class CANAdafruit : public CANLibrary {
public:
    bool init() override;
    void send(Message message) override;
    Message read() override;

private:
    CANSAME5x CAN;
};

#endif
