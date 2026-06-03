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

### `BO_ 385 Inverter2Tx`

Register-multiplexed inverter transmit frame. The first byte is `inverter_register`; the value payload starts at byte 1.

Current DBC assumption:

- payload value is little-endian signed 32-bit
- `Iq Cmd` scale is `0.1387684 A/raw`
- `Iq Actual` scale is `0.13876843 A/raw`

Important distinction: inverter `Iq` is q-axis/motor current. It is related to torque and inverter behavior, but it is not exactly the same as DC battery current.

Open items:

- confirm signedness of every inverter register
- confirm exact scales for speed, voltage, and temperatures
- decide whether inverter values should keep raw fallback columns during migration

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

### `BO_ 784 MasterStatus`

Contains old GUI bit-sliced 10-bit raw values.

Current signals:

- `ts_current_raw`
- `ams_temperature_raw`

Legacy packing is unusual:

- `ts_current_raw = (byte1 << 2) | (byte0 & 0b11)`
- `ams_temperature_raw = (byte3 << 2) | (byte2 & 0b11)`

That means each value uses the low two bits of one byte plus all eight bits of
the next byte. A normal DBC little-endian `start|10@1+` signal cannot represent
that as one contiguous signal; it would consume all eight bits of the first byte
and two bits of the second byte. During migration, the Python DBC adapter applies
a narrow compatibility correction for `MasterStatus` so the verification GUI
matches the old parser.

The SOC estimator currently converts current outside the DBC through `SocConfig`, because current sensor scale, offset, and zero bias still need final confirmation.

Assumption for PC model: discharge current is positive. The car currently has no regenerative braking.

Open items:

- confirm current sensor zero raw value
- confirm current scale in A/raw
- confirm AMS temperature conversion
- decide whether these should become physical DBC signals once confirmed

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

## BMS Voltage Messages

Each segment has 24 cell voltages split over three CAN messages:

- Segment 1: `BmsSegment1VoltagesA/B/C`
- Segment 2: `BmsSegment2VoltagesA/B/C`
- Segment 3: `BmsSegment3VoltagesA/B/C`

Current layout:

- one byte per cell
- byte-aligned start bits: `0, 8, 16, ..., 56`
- endian is irrelevant for 8-bit signals, but the DBC marks them little-endian for consistency
- physical conversion: `voltage_V = raw * 0.01 + 2.0`

This represents cell voltages from `2.00 V` to `4.55 V`.

## BMS Temperature Messages

Each segment currently has one temperature message with 8 cell temperature values:

- Segment 1: `BmsSegment1Temperatures`
- Segment 2: `BmsSegment2Temperatures`
- Segment 3: `BmsSegment3Temperatures`

Current layout:

- one byte per temperature
- byte-aligned start bits: `0, 8, 16, ..., 56`
- physical conversion: `temperature_degC = raw * 0.1`

Open item: current DBC max is `25.5 degC` because the old mapping used one byte with scale `0.1`. That is too low for real accumulator temperatures. We need either a different scale, offset, or wider signal if temperatures above `25.5 degC` must be represented.

## Notes For PC Decoder

The PC decoder loads this file with `cantools`.

The decoder returns physical values, not raw values, when DBC scaling is defined. It also handles:

- byte order
- signedness
- bit slicing
- multiplexing
- value-table choices

During migration, the decoder allows truncated frames because some legacy frames send fewer bytes than the final DBC message length. This should be tightened once firmware and DBC layouts are stable.
