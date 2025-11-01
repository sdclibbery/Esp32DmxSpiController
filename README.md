# Esp32DmxSpiController
ESP32 based DMX controller for SPI LED strips

## Libraries
NeoPixelBus
Dmx_ESP32 https://github.com/devarishi7/Dmx_ESP32

## DMX Channel Mapping
1: Mode
2: Control
3: Smoothing
4-6: Back RGB
7-9: Fore RGB

## Modes
### Palettes
There are 4 variations of each mode with the following palettes:
0: BF: lerp from back colour to fore colour
1: OBF: lerp from off (black) to back to fore
2: BFO: lerp from back to fore to off (black)
3: BOF: lerp from back to off (black) to fore

### Solid
0-3: Solid: Blend entire strip through the palette based on control, smoothing does nothing
 ??? Smoothing applies a bow curve along the strip?
4-7: Noise: Perlin noise. Control is seed. Smoothing is scale (or octaves??)
8-11: Blocks: alternating blocks. Control is block size, smoothing is lerp power
 ??? This is a bit crap? WHat use is it?

### Meter
12-15: StartBar: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
16-19: EndBar: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
20-23: MidBar: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip

### Drawing
24-27: Draw: Control sets draw pos. Fore is drawn into the strip at draw pos. Smoothing is scroll pos.
       ????how does scroll work when channel wraps a byte? - same as for promql rate() :-)
28-31: DrawLine: Same as draw, but draw all pixels between last pos and new pos
32-35: Fade: Same as Draw, but drawn pixels slowly fade back to back colour. Smoothing is fade time
36-39: FadeLine: Same as Fade but subsequent draw positions are connected not separate


