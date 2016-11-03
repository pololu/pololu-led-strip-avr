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

typedef struct rgb_color
{
  unsigned char red, green, blue;
} rgb_color;

/** The timing of this function is the same as led_strip_write() in led_strip.c except
  it does two chains of LED strips simultaneously.
  Updating 3*30 LEDs takes less than 1 ms.  **/
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
    unsigned char i;           // counts down from 8 to 0

    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile(
        "ld %3, %a0+\n"
        "ld %4, %a1+\n"
        "ld %5, %a2+\n"
        "ld %3, %a0\n"
        "ld %4, %a1\n"
        "ld %5, %a2\n"
        "rcall send_led_strip_byte%=\n"  // Send green component.
        "ld %3, -%a0\n"
        "ld %4, -%a1\n"
        "ld %5, -%a2\n"
        "rcall send_led_strip_byte%=\n"  // Send red component.
        "ld %3, %a0+\n"
        "ld %4, %a1+\n"
        "ld %5, %a2+\n"
        "ld %3, %a0+\n"
        "ld %4, %a1+\n"
        "ld %5, %a2+\n"
        "ld %3, %a0+\n"
        "ld %4, %a1+\n"
        "ld %5, %a2+\n"
        "rcall send_led_strip_byte%=\n"  // Send blue component.
        "rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

// Macros for driving an output high
#define SBI1 "sbi %10, %11\n"   // happens on cycle 1
#define SBI2 "sbi %12, %13\n"   // happens on cycle 3
#define SBI3 "sbi %14, %15\n"   // happens on cycle 7

// Macros for driving an output low
#define CBI1 "cbi %10, %11\n"   // ideally happens on cycle 9 or 17
#define CBI2 "cbi %12, %13\n"   // ideally happens on cycle 11 or 19
#define CBI3 "cbi %14, %15\n"   // ideally happens on cycle 15 or 23

#define ROL1 "rol %3\n"
#define ROL2 "rol %4\n"
#define ROL3 "rol %5\n"
#define BRNE_ALT "brne bxxx_c21\n"
#define BRNE_0   "brne bxxx_c0\n"
#define DEC  "dec %6\n"
#define DONE "ret\n"

        "b1xx_c5:\n"      // cycle 5, 6
        SBI3              // cycle 7, 8
        ROL2              // cycle 9
        "brcs b11x_c10\n" // cycle 10
        "b10x_c10:\n"
        CBI2              // cycle 11, 12
        ROL3              // cycle 13
        "brcs b101_c14\n" // cycle 14
        "b100_c14:\n"
        CBI3              // cycle 15, 16
        CBI1              // cycle 17, 18
        "nop\n"           // cycle 19
        "nop\n"           // cycle 20
        "nop\n"           // cycle 21
        DEC               // cycle 22
        BRNE_0            // cycle 23, 24
        "nop\n"
        DONE

        "b11x_c10:\n"     // cycle 10, 11
        ROL3              // cycle 12
        "brcs b111_c13\n" // cycle 13
        "b110_c13:\n"
        "nop\n"           // cycle 14
        CBI3              // cycle 15, 16
        CBI1              // cycle 17, 18
        CBI2              // cycle 19, 20
        "nop\n"           // cycle 21
        DEC               // cycle 22
        BRNE_0            // cycle 23
        "nop\n"
        DONE

        "b111_c13:\n"     // cycle 13, 14
        "nop\n"           // cycle 15
        DEC               // cycle 16
        CBI1              // cycle 17, 18
        CBI2              // cycle 19, 20
        BRNE_ALT          // cycle 21
        "nop\n"           // cycle 22
        CBI3              // cycle 23, 24
        DONE

        "b101_c14:\n"     // cycle 14, 15
        "nop\n"           // cycle 16
        CBI1              // cycle 17, 18
        "nop\n"           // cycle 19
        DEC               // cycle 20
        BRNE_ALT          // cycle 21
        "nop\n"           // cycle 22
        CBI3              // cycle 23, 24
        DONE

        // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
        "send_led_strip_byte%=:\n"
        "ldi %6, 8\n"          // set up the bit counter

        "bxxx_c21:"       // cycle 21, 22 for the previous iteration
        CBI3              // cycle 23, 24 for the previous iteration
        "bxxx_c0:"
        ROL1              // cycle 0
        SBI1              // cycle 1, 2
        SBI2              // cycle 3, 4
        "brcs b1xx_c5\n"  // cycle 5
        "b0xx_c5:"
        ROL2              // cycle 6
        SBI3              // cycle 7, 8
        CBI1              // cycle 9, 10
        "brcs b01x_c11\n" // cycle 11
        "b00x_c11:\n"
        CBI2              // cycle 12, 13  (one cycle late)
        ROL3              // cycle 14
        "brcs b001_c15\n" // cycle 15
        "b000_c15:\n"
        CBI3              // cycle 16, 17  (one cycle late)
        "nop\n"           // cycle 18
        "nop\n"           // cycle 19
        "nop\n"           // cycle 20
        "nop\n"           // cycle 21
        DEC               // cycle 22
        BRNE_0            // cycle 23, 24
        "nop\n"
        DONE

        "b01x_c11:\n"     // cycle 11, 12
        ROL3              // cycle 13
        "brcs b011_c14\n" // cycle 14
        "b010_c14:\n"
        CBI3              // cycle 15, 16
        "nop\n"           // cycle 17
        "nop\n"           // cycle 18
        CBI2              // cycle 19, 20
        "nop\n"           // cycle 21
        DEC               // cycle 22
        BRNE_0            // cycle 23
        "nop\n"
        DONE

        "b011_c14:"       // cycle 14, 15
        "nop\n"           // cycle 16
        "nop\n"           // cycle 17
        DEC               // cycle 18
        CBI2              // cycle 19, 20
        BRNE_ALT          // cycle 21
        "nop\n"           // cycle 22
        CBI3              // cycle 23, 24
        DONE

        "b001_c15:\n"     // cycle 15, 16
        "nop\n"           // cycle 17
        "nop\n"           // cycle 18
        "nop\n"           // cycle 19
        DEC               // cycle 20
        BRNE_ALT          // cycle 21
        "nop\n"           // cycle 22
        CBI3              // cycle 23, 24
        DONE

#undef SBI1
#undef SBI2
#undef SBI3
#undef CBI1
#undef CBI2
#undef CBI3
#undef ROL1
#undef ROL2
#undef ROL3
#undef BRNE_ALT
#undef BRNE_0
#undef DONE

        "led_strip_asm_end%=: "
        : "=e" (colors1),
          "=e" (colors2),
          "=e" (colors3),
          "=&r" (b1),            // %3 temporarily holds the brightness of strip1
          "=&r" (b2),            // %4 temporarily holds the brightness of strip2
          "=&r" (b3),            // %5 temporarily holds the brightness of strip3
          "=&r" (i)
        : "0" (colors1),         // %a0 points to the next color for strip1
          "1" (colors2),         // %a1 points to the next color for strip2
          "2" (colors3),         // %a2 points to the next color for strip3
          "I" (_SFR_IO_ADDR(LED_STRIP1_PORT)),  //  %10 is a port register (e.g. PORTC)
          "I" (LED_STRIP1_PIN),                 // %11 is a pin number (0-8)
          "I" (_SFR_IO_ADDR(LED_STRIP2_PORT)),  // %12 is a port register
          "I" (LED_STRIP2_PIN),                 // %13 is a pin number
          "I" (_SFR_IO_ADDR(LED_STRIP3_PORT)),  // %14 is a port register
          "I" (LED_STRIP3_PIN)                  // %15 is a pin number
    );

    // Uncomment the line below to temporarily enable interrupts between each color.
    //sei(); asm volatile("nop\n"); cli();
  }
  sei();          // Re-enable interrupts now that we are done.
  _delay_us(80);  // Send the reset signal.
}

#define LED_COUNT 60
rgb_color colors1[LED_COUNT], colors2[LED_COUNT], colors3[LED_COUNT];

int main()
{
  unsigned int time = 0;

  while(1)
  {
    unsigned char x;
    unsigned int i;

    // Dislay pretty patterns on most of the LEDs
    for(i = 0; i < LED_COUNT; i++)
    {
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

    // Display a test pattern on the first LEDs that helps us debug the code.
    x = (time >> 7 & 7);
    for (i = 0; i < 4; i++)
    {
      colors1[i] = (x & 1) ? (rgb_color){ 0xFF, 0xFF, 0xFF } : (rgb_color){ 0x00, 0x00, 0x00 };
      colors2[i] = (x & 2) ? (rgb_color){ 0xFF, 0xFF, 0xFF } : (rgb_color){ 0x00, 0x00, 0x00 };
      colors3[i] = (x & 4) ? (rgb_color){ 0xFF, 0xFF, 0xFF } : (rgb_color){ 0x00, 0x00, 0x00 };
    }
    colors1[4] = colors2[4] = colors3[4] = (rgb_color){ (x << 5), 0, 0 };


    led_strip_write3(colors1, colors2, colors3, LED_COUNT);

    _delay_ms(20);
    time += 20;
  }
}
