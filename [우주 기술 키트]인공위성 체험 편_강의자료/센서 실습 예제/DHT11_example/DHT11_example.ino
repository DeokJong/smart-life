#include <DHT.h>
//아두이노 온습도센서 DHT11을 사용하기위해 위에서 
//설치해두었던 라이브러리를 불러옵니다.

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
//불러온 라이브러리 안에 몇번 PIN에서 데이터값이 나오는지, 
//어떤 모듈인지 설정해줘야 합니다. 
//디지털 2번 PIN인 2로 설정했습니다.

void setup() {
  Serial.begin(9600); 
  //온습도값을 PC모니터로 확인하기위해 시리얼 통신을 설정해 줍니다.
  dht.begin();    
}

void loop() {

  Serial.print("temperature:"); 
  Serial.print(dht.readTemperature()); //온도값이 출력됩니다.
  Serial.print(" humidity:");
  Serial.print(dht.readHumidity()); //습도값이 출력됩니다.
  Serial.println();

  delay(1000);
}
