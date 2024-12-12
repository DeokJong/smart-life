#include <SSD1306Ascii.h>       //ssd1306 oled display
#include <SSD1306AsciiWire.h> 

#include <SoftwareSerial.h>
#include <TinyGPS.h>            //gps module

#include <Adafruit_BMP280.h>    //bmp280 module
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Wire.h>

#include <DHT.h>                //dht11 

#define SCL 8     //BMP SCL PIN
#define SDA 9     //BMP SDA PIN
#define CSB 10    //BMP CSB PIN
#define SDO 11    //BMP SDO PIN

#define DHTPIN 2                //dht 센서 핀 번호
#define DHTTYPE DHT11           //dht 센서 종류

#define RST_PIN -1              //reset pin
#define I2C_ADDRESS 0x3C        //oled display address

SSD1306AsciiWire oled;

Adafruit_BMP280 bmp(CSB, SDA, SDO, SCL);  

TinyGPS gps;

SoftwareSerial ss(4, 3);

DHT dht(DHTPIN, DHTTYPE);

bool newData = false;         //gps data
float flat, flon;             //gps lat, lon
unsigned long age;            //gps age

const int MPU_addr = 0x68; //MPU Address
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ; //가속도 센서값, 자이로 센서값

//세가지 형태의 Roll, Pitch, Yaw 각도를 저장하기 위한 변수
float accel_angle_x, accel_angle_y, accel_angle_z;
float filtered_angle_x, filtered_angle_y, filtered_angle_z;

//보정형 변수
float baseAcX, baseAcY, baseAcZ;
float baseGyX, baseGyY, baseGyZ;

//시간관련 값
float dt;
unsigned long t_now;
unsigned long t_prev;

//자이로 센서를 이용한 각도구하기
float gyro_x, gyro_y, gyro_z;

float base_roll_target_angle;
float base_pitch_target_angle;
float base_yaw_target_angle;

String a, b, ipAddress;  //아이피 주소값을 저장할 변수

void setup() {
  initMPU6050();      //MPU6050초기화
  Serial.begin(9600); //esp32-cam으로부터 아이피를 받아오는 시리얼
  calibAccelGyro();   //가속도 자이로 센서의 초기 평균값을 구한다.
  initDT();           //시간 간격 초기화
  initYPR();          //Roll, Pitch, Yaw의 초기각도 값을 설정(평균을 구해 초기 각도로 설정, 호버링을 위한 목표 각도로 사용)
  initSSD1306();      //ssd1306 초기화
  ss.begin(9600);
  dht.begin();
  bmp.begin();
  
}

void initSSD1306(){
  #if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(System5x7);
  oled.clear();
}

void initMPU6050(){ //MPU6050 초기화
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true); //I2C의 제어권을 반환
}

void calibAccelGyro(){
  float sumAcX = 0;
  float sumAcY = 0;
  float sumAcZ = 0;
  float sumGyX = 0;
  float sumGyY = 0;
  float sumGyZ = 0;

  readAccelGyro();

  //초기 보정값은 10번의 가속도 자이로 센서의 값을 받아 해당 평균값을 가진다.
  for(int i=0; i<10; i++){
    readAccelGyro();
    sumAcX += AcX, sumAcY += AcY, sumAcZ += AcZ;
    sumGyX += GyX, sumGyY += GyY, sumGyZ += GyZ;
    delay(100);
  }

  baseAcX = sumAcX / 10;
  baseAcY = sumAcY / 10;
  baseAcZ = sumAcZ / 10;
  baseGyX = sumGyX / 10;
  baseGyY = sumGyY / 10;
  baseGyZ = sumGyZ / 10;
}

void initDT(){
  t_prev = micros(); //초기 t_prev값은 근사값
}

void initYPR(){
  //초기 호버링의 각도를 잡아주기 위해서 Roll, Pitch, Yaw 상보필터 구하는 과정을 10번 반복한다.
  for(int i=0; i<10; i++) {
    readAccelGyro();
    calcDT();
    calcAccelYPR();
    calcGyroYPR();
    calcFilteredYPR();

    base_roll_target_angle += filtered_angle_y;
    base_pitch_target_angle += filtered_angle_x;
    base_yaw_target_angle += filtered_angle_z;

    delay(100);
  }

  //평균값을 구한다.
  base_roll_target_angle /= 10;
  base_pitch_target_angle /= 10;
  base_yaw_target_angle /= 10;
}

void loop() {
  
  readAccelGyro();
  calcDT();
  
  calcAccelYPR(); //가속도 센서 Roll, Pitch, Yaw의 각도를 구하는 루틴
  calcGyroYPR(); //자이로 센서 Roll, Pitch, Yaw의 각도를 구하는 루틴
  calcFilteredYPR(); //상보필터를 적용해 Roll, Pitch, Yaw의 각도를 구하는 루틴
  
  camCheck();
  gpsCheck();
  //oledSet();
  //sendData();
  
}

void calcDT(){
  t_now = micros();
  dt = (t_now - t_prev) / 1000000.0;
  t_prev = t_now;
}

void calcAccelYPR(){
  float accel_x, accel_y, accel_z;
  float accel_xz, accel_yz;
  const float RADIANS_TO_DEGREES = 180 / M_PI;

  accel_x = AcX - baseAcX;
  accel_y = AcY - baseAcY;
  accel_z = AcZ + (16384 - baseAcZ);

  //accel_angle_y는 Roll각을 의미
  accel_yz = sqrt(pow(accel_y, 2) + pow(accel_z, 2));
  accel_angle_y = atan(-accel_x / accel_yz) * RADIANS_TO_DEGREES;

  //accel_angle_x는 Pitch값을 의미
  accel_xz = sqrt(pow(accel_x, 2) + pow(accel_z, 2));
  accel_angle_x = atan(accel_y / accel_xz) * RADIANS_TO_DEGREES;

  accel_angle_z = 0; //중력 가속도(g)의 방향과 정반대의 방향을 가리키므로 가속도 센서를 이용해서는 회전각을 계산할 수 없다.
}

void calcGyroYPR() {
  const float GYROXYZ_TO_DEGREES_PER_SEC = 131;

  gyro_x = (GyX - baseGyX) / GYROXYZ_TO_DEGREES_PER_SEC;
  gyro_y = (GyY - baseGyY) / GYROXYZ_TO_DEGREES_PER_SEC;
  gyro_z = (GyZ - baseGyZ) / GYROXYZ_TO_DEGREES_PER_SEC;
}

void calcFilteredYPR() {
  const float ALPHA = 0.96;
  float tmp_angle_x, tmp_angle_y, tmp_angle_z;

  tmp_angle_x = filtered_angle_x + gyro_x * dt;
  tmp_angle_y = filtered_angle_y + gyro_y * dt;
  tmp_angle_z = filtered_angle_z + gyro_z * dt;

  //상보필터 값 구하기(가속도, 자이로 센서의 절충)
  filtered_angle_x = ALPHA * tmp_angle_x + (1.0-ALPHA) * accel_angle_x;
  filtered_angle_y = ALPHA * tmp_angle_y + (1.0-ALPHA) * accel_angle_y;
  filtered_angle_z = tmp_angle_z;
}

void readAccelGyro() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);  //I2C의 제어권을 이어간다
  Wire.requestFrom(MPU_addr, 14, true);

  //가속도, 자이로 센서의 값을 읽어온다.
  AcX = Wire.read() << 8|Wire.read();
  AcY = Wire.read() << 8|Wire.read();
  AcZ = Wire.read() << 8|Wire.read();
  Tmp = Wire.read() << 8|Wire.read();
  GyX = Wire.read() << 8|Wire.read();
  GyY = Wire.read() << 8|Wire.read();
  GyZ = Wire.read() << 8|Wire.read();
}

void gpsCheck() { 
  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (ss.available()) { 
      char c = ss.read(); // uncomment this line if you want to see the GPS data flowing    
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }
  if (newData) {  //gps 가 연결되어 정보를 받아 왔을 경우 동작
    gps.f_get_position(&flat, &flon, &age);
    oledSet();
    sendData();
  }
  else {  //gps 가 연결되지 않았을 때 gps 정보 없이 출력
    oledSet();
    sendData();
  }
}



void oledSet() {    //디스플레이에 센서값 출력
  oled.setCursor(0, 0); 
  oled.println("Atm: " + String(bmp.readPressure()) + "Pa");
  oled.setCursor(0, 10);
  oled.println("T: " + String(dht.readTemperature(),1) + "*C      Accel");
  oled.setCursor(0, 20); 
  oled.println("H: " + String(dht.readHumidity(), 0) + "%      X: " + String(filtered_angle_x, 1));
  oled.setCursor(0, 30); 
  oled.println("ALT: " + String(bmp.readAltitude(1011.7)) + "m Y: " + String(filtered_angle_y, 1));
  oled.setCursor(0, 40); 
  oled.println("Sats:" + String(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites()) + "      Z: " + String(filtered_angle_z, 1));
  oled.setCursor(0, 50); 
  oled.println("LAT: " + String(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6));
  oled.setCursor(0, 60); 
  oled.println("LON: "+ String(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6));
  oled.setCursor(0, 70); 
  oled.println("CAM IP:" + String(ipAddress));
  
}

void sendData() {  //ESP32 CAM에 보내는 데이터
  String msg;
  msg.concat(String(dht.readTemperature()));
  msg.concat("/" + String(dht.readHumidity()));
  msg.concat("/" + String(bmp.readPressure()));
  msg.concat("/" + String(bmp.readAltitude(1011.7)));
  msg.concat("/" + String(filtered_angle_x));
  msg.concat("/" + String(filtered_angle_y));
  msg.concat("/" + String(filtered_angle_z));
  msg.concat("/" + String(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6));
  msg.concat("/" + String(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6));
  msg.concat("/" + String(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites()) + "/");
  Serial.println(msg);
  delay(1000);
}

void camCheck() { //ESP 캠에게서 ip 받아오는 함수
  if(a.equals(b)) {
    while (Serial.available()) {
      char transmit = Serial.read();
      b.concat(transmit);
    }
    a.concat(b);
    b = "";
  }
  int index = a.indexOf("\n");
  ipAddress = a.substring(0, index);
}
