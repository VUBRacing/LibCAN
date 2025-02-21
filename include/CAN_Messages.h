 #ifndef CAN_MESSAGES
#define CAN_MESSAGES_

/**
 * header to define usefull messages
 */

// messages for dashboard
const Message MESSAGE_dash_board_switch_on = {DASH_BOARD_INFO, 1, {DASH_BOARD_SWITCH_R2D_ON}};
const Message MESSAGE_dash_board_switch_off = {DASH_BOARD_INFO, 1, {DASH_BOARD_SWITCH_R2D_OFF}};
const Message MESSAGE_dash_board_button_on = {DASH_BOARD_INFO, 1, {DASH_BOARD_BUTTON_R2D_ON}};
const Message MESSAGE_dash_board_led_R2D_on = {DASH_BOARD_INFO, 1, {DASH_BOARD_LED_R2D_ON}};
const Message MESSAGE_dash_board_led_R2D_off = {DASH_BOARD_INFO, 1, {DASH_BOARD_LED_R2D_OFF}};
const Message MESSAGE_dash_board_led_CAN_on = {DASH_BOARD_INFO, 1, {DASH_BOARD_LED_CAN_ON}};
const Message MESSAGE_dash_board_led_CAN_off = {DASH_BOARD_INFO, 1, {DASH_BOARD_LED_CAN_OFF}};

// messages for precharge
const Message MESSAGE_precharge_on = {PRECHARGE_STATUS_ID, 1, {PRECHARGE_MESSAGE_ON}};
const Message MESSAGE_precharge_off = {PRECHARGE_STATUS_ID, 1, {PRECHARGE_MESSAGE_OFF}};

#endif