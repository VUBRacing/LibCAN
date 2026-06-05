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
  -> LibCAN/src/generated/vubr_can.c and vubr_can.h
  -> MCU firmware depends on LibCAN
  -> MCU firmware includes generated/vubr_can.h
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
python -m cantools generate_c_source include\VUBR.dbc --database-name vubr_can --output-directory src\generated --use-float --use-round --no-strict
```

Expected output:

```text
src/generated/
  vubr_can.h
  vubr_can.c
```

LibCAN also exposes a public wrapper header:

```text
include/generated/vubr_can.h
```

Firmware should include the wrapper:

```cpp
#include "generated/vubr_can.h"
```

With the command above, the generated API uses the `vubr_can_` prefix.

## Use Generated Files In Firmware Repositories

Do not copy generated files into every MCU repository. Keep them in LibCAN and
consume LibCAN as a PlatformIO dependency:

```ini
lib_deps =
  https://github.com/VUBRacing/LibCAN.git
```

For local development in the VUBR workspace, a firmware project can temporarily
point at the sibling checkout instead:

```ini
lib_deps =
  file://../LibCAN
```

Use the Git dependency for the committed/shared configuration. Use the local
path only when you need to test unpushed LibCAN changes.

For C++ firmware, PlatformIO compiles `src/generated/vubr_can.c` as part of the
LibCAN library. The generated header includes `extern "C"` guards.

Example firmware layout:

```text
pedalbox/
  platformio.ini       # depends on LibCAN
  src/main.cpp
  include/Gaspedal.hpp # includes generated/vubr_can.h
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
#define VUBR_CAN_POWER_DISTRIBUTION_STATUS_FRAME_ID (0x520u)
#define VUBR_CAN_POWER_DISTRIBUTION_STATUS_LENGTH (8u)

struct vubr_can_power_distribution_status_t {
    uint16_t pd_lv_battery_voltage;
    uint16_t pd_branch_1_current;
    uint16_t pd_branch_2_current;
    uint16_t pd_branch_3_current;
};

int vubr_can_power_distribution_status_pack(
    uint8_t *dst_p,
    const struct vubr_can_power_distribution_status_t *src_p,
    size_t size
);

int vubr_can_power_distribution_status_unpack(
    struct vubr_can_power_distribution_status_t *dst_p,
    const uint8_t *src_p,
    size_t size
);

uint16_t vubr_can_power_distribution_status_pd_lv_battery_voltage_encode(float value);
float vubr_can_power_distribution_status_pd_lv_battery_voltage_decode(uint16_t value);
bool vubr_can_power_distribution_status_pd_lv_battery_voltage_is_in_range(uint16_t value);
```

Exact names may vary slightly with generator version, but the pattern is:

```text
vubr_can_<message_name>_pack()
vubr_can_<message_name>_unpack()
vubr_can_<message_name>_<signal_name>_encode()
vubr_can_<message_name>_<signal_name>_decode()
vubr_can_<message_name>_<signal_name>_is_in_range()
```

The generated `.c` file contains the implementation of these functions.

## Generated Function Data Flow

The generated API separates byte layout from physical scaling. This is the most
important mental model when using the generated code.

For receiving:

```text
CAN bytes -> unpack() -> raw struct fields -> decode() -> physical values
```

For sending:

```text
physical values -> encode() -> raw struct fields -> pack() -> CAN bytes
```

The difference between `unpack()` and `decode()` is:

| Function | Input | Output | Purpose |
| --- | --- | --- | --- |
| `unpack()` | CAN payload bytes | raw struct fields | bit slicing, endian handling, signed/unsigned extraction |
| `decode()` | one raw signal value | physical value | DBC scale/offset conversion |
| `encode()` | one physical value | raw signal value | inverse DBC scale/offset conversion |
| `pack()` | raw struct fields | CAN payload bytes | bit packing, endian handling |

Example:

```dbc
SG_ inverter_t_igbt : 7|16@0- (0.1,0) [-55|200] "degC" Vector__XXX
```

This signal has scale `0.1` and offset `0`, so:

```text
physical_temperature_degC = raw_integer * 0.1
```

If `unpack()` returns raw `450`, then `decode(450)` returns `45.0 degC`.

Generated structs store raw bus values, not physical values. That is why
firmware normally calls `decode()` after `unpack()`, and calls `encode()` before
`pack()`.

## Using DBC Names Instead Of Hex IDs

Yes: firmware code should use the generated DBC names instead of handwritten
hex CAN IDs whenever possible.

For example, do this:

```cpp
BoardCAN_send(
    VUBR_CAN_POWER_DISTRIBUTION_STATUS_FRAME_ID,
    data,
    VUBR_CAN_POWER_DISTRIBUTION_STATUS_LENGTH
);
```

instead of this:

```cpp
BoardCAN_send(0x520, data, 8);
```

The CAN bus still transmits the numeric identifier `0x520`. The difference is
that the firmware source code uses the generated readable constant:

```c
#define VUBR_CAN_POWER_DISTRIBUTION_STATUS_FRAME_ID (0x520u)
```

This constant comes from the DBC message name:

```dbc
BO_ 1312 PowerDistributionStatus: 8 PowerDistribution
```

The generator also creates readable length and name constants:

```c
#define VUBR_CAN_POWER_DISTRIBUTION_STATUS_LENGTH (8u)
#define VUBR_CAN_POWER_DISTRIBUTION_STATUS_NAME "PowerDistributionStatus"
```

Signal names are also generated:

```c
#define VUBR_CAN_POWER_DISTRIBUTION_STATUS_PD_LV_BATTERY_VOLTAGE_NAME "pd_lv_battery_voltage"
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
#include "generated/vubr_can.h"  // generated from VUBR.dbc, exposed by LibCAN
#include "BoardCAN.h"  // board-specific CAN driver

void sendPowerDistributionStatus(
    float lv_battery_voltage_v,
    float branch_1_current_a,
    float branch_2_current_a,
    float branch_3_current_a
) {
    struct vubr_can_power_distribution_status_t msg;
    uint8_t data[VUBR_CAN_POWER_DISTRIBUTION_STATUS_LENGTH];

    vubr_can_power_distribution_status_init(&msg);

    msg.pd_lv_battery_voltage =
        vubr_can_power_distribution_status_pd_lv_battery_voltage_encode(lv_battery_voltage_v);
    msg.pd_branch_1_current =
        vubr_can_power_distribution_status_pd_branch_1_current_encode(branch_1_current_a);
    msg.pd_branch_2_current =
        vubr_can_power_distribution_status_pd_branch_2_current_encode(branch_2_current_a);
    msg.pd_branch_3_current =
        vubr_can_power_distribution_status_pd_branch_3_current_encode(branch_3_current_a);

    int result = vubr_can_power_distribution_status_pack(data, &msg, sizeof(data));
    if (result < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_CAN_POWER_DISTRIBUTION_STATUS_FRAME_ID,
        data,
        VUBR_CAN_POWER_DISTRIBUTION_STATUS_LENGTH
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
    struct vubr_can_power_distribution_currents_a_t msg;
    uint8_t data[VUBR_CAN_POWER_DISTRIBUTION_CURRENTS_A_LENGTH];

    vubr_can_power_distribution_currents_a_init(&msg);

    msg.pd_branch_4_current =
        vubr_can_power_distribution_currents_a_pd_branch_4_current_encode(branch_4_current_a);
    msg.pd_branch_5_current =
        vubr_can_power_distribution_currents_a_pd_branch_5_current_encode(branch_5_current_a);
    msg.pd_branch_6_current =
        vubr_can_power_distribution_currents_a_pd_branch_6_current_encode(branch_6_current_a);
    msg.pd_branch_7_current =
        vubr_can_power_distribution_currents_a_pd_branch_7_current_encode(branch_7_current_a);

    if (vubr_can_power_distribution_currents_a_pack(data, &msg, sizeof(data)) < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_CAN_POWER_DISTRIBUTION_CURRENTS_A_FRAME_ID,
        data,
        VUBR_CAN_POWER_DISTRIBUTION_CURRENTS_A_LENGTH
    );
}
```

## Sending DTI Inverter Commands From Pedalbox

Pedalbox should use the generated DBC command frames instead of the old
`LibInvertor` `DriveEnable()` and `SetACCurrent()` helpers.

For the current DTI inverter node ID `4`, the command frames are:

| Register | CAN ID | Generated frame constant | Length |
| --- | --- | --- | --- |
| `0x01` | `0x024` | `VUBR_CAN_DTI_SET_AC_CURRENT_FRAME_ID` | `8` |
| `0x02` | `0x044` | `VUBR_CAN_DTI_SET_BRAKE_CURRENT_FRAME_ID` | `8` |
| `0x03` | `0x064` | `VUBR_CAN_DTI_SET_ERPM_FRAME_ID` | `4` |
| `0x04` | `0x084` | `VUBR_CAN_DTI_SET_POSITION_FRAME_ID` | `2` |
| `0x05` | `0x0A4` | `VUBR_CAN_DTI_SET_RELATIVE_CURRENT_FRAME_ID` | `2` |
| `0x06` | `0x0C4` | `VUBR_CAN_DTI_SET_RELATIVE_BRAKE_CURRENT_FRAME_ID` | `2` |
| `0x07` | `0x0E4` | `VUBR_CAN_DTI_SET_DIGITAL_OUTPUT_FRAME_ID` | `8` |
| `0x08` | `0x104` | `VUBR_CAN_DTI_SET_MAX_AC_CURRENT_FRAME_ID` | `2` |
| `0x09` | `0x124` | `VUBR_CAN_DTI_SET_MAX_AC_BRAKE_CURRENT_FRAME_ID` | `2` |
| `0x0A` | `0x144` | `VUBR_CAN_DTI_SET_MAX_DC_CURRENT_FRAME_ID` | `2` |
| `0x0B` | `0x164` | `VUBR_CAN_DTI_SET_MAX_DC_BRAKE_CURRENT_FRAME_ID` | `2` |
| `0x0C` | `0x184` | `VUBR_CAN_DTI_SET_DRIVE_ENABLE_FRAME_ID` | `2` |

Example C++ firmware code:

```cpp
#include "generated/vubr_can.h"
#include "BoardCAN.h"

void sendDtiDriveEnable(bool enabled)
{
    struct vubr_can_dti_set_drive_enable_t msg;
    uint8_t data[VUBR_CAN_DTI_SET_DRIVE_ENABLE_LENGTH];

    vubr_can_dti_set_drive_enable_init(&msg);
    msg.inverter_cmd_drive_enable =
        vubr_can_dti_set_drive_enable_inverter_cmd_drive_enable_encode(enabled ? 1.0f : 0.0f);

    if (vubr_can_dti_set_drive_enable_pack(data, &msg, sizeof(data)) < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_CAN_DTI_SET_DRIVE_ENABLE_FRAME_ID,
        data,
        VUBR_CAN_DTI_SET_DRIVE_ENABLE_LENGTH
    );
}

void sendDtiTargetAcCurrent(float current_a)
{
    struct vubr_can_dti_set_ac_current_t msg;
    uint8_t data[VUBR_CAN_DTI_SET_AC_CURRENT_LENGTH];

    vubr_can_dti_set_ac_current_init(&msg);
    msg.inverter_cmd_target_ac_current =
        vubr_can_dti_set_ac_current_inverter_cmd_target_ac_current_encode(current_a);

    if (vubr_can_dti_set_ac_current_pack(data, &msg, sizeof(data)) < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_CAN_DTI_SET_AC_CURRENT_FRAME_ID,
        data,
        VUBR_CAN_DTI_SET_AC_CURRENT_LENGTH
    );
}
```

These constants are generated from the DBC. If the DTI inverter node ID changes,
the DBC IDs must be recomputed and LibCAN regenerated.

The current pedalbox firmware keeps the old DTI command-frame padding behavior:
after the generated packer writes the real signal bytes, unused command bytes
are filled with `0xFF`. The DBC defines the signal layout and frame length, but
it does not express preferred filler values for unused bytes.

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
    struct vubr_can_power_distribution_accel_t msg;
    uint8_t data[VUBR_CAN_POWER_DISTRIBUTION_ACCEL_LENGTH];

    vubr_can_power_distribution_accel_init(&msg);

    msg.pd_accel_x =
        vubr_can_power_distribution_accel_pd_accel_x_encode(accel_x_g);
    msg.pd_accel_y =
        vubr_can_power_distribution_accel_pd_accel_y_encode(accel_y_g);
    msg.pd_accel_z =
        vubr_can_power_distribution_accel_pd_accel_z_encode(accel_z_g);

    if (vubr_can_power_distribution_accel_pack(data, &msg, sizeof(data)) < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_CAN_POWER_DISTRIBUTION_ACCEL_FRAME_ID,
        data,
        VUBR_CAN_POWER_DISTRIBUTION_ACCEL_LENGTH
    );
}
```

Because the signal scale is `0.001 g/raw`:

```text
1.000 g  -> raw 1000
-1.000 g -> raw -1000
```

## Sending Indexed SDC Status

The shutdown circuit uses one shared indexed status frame:

```dbc
BO_ 772 SdcStatus: 2 Vector__XXX
 SG_ sdc_index : 0|8@1+ (1,0) [0|255] "" Vector__XXX
 SG_ sdc_closed : 8|1@1+ (1,0) [0|1] "" Vector__XXX
```

Current index map:

| Index | Generated choice constant | Sender |
| --- | --- | --- |
| `1` | `VUBR_CAN_SDC_STATUS_SDC_INDEX_DASHBOARD_CHOICE` | Dashboard |
| `2` | `VUBR_CAN_SDC_STATUS_SDC_INDEX_PEDALBOX_CHOICE` | Pedalbox |
| `3` | `VUBR_CAN_SDC_STATUS_SDC_INDEX_AMS_CHOICE` | AMS |

Each module sends the same frame ID, but with its own fixed index. The PC
dashboard keeps a per-index state map and displays the lowest index that reports
`sdc_closed == 0` as the earliest known open point.

The dashboard is a special case. It owns the SDC decision/display logic and has
its own local SDC measurement, so it does not need to transmit index `1` on the
CAN bus and then receive its own message back. Instead, dashboard firmware builds
the same generated `SdcStatus` frame in memory and sends that CAN-shaped frame
directly over LoRa for logging:

```text
dashboard local SDC input
  -> generated SdcStatus payload with index 1
  -> LoRa telemetry frame
  -> PC raw log / parsed log / GUI
```

Pedalbox and AMS do transmit their indexed SDC frames on CAN. The dashboard
receives those frames, updates its SDC display logic, and forwards the received
frames over LoRa like any other CAN frame.

Dashboard firmware rule:

```text
Every frame received from CAN is forwarded to LoRa.
Every frame generated locally by dashboard and sent on CAN is also mirrored to LoRa immediately.
Dashboard SDC index 1 is not sent on CAN; it is built and sent to LoRa only.
```

AMS index `3` reports `closed` only when both AMS-side SDC measurements are
closed:

```cpp
sdc_closed = SDC_IN && SDC_OUT;
```

This means:

- `SDC_IN` confirms the shutdown loop reaches the AMS input.
- `SDC_OUT` confirms the AMS output/relay side is still closed.
- if either is false, the AMS reports index `3` open.

The dashboard combines the indexed reports in loop order:

```text
index 1 dashboard
index 2 pedalbox
index 3 AMS
```

The earliest known open point is the lowest index currently reporting open.

Example sender:

```cpp
void sendSdcStatus(uint8_t index, bool sdc_closed)
{
    struct vubr_can_sdc_status_t msg;
    uint8_t data[VUBR_CAN_SDC_STATUS_LENGTH] = {0};

    vubr_can_sdc_status_init(&msg);
    msg.sdc_index = vubr_can_sdc_status_sdc_index_encode(index);
    msg.sdc_closed = vubr_can_sdc_status_sdc_closed_encode(sdc_closed ? 1.0f : 0.0f);

    if (vubr_can_sdc_status_pack(data, &msg, sizeof(data)) < 0) {
        return;
    }

    BoardCAN_send(
        VUBR_CAN_SDC_STATUS_FRAME_ID,
        data,
        VUBR_CAN_SDC_STATUS_LENGTH
    );
}
```

## Receiving DTI Temperatures

`DtiTemperatures` is a frame sent by the DTI inverter. Most firmware modules
should only receive this frame, unpack it, decode the signals they care about,
and store the physical values in module state.

DBC definition:

```dbc
BO_ 1092 DtiTemperatures: 8 Inverter
 SG_ inverter_t_igbt : 7|16@0- (0.1,0) [-55|200] "degC" Vector__XXX
 SG_ inverter_t_motor : 23|16@0- (0.1,0) [-55|200] "degC" Vector__XXX
 SG_ inverter_fault_code : 39|8@0+ (1,0) [0|255] "" Vector__XXX
```

Generated constants:

```cpp
VUBR_CAN_DTI_TEMPERATURES_FRAME_ID  // 0x444
VUBR_CAN_DTI_TEMPERATURES_LENGTH    // 8
```

Generated struct:

```cpp
struct vubr_can_dti_temperatures_t {
    int16_t inverter_t_igbt;      // raw, scale 0.1 degC/raw
    int16_t inverter_t_motor;     // raw, scale 0.1 degC/raw
    uint8_t inverter_fault_code;  // raw, scale 1
};
```

Recommended local application struct:

```cpp
struct DtiTemperatures {
    float igbt_deg_c = 0.0f;
    float motor_deg_c = 0.0f;
    uint8_t fault_code = 0;
    bool valid = false;
};
```

Reusable decoder:

```cpp
#include "generated/vubr_can.h"

bool decodeDtiTemperatures(
    const uint8_t *data,
    uint8_t dlc,
    DtiTemperatures &out)
{
    if (dlc < VUBR_CAN_DTI_TEMPERATURES_LENGTH) {
        return false;
    }

    struct vubr_can_dti_temperatures_t msg;
    vubr_can_dti_temperatures_init(&msg);

    if (vubr_can_dti_temperatures_unpack(&msg, data, dlc) < 0) {
        return false;
    }

    out.igbt_deg_c =
        vubr_can_dti_temperatures_inverter_t_igbt_decode(msg.inverter_t_igbt);

    out.motor_deg_c =
        vubr_can_dti_temperatures_inverter_t_motor_decode(msg.inverter_t_motor);

    out.fault_code = msg.inverter_fault_code;
    out.valid = true;

    return true;
}
```

Use it from the board CAN receive callback:

```cpp
DtiTemperatures dtiTemps;

void onCanFrame(uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    if (can_id != VUBR_CAN_DTI_TEMPERATURES_FRAME_ID) {
        return;
    }

    if (!decodeDtiTemperatures(data, dlc, dtiTemps)) {
        return;
    }

    if (dtiTemps.igbt_deg_c > 80.0f) {
        // React to high inverter temperature.
    }
}
```

The function returns `bool` because decoding can fail. The decoded values are
written into `out`, which is passed by C++ reference.

### Why `const uint8_t *data` But `DtiTemperatures &out`

The CAN payload is a byte buffer. Most CAN drivers provide payload bytes as an
array or pointer, so the decoder accepts:

```cpp
const uint8_t *data
```

`const` means the function reads the CAN bytes but does not modify them.

The output object is passed by reference:

```cpp
DtiTemperatures &out
```

This lets the function fill the caller's object without copying it and without
allowing a null pointer. A pointer-based output is also valid:

```cpp
bool decodeDtiTemperatures(const uint8_t *data, uint8_t dlc, DtiTemperatures *out)
```

but then the function must check `out != nullptr`, and callers must pass
`&temps`. For C++ firmware, a reference is usually clearer for required output
objects.

### What `init()` Does

Generated init functions clear the message struct and apply generated defaults.
For this message, the implementation is effectively:

```cpp
memset(msg_p, 0, sizeof(struct vubr_can_dti_temperatures_t));
```

Use:

```cpp
struct vubr_can_dti_temperatures_t msg;
vubr_can_dti_temperatures_init(&msg);
```

before packing, and preferably before unpacking as a consistent pattern. For
this specific message, `unpack()` fills every field, so `init()` is not strictly
required before unpacking, but it is cheap and keeps receive code predictable.

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

## Receiving A Generic Message On Firmware

If a firmware module receives a CAN frame and wants to decode only one message,
the pattern is always:

1. Compare `can_id` with the generated `*_FRAME_ID`.
2. Check `dlc` against the generated `*_LENGTH`.
3. Create the generated message struct.
4. Call the generated `*_init()` helper.
5. Call the generated `*_unpack()` helper.
6. Call generated `*_decode()` helpers for the signals you need.
7. Store/use the physical values in your firmware logic.

```cpp
void onCanFrame(uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    if (can_id == VUBR_CAN_POWER_DISTRIBUTION_STATUS_FRAME_ID) {
        struct vubr_can_power_distribution_status_t msg;

        vubr_can_power_distribution_status_init(&msg);

        if (vubr_can_power_distribution_status_unpack(&msg, data, dlc) < 0) {
            return;
        }

        float lv_voltage =
            vubr_can_power_distribution_status_pd_lv_battery_voltage_decode(
                msg.pd_lv_battery_voltage
            );

        // Use lv_voltage here.
    }
}
```

The firmware still chooses which frame IDs it cares about. It does not need to
process every message in the DBC.

### Using `std::vector<uint8_t>` Payloads

Some VUBR firmware CAN wrappers store payload bytes in a `std::vector<uint8_t>`,
for example:

```cpp
Message read = myCAN.receive();
// read.data_field is std::vector<uint8_t>
```

The generated unpack functions expect a pointer to contiguous bytes:

```cpp
const uint8_t *src_p
```

Use `.data()` to pass the vector's underlying byte buffer:

```cpp
if (read.id == VUBR_CAN_SDC_STATUS_FRAME_ID &&
    read.packet_size >= VUBR_CAN_SDC_STATUS_LENGTH &&
    read.data_field.size() >= VUBR_CAN_SDC_STATUS_LENGTH) {

    struct vubr_can_sdc_status_t msg;
    vubr_can_sdc_status_init(&msg);

    if (vubr_can_sdc_status_unpack(&msg, read.data_field.data(), read.packet_size) < 0) {
        return;
    }

    uint8_t index = msg.sdc_index;
    bool closed = vubr_can_sdc_status_sdc_closed_decode(msg.sdc_closed) >= 0.5f;

    // Use index and closed here.
}
```

Important checks:

- check the CAN ID before unpacking
- check both the reported DLC/packet size and the vector size
- only call `.data()` after confirming the vector contains enough bytes

For sending, pack into a fixed array first, then copy the bytes into the vector
owned by the board CAN wrapper:

```cpp
uint8_t data[VUBR_CAN_SDC_STATUS_LENGTH] = {0};

if (vubr_can_sdc_status_pack(data, &msg, sizeof(data)) < 0) {
    return;
}

Message frame;
frame.id = VUBR_CAN_SDC_STATUS_FRAME_ID;
frame.packet_size = VUBR_CAN_SDC_STATUS_LENGTH;
frame.data_field.assign(data, data + VUBR_CAN_SDC_STATUS_LENGTH);
```

## Recommended Build Practice

Generated files should be reproducible from the DBC. The preferred workflow is:

1. Edit `LibCAN/include/VUBR.dbc`.
2. Regenerate `LibCAN/src/generated/vubr_can.h` and `LibCAN/src/generated/vubr_can.c`.
3. Commit the DBC and generated files together.
4. Firmware repositories consume the generated files from `LibCAN`.

Regenerate after every DBC change:

```powershell
python -m cantools generate_c_source include\VUBR.dbc --database-name vubr_can --output-directory src\generated --use-float --use-round --no-strict
```

