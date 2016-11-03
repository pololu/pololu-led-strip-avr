# Example AVR code for addressable RGB LED strips from Pololu

[www.pololu.com](https://www.pololu.com/)

## Summary

This is example AVR C code for controlling these addressable RGB LED products from Pololu:

* [Addressable RGB 30-LED Strip, 5V, 1m &#40;SK6812)](https://www.pololu.com/product/2526)
* [Addressable RGB 60-LED Strip, 5V, 2m &#40;SK6812)](https://www.pololu.com/product/2527)
* [Addressable RGB 150-LED Strip, 5V, 5m &#40;SK6812)](https://www.pololu.com/product/2528)
* [Addressable RGB 60-LED Strip, 5V, 1m &#40;SK6812)](https://www.pololu.com/product/2529)
* [Addressable RGB 120-LED Strip, 5V, 2m &#40;SK6812)](https://www.pololu.com/product/2530)
* [Addressable High-Density RGB 72-LED Strip, 5V, 0.5m &#40;SK6812)](https://www.pololu.com/product/2531)
* [Addressable RGB 30-LED Strip, 5V, 1m &#40;WS2812B)](https://www.pololu.com/product/2546)
* [Addressable RGB 60-LED Strip, 5V, 2m &#40;WS2812B)](https://www.pololu.com/product/2547)
* [Addressable RGB 150-LED Strip, 5V, 5m &#40;WS2812B)](https://www.pololu.com/product/2548)
* [Addressable RGB 60-LED Strip, 5V, 1m &#40;WS2812B)](https://www.pololu.com/product/2549)
* [Addressable RGB 120-LED Strip, 5V, 2m &#40;WS2812B)](https://www.pololu.com/product/2550)
* [Addressable High-Density RGB 72-LED Strip, 5V, 0.5m &#40;WS2812B)](https://www.pololu.com/product/2551)
* [Addressable Through-Hole 5mm RGB LED with Diffused Lens, WS2811 Driver (10-Pack)](https://www.pololu.com/product/2535)
* [Addressable Through-Hole 8mm RGB LED with Diffused Lens, WS2811 Driver (10-Pack)](https://www.pololu.com/product/2536)
* [Adafruit 16 WS2812 LED NeoPixel Ring](https://www.pololu.com/product/2537)
* [Adafruit 24 WS2812 LED NeoPixel Ring](https://www.pololu.com/product/2538)
* [Adafruit 15 WS2812 LED NeoPixel 1/4-Ring](https://www.pololu.com/product/2539)
* [Adafruit 5&times;8 WS2812 LED NeoPixel Shield for Arduino](https://www.pololu.com/product/2772)
* [Addressable RGB 30-LED Strip, 5V, 1m &#40;High-Speed TM1804)](https://www.pololu.com/product/2543)
* [Addressable RGB 60-LED Strip, 5V, 2m &#40;High-Speed TM1804)](https://www.pololu.com/product/2544)
* [Addressable RGB 150-LED Strip, 5V, 5m &#40;High-Speed TM1804)](https://www.pololu.com/product/2545)

This example code is optimized for the SK6812 and WS2812B, so it transmits the colors in green-red-blue order.

If you have a WS2811 LED or a high-speed TM1804 LED strip, please note that its red and green channels are swapped relative to the SK6812 and WS2812B, so you will need to swap those channels in your code.  You might prefer to use the version of this code from commit 96bee54 (committed on 2013-10-10), which does not require you to swap red and green.

This version of the code does NOT work with the older, low-speed TM1804 strips (items #2540, #2541, and #2542).  If you have one of those, you should use the version of this code from commit edc9e9d (committed on 2013-05-09).

This code allows complete control over the color of an arbitrary number of LED strips with an arbitrary number of LEDs.  Each LED can be individually controlled, and LED strips can be chained together.

For more details, see `led_strip.c`.
