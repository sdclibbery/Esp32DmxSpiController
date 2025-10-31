#include <cmath>
#include <Arduino.h>
// #include <SparkFunDMX.h>
#include <NeoPixelBus.h>
#include "modes.h"

/* TODO
 ?? Should have a dedicated palette channel, and it should include curves and asymmetric, as well as animated?
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
// The SparkFun DMX Shield typically uses UART2 on the ESP32.
#define DMX_UART_NUM  2
#define DMX_RX_PIN    16 // UART2 Receive Pin
#define DMX_TX_PIN    17 // UART2 Transmit Pin
#define DMX_EN_PIN    21 // Direction Enable Pin (Controls RS-485 transceiver)

// DMX 
HardwareSerial dmxSerial(DMX_UART_NUM);
// SparkFunDMX dmx;
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
typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812xMethod> NeoPixelStrip;
// typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1X8Sk6812Method> NeoPixelStrip;

// Strip 1
NeoPixelStrip neoStrip1(pixelCount, DATA2);
void setPixel1 (uint16_t index, Rgb color) { neoStrip1.SetPixelColor(index, RgbColor(color.red, color.green, color.blue)); }
PixelStrip pixelStrip1(pixelCount, setPixel1);

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial attach
  Serial.println("Setup starting.");

//   dmxSerial.begin(250000, SERIAL_8N2, DMX_RX_PIN, -1);
// pinMode(DMX_EN_PIN, OUTPUT);
// digitalWrite(DMX_EN_PIN, LOW);

  neoStrip1.Begin(); neoStrip1.Show(); // Clear strip
  // neoStrip1.SetPixelColor(0,RgbColor(0,16,0)); neoStrip1.Show(); // Set first LED green at startup to verify if update is running without Serial monitor

  Serial.println("Setup complete.");
}

uint8_t c = 0;
uint8_t m = 0;
bool readingC = true;
void loop() {
  delay(10);

// Serial.printf("About to read\n");
// dmxSerial.flush(false);

// // Wait for one byte read to take a long time
// unsigned long lastReadTime = micros();
// unsigned long readTime = 0;
// while (readTime < 88) { // DMX break time is min 88 microseconds
//     while (dmxSerial.available() <= 0);
//     dmxSerial.read();
//     unsigned long timeNow = micros();
//     readTime = timeNow - lastReadTime;
//     lastReadTime = timeNow;
// }
// // Then read dmx frame
// uint8_t dmxFrame[513];
// int writeIdx = 0;
// while (writeIdx < 513) {
//   dmxFrame[writeIdx++] = dmxSerial.read();
// }
// memcpy(dmxData, dmxFrame, DMX_CHANNELS);

// !!!! interesting. The timing approach may yet work, but its not reliable and it may be screwing the LED update up??

// uint8_t serialData[1024] = {0};
// while (true) {
//   int avail = dmxSerial.available();
//   if (avail > 0) {
//     // int read = dmxSerial.readBytes(serialData, avail);
//     int data = -1;
//     do {
//       data = dmxSerial.read()
//     } while (data >= 0);
//     unsigned long readTime = micros();
//     Serial.printf("data %d %d %d %d %d %d\n", avail, read, serialData[0], serialData[1], serialData[2], readTime - lastReadTime);
//     lastReadTime = readTime;
//   }
// }

  // while(dmx.dataAvailable() == false) {
  //     dmx.update();
  // }
  // dmx.readBytes(dmxData, DMX_CHANNELS, dmxStartChannel);
// Serial.printf("bytes %d %d %d\n", dmxData[DMX_MODE], dmxData[DMX_CONTROL], dmxData[DMX_SMOOTH]);
  FixtureData fixtureData;
  // fixtureData.mode = dmxData[DMX_MODE];
  // fixtureData.control = ((float)dmxData[DMX_CONTROL])/255;
  // fixtureData.smooth = ((float)dmxData[DMX_SMOOTH])/255;
  // fixtureData.back = Rgb(dmxData[DMX_BACK_R],dmxData[DMX_BACK_G],dmxData[DMX_BACK_B]);
  // fixtureData.fore = Rgb(dmxData[DMX_FORE_R],dmxData[DMX_FORE_G],dmxData[DMX_FORE_B]);

  if (readingC && Serial.available()) { c = Serial.parseInt(); Serial.printf("c: %d\n", c); readingC=false; }
  if (!readingC && Serial.available()) { m = Serial.parseInt(); Serial.printf("m: %d\n", m); readingC=true; }
  fixtureData.mode = 21;
  fixtureData.control = ((float)c)/255.0f;
  fixtureData.smooth = ((float)m)/255.0f;
  fixtureData.back = Rgb(16,0,0);
  fixtureData.fore = Rgb(0,0,16);

  updateStrip(fixtureData, pixelStrip1);
  neoStrip1.Show();
}
