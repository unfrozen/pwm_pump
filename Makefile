SDCC = sdcc -mstm8 -I../libs -L../libs -DSTM8103
NAME = pwm_pump
LIBS = lib_stm8.lib
HDRS = $(NAME).h

all: $(NAME).ihx

.SUFFIXES : .rel .c


$(NAME).ihx: $(NAME).c $(INCS) $(HDRS) $(OBJS)
	$(SDCC) $(NAME).c  $(LIBS) $(OBJS)
	cp $(NAME).ihx /media/share/stm8

.c.rel :
	$(SDCC) -c $<
clean:
	- rm -f *.adb *.asm *.cdb *.ihx *.lk *.lst *.map *.rel *.rst *.sym

