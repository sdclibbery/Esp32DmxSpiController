# Esp32DmxSpiController
ESP32 based DMX controller for SPI LED strips

## TODO
### SOFTWARE
* plotScroll lineScroll DO NOT MAKE SENSE because they plot with 1.0f and never darken anything
 * Have to remove and replace with:
 * Want a mode to draw at end, with palette draw control, scrolling, and fixed fade (ticker/matrix mode)
* Fizzle fade modes
* EndsGradient and EndsFade: opposite of mid
* Fade range: no point going to zero fade time, might want to go to more than 1s?
* Blur (diffuse and slight fade): own mode and for more plot/line modes
* More modes
* More palettes
* Set DMX base channel and remember it: how to set? Web interface? Serial? DIPs? Buttons+display? How to remember?
* Support for 3 strips
### HARDWARE
* Project case
* DMX passhrough connectors
* Large PSU
* 5 strip connectors
* 5 SK6812 strips
* Stands/mounting for strips
* Leads for strips
* USB port on case?

## Libraries used
NeoPixelBus
Dmx_ESP32 https://github.com/devarishi7/Dmx_ESP32

## DMX Channel Mapping
1. Mode
2. Palette
3. Control
4. Smoothing
5-7. Back RGB
8-10. Fore RGB

## Palettes
0. BF: lerp from back colour to fore colour
1. OBF: lerp from off (black) to back to fore
2. BFO: lerp from back to fore to off (black)
3. BOF: lerp from back to off (black) to fore
... OFB for flames :-p
...asymmetric, rainbow, off->rainbow(red->pink), rotating, with white, with rainbow in, OBFO etc...
...BFBFBF, black body, oil, neon, flame, sunset etc
...Modes that interpolate in gamma space? or hsv? or lablch? or all should???

## Modes
### Background
0. Fade: Whatever is currently showing, fade it down through the palette. Control does nothing, smooth is fade time.
 ??? control should set target palette position, and it fades to that?
1. Scroll: Scroll whatever is currently showing. Control does nothing, smooth is scroll pos.
2. Fizzle: Whatever is currently showing, fizzle it down through the palette. Control does nothing, smooth is fizzle time.
 ??? control should set target palette position, and it fades to that?

### Full strip
10. Solid: Blend entire strip through the palette based on control, smoothing applies a curved palette profile
11. Gradient: control sets start palette position, smoothing sets end palette position, blend between the two
12. Sine: Sine waves. Control is phase, smoothing is wavelength
13. Noise: Perlin noise. Control is seed. Smoothing is scale

### Meter
20. StartGradient: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
21. EndGradient: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
22. MidGradient: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
23. EndsGradient: solid bar expands from both ends of strup, control is length of bar, smooth is lerp power in rest of strip
25. StartFade: solid bar rises from start of strip, control is length of bar, smooth is fade time
26. EndFade: solid bar falls from end of strip, control is length of bar, smooth is fade time
27. MidFade: solid bar expands from centre of strip, control is length of bar, smooth is fade time
28. EndsFade: solid bar expands from both ends of strip, control is length of bar, smooth is fade time

### Plotting
30. Plot: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is palette pos to plot with.
31. PlotFade: Same as Plot, but drawn pixels slowly fade back to back colour. Smoothing is fade time
32. PlotScroll: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos.
33. PlotScrollFade: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos. Fade time is fixed long.
34. PlotFizzle: Same as Plot, but drawn pixels slowly fizzle back to back colour. Smoothing is fizzle time

### Line drawing
40. Line: Same as Plot, but plot all pixels between last pos and new pos
41. LineFade: Same as PlotFade but subsequent plot positions are connected not separate
42. LineScroll: Same as PlotScroll, but plot all pixels between last pos and new pos
43. LineScrollFade: Same as PlotScrollFade, but plot all pixels between last pos and new pos
44. LineFizzle: Same as PlotFizzle but subsequent plot positions are connected not separate

