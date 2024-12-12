#include <Adafruit_BMP280.h>    //bmp280 module

#define SCL 11     //BMP SCL PIN
#define SDA 10     //BMP SDA PIN
#define CSB 9    //BMP CSB PIN
#define SDO 8    //BMP SDO PIN

Adafruit_BMP280 bmp(CSB, SDA, SDO, SCL);  

void setup() {
  Serial.begin(9600);
  bmp.begin();
}

void loop() {
  float temp = bmp.readTemperature();     //온도
  float press = bmp.readPressure();       //기압
  float alt = bmp.readAltitude(1011.7);   //고도

  /* 아래 링크에서 현재 위치하는 지역의 해면 기압을 
  위 alt 변수의 값bmp.readAltitude(1011.7) 에 넣어주세요.
  https://www.weather.go.kr/w/observation/land/city-obs.do
  */

  Serial.println("온도: " + String(temp) + " *C");
  Serial.println("기압: " + String(press) + " Pa");
  Serial.println("고도: " + String(alt) + " m");
  delay(1000);
}
