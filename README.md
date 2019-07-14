# pwm_pump
Control pump speed using PWM. Uses TM1638 keypad/display. Use it to start your PWM project!

This project uses the STM8S103P3 on a cheap Ebay board and a TM1638 display and keypad (8 key) module. The PWM output is on
pin A3 (can be changed to D4). The power circuit (not included here) includes a IRLZ24 MOSFET transistor, which accepts
a logic level (5 volts) input on its gate.

The display shows the Off/Run status and the PWM percentage, from 0 to 100. The display can also show the total run time,
up to 9999 hours. You can turn the PWM on and off, and change the PWM percentage with the keypad buttons (S1 to S8).

S1: Choose OFF mode.
S2: Choose RUN mode.
S3: Switch between status and run time display.
S4: Subtract 10% from PWM.
S5: Add 10% to PWM.
S6: Hold 2 seconds to reset run time to zero.
S7: Subtract 1% from PWM.
S8: Add 1% to PWM.

The configuration items are in pwm_pump.h in case you wish to change something.

This project uses SDCC to compile and my stm8_libs library. Please install that first.
The Makefile expects stm8_libs and header files in ../libs, change for your layout.

Feel free to use this as a working prototype to develop your own application.

You may contact the author at: richard@hodges.org
I am available for embedded projects, large or small.
