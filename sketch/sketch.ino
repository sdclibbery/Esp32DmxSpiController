#include <cmath>
#include <Arduino.h>
#include <SparkFunDMX.h>
#include <NeoPixelBus.h>

/* TODO
 ?? Should have a dedicated palette channel, and it should include curves and asymmetric, as well as animated?
 Try DMX
 More modes
 DMX base channel
 3 strips
 Consider rewriting Serial handling: https://gemini.google.com/app/9bacdb891b977834 (but Geminis code is all over the place, inc setTimeout taking MILLIs)
 DMX pass through
*/

// Hardware Definitions for ESP32 DMX Shield (UART2)
// The SparkFun DMX Shield typically uses UART2 on the ESP32.
#define DMX_UART_NUM  2
#define DMX_RX_PIN    16 // UART2 Receive Pin
#define DMX_TX_PIN    17 // UART2 Transmit Pin
#define DMX_EN_PIN    21 // Direction Enable Pin (Controls RS-485 transceiver)

// DMX 
HardwareSerial dmxSerial(DMX_UART_NUM);
SparkFunDMX dmx;
#define DMX_MODE      0
#define DMX_CONTROL   1
#define DMX_SMOOTH    2
#define DMX_BACK_R    3
#define DMX_BACK_G    4
#define DMX_BACK_B    5
#define DMX_FORE_R    6
#define DMX_FORE_G    7
#define DMX_FORE_B    8
#define DMX_CHANNELS  9
const uint16_t dmxStartChannel = 1;
uint8_t dmxData[DMX_CHANNELS] = {0};

// LED setup
// Pin Definitions
#define CLOCK 5
#define DATA0 19
#define DATA1 18
#define DATA2 27 
const uint16_t pixelCount = 16;
typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812xMethod> PixelStrip;
// typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1X8Sk6812Method> PixelStrip;
PixelStrip strip1(pixelCount, DATA2);

struct FixtureData {
  uint8_t mode;
  float control;
  float smooth;
  RgbColor fore;
  RgbColor back;
};

float limit (float x) {
  if (std::isnan(x)) { return 0.0f; }
  if (x < 0.0f) { return 0.0f; }
  if (x > 1.0f) { return 1.0f; }
  return x;
}

float powerSmooth (float x, float p) {
  x = limit(x);
  p = limit(p);
  if (p < 0.5f) { return std::pow(x, 1.0f + (0.5f - p)*128.0f); }
  else { return std::pow(x, 1.0f / (1.0f + (p - 0.5f)*128.0f)); }
}

RgbColor palette3Way(const RgbColor& a, const RgbColor& b, const RgbColor& c, float lerp) {
  if (lerp < 0.5) {
    return RgbColor::LinearBlend(a, b, lerp*2.0f);
  } else {
    return RgbColor::LinearBlend(b, c, (lerp-0.5f)*2.0f);
  }
}

const RgbColor off = RgbColor(0,0,0);
RgbColor palette(uint8_t type, const RgbColor& back, const RgbColor& fore, float lerp) {
  lerp = limit(lerp);
  switch (type) {
    case 0: return RgbColor::LinearBlend(back, fore, lerp);
    case 1: return palette3Way(off, back, fore, lerp);
    case 2: return palette3Way(back, fore, off, lerp);
    case 3: return palette3Way(back, off, fore, lerp);
  }
  return off;
}

float gradient (float lerp, float con, float smooth) {
  if (lerp < con) { lerp = 1.0f; } // Solid section
  else { lerp = 1.0f - (lerp - con)/(1.0f - con); } // Gradient sectioon
  lerp = limit(lerp);
  lerp = powerSmooth(lerp, smooth);
  return lerp;
}

// 0-3: Solid: Blend entire strip through the palette based on control, smoothing does nothing
void modeSolid(const FixtureData& data, PixelStrip& strip) {
  uint8_t paletteType = data.mode & 0x03;
  for (uint16_t i=0; i<strip.PixelCount(); i++ ) {
    strip.SetPixelColor(i, palette(paletteType, data.back, data.fore, data.control));
  }
}

// 12-15: StartBar: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
void modeStartBar(const FixtureData& data, PixelStrip& strip) {
  uint8_t paletteType = data.mode & 0x03;
  for (uint16_t i=0; i<strip.PixelCount(); i++ ) {
    float lerp = (float)i / (float)(strip.PixelCount()-1); // Position along strip
    lerp = gradient(lerp, data.control, data.smooth);
    strip.SetPixelColor(i, palette(paletteType, data.back, data.fore, lerp));
  }
}

// 16-19: EndBar: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
void modeEndBar(const FixtureData& data, PixelStrip& strip) {
  uint8_t paletteType = data.mode & 0x03;
  for (uint16_t i=0; i<strip.PixelCount(); i++ ) {
    float lerp = (float)i / (float)(strip.PixelCount()-1); // Position along strip
    lerp = 1.0f - lerp; // Go from end
    lerp = gradient(lerp, data.control, data.smooth);
    strip.SetPixelColor(i, palette(paletteType, data.back, data.fore, lerp));
  }
}

// 20-23: MidBar: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
void modeMidBar(const FixtureData& data, PixelStrip& strip) {
  uint8_t paletteType = data.mode & 0x03;
  for (uint16_t i=0; i<strip.PixelCount(); i++ ) {
    float lerp = (float)i / (float)(strip.PixelCount()-1); // Position along strip
    lerp = std::abs(0.5f - lerp)*2.0f; // Go outwards from middle
    lerp = gradient(lerp, data.control, data.smooth);
    strip.SetPixelColor(i, palette(paletteType, data.back, data.fore, lerp));
  }
}

void updateStrip(const FixtureData& data, PixelStrip& strip) {
  uint8_t mode = data.mode & 0xfc;
  switch (mode) {
    case 0: modeSolid(data, strip); break;
    // case 4: modeNoise(data, strip); break;
    // case 8: modeBlocks(data, strip); break;
    case 12: modeStartBar(data, strip); break;
    case 16: modeEndBar(data, strip); break;
    case 20: modeMidBar(data, strip); break;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial attach
  Serial.println("Setup starting.");

  dmxSerial.begin(250000, SERIAL_8N2, DMX_RX_PIN, DMX_TX_PIN);
  dmx.begin(dmxSerial, DMX_EN_PIN, 512); 
  dmx.setComDir(DMX_READ_DIR);

  strip1.Begin(); strip1.Show(); // Clear strip

  Serial.println("Setup complete.");
}

uint8_t c = 0;
uint8_t m = 0;
void loop() {
  c+=2;
  if (c == 0) { m += 8; Serial.printf("update t=0 m=%d\n", m); }
  delay(10);

  while(dmx.dataAvailable() == false) {
      dmx.update();
  }
  dmx.readBytes(dmxData, DMX_CHANNELS, dmxStartChannel);
Serial.printf("bytes %d %d %d\n", dmxData[DMX_MODE], dmxData[DMX_CONTROL], dmxData[DMX_SMOOTH]);
  FixtureData fixtureData;
  fixtureData.mode = dmxData[DMX_MODE];
  fixtureData.control = ((float)dmxData[DMX_CONTROL])/255;
  fixtureData.smooth = ((float)dmxData[DMX_SMOOTH])/255;
  fixtureData.fore = RgbColor(dmxData[DMX_FORE_R],dmxData[DMX_FORE_G],dmxData[DMX_FORE_B]);
  fixtureData.back = RgbColor(dmxData[DMX_BACK_R],dmxData[DMX_BACK_G],dmxData[DMX_BACK_B]);
  // fixtureData.mode = 13;
  // fixtureData.control = ((float)c)/255.0f;
  // fixtureData.smooth = ((float)m)/255.0f;
  // fixtureData.fore = RgbColor(0,0,16);
  // fixtureData.back = RgbColor(16,0,0);

  updateStrip(fixtureData, strip1);
  strip1.Show();
}
