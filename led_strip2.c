/* This is AVR code for driving the RGB LED strips from Pololu.
   This version can actually drive two LED strips at the same time.
   For a simpler version with more comments that does one LED strip at a time,
   see led_strip.c.
   This version only supports 20 MHz processors.
 */

#define F_CPU 20000000

#define LED_STRIP1_PORT PORTC
#define LED_STRIP1_DDR  DDRC
#define LED_STRIP1_PIN  0

#define LED_STRIP2_PORT PORTC
#define LED_STRIP2_DDR  DDRC
#define LED_STRIP2_PIN  1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>

typedef struct rgb_color
{
  unsigned char red, green, blue;
} rgb_color;

/** The timing of this function is the same as led_strip_write() in led_strip.c except
  it does two chains of LED strips simultaneously.
  Updating 2*30 LEDs takes less than 2 ms.  **/
void __attribute__((noinline)) led_strip_write2(rgb_color * colors1, rgb_color * colors2, unsigned int count) 
{
  LED_STRIP1_PORT &= ~(1<<LED_STRIP1_PIN);
  LED_STRIP1_DDR |= (1<<LED_STRIP1_PIN);

  LED_STRIP2_PORT &= ~(1<<LED_STRIP2_PIN);
  LED_STRIP2_DDR |= (1<<LED_STRIP2_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  while(count--)
  {
    unsigned char b1, b2;  // brightness values
    
    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile(
        "rcall send_led_strip_byte%=\n"  // Send red component.
        "rcall send_led_strip_byte%=\n"  // Send green component.
        "rcall send_led_strip_byte%=\n"  // Send blue component.
        "rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

        // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
        "send_led_strip_byte%=:\n"
        "ld %2, %a0+\n"        // Read the next color brightness byte and advance the pointer
        "ld %3, %a1+\n"        // Read the next color brightness byte and advance the pointer
        "rcall send_led_strip_bit%=\n"  // Send most-significant bit (bit 7).
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"  // Send least-significant bit (bit 0).
        "ret\n"

        // send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line
        // high for some time.  The amount of time the line is high depends on whether the bit is 0 or 1,
        // but this function always takes the same time (2 us).
        "send_led_strip_bit%=:\n"
        "sbi %6, %7\n"                           // Drive the line high.   
        "nop\n" "nop\n" "nop\n" "nop\n"
        
        "sbi %8, %9\n"                           // Drive the line2 high.
        "nop\n" "nop\n" "nop\n" "nop\n"
        
        "rol %2\n"                               // Rotate left through carry.
        "brcs .+2\n" "cbi %6, %7\n"              // If the bit to send is 0, drive the line low now.    
        "brcc .+4\n" "nop\n" "nop\n"             // Fix the timing.
        
        "rol %3\n"                               // Rotate left through carry.
        "brcs .+2\n" "cbi %8, %9\n"              // If the bit to send is 0, drive the line low now.    
        "brcc .+4\n" "nop\n" "nop\n"             // Fix the timing.

        "nop\n" "nop\n"

        "cbi %6, %7\n"
        "nop\n" "nop\n" "nop\n" "nop\n"
        
        "cbi %8, %9\n"
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n" "nop\n" "nop\n"

        "ret\n"
        "led_strip_asm_end%=: "
        : "=b" (colors1),
          "=b" (colors2),
          "=&r" (b1),            // %2 temporarily holds the brightness of strip1
          "=&r" (b2)             // %3 temporarily holds the brightness of strip2
        : "0" (colors1),         // %a0 points to the next color to display
          "1" (colors2),         // %a1 points to the next color to display for the other strip
          "I" (_SFR_IO_ADDR(LED_STRIP1_PORT)),   // %6 is the port register (e.g. PORTC)
          "I" (LED_STRIP1_PIN),                  // %7 is the pin number (0-8)
          "I" (_SFR_IO_ADDR(LED_STRIP2_PORT)),  // %8 is the port registers
          "I" (LED_STRIP2_PIN)                  // %9 is the pin number
    );

    // Uncomment the line below to temporarily enable interrupts between each color.
    //sei(); asm volatile("nop\n"); cli();
  }
  sei();          // Re-enable interrupts now that we are done.
  _delay_us(24);  // Hold the line low for 15 microseconds to send the reset signal.
}

#define LED_COUNT 60
rgb_color colors1[LED_COUNT], colors2[LED_COUNT];

int main()
{
  unsigned int time = 0;
  
  while(1)
  {
    unsigned int i;
    for(i = 0; i < LED_COUNT; i++)
    {
      unsigned char x = (time >> 2) - 8*i;
      colors1[i] = (rgb_color){ x, 255 - x, x };
      
      x = (time >> 2) - 50*i;
	  if (x > 127){ x = 255 - x; }
      x = x*x >> 8;
      colors2[i] = (rgb_color){ 0, 0, 2*x };
    }

    led_strip_write2(colors1, colors2, LED_COUNT);

    _delay_ms(20);
    time += 20;
  }
}