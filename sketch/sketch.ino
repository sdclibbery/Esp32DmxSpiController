#include <cmath>
#include <Arduino.h>
#include <NeoPixelBus.h>
#include <Dmx_ESP32.h>
#include "modes.h"

/* TODO
 Palette asymmetry should be a channel
 Fade time should be a channel, and should be available on all modes
  ?No? Fade doesn't make sense for most modes. Some kind of Temporal blur might, but ould it even be useful??
 Try DMX
  ?https://github.com/devarishi7/Dmx_ESP32
  ?https://github.com/mathertel/DMXSerial
 More modes
 DMX base channel
 3 strips
 Consider rewriting Serial handling: https://gemini.google.com/app/9bacdb891b977834 (but Geminis code is all over the place, inc setTimeout taking MILLIs)
 DMX pass through
*/

// Hardware Definitions for ESP32 DMX Shield (UART2)
#define DMX_UART_NUM  2
#define DMX_RX_PIN    16 // UART2 Receive Pin
#define DMX_TX_PIN    17 // UART2 Transmit Pin
#define DMX_EN_PIN    21 // Direction Enable Pin (Controls RS-485 transceiver)
#define LED_GREEN     13
dmxRx dmxReceive = dmxRx(&Serial1, DMX_RX_PIN, DMX_RX_PIN, DMX_EN_PIN, LED_GREEN, LOW);  // the toggle LED is

// LED setup
#define CLOCK 5
#define DATA0 27 // Top most output
#define DATA1 18
#define DATA2 19 // Bottom output
typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812xMethod> NeoPixelStrip;
// typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1X8Sk6812Method> NeoPixelStrip;

// Strips
const uint16_t pixelCount1 = 16;
const uint16_t dmxStartChannel1 = 1;
NeoPixelStrip neoStrip1(pixelCount1, DATA0);
void setPixel1 (uint16_t index, Rgb color) { neoStrip1.SetPixelColor(index, RgbColor(color.red, color.green, color.blue)); }
PixelStrip pixelStrip1(pixelCount1, setPixel1);
FixtureData fixtureData1(Rgb(0,0,4),Rgb(12,12,0));

void parseSerial (FixtureData& fixtureData, String data) { // For testing
  Serial.println(data);
  if (data.startsWith("m")) { fixtureData.mode = data.substring(1).toInt(); }
  if (data.startsWith("c")) { fixtureData.control = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("s")) { fixtureData.smooth = ((float)data.substring(1).toInt())/255; }
  if (data.startsWith("r")) { fixtureData.back.red = data.substring(1).toInt(); }
  if (data.startsWith("g")) { fixtureData.back.green = data.substring(1).toInt(); }
  if (data.startsWith("b")) { fixtureData.back.blue = data.substring(1).toInt(); }
  if (data.startsWith("R")) { fixtureData.fore.red = data.substring(1).toInt(); }
  if (data.startsWith("G")) { fixtureData.fore.green = data.substring(1).toInt(); }
  if (data.startsWith("B")) { fixtureData.fore.blue = data.substring(1).toInt(); }
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial attach
  Serial.println("Setup starting.");
  Serial.printf("ESP32 Chip Revision: %d\n", ESP.getChipRevision());
  Serial.printf("ESP32 Arduino core version: %s\n", ESP_ARDUINO_VERSION_STR);

  if (!dmxReceive.configure()) { Serial.println("DMX Configure failed."); }
  else { Serial.println("DMX Configured."); }
  delay(10);
  if (dmxReceive.start()) { Serial.println("DMX reception Started"); }
  else { Serial.println("DMX aborted"); }

  neoStrip1.Begin(); neoStrip1.Show(); // Clear strip

  Serial.println("Setup complete.");
}

void loop() {
  delay(10);
  if (Serial.available()) { parseSerial(fixtureData1, Serial.readString()); }

  if (dmxReceive.hasUpdated()) {  // only read new values
    Serial.printf("DMX read: %d", dmxReceive.read(dmxStartChannel1 + 0));
  }

  updateStrip(fixtureData1, pixelStrip1);
  neoStrip1.Show();
}
