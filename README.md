# Esp32DmxSpiController
ESP32 based DMX controller for SPI LED strips

## TODO
### SOFTWARE
* Elastic modes
* PlotBlur mode: control is plot pos, smoothing is blur rate
   Once plotted one frame at one pos, dont after that until pos moves again? Or this is a separate mode
* Also LineBlur
* Vu meter mode with slowly falling peak indicator
* ? Control usage for background modes..??
* Set DMX base channel from DIP switches
* Support for 3 strips
### HARDWARE
* Project case
* DMX passhrough connectors
* DMX channel DIP switches - 4 switches to select bank of 32 channels
* Large PSU
* 5 strip connectors
* 5 SK6812 strips ?? Or just stick with ws2812b??
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
### Using DMX specified back/fore colours, with extra black option, blended in RGB space
0. BF: lerp from back colour to fore colour
1. OBF: lerp from off (black) to back to fore
2. BFO
3. BOF
4. OFB
5. BFBF
6. BFBFBF
7. OBFBF
8. OBFBFBF
### Back/fore blends, but blended in HSV space
10-18. Same as 0-8 but blemd in HSV space
### Back/fore blends, but non linear lerp
20-28. Same as 0-8 but lerp pushed to end (x^2 + x)/2
### Back/fore blends, but with fizzle
30-38. Same as 0-8 but with fizzle
### Back/fore blends, but blended via scaled sum
40-48. Same as 0-8 but with blended via scaled sum
### Back/fore blends, but blended via fore-back
50-58. Same as 0-8 but with fore-back
### Back/fore blends, but with wizzle
60-68. Same as 0-8 but with wizzle (occasional random flick to white, irrespective of palette)
### Preset modes not using DMX back/fore colours at all
240. Rainbow
241. Black body
242. Oil
243. Neon
244. Fire
245. Heat

## Modes
### Background
0. Fade: Whatever is currently showing, fade it down through the palette. Control does nothing, smooth is fade time.
 ??? control should set target palette position, and it fades to that?
1. Fizzle: Whatever is currently showing, fizzle it down through the palette. Control does nothing, smooth is fizzle time.
 ??? control should set target palette position, and it fizzles to that?
2. Scroll: Scroll whatever is currently showing. Control does nothing, smooth is scroll pos.
3. Blur: Blur whatever is currently showing with a slight fade. Control does nothing, smooth is blur rate.

### Full strip / misc
10. Solid: Blend entire strip through the palette based on control, smoothing applies a curved palette profile
11. Gradient: control sets start palette position, smoothing sets end palette position, blend between the two
12. Sine: Sine waves. Control is phase, smoothing is wavelength
13. Noise: Perlin noise. Control is seed. Smoothing is scale
14. Droplet: plot at random pos when control has a rising edge. Smooth is blur rate.

### Meter Gradient
20. StartGradient: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
21. EndGradient: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
22. MidGradient: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
23. EndsGradient: solid bar expands from both ends of strup, control is length of bar, smooth is lerp power in rest of strip

### MeterFade
30. StartFade: solid bar rises from start of strip, control is length of bar, smooth is fade time
31. EndFade: solid bar falls from end of strip, control is length of bar, smooth is fade time
32. MidFade: solid bar expands from centre of strip, control is length of bar, smooth is fade time
33. EndsFade: solid bar expands from both ends of strip, control is length of bar, smooth is fade time

### Meterfizzle
40. Startfizzle: solid bar rises from start of strip, control is length of bar, smooth is fizzle time
41. Endfizzle: solid bar falls from end of strip, control is length of bar, smooth is fizzle time
42. Midfizzle: solid bar expands from centre of strip, control is length of bar, smooth is fizzle time
43. Endsfizzle: solid bar expands from both ends of strip, control is length of bar, smooth is fizzle time

### Plotting
50. Plot: Control sets plot pos. Smoothing is palette pos to plot at the plot pos.
51. PlotFade: Same as Plot, but drawn pixels slowly fade back to back colour. Smoothing is fade time
52. PlotScrollFade: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos. Fade time is fixed long.
53. PlotFizzle: Same as Plot, but drawn pixels slowly fizzle back to back colour. Smoothing is fizzle time

### Line drawing
60. Line: Same as Plot, but plot all pixels between last pos and new pos
61. LineFade: Same as PlotFade but subsequent plot positions are connected not separate
62. LineScrollFade: Same as PlotScrollFade, but plot all pixels between last pos and new pos
63. LineFizzle: Same as PlotFizzle but subsequent plot positions are connected not separate

### Ticker
70. StartTicker: Control sets palette entry to draw at start of strip. Smoothing is scroll pos
71. EndTicker: Control sets palette entry to draw at end of strip. Smoothing is scroll pos
72. MidTicker: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways
73. EndsTicker: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways

### TickerFade
80. StartTickerFade: Control sets palette entry to draw at start of strip. Smoothing is scroll pos. A fixed slow fade is applied
81. EndTickerFade: Control sets palette entry to draw at end of strip. Smoothing is scroll pos. A fixed slow fade is applied
82. MidTickerFade: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways. A fixed slow fade is applied
83. EndsTickerFade: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways. A fixed slow fade is applied

### MeterBlur
90. StartBlur: pixel drawn at start of strip, control is palette entry of pixel, smooth is blur rate
91. EndBlur: pixel drawn at end of strip, control is palette entry of pixel, smooth is blur rate
92. MidBlur: pixel drawn at centre of strip, control is palette entry of pixel, smooth is blur rate
93. EndsBlur: pixels drawn at both ends of strip, control is palette entry of pixel, smooth is blur rate
