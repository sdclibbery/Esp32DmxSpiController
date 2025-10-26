# Esp32DmxSpiController
ESP32 based DMX controller for SPI LED strips

## Libraries
FastLED, SparkFunDMX

## DMX Channel Mapping
1: Mode
2: Control
3: Smoothing
4-6: Back RGB
7-9: Fore RGB

## Modes
0: Solid: Blend entire strip between back and fore based on control, smoothing does nothing
1: VU: fore colour rises from start of strip, control is length of fore section, smoothing is length of lerp at end of section
2: VU: fore colour falls from end of strip, control is length of fore section, smoothing is length of lerp at end of section
3: VU: fore colour expands from centre of strip, control is length of fore section, smoothing is length of lerp at ends of section
blocks, flow, fizzle, race, hsv blended VU modes allowing rainbow?

