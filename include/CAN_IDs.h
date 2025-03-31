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
#define ERROR_10percent 0x01
#define Inverter_ERROR 0x02
#define Precharge_ERROR 0x03
#define BMS_ERROR 0x04

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
#define DASH_BOARD_LED_R2D_ON 0x04
#define DASH_BOARD_LED_R2D_OFF 0x05
#define DASH_BOARD_LED_CAN_ON 0x06
#define DASH_BOARD_LED_CAN_OFF 0x07


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
// regids for the user
#define USER_DATA_REG 0x02
#define BRAKE_REG 0x20
#define APPS_REG 0x30

#define BRAKESENS1_AVAL_ID 0x302
#define BRAKESENS2_AVAL_ID 0x303

// AMS/BMS
#define BMS_VOLTAGE_ID 0x400
#define BMS_CURRENT_ID 0x401
#define BMS_TEMP_ID 0x402

// reg ids  for the BMS
#define BMS_SEG1_REG1 0x30
#define BMS_SEG1_REG2 0x31
#define BMS_SEG2_REG1 0x40
#define BMS_SEG2_REG2 0x41
#define BMS_SEG3_REG3 0x50
#define BMS_SEG3_REG3 0x51


// BrakeLight
#define BL_STATUS_ID 0x500

// GPS:
#endif /* CAN_IDs_H_ */
