/*
 *  File name:  pwm_pump.c
 *  Date first: 06/30/2019
 *  Date last:  10/11/2019
 *
 *  Description: Control motor (pump) speed with PWM.
 *
 *  Author:     Richard Hodges
 *
 *  Copyright (C) 2019 Richard Hodges. All rights reserved.
 *  Permission is hereby granded for any use.
 *
 ******************************************************************************
 *
 *  This project uses a TM1638 display/keypad (8 key type).
 *  This is sold on ebay for just over two dollars.
 */

#include "stm8s_header.h"

#include <string.h>

#include "lib_board.h"
#include "lib_clock.h"
#include "lib_eeprom.h"
#include "lib_pwm.h"
#include "lib_tm1638.h"

#include "lib_bindec.h"

#include "pwm_pump.h"	/* configuration items */

char clock_tenths;	/* 1/10 second 0-255 */
char pwm_cur;		/* current PWM percentage, 0-100 */
char mode_cur;		/* RUN or OFF */
char last_led;		/* last top LED shown */
char display;		/* show status or run time? */
long hour_frac;		/* thousandths of an hour */
int countdown;		/* if set, number tenth-seconds before turn off */

char cycle_on;		/* counter for ON time */
char cycle_off;		/* counter for OFF time */
char cycle_key;		/* key for on or off time to modify */

char key_time[8];	/* key hold down time, times 1/10 second */

void show_status(void);
void show_time(void);
void show_cycle(void);

void do_key(char);	/* handle key press */
void key_pct(char);	/* keypress for PWM percent display */
void key_hours(char);	/* keypress for hour time display */
void key_cycle(char);	/* keypress for on/off cycle display */

void local_init(void);	/* project-specific setup */
void local_beep(char);	/* turn beeper on or off */

void timer_ms(void);	/* millisecond timer call */
void timer_10(void);	/* 1/10 second timer call */

void hours_load(void);	/* load hour count from EEPROM */
void hours_save(void);	/* save hour count to EEPROM */
void hours_update(void); /* update hours count, call every 1/10 second */

void put_bin8_dp(char); /* output binary as 2 digit dec with point */

#define putc    tm1638_putc
#define puts    tm1638_puts
#define curs    tm1638_curs
#define clear   tm1638_clear
#define getc    tm1638_getc
#define setled  tm1638_setled

char key_index(char);		/* get index from key value 0-7 */

/* Non-volatile configuration and data stored in EEPROM.
 * Wrap with eeprom_unlock() and eeprom_lock() when writing.
 */
typedef struct {
    long	hour_frac;	/* hour count, in thousandths */
    char	cycle_on;	/* cycle on  time, in 0.1 seconds */
    char	cycle_off;	/* cycle off time, in 0.1 seconds */
} CONFIG;

CONFIG *config;

/******************************************************************************
 *
 *  Control the pump motor speed
 */

int main() {
    char	clock_last;
    char	key, i;
    char	reset_idx;	/* index of hour reset key */
    
    board_init(0);
    local_init();
    clock_init(timer_ms, timer_10);
    tm1638_init(TM1638_8);	/* uses D1, D2, and D3 */
    pwm_init(PWM_DUTY, PWM_C3);	/* using A3 (pin 10) for PWM */

    config = (CONFIG *)&EEPROM;
    hours_load();		/* load hours from EEPROM */
    reset_idx = key_index(KEY_RESET);
    
    clock_tenths = 0;
    clock_last = 0;
    countdown = 0;
    
    pwm_cur = 0;
    mode_cur = MODE_OFF;
    display = DISP_PCT;
    last_led = 0;

    clear();
    local_beep(1);
    while(clock_last == clock_tenths);
    while(getc());		/* discard any phoney keys */

    while (1) {
	if (clock_last == clock_tenths)	/* update every 1/10 second */
	    continue;
	clock_last = clock_tenths;
	local_beep(0);		/* beep is 1/10 second */
	
	hours_update();		/* update hours count */
	
	for (i = 0; i < 8; i++)
	    if (key_time[i])	/* key still held down */
		key_time[i]++;	/* hold time, times 1/10 second */

	/* check if HOURS RESET key is held down long enough for reset */
	if (display == DISP_TIME &&
	    key_time[reset_idx] == RESET_TIME) {
	    hour_frac = 0;	/* reset the hour counter */
	    tm1638_blink(0);
	}
	if (countdown) {	/* is running for specific time? */
	    countdown--;
	    if (!countdown) {
		mode_cur = MODE_OFF;
		pwm_duty(PWM_C3, 0);
	    }
	}
	key = getc();
	if (key)
	    do_key(key);

	switch (display) {
	case DISP_PCT :
	    show_status();
	    break;
	case DISP_TIME :
	    show_time();
	    break;
	case DISP_CYCLE :
	    show_cycle();
	    break;
	}
    }
}

/******************************************************************************
 *
 *  Show status
 */

void show_status(void)
{
    char	display[6];

    clear();
    curs(0);

    if (countdown) {	/* show remaining time before turning off */
	bin16_dec_rlz(countdown, display);
	display[1] = display[2];
	display[2] = display[3];
	display[3] = '.';
	puts(display + 1);
	puts("  ");
    }
    else {
	switch(mode_cur) {
	case MODE_OFF :
	    puts("OFF  ");
	    break;
	case MODE_RUN :
	    puts("RUN  ");
	    break;
	default :
	    puts("ERR  ");
	}
    }
    bin16_dec_rlz(pwm_cur, display);
    puts(display + 2);
}

/******************************************************************************
 *
 *  Show hours run time
 */

void show_time(void)
{
    char	display[12];

    bin32_dec(hour_frac, display);
    decimal_rlz(display, 7);

    display[11] = 0;
    display[10] = display[9];
    display[ 9] = display[8];
    display[ 8] = display[7];
    display[ 7] = '.';
    
    curs(0);
    putc('H');
 
    puts(display + 3);
}

/******************************************************************************
 *
 *  Show on/off cycle values
 *  display example: "C 1.5 3.4 "
 */

void show_cycle(void)
{
    static char blink;

    clear();
    curs(0);
    putc('C');

    blink++;

    curs(2);
    if ((blink & 4) ||
	cycle_key != KEY_CY_ON)
	put_bin8_dp(config->cycle_on);
    curs(5);
    if ((blink & 4) ||
	cycle_key != KEY_CY_OFF)
	put_bin8_dp(config->cycle_off);
}

/******************************************************************************
 *
 *  Output 8 bit binary to two decimal char with decimal point
 *  in: binary
 */

void put_bin8_dp(char bin)
{
    char	buf[3];
    
    bin8_dec2(bin, buf);
    putc(buf[0] | 0x80);	/* add decimal point */
    putc(buf[1]);
}

/******************************************************************************
 *
 *  Handle key press or release
 *  in: key (bit-7 set on release)
 */

void do_key(char key)
{
    char	set_pwm;
    char	index;

    index = key_index(key);
    if (key & 0x80) {	/* key released */
	key_time[index] = 0;
	if ((key & 0x7f) == KEY_RESET)
	    tm1638_blink(0);
	return;
    }
    local_beep(1);		/* 1/10 second, turns off in main() */
    key_time[index] = 1;

    switch(key) {		/* On/off keys active in all modes */
    case KEY_OFF :
	countdown = 0;
	mode_cur = MODE_OFF;
	break;
    case KEY_RUN :
	mode_cur = MODE_RUN;
	break;
    }
    switch (display) {
    case DISP_PCT :		/* PWM percent display mode */
	cycle_key = 0;
	key_pct(key);
	break;
    case DISP_TIME :		/* hour time display mode */
	cycle_key = 0;
	key_hours(key);
	break;
    case DISP_CYCLE :		/* on/off cycle display mode */
	key_cycle(key);
	break;
    }
    /* set new PWM value, zero if MODE_OFF */
    
    set_pwm = pwm_cur;
    if (mode_cur == MODE_OFF)
	set_pwm = 0;
    pwm_duty(PWM_C3, set_pwm * 2);

    /* show rough PWM value with LED on top of display */
    
    setled(last_led, 0);
    last_led = (set_pwm - 6) / 12;
    if (last_led > 7)
	last_led = 7;
    if (mode_cur == MODE_RUN)
	setled(last_led, 1);
    
    hours_save();		/* save hours on any keypress */
}

/******************************************************************************
 *
 * Handle key press for PWM percent display mode
 * in: key
 */

void key_pct(char key)
{
    switch (key) {
    case KEY_10U :		/* PWM +10 */
	pwm_cur += 10;
	if (pwm_cur > 100)
	    pwm_cur = 100;
	break;
    case KEY_10D :		/* PWM -10 */
	if (pwm_cur < 10)
	    pwm_cur = 10;
	pwm_cur -= 10;
	break;
    case KEY_1U :		/* PWM +1 */
	if (pwm_cur < 100)
	    pwm_cur++;
	break;
    case KEY_1D :		/* PWM -1 */
	if (pwm_cur)
	    pwm_cur--;
	break;
    case KEY_DISP :		/* switch to hour time display mode */
	display = DISP_TIME;
	break;
    case KEY_RESET :		/* start timed "on" operation */
	countdown = COUNTDOWN;
	mode_cur = MODE_RUN;
	break;
    }	
}

/******************************************************************************
 *
 * Handle key press for hour time display mode
 * in: key
 */

void key_hours(char key)
{
    switch (key) {
    case KEY_DISP :		/* switch to on/off cycle display mode */
	display = DISP_CYCLE;
	break;
    case KEY_RESET :		/* reset hour counter */
	tm1638_blink(RESET_BLINK);	/* S6 starts hours reset */
    }	
}

/******************************************************************************
 *
 * Handle key press for on/off cycle display mode
 * in: key
 */

void key_cycle(char key)
{
    signed char	modval, newval;

    modval = 0;
    switch (key) {
    case KEY_DISP :		/* switch to PWM percent display mode */
	display = DISP_PCT;
	break;
    case KEY_CY_ON :		/* adjust ON time */
	if (cycle_key == KEY_CY_ON)
	    cycle_key = 0;
	else
	    cycle_key = KEY_CY_ON;
	break;
    case KEY_CY_OFF :		/* adjust OFF time */
	if (cycle_key == KEY_CY_OFF)
	    cycle_key = 0;
	else
	    cycle_key = KEY_CY_OFF;
	break;
    case KEY_CY_1D :		/* adjust time down 0.1 second */
	modval = -1;
	break;
    case KEY_CY_1U :		/* adjust time up 0.1 second */
	modval = 1;
	break;
    }	
    if (!modval)
	return;
    switch(cycle_key) {
    case KEY_CY_ON :
	newval = config->cycle_on + modval;
	if (newval < 0 || newval > CYCLE_MAX)
	    break;
	eeprom_unlock();
	config->cycle_on = newval;
	eeprom_lock();
	break;
    case KEY_CY_OFF :
	newval = config->cycle_off + modval;
	if (newval < 0 || newval > CYCLE_MAX)
	    break;
	eeprom_unlock();
	config->cycle_off = newval;
	eeprom_lock();
	break;
    }
}

/******************************************************************************
 *
 *  Get index from key value
 *  in: key value
 * out: key index
 */

char key_index(char key)
{
    return (key & 7);		/* we don't use a custom key map */
}

/******************************************************************************
 *
 *  Update hours count (call every 1/10 second)
 */

void hours_update(void)
{
    static char	cnt36 = 36;	/* count 36 tenths per 1/1000 hour */
    static int save_count = HOUR_SAVE;

    if (mode_cur == MODE_RUN) {
	cnt36--;
	if (!cnt36) {	/* update the run time */
	    cnt36 = 36;
	    hour_frac++;	/* thousandths of an hour */
	}
    }
    save_count--;
    if (!save_count) {
	save_count = HOUR_SAVE;
	hours_save();
    }
}

/******************************************************************************
 *
 * Available ports on STM8S103F3:
 *
 * A1..A3	A3 is HS
 * B4..B5	Open drain
 * C3..C7	HS
 * D1..D6	HS
 *
 * PIN CONNECTIONS for this project:
 *
 * pin  1 (D4) PWM channel 1
 * pin 20 (D3) PWM channel 2 (conflict with TM1638)
 * pin 10 (A3) PWM channel 3
 *
 * pin 18 (D1) TM1638 CLK
 * pin 19 (D2) TM1638 CS/STROBE
 * pin 20 (D3) TM1638 DIN
 *
 * pin  1 (D4) Beeper output
 * pin  2 (D5) UART TX (future debugging, not used now)
 * pin  3 (D6) UART RX (future debugging, not used now)
 *
 * This CPU runs at 5 volts to drive the gate of a IRLZ24. On the Ebay
 * breakout board with the stm8s103, I put a jumper between the 5V and 3.3V
 * pins to bypass the LDO regulator.
 *
 ******************************************************************************
 *
 *  Millisecond timer callback
 */

void timer_ms(void)
{
    tm1638_poll();		/* keypad and blink features */
}

/******************************************************************************
 *
 *  Tenths second timer callback
 *  Blink the LED for two-pulse "heartbeat".
 */

void timer_10(void)
{
    static char blink;
    
    clock_tenths++;

    blink++;
    if (blink < 4) {
        board_led(blink & 1);   /* blink twice */
        return;
    }
    board_led(0);               /* off for 7/10 second */
    if (blink < 10)
        return;
    blink = 0;
}

/******************************************************************************
 *
 *  Load hours from EEPROM
 */

void hours_load(void)
{
    hour_frac = config->hour_frac;
}

/******************************************************************************
 *
 *  Save hours to EEPROM
 */

void hours_save(void)
{
    eeprom_unlock();
    config->hour_frac = hour_frac;
    eeprom_lock();
}

/******************************************************************************
 *
 *  Setup specific to this project
 *  Enable beeper
 */

void local_init(void)
{
    BEEP_CSR = 0x06;	/* beeper 4khz */
}

/******************************************************************************
 *
 *  Turn beeper on or off
 *  in:  zero = off, non-zero = on
 */

void local_beep(char on)
{
    if (on)
	BEEP_CSR |= 0x20;
    else
	BEEP_CSR &= 0xdf;
}
