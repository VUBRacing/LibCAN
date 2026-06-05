# VUBR DBC Notes

This document explains `VUBR.dbc` in normal engineering language. Keep the DBC itself parser-friendly; put migration notes, assumptions, and open questions here.

## DBC Syntax Quick Reference

Message definition:

```dbc
BO_ <decimal_can_id> <MessageName>: <dlc_bytes> <transmitter>
```

Signal definition:

```dbc
SG_ <signal_name> : <start_bit>|<bit_length>@<byte_order><signedness> (<scale>,<offset>) [<min>|<max>] "<unit>" <receiver>
```

Important fields:

- `@1` means little-endian / Intel.
- `@0` means big-endian / Motorola.
- `+` means unsigned.
- `-` means signed.
- Physical value is `raw * scale + offset`.
- Multiplexed frames use `M` for the selector signal and `m<value>` for signals that are present only when the selector has that value.

## Global Migration Notes

This DBC is a draft migration file. Several layouts and scale factors are inferred from the old LoRa GUI and CSV plotting code. The PC dashboard can use it now, but final firmware integration should confirm every signal width, signedness, byte order, scale, and unit.


## Message Notes

### `BO_ 1 Error`

One byte error code. Event-based message.

Open item: define complete error-code value table.

### `BO_ 6 PrechargeStatus`

Register-multiplexed precharge frame.

Known selector values:

- `0x20`: precharge status
- `0x30`: battery-side AIR voltage
- `0x40`: tractive-system-side voltage

Voltage values are currently one byte and should be interpreted as one segment voltage, not full pack voltage. Full pack voltage is approximately `3 * segment_voltage`.

Open item: confirm whether the old firmware truly sends segment voltage here or whether the signal should become wider for full pack voltage.

### `BO_ 18 AppsAnalogValues`

Two raw APPS analog channels, currently 16-bit little-endian raw values.

Open item: confirm physical conversion to pedal percentage and plausibility range.

### `BO_ 21 DashboardInfo`

One byte dashboard event code.

Current value table contains R2D and steering wheel switch events.

### DTI HV500 inverter, standard ID map

The new DTI/HV500 inverter uses message IDs composed from a 6-bit register/page and a 5-bit inverter node ID:

```cpp
can_id = ((reg_id & 0x3F) << 5) | (inverter_id & 0x1F)
```

For the current DTI vendor DBC, the inverter node ID is `4`, so the standard-ID telemetry frames are already the full bus CAN IDs:

- `0x3E4` (`996`) `DtiTargetIq`
- `0x404` (`1028`) `DtiErpmDutyVoltage`
- `0x424` (`1060`) `DtiAcDcCurrent`
- `0x444` (`1092`) `DtiTemperatures`
- `0x464` (`1124`) `DtiFoc`
- `0x484` (`1156`) `DtiMisc`
- `0x4A4` (`1188`) `DtiMinMaxAcCurrent`
- `0x4C4` (`1220`) `DtiMinMaxDcCurrent`

Important: these IDs assume `inverter_id = 4`. The DBC cannot express the ID formula dynamically; it only stores concrete arbitration IDs. Therefore, if the DTI inverter node ID changes, the DTI message IDs in `VUBR.dbc` must be recomputed and checked for conflicts.

Example:

```text
reg_id = 0x20
inverter_id = 4
can_id = (0x20 << 5) | 0x04 = 0x404
```

If the inverter were changed to node ID `19` decimal:

```text
reg_id = 0x20
inverter_id = 19 = 0x13
can_id = (0x20 << 5) | 0x13 = 0x413
```

That would collide with the current BMS message `BmsSegment2Temperatures` at `0x413`, so node ID changes are not just a DTI setting; they affect the whole CAN architecture.

The DTI payloads are Motorola/big-endian (`@0`) and already include physical scaling from the vendor DBC. Canonical GUI-facing signal names are reused where practical:

- `inverter_speed_actual`: DTI actual ERPM
- `inverter_vdc_bus`: DTI actual DC input voltage
- `inverter_iq_command_ramp`: DTI target Iq
- `inverter_iq_actual`: DTI FOC Iq
- `inverter_t_igbt`: DTI controller/semiconductor temperature
- `inverter_t_motor`: DTI motor temperature

Extra DTI-only signals include AC/DC current, duty cycle, fault code, limit flags, throttle/brake, drive-enable state, and configured/available AC/DC current limits.

Open item: confirm whether dashboard speed should display ERPM directly or convert ERPM to mechanical RPM using motor pole-pair count.

#### DTI command frames

Pedalbox now uses generated DBC code from LibCAN to transmit DTI command frames
instead of calling the old `LibInvertor` helper directly. For inverter node ID
`4`, the concrete command IDs are:

| Register | CAN ID | DBC message | Purpose |
| --- | --- | --- | --- |
| `0x01` | `0x024` (`36`) | `DtiSetAcCurrent` | target AC current |
| `0x02` | `0x044` (`68`) | `DtiSetBrakeCurrent` | target brake current |
| `0x03` | `0x064` (`100`) | `DtiSetErpm` | target speed in ERPM |
| `0x04` | `0x084` (`132`) | `DtiSetPosition` | target motor position |
| `0x05` | `0x0A4` (`164`) | `DtiSetRelativeCurrent` | relative current command |
| `0x06` | `0x0C4` (`196`) | `DtiSetRelativeBrakeCurrent` | relative brake current command |
| `0x07` | `0x0E4` (`228`) | `DtiSetDigitalOutput` | digital output command bits |
| `0x08` | `0x104` (`260`) | `DtiSetMaxAcCurrent` | configured max AC current |
| `0x09` | `0x124` (`292`) | `DtiSetMaxAcBrakeCurrent` | configured max AC brake current |
| `0x0A` | `0x144` (`324`) | `DtiSetMaxDcCurrent` | configured max DC current |
| `0x0B` | `0x164` (`356`) | `DtiSetMaxDcBrakeCurrent` | configured max DC brake current |
| `0x0C` | `0x184` (`388`) | `DtiSetDriveEnable` | periodic drive-enable command |

These IDs are also produced by the DTI formula:

```cpp
can_id = ((reg_id & 0x3F) << 5) | (inverter_id & 0x1F)
```

Firmware should use the generated `VUBR_CAN_DTI_SET_*_FRAME_ID` constants,
not handwritten hex IDs.

No ID overlap exists for the current node ID `4` mapping.

### `BO_ 768 UserStatus`

Register-multiplexed VCU/user frame.

Known selector values:

- `0x01`: vehicle state
- `0x02`: APPS and brake flag

The vehicle state value table is currently copied from the old GUI.

### `BO_ 770 BRAKESENS1_AVAL_ID`

Brake pressure sensor 1.

Current assumption: 16-bit little-endian raw value.

Open item: confirm pressure units and calibration.

### `BO_ 771 BRAKESENS2_AVAL_ID`

Brake pressure sensor 2.

Current assumption: 16-bit little-endian raw value.

Open item: confirm pressure units and calibration.

### `BO_ 772 SdcStatus`

Indexed shutdown-circuit measurement frame.

This frame is intentionally shared by multiple modules. Each sender transmits
the same CAN ID with:

- `sdc_index`: measurement point in loop order
- `sdc_closed`: `1` when that point still sees the closed 12 V SDC loop, `0`
  when the loop is open at or before that point

Current index map:

| Index | Sender | Meaning |
| --- | --- | --- |
| `1` | Dashboard | Earliest/current first dashboard-side SDC measurement |
| `2` | Pedalbox | Pedalbox SDC measurement |
| `3` | AMS | AMS input measurement, before the AMS relay |
| `4` | AMS | AMS output/effective measurement, after the AMS relay |

Dashboard index `1` is not transmitted onto the CAN bus by the dashboard
firmware. The dashboard owns the local SDC measurement and the SDC display
logic, so it builds the same `SdcStatus` payload and sends it directly over LoRa
for PC logging. This keeps the raw telemetry log complete without requiring the
dashboard to listen to its own CAN transmission.

Pedalbox index `2`, AMS input index `3`, and AMS output index `4` are normal
CAN messages. The dashboard receives them, updates its SDC status logic, and
forwards the received CAN frames over LoRa.

AMS index `3` is the direct `SDC_IN` measurement. AMS index `4` currently uses:

```cpp
SDC_IN && SDC_OUT
```

where `SDC_IN` means the SDC loop reaches the AMS input and `SDC_OUT` is the AMS
internal output/relay state. If index `3` is closed and index `4` is open, the
AMS is the earliest known module opening the SDC.

The PC dashboard keeps a per-index state map. The earliest open point is the
lowest known index where `sdc_closed == 0`.

This replaces the old max-hold idea. The current state can change back to
closed without power-cycling, and the raw log still preserves the exact sequence
for later fault analysis.

### `BO_ 773 PedalboxStatus`

Pedalbox status frame.

Current signals:

- `bspd_hard_braking`: boolean copy of the BSPD hard-braking output read by the
  pedalbox

The BSPD hard-braking signal means the BSPD sees the hard-braking condition
active. In project terms this is approximately:

```text
brake pressure > 30 bar and tractive power > 5 kW
```

The exact thresholds are implemented in the BSPD hardware/firmware, not in the
DBC. A high value is useful telemetry because if this condition remains active
longer than the BSPD trip delay, approximately 500 ms, the BSPD should open the
SDC. The PC GUI only displays/logs the state; it must not be the safety logic.

### `BO_ 784 MasterStatus`

Accumulator/AMS status frame sent by the AMS firmware.

Current signals:

- `ts_current`: 16-bit little-endian signed, `raw * 0.1 A`
- `ams_temperature`: 16-bit little-endian signed, `raw * 0.1 degC`
- `ams_error_flags`: 8-bit bitfield
- `sdc_in`: 1-bit boolean
- `sdc_out`: 1-bit boolean

This replaces the old 10-bit raw compatibility layout. The Python decoder still
keeps a narrow legacy correction for old logs/firmware that sent:

- `ts_current_raw = (byte1 << 2) | (byte0 & 0b11)`
- `ams_temperature_raw = (byte3 << 2) | (byte2 & 0b11)`

New firmware should send the physical DBC signals above. The SOC estimator uses
`dbc.ts_current` directly when present and falls back to the old raw conversion
only for legacy logs.

Assumption for PC model: discharge current is positive. The car currently has no regenerative braking.

Open items:

- confirm final current sensor calibration in AMS firmware
- define the `ams_error_flags` bit table

### `BO_ 1312 PowerDistributionStatus`

PowerDistribution low-voltage monitoring frame.

Current layout:

- `pd_lv_battery_voltage`: 16-bit little-endian unsigned, `raw * 0.01 V`
- `pd_branch_1_current`: 16-bit little-endian unsigned, `raw * 0.01 A`
- `pd_branch_2_current`: 16-bit little-endian unsigned, `raw * 0.01 A`
- `pd_branch_3_current`: 16-bit little-endian unsigned, `raw * 0.01 A`

This message intentionally sends measured values only. It does not send total
current, fuse-blown flags, or board-level status. Those values are derived on
the PC so the logic can evolve without changing firmware.

### `BO_ 1313 PowerDistributionCurrentsA`

PowerDistribution branch-current frame.

Current layout:

- `pd_branch_4_current`: 16-bit little-endian unsigned, `raw * 0.01 A`
- `pd_branch_5_current`: 16-bit little-endian unsigned, `raw * 0.01 A`
- `pd_branch_6_current`: 16-bit little-endian unsigned, `raw * 0.01 A`
- `pd_branch_7_current`: 16-bit little-endian unsigned, `raw * 0.01 A`

### `BO_ 1314 PowerDistributionCurrentsB`

PowerDistribution branch-current frame.

Current layout:

- `pd_branch_8_current`: 16-bit little-endian unsigned, `raw * 0.01 A`
- `pd_branch_9_current`: 16-bit little-endian unsigned, `raw * 0.01 A`
- bytes 4-7 are reserved for future PD measurements

The DBC keeps DLC at 8 for consistency with the other PD messages and to leave
future expansion space.

PC-derived values:

- `pd_total_current = sum(pd_branch_1_current ... pd_branch_9_current)`
- branch display labels come from GUI/config mapping, not from the DBC
- global PD status is computed on the PC
- suspected fuse issues should use branch-specific expected-on logic, not only
  `current == 0`, because some branches can legitimately be off

Open items:

- confirm exact current sensor scale and offset
- confirm branch names for fuses 1-9
- define per-branch minimum-current thresholds when a branch is expected on
- define overcurrent warning thresholds per branch

### `BO_ 1315 PowerDistributionAccel`

PowerDistribution accelerometer frame.

Current layout:

- `pd_accel_x`: 16-bit little-endian signed, `raw * 0.001 g`
- `pd_accel_y`: 16-bit little-endian signed, `raw * 0.001 g`
- `pd_accel_z`: 16-bit little-endian signed, `raw * 0.001 g`
- bytes 6-7 are reserved for future accelerometer-related data

This frame is separated from the LV/fuse-current messages so accelerometer data
can be sent at a higher rate without forcing the electrical monitoring values to
update at the same rate.

Recommended rates:

- PD voltage/current frames: 5-10 Hz
- PD accelerometer frame: 50-100 Hz

Open items:

- confirm accelerometer axis orientation in the car
- confirm whether values should be raw sensor axes or transformed vehicle axes
- decide whether bytes 6-7 should become sensor temperature, sample counter, or
  accelerometer status

### `BO_ 1316 PowerDistributionGyro`

PowerDistribution gyroscope frame.

Current layout:

- `pd_gyro_x`: 16-bit little-endian signed, `raw * 0.01 deg/s`
- `pd_gyro_y`: 16-bit little-endian signed, `raw * 0.01 deg/s`
- `pd_gyro_z`: 16-bit little-endian signed, `raw * 0.01 deg/s`
- bytes 6-7 are reserved for future 6-DOF data

This frame completes the 6-DOF PowerDistribution IMU data together with
`PowerDistributionAccel`. It is intentionally separated from accelerometer data
so rates and future filtering can be tuned independently.

GUI note:

- acceleration magnitude can be computed as
  `sqrt(x^2 + y^2 + z^2)`
- a basic car tilt view can estimate pitch/roll from accelerometer gravity
  direction
- true attitude/yaw from gyro requires sensor fusion and drift correction, so it
  should not be treated as absolute orientation without a filter

Open items:

- confirm gyro axis orientation
- confirm gyro full-scale range and scale
- decide whether PC should implement complementary/Madgwick/Mahony filtering
- decide whether bytes 6-7 should become sample counter or IMU status

## AMS Voltage Messages

Each accumulator segment has 24 cell voltages split over three CAN messages:

- Segment 1: `BmsSegment1VoltagesA/B/C`
- Segment 2: `BmsSegment2VoltagesA/B/C`
- Segment 3: `BmsSegment3VoltagesA/B/C`

Current layout:

- one byte per cell
- byte-aligned start bits: `0, 8, 16, ..., 56`
- endian is irrelevant for 8-bit signals, but the DBC marks them little-endian for consistency
- physical conversion: `voltage_V = raw * 0.01 + 2.0`

This represents cell voltages from `2.00 V` to `4.55 V`.

Note: the message and signal names currently keep the `Bms...` / `bms_s...`
prefix for GUI compatibility. In Formula Student terminology this module is the
AMS.

## AMS Temperature Messages

Each accumulator segment has 10 temperature values split over two CAN messages:

- Segment 1: `BmsSegment1Temperatures` and `BmsSegment1TemperaturesB`
- Segment 2: `BmsSegment2Temperatures` and `BmsSegment2TemperaturesB`
- Segment 3: `BmsSegment3Temperatures` and `BmsSegment3TemperaturesB`

Current layout:

- one byte per temperature
- first frame carries temperature channels 1-8
- second frame carries temperature channels 9-10
- byte-aligned start bits
- physical conversion: `temperature_degC = raw * 0.5 - 40`

This represents temperatures from `-40.0 degC` to `87.5 degC`, covering the
cell operating range while keeping each temperature to one byte.

## Inverter Cooling

### `BO_ 1120 InverterCoolingStatus`

This message preserves the legacy cooling CAN ID:

- `KOEL_ID = 0x460`
- `KOEL_sens = 0x01`
- `KOEL_PWM = 0x02`

The DBC models this as a multiplexed frame. Byte 0 is the register/multiplexer:

- register `0x01`: sensor/temperature payload
- register `0x02`: actuator duty payload

Current layout:

- `inverter_cooling_register`: byte 0, multiplexer
- `inverter_cooling_temp_1`: register `0x01`, bytes 1-2, signed little-endian, `raw * 0.1 degC`
- `inverter_cooling_temp_2`: register `0x01`, bytes 3-4, signed little-endian, `raw * 0.1 degC`
- `inverter_cooling_pump_duty`: register `0x02`, byte 1, `0-100 %`
- `inverter_cooling_vent_duty`: register `0x02`, byte 2, `0-100 %`

The two temperature sensors are currently not read in firmware, but they are kept
in the DBC so the PC decoder and future firmware have the final signal contract.

Open items:

- confirm which physical locations `temp_1` and `temp_2` represent
- confirm if percentage values are exact duty cycle percent or command percent
- confirm whether temperature scale should remain `0.1 degC`

## Notes For PC Decoder

The PC decoder loads this file with `cantools`.

## Vendor DBC Integration

DBC files do not have a native `import` mechanism like C/C++ or Python. In normal use, the parser receives one flat CAN database file.

For the DTI/HV500 inverter integration, we therefore copied the relevant definitions from the DTI standard-ID vendor DBC into the project master DBC:

- CAN message IDs
- DLC
- signal start bits
- signal lengths
- byte order
- signed/unsigned type
- scale and offset
- units

The vendor file remains useful reference material, but the runtime parser and firmware code generation should use `VUBR.dbc` as the single source of truth.

Some names were intentionally changed while copying:

- Vendor message names like `HV500_ERPM_DUTY_VOLTAGE` became project names like `DtiErpmDutyVoltage`.
- Vendor signal names like `Actual_ERPM` became canonical project names like `inverter_speed_actual`.
- Vendor signal names like `Actual_InputVoltage` became `inverter_vdc_bus`.

This keeps the PC GUI and analysis code independent of the inverter vendor. The dashboard can continue reading `dbc.inverter_speed_actual`, `dbc.inverter_vdc_bus`, `dbc.inverter_iq_actual`, etc., even while the underlying inverter hardware changes.

For now, manual copying is acceptable because there is only one vendor DBC fragment and the copied section is small enough to review. If more vendor DBCs are added, or if the DTI file changes often, we should move to a generated master DBC workflow. One possible structure:

```text
LibCAN/
  dbc_fragments/
    vubr_base.dbc
    vendor_dti_hv500_sid.dbc
    power_distribution.dbc
  tools/
    build_master_dbc.py
  include/
    VUBR.dbc
```

The build script would combine fragments, rename vendor signals into VUBR canonical names, check for CAN ID conflicts, validate with `cantools`, and write the final `include/VUBR.dbc`.

The decoder returns physical values, not raw values, when DBC scaling is defined. It also handles:

- byte order
- signedness
- bit slicing
- multiplexing
- value-table choices

During migration, the decoder allows truncated frames because some legacy frames send fewer bytes than the final DBC message length. This should be tightened once firmware and DBC layouts are stable.
