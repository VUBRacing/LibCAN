#ifndef CAN_IDs_H_
#define CAN_IDs_H_

// CAN SETUP:
// CAN0: Inverter1&2, APPS(1&2)
// CAN1:  AMS/BMS, PreCharge, BrakeLight, GPS
// CAN2: Debug/Extra
// More info: https://trello.com/c/7XiEiakx

// ID Specs
#define MAX_CAN_ID 0x7FF

// CAN BUS DEBUG/IMPORTANT SIGNALS
// ID Range: 0x000 - 0x009
// #define CAN_ERROR_ID 0x000

#define ERROR_ID 0x001
#define SDC_OPEN_TS_box 0x01
#define SDC_OPEN_TSAC 0x02
#define SDC_OPEN_PEDAL 0x03

#define ERROR_10percent 0x04
#define Inverter_ERROR 0x05
#define Precharge_ERROR 0x06
#define BMS_ERROR 0x07
#define LV_BAT_LOW 0X08

// PreCharge
#define PRECHARGE_STATUS_ID 0x006
// reg ids for the precharge
#define PRECHARGE_STATUS_REG 0x20
#define PRECHARGE_VOLTAGE_BAT_REG 0x30
#define PRECHARGE_VOLTAGE_TS_REG 0x40
// Message form precharge
#define PRECHARGE_MESSAGE_ON 0x01
#define PRECHARGE_MESSAGE_OFF 0x02

// APPS Analog Values
#define APPS_AVAL_ID 0x012   

// button and switch of dashbord
#define DASH_BOARD_INFO 0x015
// information transmitted
#define DASH_BOARD_BUTTON_R2D_ON 0x01
#define DASH_BOARD_SWITCH_R2D_ON 0x02
#define DASH_BOARD_SWITCH_R2D_OFF 0x03
#define DASH_BOARD_SW1_ON 0x04
#define DASH_BOARD_SW1_OFF 0x05
#define DASH_BOARD_SW2_ON 0x06
#define DASH_BOARD_SW2_OFF 0x07


// Inverters
// INV RX Range: 0x201 - 0x27F
// INV TX Range: 0x181 - 0x1FF
#define INV1_RX_ID 0x202
#define INV1_TX_ID 0x182
#define INV2_RX_ID 0x201
#define INV2_TX_ID 0x181

// RegIDs for the invertor
#define SPEED_CMD_RAMP_REG 0x32
#define SPEED_ACTUAL_REG  0x30
#define IQ_CMD_RAMP_REG 0x22
#define IQ_ACTUAL_REG 0x27
#define VOUT_REG 0x8A
#define VDC_BUS_REG 0x66
#define T_IGBT_REG 0x4A
#define T_MOTOR_REG 0x49

// BrakeSens Analog Values
#define USER_ID 0x300
// RegIDs voor de states
#define STATE_REG 0x01

#define STATE_OFF_REG  0x01
#define STATE_BEGINSEQUENCE_BEFORE_PRECHARGE_REG 0x02
#define STATE_PRECHARGE_REG 0x03
#define STATE_BEGINSEQUENCE_AFTER_PRECHARGE_REG 0x04
#define STATE_CALIBRATION_MIN_REG 0x05
#define STATE_CALIBRATION_MAX_REG 0x06
#define STATE_LAST_STEP_REG 0x07
#define STATE_ON_REG 0x08
#define STATE_TEST_REG 0x09
#define STATE_BSPD_REG 0x10
// regids for the user
#define USER_DATA_REG 0x02
#define BRAKE_REG 0x20
#define APPS_REG 0x30

#define BRAKESENS1_AVAL_ID 0x302
#define BRAKESENS2_AVAL_ID 0x303

// ID for the master
#define MASTER_ID 0x310
// ids  for the BMS
#define BMS_SEG1_V1 0x400
#define BMS_SEG1_V2 0x401
#define BMS_SEG1_V3 0x402
#define BMS_SEG1_T  0x403

#define BMS_SEG2_V1 0x410
#define BMS_SEG2_V2 0x411
#define BMS_SEG2_V3 0x412
#define BMS_SEG2_T  0x413

#define BMS_SEG3_V1 0x420
#define BMS_SEG3_V2 0x421
#define BMS_SEG3_V3 0x422
#define BMS_SEG3_T  0x423

#define POWERDIS 0x450
#define POWERDIS_TSAC 0x01
#define POWERDIS_DASH 0x02 
#define POWERDIS_TSAL 0x03
#define POWERDIS_TSBOX 0x04
#define POWERDIS_DTI 0x05
#define POWERDIS_PUMP 0x06
#define POWERDIS_SDC 0x07
#define POWERDIS_DISCHARGE  0x08
#define POWERDIS_VENT 0x09

#define KOEL_ID 0X460
#define KOEL_sens 0x01
#define KOEL_PWM 0x02

#define MECH_info_ID 0X470
#define MECH_GYR 0X01

// BrakeLight
#define BL_STATUS_ID 0x500

// GPS:
#endif /* CAN_IDs_H_ */
