#ifndef CAN_LIBRARY
#define CAN_LIBRARY

#include "Arduino.h"
#include <vector>

struct Message {
	uint16_t id;
	uint16_t packet_size;
	std::vector<uint8_t> data_field;
};

class CANLibrary {
public:
	virtual ~CANLibrary() = default;

	virtual bool init() = 0;
	virtual void send(Message message) = 0;
	virtual Message read() = 0;

};

#endif
