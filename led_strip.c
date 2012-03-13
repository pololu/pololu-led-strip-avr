/* This is AVR code for driving the RGB LED strips from Pololu.
   It allows complete control over the color of an arbitrary number of LEDs.
   This implementation disables interrupts while it does bit-banging with inline assembly.
   Date written: 2011-12-20
 */

 // The frequency of your AVR.  This code supports 20 MHz and 16 MHz.
#define F_CPU 20000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>

/** These lines define what pin the LED strip is on.
    You will either need to attach the LED strip's data line to PC0 or change these
    lines to specify a different pin.
    
    If you are using an Arduino, you will either need to attach the LED strip's data
    line to Analog Input 0 (a.k.a. PC0) or change these lines to specify a different
    pin.  Please refer to http://www.arduino.cc/en/Hacking/PinMapping    
**/
#define LED_STRIP_PORT PORTC
#define LED_STRIP_DDR  DDRC
#define LED_STRIP_PIN  0


/** The led_color struct represents the color for an 8-bit RGB LED.
    Examples:
      Black:      (led_color){ 0, 0, 0 }
      Pure red:   (led_color){ 255, 0, 0 }
      Pure green: (led_color){ 0, 255, 0 }
      Pure blue:  (led_color){ 0, 0, 255 }
      White:      (led_color){ 255, 255, 255} */
typedef struct rgb_color
{
  unsigned char red, green, blue;
} rgb_color;


/** led_strip_write sends a series of colors to the LED strip, updating the LEDs.
 The colors parameter should point to an array of led_color structs that hold the colors to send.
 The count parameter is the number of colors to send.

 Running on a 20 MHz AVR, this function takes about 1.5 ms to update 30 LEDs.
 Running on a 16 MHz AVR, this function takes about 1.9 ms to update 30 LEDs.
 Interrupts must be disabled during that time, so any interrupt-based library
 can be negatively affected by this function.
 */
void __attribute__((noinline)) led_strip_write(rgb_color * colors, unsigned int count) 
{
  // Set the pin to be an output driving low.
  LED_STRIP_PORT &= ~(1<<LED_STRIP_PIN);
  LED_STRIP_DDR |= (1<<LED_STRIP_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  while(count--)
  {
    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile(
    "rcall send_led_strip_byte\n"  // Send red component.
    "rcall send_led_strip_byte\n"  // Send green component.
    "rcall send_led_strip_byte\n"  // Send blue component.
    "rjmp led_strip_asm_end\n"     // Jump past the assembly subroutines.

    // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
    "send_led_strip_byte:\n"
    "ld __tmp_reg__, %a0+\n"      // Read the next color brightness byte and advance the pointer
    "rcall send_led_strip_bit\n"  // Send most-significant bit (bit 7).
    "rcall send_led_strip_bit\n"
    "rcall send_led_strip_bit\n"
    "rcall send_led_strip_bit\n"
    "rcall send_led_strip_bit\n"
    "rcall send_led_strip_bit\n"
    "rcall send_led_strip_bit\n"
    "rcall send_led_strip_bit\n"  // Send least-significant bit (bit 0).
    "ret\n"

    // send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line
    // high for some time.  The amount of time the line is high depends on whether the bit is 0 or 1,
    // but this function always takes the same time (2 us).
    "send_led_strip_bit:\n"
    "sbi %2, %3\n"                           // Drive the line high.
    "rol __tmp_reg__\n"                      // Rotate left through carry.
    "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
    "brcs .+2\n" "cbi %2, %3\n"              // If the bit to send is 0, drive the line low now.    
    "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
    "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
    "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
    "brcc .+2\n" "cbi %2, %3\n"              // If the bit to send is 1, drive the line low now.
    "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
    "ret\n"
    
    "led_strip_asm_end: "
    : "=b" (colors)
    : "0" (colors),                        // %a0 is a 16-bit indirect address register pointer,
                                           // pointing to the next color to display
      "I" (_SFR_IO_ADDR(LED_STRIP_PORT)),  // %2 is the port register (e.g. PORTC)
      "I" (LED_STRIP_PIN)                  // %3 is the pin number (0-8)
    );
  }
  sei();   // Re-enable interrupts now that we are done.
  _delay_us(15);  // Hold the line low for 15 microseconds to send the reset signal.
}

#define LED_COUNT 30
rgb_color colors[LED_COUNT];

int main()
{
  unsigned int time = 0;
  
  while(1)
  {
    unsigned int i;
	for(i = 0; i < LED_COUNT; i++)
    {
      unsigned char x = (time >> 2) - 8*i;
      colors[i] = (rgb_color){ x, 255 - x, x };
    }
 
    led_strip_write(colors, LED_COUNT);
    
    _delay_ms(20);
    time += 20;
  }
}