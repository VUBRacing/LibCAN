#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_MCP2515.h"
#include "CAN_Library.h"

// ── Buffer size ────────────────────────────────────────────────────────────────
#define CAN_BUFFER_SIZE 32

// ── Internal fixed-size message for the ring buffer ───────────────────────────
// We cannot change the public Message struct (it uses std::vector).
// Internally we store raw bytes to avoid any heap allocation inside the ISR
// or the ring buffer. We convert to Message only in receive(), in the main loop.
struct RawMessage {
    uint16_t id          = 0;
    uint16_t packet_size = 0;
    uint8_t  data[8]     = {0};   // CAN max payload is always 8 bytes
};

// ── Ring buffer ────────────────────────────────────────────────────────────────
struct RingBuffer {
    RawMessage       messages[CAN_BUFFER_SIZE];
    volatile uint8_t head = 0;   // written in drainMCP() (main loop)
    volatile uint8_t tail = 0;   // written in receive()   (main loop)
};

// ── CANRP2040 ─────────────────────────────────────────────────────────────────
class CANRP2040 : public CANLibrary {
public:
    CANRP2040();

    bool    init()            override;
    void    send(Message msg) override;
    Message read()            override; 

    // available() does the real SPI drain if the ISR flagged a pending frame
    bool    available()       override;
    Message receive()         override;


private:
    static constexpr uint8_t CS_PIN       = 19u;
    static constexpr uint8_t interruptPin = 22u;  

    Adafruit_MCP2515 CAN;

    // ── ISR flag ───────────────────────────────────────────────────────────────
    // ISR does nothing except set this. All SPI work stays in the main loop.
    volatile bool rxPending = false;

    // ── Ring buffer ────────────────────────────────────────────────────────────
    RingBuffer rxBuffer;

    // ── ISR machinery ─────────────────────────────────────────────────────────
    static CANRP2040* instance;
    static void ISR();   // attached to INT pin – flag only, no SPI

    // SPI drain – called from available() in the main loop
    void drainMCP();
};