# Firmware DBC Code Generation Guide

This guide explains how vehicle firmware should use `VUBR.dbc`.

Important rule:

```text
The MCU does not load or parse the .dbc file at runtime.
```

Instead, the DBC is used on the development PC as an input to a code generator.
The generator produces C source/header files. Firmware then includes those
generated files and uses the generated pack/unpack helpers.

## Concept

```text
LibCAN/include/VUBR.dbc
  -> DBC code generator on PC
  -> generated C files
  -> MCU firmware includes generated files
  -> firmware fills generated message structs
  -> generated pack() creates CAN bytes
  -> board-specific CAN driver sends ID + DLC + bytes
```

The generated DBC code handles:

- CAN frame IDs
- DLC/length constants
- C structs for each DBC message
- signal bit packing
- signal bit unpacking
- endian conversion
- signed/unsigned handling
- physical-to-raw encode helpers
- raw-to-physical decode helpers
- min/max range helper functions

The generated DBC code does **not** handle:

- CAN peripheral initialization
- CAN bus transmission
- CAN bus reception
- interrupts
- filtering
- error handling
- RTOS/task scheduling

Those remain the job of each board's existing CAN driver/library.

## Install Generator Tool

Install `cantools` on the development PC:

```powershell
python -m pip install cantools
```

Check it is available:

```powershell
python -m cantools --help
```

## Generate C Source From The DBC

From the `LibCAN` repository root:

```powershell
python -m cantools generate_c_source include\VUBR.dbc --output-directory generated
```

Expected output is typically:

```text
generated/
  vubr.h
  vubr.c
  vubr_fuzzer.c
  vubr_fuzzer.mk
```

For firmware, the important files are:

```text
generated/vubr.h
generated/vubr.c
```

The exact file names can vary depending on generator options and DBC name, but
with `VUBR.dbc` the generated API is expected to use the `vubr_` prefix.

## Use Generated Files In Firmware

Copy or include these generated files in the firmware build:

```text
vubr.h
vubr.c
```

For C++ firmware, compile `vubr.c` as C or C-compatible C++. The generated
header normally includes `extern "C"` guards when appropriate.

Example firmware layout:

```text
PowerDistributionFirmware/
  src/
    main.cpp
    BoardCAN.cpp
    BoardCAN.h
  lib/
    LibCAN/generated/vubr.c
    LibCAN/generated/vubr.h
```

Your board CAN driver should still expose something like:

```cpp
bool BoardCAN_send(uint32_t can_id, const uint8_t *data, uint8_t dlc);
```

## What Is Generated

For each DBC message, the generator creates a C struct.

For this DBC message:

```dbc
BO_ 1312 PowerDistributionStatus: 8 PowerDistribution
 SG_ pd_lv_battery_voltage : 0|16@1+ (0.01,0) [0|20] "V" Vector__XXX
 SG_ pd_branch_1_current : 16|16@1+ (0.01,0) [0|655.35] "A" Vector__XXX
 SG_ pd_branch_2_current : 32|16@1+ (0.01,0) [0|655.35] "A" Vector__XXX
 SG_ pd_branch_3_current : 48|16@1+ (0.01,0) [0|655.35] "A" Vector__XXX
```

The generated header will contain things similar to:

```c
#define VUBR_POWER_DISTRIBUTION_STATUS_FRAME_ID (0x520u)
#define VUBR_POWER_DISTRIBUTION_STATUS_LENGTH (8u)

struct vubr_power_distribution_status_t {
    uint16_t pd_lv_battery_voltage;
    uint16_t pd_branch_1_current;
    uint16_t pd_branch_2_current;
    uint16_t pd_branch_3_current;
};

int vubr_power_distribution_status_pack(
    uint8_t *dst_p,
    const struct vubr_power_distribution_status_t *src_p,
    size_t size
);

int vubr_power_distribution_status_unpack(
    struct vubr_power_distribution_status_t *dst_p,
    const uint8_t *src_p,
    size_t size
);

uint16_t vubr_power_distribution_status_pd_lv_battery_voltage_encode(double value);
double vubr_power_distribution_status_pd_lv_battery_voltage_decode(uint16_t value);
bool vubr_power_distribution_status_pd_lv_battery_voltage_is_in_range(uint16_t value);
```

Exact names may vary slightly with generator version, but the pattern is:

```text
vubr_<message_name>_pack()
vubr_<message_name>_unpack()
vubr_<message_name>_<signal_name>_encode()
vubr_<message_name>_<signal_name>_decode()
vubr_<message_name>_<signal_name>_is_in_range()
```

The generated `.c` file contains the implementation of these functions.

## Using DBC Names Instead Of Hex IDs

Yes: firmware code should use the generated DBC names instead of handwritten
hex CAN IDs whenever possible.

For example, do this:

```cpp
BoardCAN_send(
    VUBR_POWER_DISTRIBUTION_STATUS_FRAME_ID,
    data,
    VUBR_POWER_DISTRIBUTION_STATUS_LENGTH
);
```

instead of this:

```cpp
BoardCAN_send(0x520, data, 8);
```

The CAN bus still transmits the numeric identifier `0x520`. The difference is
that the firmware source code uses the generated readable constant:

```c
#define VUBR_POWER_DISTRIBUTION_STATUS_FRAME_ID (0x520u)
```

This constant comes from the DBC message name:

```dbc
BO_ 1312 PowerDistributionStatus: 8 PowerDistribution
```

The generator also creates readable length and name constants:

```c
#define VUBR_POWER_DISTRIBUTION_STATUS_LENGTH (8u)
#define VUBR_POWER_DISTRIBUTION_STATUS_NAME "PowerDistributionStatus"
```

Signal names are also generated:

```c
#define VUBR_POWER_DISTRIBUTION_STATUS_PD_LV_BATTERY_VOLTAGE_NAME "pd_lv_battery_voltage"
```

Recommended firmware rule:

```text
Use generated *_FRAME_ID and *_LENGTH constants in firmware.
Avoid writing raw hex CAN IDs directly in application code.
```

This gives the same benefit as the old `CAN_IDs.h`, but now the definitions are
generated from the DBC single source of truth.

## Sending A PowerDistribution Status Frame

Example C++ firmware code:

```cpp
#include <stdint.h>
#include <string.h>

#include "vubr.h"      // generated from VUBR.dbc
#include "BoardCAN.h"  // board-specific CAN driver

void sendPowerDistributionStatus(
    float lv_battery_voltage_v,
    float branch_1_current_a,
    float branch_2_current_a,
    float branch_3_current_a
) {
    struct vubr_power_distribution_status_t msg;
    uint8_t data[VUBR_POWER_DISTRIBUTION_STATUS_LENGTH];

    memset(&msg, 0, sizeof(msg));

    msg.pd_lv_battery_voltage =
        vubr_power_distribution_status_pd_lv_battery_voltage_encode(lv_battery_voltage_v);
    msg.pd_branch_1_current =
        vubr_power_distribution_status_pd_branch_1_current_encode(branch_1_current_a);
    msg.pd_branch_2_current =
        vubr_power_distribution_status_pd_branch_2_current_encode(branch_2_current_a);
    msg.pd_branch_3_current =
        vubr_power_distribution_status_pd_branch_3_current_encode(branch_3_current_a);

    int result = vubr_power_distribution_status_pack(data, &msg, sizeof(data));
    if (result < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_POWER_DISTRIBUTION_STATUS_FRAME_ID,
        data,
        VUBR_POWER_DISTRIBUTION_STATUS_LENGTH
    );
}
```

The encode helpers convert physical values into raw signal integers.

For example:

```text
pd_lv_battery_voltage scale = 0.01 V/raw
12.34 V -> raw 1234
```

## Sending Branch Currents

```cpp
void sendPowerDistributionCurrentsA(
    float branch_4_current_a,
    float branch_5_current_a,
    float branch_6_current_a,
    float branch_7_current_a
) {
    struct vubr_power_distribution_currents_a_t msg;
    uint8_t data[VUBR_POWER_DISTRIBUTION_CURRENTS_A_LENGTH];

    memset(&msg, 0, sizeof(msg));

    msg.pd_branch_4_current =
        vubr_power_distribution_currents_a_pd_branch_4_current_encode(branch_4_current_a);
    msg.pd_branch_5_current =
        vubr_power_distribution_currents_a_pd_branch_5_current_encode(branch_5_current_a);
    msg.pd_branch_6_current =
        vubr_power_distribution_currents_a_pd_branch_6_current_encode(branch_6_current_a);
    msg.pd_branch_7_current =
        vubr_power_distribution_currents_a_pd_branch_7_current_encode(branch_7_current_a);

    if (vubr_power_distribution_currents_a_pack(data, &msg, sizeof(data)) < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_POWER_DISTRIBUTION_CURRENTS_A_FRAME_ID,
        data,
        VUBR_POWER_DISTRIBUTION_CURRENTS_A_LENGTH
    );
}
```

## Sending Accelerometer Data

The accelerometer message is:

```dbc
BO_ 1315 PowerDistributionAccel: 8 PowerDistribution
 SG_ pd_accel_x : 0|16@1- (0.001,0) [-32.768|32.767] "g" Vector__XXX
 SG_ pd_accel_y : 16|16@1- (0.001,0) [-32.768|32.767] "g" Vector__XXX
 SG_ pd_accel_z : 32|16@1- (0.001,0) [-32.768|32.767] "g" Vector__XXX
```

Example:

```cpp
void sendPowerDistributionAccel(float accel_x_g, float accel_y_g, float accel_z_g)
{
    struct vubr_power_distribution_accel_t msg;
    uint8_t data[VUBR_POWER_DISTRIBUTION_ACCEL_LENGTH];

    memset(&msg, 0, sizeof(msg));

    msg.pd_accel_x =
        vubr_power_distribution_accel_pd_accel_x_encode(accel_x_g);
    msg.pd_accel_y =
        vubr_power_distribution_accel_pd_accel_y_encode(accel_y_g);
    msg.pd_accel_z =
        vubr_power_distribution_accel_pd_accel_z_encode(accel_z_g);

    if (vubr_power_distribution_accel_pack(data, &msg, sizeof(data)) < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_POWER_DISTRIBUTION_ACCEL_FRAME_ID,
        data,
        VUBR_POWER_DISTRIBUTION_ACCEL_LENGTH
    );
}
```

Because the signal scale is `0.001 g/raw`:

```text
1.000 g  -> raw 1000
-1.000 g -> raw -1000
```

## Avoiding Floating Point On MCU

If floating point is undesirable, firmware can fill raw values directly.

For example:

```cpp
uint16_t encodeCentivolts(float volts)
{
    return (uint16_t)(volts * 100.0f);
}

uint16_t encodeCentiAmps(float amps)
{
    return (uint16_t)(amps * 100.0f);
}

int16_t encodeMilliG(float accel_g)
{
    return (int16_t)(accel_g * 1000.0f);
}
```

Then assign raw values directly:

```cpp
msg.pd_lv_battery_voltage = encodeCentivolts(lv_battery_voltage_v);
msg.pd_branch_1_current = encodeCentiAmps(branch_1_current_a);
msg.pd_accel_x = encodeMilliG(accel_x_g);
```

The generated `pack()` still handles byte layout, endian conversion, and bit
placement.

## Receiving A Message On Firmware

If a firmware module receives a CAN frame and wants to decode only one message:

```cpp
void onCanFrame(uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    if (can_id == VUBR_POWER_DISTRIBUTION_STATUS_FRAME_ID) {
        struct vubr_power_distribution_status_t msg;

        if (vubr_power_distribution_status_unpack(&msg, data, dlc) < 0) {
            return;
        }

        double lv_voltage =
            vubr_power_distribution_status_pd_lv_battery_voltage_decode(
                msg.pd_lv_battery_voltage
            );

        // Use lv_voltage here.
    }
}
```

The firmware still chooses which frame IDs it cares about. It does not need to
process every message in the DBC.

## Recommended Build Practice

Generated files should be reproducible from the DBC. The preferred workflow is:

1. Edit `LibCAN/include/VUBR.dbc`.
2. Regenerate `LibCAN/generated/vubr.h` and `LibCAN/generated/vubr.c`.
3. Commit the DBC and generated files together.
4. Firmware repositories consume the generated files from `LibCAN`.

Regenerate after every DBC change:

```powershell
python -m cantools generate_c_source include\VUBR.dbc --output-directory generated
```
