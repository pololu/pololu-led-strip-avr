MCU = atmega324p
AVRDUDE_DEVICE = m324p -F
PORT = \\\\.\USBSER000

CFLAGS=-g -Wall -mcall-prologues -mmcu=$(MCU) -Os
CC=avr-gcc
OBJCOPY=avr-objcopy 
OBJDUMP=avr-objdump
LDFLAGS=-Wl,-gc-sections -Wl,-relax -Wl,-Map="$(@:%.elf=%.map)"

AVRDUDE=avrdude
TARGET=led_strip2

all: $(TARGET).hex $(TARGET).lss

clean:
	rm -f *.o *.hex *.elf *.map *.lss

%.hex: %.elf
	$(OBJCOPY) -R .eeprom -O ihex $< $@

%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@
	
%.elf: %.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

program: $(TARGET).hex
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$<
