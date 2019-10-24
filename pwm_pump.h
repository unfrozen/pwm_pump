/*
 *  File name:  pwm_pump.h
 *  Date first: 06/30/2019
 *  Date last:  10/24/2019
 *
 *  Description: Control motor (pump) speed with PWM. CONFIGURATION FILE
 *
 *  Author:     Richard Hodges
 *
 *  Copyright (C) 2019 Richard Hodges. All rights reserved.
 *  Permission is hereby granded for any use.
 *
 ******************************************************************************
 *
 *  These are the project configuration items.
 *  Generally, times will be in tenths of a second (times 0.1 second).
 */

#define MODE_OFF	1
#define MODE_RUN	2

#define DISP_PCT	10	/* display mode and percentage */
#define DISP_TIME	11	/* display run time */
#define DISP_CYCLE	12	/* display cycle times */

/* This is the interval for the hour counter auto-save to EEPROM.
 * The EEPROM has a stated endurance of 300,000 cycles, and would
 * wear out quickly if we saved too frequently. The compromise here
 * is to save every 5 minutes. The variable is 16 bits (max 65535).
 */
#define HOUR_SAVE	3000	/* hour save interval, 1/10 sec */

/* These are the keys on the TM1638 module. We use the default key map,
 * which is ASCII '0' to '7'. The OFF/RUN, the 10% UP/DOWN and the
 * 1% UP/DOWN keys are separated to (try to) avoid accidental changes.
 */
#define KEY_OFF		'0'
#define KEY_RUN		'1'
#define KEY_DISP	'2' /* change display */
#define KEY_10D		'3' /* PWM down 10% */
#define KEY_10U		'4' /* PWM up 10% */
#define KEY_RESET	'5' /* reset hour counter or run 60 seconds */
#define KEY_1D		'6' /* PWM down 1% */
#define KEY_1U		'7' /* PWM up 1% */

/* The DISP_CYCLE display shows the time on and the time off. These
 * numbers allow pump rates lower than PWM can handle, at the price
 * of losing smooth and continuous feed. The first number is the
 * time ON, in tenths of a second, the second is the time off.
 * The feature is enabled when both times are non-zero. Hours are
 * only counted during the cycle ON time.
 *
 * Example: C 1.5 3.0 shows ON 1.5 seconds, OFF 3.0 seconds.
 */

#define KEY_CY_ON	'3' /* Adjust ON time */
#define KEY_CY_OFF	'4' /* Adjust OFF */
#define KEY_CY_10	'5' /* Adjust time up 1.0 second (rolls over) */
#define KEY_CY_1D	'6' /* Ajdust time down 0.1 second */
#define KEY_CY_1U	'7' /* Adjust time up 0.1 second */

#define CYCLE_MAX	99 /* Maximum on or off time is 9.9 seconds */

/* The hour counter reset button behaves as follows:
 * On keypress, the display starts flashing.
 * After key held down long enough, the hour counter resets to zero.
 * If the key is released earlier, there is no reset.
 * After the delay or release, the flashing stops.
 */
#define RESET_TIME	20	/* 2.0 seconds before reset */

/* Blink rate when RESET button is pressed, 25/100 second. */
#define RESET_BLINK	25

/* When in status display mode, button S6 sets a countdown to run the PWM
 * for a specific time, then shut off. This can be useful for measuring the
 * pump output, for example. The default is 60 seconds (times 10/second).
 */
#define COUNTDOWN 600

/* If the power fails while in "RUN" mode, you can have the device alarm
 * until you press a button. The alarm will be beeping and flashing display.
 * Comment out if you do not want the alarm (quiet reset).
 */
#define POWER_FAIL_ALARM
