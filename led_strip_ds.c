// This is AVR code for driving the RGB LED strips from Pololu.
//
// This version can control any AVR pin.  It uses "lds" and "sts" instructions,
// which means it has to write to the entire port register every time it toggles
// the I/O line, but that should be safe because interrupts are disabled.
//
// It allows complete control over the color of an arbitrary number of LEDs.
// This implementation disables interrupts while it does bit-banging with
// inline assembly.

// This line specifies the frequency your AVR is running at.
// This code supports 20 MHz, 16 MHz and 8MHz
#define F_CPU 20000000

// These lines specify what pin the LED strip is on.
// You will either need to attach the LED strip's data line to PH3
// (pin 6 on the Arduino Mega 2560) or change these
// lines to specify a different pin.
#define LED_STRIP_PORT PORTC
#define LED_STRIP_DDR  DDRC
#define LED_STRIP_PIN  0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

// The rgb_color struct represents the color for an 8-bit RGB LED.
// Examples:
//   Black:      (rgb_color){ 0, 0, 0 }
//   Pure red:   (rgb_color){ 255, 0, 0 }
//   Pure green: (rgb_color){ 0, 255, 0 }
//   Pure blue:  (rgb_color){ 0, 0, 255 }
//   White:      (rgb_color){ 255, 255, 255}
typedef struct rgb_color
{
  uint8_t red, green, blue;
} rgb_color;

// led_strip_write sends a series of colors to the LED strip, updating the LEDs.
// The colors parameter should point to an array of rgb_color structs that hold
// the colors to send.
// The count parameter is the number of colors to send.
// This function takes about 1.1 ms to update 30 LEDs.
// Interrupts must be disabled during that time, so any interrupt-based library
// can be negatively affected by this function.
// Timing details at 20 MHz:
//   0 pulse  = 400 ns
//   1 pulse  = 850 ns
//   "period" = 1300 ns
// Timing details at 16 MHz:
//   0 pulse  = 375 ns
//   1 pulse  = 812.5 ns
//   "period" = 1500 ns
void __attribute__((noinline)) led_strip_write(rgb_color * colors, uint16_t count)
{
  // Set the pin to be an output driving low.
  LED_STRIP_PORT &= ~(1 << LED_STRIP_PIN);
  LED_STRIP_DDR |= (1 << LED_STRIP_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  while (count--)
  {
    uint8_t portValue = LED_STRIP_PORT;
    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile (
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0\n"
        "rcall send_led_strip_byte%=\n"  // Send red component.
        "ld __tmp_reg__, -%a0\n"
        "rcall send_led_strip_byte%=\n"  // Send green component.
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "rcall send_led_strip_byte%=\n"  // Send blue component.
        "rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

        // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
        "send_led_strip_byte%=:\n"
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
#if F_CPU == 8000000
        "rol __tmp_reg__\n"                      // Rotate left through carry.
#endif
        "sts %2, %4\n"                           // Drive the line high.

#if F_CPU != 8000000
        "rol __tmp_reg__\n"                      // Rotate left through carry.
#endif

#if F_CPU == 16000000
        "nop\n" "nop\n"
#elif F_CPU == 20000000
        "nop\n" "nop\n" "nop\n" "nop\n"
#elif F_CPU != 8000000
#error "Unsupported F_CPU"
#endif

        // If the bit to send is 0, drive the line low now.
        "brcs .+4\n" "sts %2, %3\n"

#if F_CPU == 8000000
        "nop\n" "nop\n"
#elif F_CPU == 16000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
#elif F_CPU == 20000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n"
#endif

        // If the bit to send is 1, drive the line low now.
        "brcc .+4\n" "sts %2, %3\n"

        "ret\n"
        "led_strip_asm_end%=: "
        : "=b" (colors)
        : "0" (colors),           // %a0 points to the next color to display
          "" (&LED_STRIP_PORT),   // %2 is the port register (e.g. PORTH)
          "r" ((uint8_t)(portValue & ~(1 << LED_STRIP_PIN))),  // %3
          "r" ((uint8_t)(portValue | (1 << LED_STRIP_PIN)))    // %4
    );

    // Uncomment the line below to temporarily enable interrupts between each color.
    //sei(); asm volatile("nop\n"); cli();
  }
  sei();          // Re-enable interrupts now that we are done.
  _delay_us(80);  // Send the reset signal.
}

#define LED_COUNT 60
rgb_color colors[LED_COUNT];

int main()
{
  uint16_t time = 0;
  while (1)
  {
    for (uint16_t i = 0; i < LED_COUNT; i++)
    {
      uint8_t x = (time >> 2) - 8 * i;
      colors[i] = (rgb_color){ x, 255 - x, x };
    }

    led_strip_write(colors, LED_COUNT);

    _delay_ms(20);
    time += 20;
  }
}
