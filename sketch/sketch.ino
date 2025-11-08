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

// LED setup
#define LED_CLOCK 5
#define LED_DATA0 27 // Top most output
#define LED_DATA1 18
#define LED_DATA2 19
typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812xMethod> NeoPixelStrip;
// typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1X8Sk6812Method> NeoPixelStrip;

// Strips
const uint16_t pixelCount1 = 30;
const uint16_t dmxStartChannel1 = 1;
NeoPixelStrip neoStrip1(pixelCount1, LED_DATA0);
void setPixel1 (uint16_t index, Rgb color) { neoStrip1.SetPixelColor(index, RgbColor(color.red*255, color.green*255, color.blue*255)); }
PixelStrip pixelStrip1(pixelCount1, setPixel1);
Controls controls1(Rgb(0,0,0.1f),Rgb(0.2f,0,0));

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

  if (!dmxReceive.configure()) { Serial.println("DMX Configure failed."); }
  else { Serial.println("DMX Configured."); }
  delay(10);
  if (dmxReceive.start()) { Serial.println("DMX reception Started"); }
  else { Serial.println("DMX aborted"); }

  neoStrip1.Begin(); neoStrip1.Show(); // Clear strip

  Serial.println("Setup complete.");
}

void loop() {
  if (Serial.available()) { parseSerial(controls1, Serial.readString()); }

  if (dmxReceive.hasUpdated()) {  // only read new values
    parseDmx(controls1, dmxStartChannel1);
    // Serial.printf("DMX frame. Mode: %d Palette: %d Control: %.2f Smooth: %.2f\n", controls1.mode, controls1.palette, controls1.control, controls1.smooth);
  }

  updateStrip(controls1, pixelStrip1, micros());
  neoStrip1.Show();
  delay(10);
}
