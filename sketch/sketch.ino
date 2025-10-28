#include <Arduino.h>
#include <SparkFunDMX.h>
#include <NeoPixelBus.h>

/* TODO
 First actual mode: 0: Solid: Blend entire strip between back and fore based on control, smoothing does nothing
  0: Solid: Blend entire strip through the palette based on control, smoothing does nothing
  4 palettes from mode
  function for actual mode; pass palette fn in as fn ptr
 Consider rewriting Serial handling: https://gemini.google.com/app/9bacdb891b977834 (but Geminis code is all over the place, inc setTimeout taking MILLIs)
 DMX base channel
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
const uint16_t pixelCount = 4;
typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812xMethod> PixelStrip;
// typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1X8Sk6812Method> PixelStrip;
PixelStrip strip1(pixelCount, DATA0);

struct FixtureData {
  uint8_t mode;
  float control;
  float smooth;
  RgbColor fore;
  RgbColor back;
};

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

RgbColor paletteBF(const RgbColor& back, const RgbColor& fore, float lerp) {
  return RgbColor::LinearBlend(back, fore, lerp);
}

void modeSolid(const FixtureData& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.PixelCount(); i++ ) {
    strip.SetPixelColor(i, paletteBF(data.back, data.fore, data.control));
  }
}

uint8_t t = 0;
void loop() {
  delay(100);

  while(dmx.dataAvailable() == false) {
      dmx.update();
  }
  dmx.readBytes(dmxData, DMX_CHANNELS, dmxStartChannel);
  FixtureData fixtureData;
  fixtureData.mode = dmxData[DMX_MODE];
  fixtureData.control = ((float)dmxData[DMX_CONTROL])/255;
  fixtureData.smooth = ((float)dmxData[DMX_SMOOTH])/255;
  fixtureData.fore = RgbColor(dmxData[DMX_FORE_R],dmxData[DMX_FORE_G],dmxData[DMX_FORE_B]);
  fixtureData.back = RgbColor(dmxData[DMX_BACK_R],dmxData[DMX_BACK_G],dmxData[DMX_BACK_B]);

  modeSolid(fixtureData, strip1);
  strip1.Show();
}
