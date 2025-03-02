#ifndef CAN_LIBRARY
#define CAN_LIBRARY

#include "Arduino.h"
#include <vector>

struct Message {
	uint16_t id;
	uint16_t packet_size;
	std::vector<uint16_t> data_field;
};

class CANLibrary {
public:

	virtual bool init();
	virtual void send(Message message);
	virtual Message read();

};

#endif
