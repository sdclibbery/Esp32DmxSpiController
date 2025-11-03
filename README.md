# Esp32DmxSpiController
ESP32 based DMX controller for SPI LED strips

## Libraries
NeoPixelBus
Dmx_ESP32 https://github.com/devarishi7/Dmx_ESP32

## DMX Channel Mapping
1: Mode
2: Palette
3: Control
4: Smoothing
5-7: Back RGB
8-10: Fore RGB

## Palettes
0: BF: lerp from back colour to fore colour
1: OBF: lerp from off (black) to back to fore
2: BFO: lerp from back to fore to off (black)
3: BOF: lerp from back to off (black) to fore
...asymmetric, rainbow, rotating, with white, with rainbow in, OBFO etc...

## Modes
### Solid
0: Fade: Whatever is currently showing, fade it down through the palette. Control does nothing, smooth is fade time.
1: Solid: Blend entire strip through the palette based on control, smoothing does nothing
 ??? Smoothing applies a bow curve along the strip?
2: Noise: Perlin noise. Control is seed. Smoothing is scale (or octaves??)

### Meter
10: StartGradient: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
11: EndGradient: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
12: MidGradient: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
13: StartFade: solid bar rises from start of strip, control is length of bar, smooth is fade time
14: EndFade: solid bar falls from end of strip, control is length of bar, smooth is fade time
15: MidFade: solid bar expands from centre of strip, control is length of bar, smooth is fade time

### Drawing
20: Draw: Control sets draw pos. Fore is drawn into the strip at draw pos. Smoothing is palette pos to draw with.
21: DrawFade: Same as Draw, but drawn pixels slowly fade back to back colour. Smoothing is fade time
22: DrawScroll: Control sets draw pos. Fore is drawn into the strip at draw pos. Smoothing is scroll pos.
23: DrawScrollFade: Control sets draw pos. Fore is drawn into the strip at draw pos. Smoothing is scroll pos. Fade time is fixed long.

### Line drawing
30: Line: Same as Draw, but draw all pixels between last pos and new pos
31: LineFade: Same as DrawFade but subsequent draw positions are connected not separate
32: LineScroll: Same as DrawScroll, but draw all pixels between last pos and new pos
32: LineScrollFade: Same as DrawScrollFade, but draw all pixels between last pos and new pos


