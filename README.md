# Esp32DmxSpiController
ESP32 based DMX controller for SPI LED strips

## Libraries
NeoPixelBus, SparkFunDMX

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
0: Solid: Blend entire strip through the palette based on control, smoothing does nothing
Blocks: Alternating blocks betwen back and fore. Control is block size, Smoothing is lerp size

### Gradient
StartGradient: fore colour rises from start of strip, control is length of fore section, smoothing is lerp size
EndGradient: fore colour falls from end of strip, control is length of fore section, smoothing is lerp size
MidGradient: fore colour expands from centre of strip, control is length of fore section, smoothing is lerp size
Paddle: One light is on with fore colour. Control is location. Fades through back colour when it moves. Smoothing is trail fade rate

### Drawing
Draw: Control sets draw pos. Fore is drawn into the strip at draw pos. Smoothing is scroll pos.
       ????how does scroll work when channel wraps a byte?
Fade: Same as Draw, but drawn pixels slowly fade back to back colour

### Noise
Noise: Perlin noise between fore and back. Control is seed. Smoothing is scale (or octaves??).
Noise2: Same as Noise, but noise is between off/back/fore
Flame: Back/Fore/off noise from bottom of strip. Control is height. Smoothing is speed.
        ??Can this be a gradient variation?? Have gradients from back/fore/off not just back/fore??

