#include "CAN_RP2040.h"

// ── Static instance pointer ───────────────────────────────────────────────────
CANRP2040* CANRP2040::instance = nullptr;

// ── Constructor ───────────────────────────────────────────────────────────────
CANRP2040::CANRP2040()
    : CAN(CS_PIN, &SPI1)
{
    instance = this;
}

// ── init ──────────────────────────────────────────────────────────────────────
bool CANRP2040::init()
{
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

// ── send ──────────────────────────────────────────────────────────────────────
// Called from main loop only – SPI is safe here
void CANRP2040::send(Message message)
{
    CAN.beginPacket(message.id);
    CAN.write(message.data_field.data(), message.packet_size);
    CAN.endPacket();
}

// ── Read ──────────────────────────────────────────────────────────────────────
// read() is the old polling method, now unused.
// available() + receive() replace it entirely.
Message CANRP2040::read()
{
    return Message{};
}

// ── ISR – flag only, zero SPI, zero heap ─────────────────────────────────────
/*
   The MCP2515 INT pin goes LOW when a frame is ready.
   We record that fact and return immediately.
   The actual SPI read happens in drainMCP(), called from available().
*/
void CANRP2040::ISR()
{
    if (instance)
        instance->rxPending = true;
}

// ── drainMCP – real SPI work, main loop only ─────────────────────────────────
void CANRP2040::drainMCP()
{
    // The MCP2515 can hold 2 frames in hardware; drain until empty
    while (true) {
        int size = CAN.parsePacket();
        if (size <= 0) break;

        RawMessage raw;
        raw.id          = (uint16_t)CAN.packetId();
        raw.packet_size = (uint16_t)(size > 8 ? 8 : size);  // clamp to 8

        // Fixed-size read – no heap allocation
        CAN.readBytes(raw.data, raw.packet_size);

        // Push into ring buffer
        // head only written here (main loop), tail only written in receive()
        // (also main loop) so no noInterrupts() needed for the buffer itself.
        uint8_t next = (rxBuffer.head + 1) % CAN_BUFFER_SIZE;
        if (next != rxBuffer.tail) {          // drop frame silently if full
            rxBuffer.messages[rxBuffer.head] = raw;
            rxBuffer.head = next;
        }
    }
}

// ── available – call every loop iteration ────────────────────────────────────
bool CANRP2040::available()
{
    if (rxPending) {
        // Clear flag BEFORE draining so an interrupt that fires during
        // drainMCP() is not lost – it will set rxPending again.
        rxPending = false;
        drainMCP();
    }
    return rxBuffer.head != rxBuffer.tail;
}

// ── receive – dequeue one message and convert to public Message type ──────────
/*
   This is the only place we call std::vector::assign() – it is in the
   main loop, never in an ISR, so heap allocation is safe here.
*/
Message CANRP2040::receive()
{
    Message empty;   // default: id=0, packet_size=0, data_field empty
    if (rxBuffer.head == rxBuffer.tail)
        return empty;

    // Defensive: guard tail update against any future ISR change
    noInterrupts();
    RawMessage raw = rxBuffer.messages[rxBuffer.tail];
    rxBuffer.tail  = (rxBuffer.tail + 1) % CAN_BUFFER_SIZE;
    interrupts();

    // Convert RawMessage → public Message (vector allocation happens here,
    // safely in the main loop)
    Message msg;
    msg.id          = raw.id;
    msg.packet_size = raw.packet_size;
    msg.data_field.assign(raw.data, raw.data + raw.packet_size);

    return msg;
}