#include <SSD1306Ascii.h>       //ssd1306 oled display
#include <SSD1306AsciiWire.h> 
#include <Wire.h>

#define RST_PIN -1              //reset pin
#define I2C_ADDRESS 0x3C        //oled display address

SSD1306AsciiWire oled;


void setup() {
  Serial.begin(9600);
  Wire.begin();
  #if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(System5x7);
  oled.clear();
}

void loop() {
  oled.setCursor(0, 0);
  oled.println("Hello world");
  oled.setCursor(0, 10);
  oled.println("Hello Arduino");
}
