#ifndef CAN_MESSAGES
#define CAN_MESSAGES_

/**
 * header to define usefull messages
 */

// messages for dashboard
const Message MESSAGE_dash_board_R2D_ON_OFF_pressed = {DASH_BOARD_INFO, 1, {DASH_BOARD_SWITCH_R2D_ON}};
const Message MESSAGE_dash_board_R2D_activate = {DASH_BOARD_INFO, 1, {DASH_BOARD_BUTTON_R2D_ON}};
const Message MESSAGE_dash_board_SW1_high = {DASH_BOARD_INFO, 1, {DASH_BOARD_SW1_ON}};
const Message MESSAGE_dash_board_SW1_low = {DASH_BOARD_INFO, 1, {DASH_BOARD_SW1_OFF}};
const Message MESSAGE_dash_board_SW2_high = {DASH_BOARD_INFO, 1, {DASH_BOARD_SW2_ON}};
const Message MESSAGE_dash_board_SW2_low = {DASH_BOARD_INFO, 1, {DASH_BOARD_SW2_OFF}};




// messages for precharge
const Message MESSAGE_precharge_on = {PRECHARGE_STATUS_ID, 1, {PRECHARGE_MESSAGE_ON}};
const Message MESSAGE_precharge_off = {PRECHARGE_STATUS_ID, 1, {PRECHARGE_MESSAGE_OFF}};

// messages to pass the states
const Message MESSAGE_USER_State_Off = {USER_ID, 2,{STATE_REG, STATE_OFF_REG}};
const Message MESSAGE_USER_State_before_precharge = {USER_ID, 2,{STATE_REG, STATE_BEGINSEQUENCE_BEFORE_PRECHARGE_REG}};
const Message MESSAGE_USER_State_after_precharge = {USER_ID, 2,{STATE_REG, STATE_BEGINSEQUENCE_AFTER_PRECHARGE_REG}};
const Message MESSAGE_USER_State_precharge = {USER_ID, 2,{STATE_REG, STATE_PRECHARGE_REG}};
const Message MESSAGE_USER_State_calibration_min = {USER_ID, 2,{STATE_REG, STATE_CALIBRATION_MIN_REG}};
const Message MESSAGE_USER_State_calibration_max = {USER_ID, 2,{STATE_REG, STATE_CALIBRATION_MAX_REG}};
const Message MESSAGE_USER_State_last_step = {USER_ID, 2,{STATE_REG, STATE_LAST_STEP_REG}};
const Message MESSAGE_USER_State_on = {USER_ID, 2,{STATE_REG, STATE_ON_REG}};
const Message MESSAGE_USER_State_test = {USER_ID, 2,{STATE_REG, STATE_TEST_REG}};
const Message MESSAGE_USER_State_BSPD = {USER_ID, 2, {STATE_REG, STATE_BSPD_REG}};

// messages to pass an Error
const Message MESSAGE_ERROR_10percent = {ERROR_ID, 1, {ERROR_10percent}};
const Message MESSAGE_ERROR_INVERTER = {ERROR_ID, 1, {Inverter_ERROR}};
const Message MESSAGE_ERROR_PRECHARGE =  {ERROR_ID, 1, {Precharge_ERROR}};
const Message MESSAGE_ERROR_BMS = {ERROR_ID, 1, {BMS_ERROR}};
const Message MESSAGE_ERROR_LOW_LV = {ERROR_ID, 1, {LV_BAT_LOW}};

const Message MESSAGE_SDC_TS_box ={ERROR_ID,1,{SDC_OPEN_TS_box}};
const Message MESSAGE_SDC_TSAC ={ERROR_ID,1,{SDC_OPEN_TSAC}};
const Message MESSAGE_SDC_Pedal ={ERROR_ID,1,{SDC_OPEN_PEDAL}};


#endif