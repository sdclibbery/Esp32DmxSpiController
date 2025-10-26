#include <Arduino.h>
#include <SparkFunDMX.h>
#include <FastLED.h>

/* TODO
 First actual mode
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

// Channel Defintions
#define TOTAL_CHANNELS 9
const uint16_t dmxStartChannel = 1;

// FastLED Pin Definitions for ESP32 WROOM
#define CLOCK 5
#define DATA0 19
#define DATA1 18
#define DATA2 27

// LED setup
#define NUM_LEDS 30
CRGB leds[NUM_LEDS];

void setup() {
  delay( 500 ); // power up delay
  Serial.println("Setup starting.");

  Serial.begin(115200);
  dmxSerial.begin(250000, SERIAL_8N2, DMX_RX_PIN, DMX_TX_PIN);
  dmx.begin(dmxSerial, DMX_EN_PIN, 512); 
  dmx.setComDir(DMX_READ_DIR);

  FastLED.addLeds<WS2812B, DATA0, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( 64 );

  Serial.println("Setup complete.");
}

unsigned char t = 0;
void loop() {
  while(dmx.dataAvailable() == false) {
      dmx.update();
  }
  dmx.readByte(dmxStartChannel);
  
  leds[0].r = t++;
  FastLED.show();
}
