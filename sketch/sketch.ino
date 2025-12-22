#include <cmath>
#include <Arduino.h>
#include <NeoPixelBus.h>
#include <Dmx_ESP32.h>
#include "modes.h"

// Hardware Definitions for ESP32 DMX Shield (UART2)
#define DMX_UART_NUM  2
#define DMX_RX_PIN    16 // UART2 Receive Pin
#define DMX_TX_PIN    17 // UART2 Transmit Pin
#define DMX_EN_PIN    21 // Direction Enable Pin (Controls RS-485 transceiver)
#define LED_DMX       13 // Onboard red led
dmxRx dmxReceive = dmxRx(&Serial1, DMX_RX_PIN, DMX_RX_PIN, DMX_EN_PIN, LED_DMX, LOW);  // The LED is on the board to show when DMX is being received

// DIP pins
#define DIP_PIN_32    25
#define DIP_PIN_64    26
#define DIP_PIN_128   33
#define DIP_PIN_256   32

// LED setup
#define LED_CLOCK 5
#define LED_DATA0 27 // Top most output
#define LED_DATA1 18
#define LED_DATA2 19
// typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812xMethod> NeoPixelStrip;
typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1X8Sk6812Method> NeoPixelStrip;

// DMX
uint16_t dmxStartChannel = 1; // Default to 1 but gets set from DIP switches

// Output color conversion
static float dmxDimmer = 0.0f;
static float dmxGamma = 0.0f;
static float applyGamma (float v) {
  float gammaValue = dmxGamma == 0.0f ? 1.25f : 0.25f+dmxGamma*3.75f; // For convenience, the default dmx value of 0 is gamma of 1.25. Otherwise you have to always set the global gamma channel to get useful output
  return std::pow(v, gammaValue);
}
static uint8_t channel (float v) {
  float dimmer = dmxDimmer == 0.0f ? 1.0f : dmxDimmer; // For convenience, the default dmx value of 0 is full-on. Otherwise you have to always set the global dimmer channel to do anything
  return applyGamma(v)*dimmer*255;
}

static RgbColor toRgb (Rgb color, uint16_t index) { return RgbColor(channel(color.red), channel(color.green), channel(color.blue)); }
static RgbwColor toRgbw (Rgb color, uint16_t index ) {
  uint8_t red = channel(color.red);
  uint8_t green = channel(color.green);
  uint8_t blue = channel(color.blue);
  // if (index%2 == 0) { return RgbwColor(255,255,255, 0); } // For testing RGB vs W balance
  uint8_t white = std::min(std::min(red, green), blue);
  red -= white; green -= white; blue -= white/4; // Account for W LED being yellowy compareed to RGB
  return RgbwColor(red, green, blue, white);
}

// Strips
const uint16_t pixelCount1 = 60;
NeoPixelStrip neoStrip1(pixelCount1, LED_DATA0);
static void setPixel1 (uint16_t index, Rgb color) { neoStrip1.SetPixelColor(index, toRgbw(color, index)); }
PixelStrip pixelStrip1(pixelCount1, setPixel1);
Controls controls1(Rgb(0.1f,0,0),Rgb(0.2f,0,0));

const uint16_t pixelCount2 = 60;
NeoPixelStrip neoStrip2(pixelCount2, LED_DATA1);
static void setPixel2 (uint16_t index, Rgb color) { neoStrip2.SetPixelColor(index, toRgbw(color, index)); }
PixelStrip pixelStrip2(pixelCount2, setPixel2);
Controls controls2(Rgb(0,0.1f,0),Rgb(0,0.2f,0));

const uint16_t pixelCount3 = 60;
NeoPixelStrip neoStrip3(pixelCount3, LED_DATA2);
static void setPixel3 (uint16_t index, Rgb color) { neoStrip3.SetPixelColor(index, toRgbw(color, index)); }
PixelStrip pixelStrip3(pixelCount3, setPixel3);
Controls controls3(Rgb(0,0,0.1f),Rgb(0,0,0.2f));

static void parseSerial (Controls& controls, String data) { // For testing
  Serial.println(data);
  if (data.startsWith("m")) { controls.mode = data.substring(1).toInt(); }
  if (data.startsWith("p")) { controls.palette = data.substring(1).toInt(); }
  if (data.startsWith("c")) { controls.control = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("s")) { controls.smooth = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("r")) { controls.back.red = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("g")) { controls.back.green = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("b")) { controls.back.blue = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("R")) { controls.fore.red = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("G")) { controls.fore.green = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("B")) { controls.fore.blue = ((float)data.substring(1).toInt())/255; }
}

static void parseDmx (Controls& controls, const uint16_t dmxStartChannel) {
  controls.mode = dmxReceive.read(dmxStartChannel + 0);
  controls.palette = dmxReceive.read(dmxStartChannel + 1);
  controls.control = ((float)dmxReceive.read(dmxStartChannel + 2))/255;
  controls.smooth = ((float)dmxReceive.read(dmxStartChannel + 3))/255;
  controls.back.red = ((float)dmxReceive.read(dmxStartChannel + 4))/255;
  controls.back.green = ((float)dmxReceive.read(dmxStartChannel + 5))/255;
  controls.back.blue = ((float)dmxReceive.read(dmxStartChannel + 6))/255;
  controls.fore.red = ((float)dmxReceive.read(dmxStartChannel + 7))/255;
  controls.fore.green = ((float)dmxReceive.read(dmxStartChannel + 8))/255;
  controls.fore.blue = ((float)dmxReceive.read(dmxStartChannel + 9))/255;
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial attach
  Serial.println("Setup starting.");
  Serial.printf("ESP32 Chip Revision: %d\n", ESP.getChipRevision());
  Serial.printf("ESP32 Arduino core version: %s\n", ESP_ARDUINO_VERSION_STR);

  pinMode(DIP_PIN_32, INPUT_PULLUP);
  pinMode(DIP_PIN_64, INPUT_PULLUP);
  pinMode(DIP_PIN_128, INPUT_PULLUP);
  pinMode(DIP_PIN_256, INPUT_PULLUP);

  dmxStartChannel = 1 + 32*(digitalRead(DIP_PIN_32)==LOW) + 64*(digitalRead(DIP_PIN_64)==LOW)
                      + 128*(digitalRead(DIP_PIN_128)==LOW) + 256*(digitalRead(DIP_PIN_256)==LOW);
  Serial.printf("DIP switch dmxStartChannel: %d\n", dmxStartChannel);

  if (!dmxReceive.configure()) { Serial.println("DMX Configure failed."); }
  else { Serial.println("DMX Configured."); }
  delay(10);
  if (dmxReceive.start()) { Serial.println("DMX reception Started"); }
  else { Serial.println("DMX aborted"); }

  neoStrip1.Begin(); neoStrip1.Show(); // Clear strip
  neoStrip2.Begin(); neoStrip2.Show(); // Clear strip
  neoStrip3.Begin(); neoStrip3.Show(); // Clear strip

  Serial.println("Setup complete.");
}

void loop() {
  if (Serial.available()) { parseSerial(controls1, Serial.readString()); }

  if (dmxReceive.hasUpdated()) {  // only read new values
    parseDmx(controls1, dmxStartChannel + 0);
    parseDmx(controls2, dmxStartChannel + 10);
    parseDmx(controls3, dmxStartChannel + 20);
    // Serial.printf("DMX frame. Mode: %d Palette: %d Control: %.2f Smooth: %.2f\n", controls1.mode, controls1.palette, controls1.control, controls1.smooth);
    dmxDimmer = ((float)dmxReceive.read(dmxStartChannel + 30))/255;
    dmxGamma = ((float)dmxReceive.read(dmxStartChannel + 31))/255;
  }

  unsigned long us = micros();
  updateStrip(controls1, pixelStrip1, us);
  updateStrip(controls2, pixelStrip2, us);
  updateStrip(controls3, pixelStrip3, us);
  neoStrip1.Show();
  neoStrip2.Show();
  neoStrip3.Show();
  delay(10);
}
