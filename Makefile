DEVICE = atmega1284p
MCU = atmega1284p
AVRDUDE_DEVICE = m1284p
PORT = \\\\.\USBSER000

CFLAGS=-g -Wall -mcall-prologues -mmcu=$(MCU) $(DEVICE_SPECIFIC_CFLAGS) -Os
CC=avr-gcc
OBJCOPY=avr-objcopy 
OBJDUMP=avr-objdump
LDFLAGS=-Wl,-gc-sections -Wl,-relax -Wl,-Map="$(@:%.elf=%.map)"

AVRDUDE=avrdude
TARGET=led_strip

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
