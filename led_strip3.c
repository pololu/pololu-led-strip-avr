/* This is AVR code for driving the RGB LED strips from Pololu.
   This version can actually drive three LED strips at the same time.
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

#define LED_STRIP3_PORT PORTD
#define LED_STRIP3_DDR  DDRD
#define LED_STRIP3_PIN  0

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
  Updating 3*30 LEDs takes less than 2 ms.  **/
void __attribute__((noinline)) led_strip_write3(rgb_color * colors1, rgb_color * colors2, rgb_color * colors3, unsigned int count) 
{
  LED_STRIP1_PORT &= ~(1<<LED_STRIP1_PIN);
  LED_STRIP1_DDR |= (1<<LED_STRIP1_PIN);

  LED_STRIP2_PORT &= ~(1<<LED_STRIP2_PIN);
  LED_STRIP2_DDR |= (1<<LED_STRIP2_PIN);

  LED_STRIP3_PORT &= ~(1<<LED_STRIP3_PIN);
  LED_STRIP3_DDR |= (1<<LED_STRIP3_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  while(count--)
  {
    unsigned char b1, b2, b3;  // brightness values
    
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
        "ld %3, %a0+\n"        // strip1: Read the next color brightness byte and advance the pointer
        "ld %4, %a1+\n"        // strip2
        "ld %5, %a2+\n"        // strip3
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
        "sbi %9, %10\n"                           // Strip1: Drive high.
        "nop\n" "nop\n" "nop\n"
        
        "sbi %11, %12\n"                         // Strip2: Drive high.
        "nop\n" "nop\n"
        
        "rol %3\n"                               // Strip1: Rotate brightness left through carry.
        "sbi %13, %14\n"                         // Strip3: Drive high.
        "brcs .+2\n" "cbi %9, %10\n"              // Strip1: If the bit to send is 0, drive low.
        "brcc .+4\n" "nop\n" "nop\n"             // Fix the timing.
        
        "rol %4\n"                               // Strip 2: Rotate left through carry.
        "brcs .+2\n" "cbi %11, %12\n"            // Strip 2: If the bit to send is 0, drive low.    
        "brcc .+4\n" "nop\n" "nop\n"             // Fix the timing.

        "rol %5\n"                               // Strip 3: Rotate left through carry.
        "brcs .+4\n" "cbi %13, %14\n" "cbi %9, %10\n"  // Strip 3: If the bit to send is 0, drive low.
        "brcc .+6\n" "cbi %9, %10\n" "nop\n" "nop\n"  // We also drive strip 1 low and fix the timing.
        
        "cbi %11, %12\n"                         // Strip 2: Drive low
        "nop\n" "nop\n" "nop\n"
        
        "cbi %13, %14\n"                         // Strip 3: Drive low.
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"

        "ret\n"
        "led_strip_asm_end%=: "
        : "=e" (colors1),
          "=e" (colors2),
          "=e" (colors3),
          "=&r" (b1),            // %3 temporarily holds the brightness of strip1
          "=&r" (b2),            // %4 temporarily holds the brightness of strip2
          "=&r" (b3)             // %5 temporarily holds the brightness of strip3
        : "0" (colors1),         // %a0 points to the next color for strip1
          "1" (colors2),         // %a1 points to the next color for strip2          
          "2" (colors3),         // %a2 points to the next color for strip3
          "I" (_SFR_IO_ADDR(LED_STRIP1_PORT)),  //  %9 is a port register (e.g. PORTC)
          "I" (LED_STRIP1_PIN),                 // %10 is a pin number (0-8)
          "I" (_SFR_IO_ADDR(LED_STRIP2_PORT)),  // %11 is a port register
          "I" (LED_STRIP2_PIN),                 // %12 is a pin number
          "I" (_SFR_IO_ADDR(LED_STRIP3_PORT)),  // %13 is a port register
          "I" (LED_STRIP3_PIN)                  // %14 is a pin number
    );

    // Uncomment the line below to temporarily enable interrupts between each color.
    //sei(); asm volatile("nop\n"); cli();
  }
  sei();          // Re-enable interrupts now that we are done.
  _delay_us(24);  // Hold the line low for 15 microseconds to send the reset signal.
}

#define LED_COUNT 60
rgb_color colors1[LED_COUNT], colors2[LED_COUNT], colors3[LED_COUNT];

int main()
{
  unsigned int time = 0;
  
  while(1)
  {
    unsigned int i;
    for(i = 0; i < LED_COUNT; i++)
    {
      unsigned char x;
	  
	  x = (time >> 2) - 8*i;
      colors1[i] = (rgb_color){ x, 255 - x, x };
     
      x = (time >> 2) - 50*i;
      if (x > 127){ x = 255 - x; }
      x = (x*x) >> 8;
      colors2[i] = (rgb_color){ 0, 2*x, x };
	  
      x = (time >> 2) - 30*i;
      if (x > 127){ x = 255 - x; }
      colors3[i] = (rgb_color){ (x*x) >> 8, 0, ((128-x)*(128-x)) >> 8};
    }

    led_strip_write3(colors1, colors2, colors3, LED_COUNT);

    _delay_ms(20);
    time += 20;
  }
}