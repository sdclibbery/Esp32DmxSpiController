# Esp32DmxSpiController
ESP32 based DMX controller for SPI LED strips. Tries to strike a balance between a sane number of channels (so not mapping each pixel to its own channels), and also allowing a good amount of programmability.

## TODO
### SOFTWARE
* Global dimmer and global gamma on channels 31 and 32
 TEST!!!
* noise shows a big block of background at the far end of the strip when smooth is 1 (eg oil purple)
 TEST!!
* blackbody palette goes to black which looks wrong (or is it just gamma is wrong?)
 TRY GAMMA; change default if needed
* TEST white balance with an alternating pixel hack
* ? Control usage for background modes..??
* ? Vu meter mode with slowly falling peak indicator

## Libraries used
NeoPixelBus
Dmx_ESP32 https://github.com/devarishi7/Dmx_ESP32

## DMX Channel Mapping
### Each strip (x3)
1. Mode
2. Palette
3. Control
4. Smoothing
5-7. Back RGB
8-10. Fore RGB
### Global
31. Global dimmer. 0 defaults to full brightness for convenience
32. Global gamma. maps from 1/4 to 4, except 0 defaults to gamma of 2 for convenience

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
### Back/fore blends, but non linear blend
20-28. Same as 0-8 but lerp pushed to end (x^2 + x)/2
### Back/fore blends, but blended via scaled sum
30-38. Same as 0-8 but with blended via scaled sum
### Back/fore blends, but blended via fore - back
40-48. Same as 0-8 but with fore-back
### Back/fore blends, but no blending (step blend)
50-58. Same as 0-8 but with no blending
### Back/fore blends, but power curves applied to each channel individually
60-68. Same as 0-8 but with per channel power curve

### Back/fore blends, but dithered
100-108. Same as 0-8 but dithered instead of blended
### Back/fore blends, but with fizzle
110-118. Same as 0-8 but with fizzle
### Back/fore blends, but with wizzle
120-128. Same as 0-8 but with wizzle (occasional random flick to white, irrespective of palette)

### Preset palettes not using DMX back/fore colours at all
240. Rainbow
241. Black body
242. Oil
243. Neon
244. Fire
245. Heat

## Modes
### 0 - Fade out
0. Fade: Whatever is currently showing, fade it down through the palette. Control does nothing, smooth is fade time.
 ??? control should set target palette position, and it fades to that?
1. Fizzle: Whatever is currently showing, fizzle it down through the palette. Control does nothing, smooth is fizzle time.
 ??? control should set target palette position, and it fizzles to that?
2. Scroll: Scroll whatever is currently showing. Control does nothing, smooth is scroll pos.
3. Blur: Blur whatever is currently showing with a slight fade. Control does nothing, smooth is blur rate.

### 10 - Full strip / Background
10. Solid: Blend entire strip through the palette based on control, smoothing applies a curved palette profile
11. Gradient: control sets start palette position, smoothing sets end palette position, blend between the two
12. Droplet: plot at random pos when control has a rising edge. Smooth is blur rate.
13. Xor: XOR bit patterns. control is mod control, smooth is pattern y axis

### 20 - Waveforms
20. Noise: Perlin noise. Control is seed. Smoothing is scale
21. Sine: Sine waves. Control is phase, smoothing is wavelength
22. Saw: Saw waves. Control is phase, smoothing is wavelength
23. Tri: Triangle waves. Control is phase, smoothing is wavelength

### 50 - Meter Gradient
50. StartGradient: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
51. EndGradient: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
52. MidGradient: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
53. EndsGradient: solid bar expands from both ends of strup, control is length of bar, smooth is lerp power in rest of strip

### 60 - MeterFade
60. StartFade: solid bar rises from start of strip, control is length of bar, smooth is fade time
61. EndFade: solid bar falls from end of strip, control is length of bar, smooth is fade time
62. MidFade: solid bar expands from centre of strip, control is length of bar, smooth is fade time
63. EndsFade: solid bar expands from both ends of strip, control is length of bar, smooth is fade time

### 70 - Meterfizzle
70. Startfizzle: solid bar rises from start of strip, control is length of bar, smooth is fizzle time
71. Endfizzle: solid bar falls from end of strip, control is length of bar, smooth is fizzle time
72. Midfizzle: solid bar expands from centre of strip, control is length of bar, smooth is fizzle time
73. Endsfizzle: solid bar expands from both ends of strip, control is length of bar, smooth is fizzle time

### 80 - MeterBlur
80. StartBlur: pixel drawn at start of strip, control is palette entry of pixel, smooth is blur rate
81. EndBlur: pixel drawn at end of strip, control is palette entry of pixel, smooth is blur rate
82. MidBlur: pixel drawn at centre of strip, control is palette entry of pixel, smooth is blur rate
83. EndsBlur: pixels drawn at both ends of strip, control is palette entry of pixel, smooth is blur rate

### 90 - MeterWave
90. StartWave: pixel drawn at start of strip, control is palette entry of pixel, smooth is spring
91. EndWave: pixel drawn at end of strip, control is palette entry of pixel, smooth is spring
92. MidWave: pixel drawn at centre of strip, control is palette entry of pixel, smooth is spring
93. EndsWave: pixels drawn at both ends of strip, control is palette entry of pixel, smooth is spring

### 100 - Ticker
100. StartTicker: Control sets palette entry to draw at start of strip. Smoothing is scroll pos
101. EndTicker: Control sets palette entry to draw at end of strip. Smoothing is scroll pos
102. MidTicker: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways
103. EndsTicker: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways

### 110 - TickerFade
110. StartTickerFade: Control sets palette entry to draw at start of strip. Smoothing is scroll pos. A fixed slow fade is applied
111. EndTickerFade: Control sets palette entry to draw at end of strip. Smoothing is scroll pos. A fixed slow fade is applied
112. MidTickerFade: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways. A fixed slow fade is applied
113. EndsTickerFade: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways. A fixed slow fade is applied

### 150 - Plot
150. Plot: Control sets plot pos. Smoothing is palette pos to plot at the plot pos.
151. PlotFade: Same as Plot, but drawn pixels slowly fade back to back colour. Smoothing is fade time
152. PlotScrollFade: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos. Fade time is fixed long.
153. PlotFizzle: Same as Plot, but drawn pixels slowly fizzle back to back colour. Smoothing is fizzle time

### 160 - Line
160. Line: Same as Plot, but plot all pixels between last pos and new pos
161. LineFade: Same as PlotFade but subsequent plot positions are connected not separate
162. LineScrollFade: Same as PlotScrollFade, but plot all pixels between last pos and new pos
163. LineFizzle: Same as PlotFizzle but subsequent plot positions are connected not separate
