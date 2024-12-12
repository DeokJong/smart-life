#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>

String a, b;
/* 
구글 api 키 발급 받는 방법: https://velog.io/@sukqbe/API-구글-지도Google-Map-추가하기-API-Key-발급받기-qumur49u
*/
//Camera related constants
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

const char *ssid = "Wifi 이름";
const char *password = "Wifi 비밀번호";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;

const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <style>
    .arrows {
      font-size:40px;
      color:red;
    }
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }
    #map{
        border: 2px solid blue;
        width: 400px;
        height: 300px;
    }
    </style>

  <script defer src="http://maps.google.com/maps/api/js?key="따옴표를 포함한 안에 있는 문자열을 지우고 api키 입력 해주세요"&region=kr"></script>
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <!--h2 style="color: teal;text-align:center;">Wi-Fi Camera &#128663; Control</h2-->
    
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <img id="cameraImage" src="" style="width:400px;height:250px"></td>
      </tr> 
      <tr/><tr/>
      <tr>
        <td style="text-align:left"><b>온도:</b></td>
        <td id="dhtt" style="text-align:right" color:red;">0.0</td>
        <td style="text-align:right"><b>℃</b></td>
        <td style="text-align:left"><b>습도:</b></td>
        <td id="dhth" style="text-align:right" color:red;">0</td>
        <td style="text-align:right"><b>%</b></td>
      </tr>

      <tr>
      </tr>              
      <tr>
        <td style="text-align:left"><b>기압:</b></td>
        <td id="bmpp" style="text-align:right" color:red;">0.0</td>
        <td style="text-align:right"><b>Pa</b></td>
        <td style="text-align:left"><b>고도:</b></td>
        <td id="bmpa" style="text-align:right" color:red;">0.0</td>
        <td style="text-align:right"><b>m</b></td>
      </tr>
      <tr>

      </tr>
      <tr>
        <td style="text-align:left"><b>가속도</b></td>
      </tr>
      <tr>
        <td style="text-align:left"><b>X:</b></td>
        <td id="mpux" style="text-align:left" color:red;">0.0</td>
        <td style="text-align:left"><b>Y:</b></td>
        <td id="mpuy" style="text-align:left" color:red;">0.0</td>
        <td style="text-align:left"><b>Z:</b></td>
        <td id="mpuz" style="text-align:left" color:red;">0.0</td>
      </tr>
      <tr>
        <td style="text-align:left"><b>위성 수:</b></td>
        <td id="gpss" style="text-align:left" color:red;">0</td>
      </tr>
      <tr>
        <td style="text-align:left"><b>위도:</b></td>
        <td id="glat" style="text-align:left" color:red;">0.000000</td>
        <td style="text-align:right"><b>경도:</b></td>
        <td id="glon" style="text-align:right" color:red;">0.000000</td>
        
      </tr>
      <tr>
      </tr>
    </table>
    <center><div id="map"></div></center>

    <script defer>
      var map;
      // 지도 출력
      function initMap(){
        // 위치데이터 경도, 위도로 구성된 jso 파일은 항상 이런식으로 구성되어있다.
        var ll = {lat: 37.500624, lng: 127.036268};
        map = new google.maps.Map(
          document.getElementById("map"),
          {zoom: 17, center: ll}
          );
        new google.maps.Marker(
          {position: ll,
            map: map,
            label: "현재 위치"}        
        );
      }
      //initMap(); // 맵 생성
    </script> 

    <script defer>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";      
      var websocketCamera;
      var websocketCarInput;
      var str = "data";
      var sensorData;
      var map;
      
      function initCameraWebSocket() {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onopen    = function(event){};
        websocketCamera.onclose   = function(event){setTimeout(initCameraWebSocket, 2000);};
        websocketCamera.onmessage = function(event) {
          var imageId = document.getElementById("cameraImage");
          imageId.src = URL.createObjectURL(event.data);
        };
      }
      function reMessage() {
        setInterval(function(){
            websocketCarInput.send(str);
        },2000);
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen    = function(event){};
        websocketCarInput.onclose   = function(event){setTimeout(initCarInputWebSocket, 2000);};
        websocketCarInput.onmessage = function(event){
          if(!!event.data) {
            
            sensorData = event.data.split('/');
            
            document.getElementById('dhtt').innerHTML = sensorData[0];
            document.getElementById('dhth').innerHTML = sensorData[1];
            document.getElementById('bmpp').innerHTML = sensorData[2];
            document.getElementById('bmpa').innerHTML = sensorData[3];
            document.getElementById('mpux').innerHTML = sensorData[4];
            document.getElementById('mpuy').innerHTML = sensorData[5];
            document.getElementById('mpuz').innerHTML = sensorData[6];
            document.getElementById('gpss').innerHTML = sensorData[9];
            document.getElementById('glat').innerHTML = sensorData[7];
            document.getElementById('glon').innerHTML = sensorData[8];
            var ll = {lat: parseFloat(sensorData[7]), lng: parseFloat(sensorData[8])};
            map = new google.maps.Map(
              document.getElementById("map"),
              {zoom: 17, center: ll}
              );
            new google.maps.Marker(
              {position: ll,
                map: map,
                label: "현재 위치"}        
            );
          }
        };  
      }

      // 지도 출력
      function initMap(){
        // 위치데이터 경도, 위도로 구성된 jso 파일은 항상 이런식으로 구성되어있다.
        var ll = {lat: 37.500624, lng: 127.036268};
        map = new google.maps.Map(
          document.getElementById("map"),
          {zoom: 17, center: ll}
          );
        new google.maps.Marker(
          {position: ll,
            map: map,
            label: "현재 위치"}        
        );
      }

      function initWebSocket() {
        initCameraWebSocket();
        reMessage();
        //initMap();
      }
      window.onload = initWebSocket;        
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";


void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");
}

void sensorData() {  // 아두이노에서 센서 값 받아오는 함수
  while (Serial2.available()) {
    char transmit = Serial2.read();
    b.concat(transmit);
  }
  a.concat(b);
    
  b = "";
}

void onCarInputWebSocketEvent(AsyncWebSocket *server,
                              AsyncWebSocketClient *client,
                              AwsEventType type,
                              void *arg,
                              uint8_t *data,
                              size_t len) {
  Serial.println("message:: " + String((char *)(data)));
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      Serial.printf("WebSocket client #%u sent message %s \n", client->id(), (uint8_t *)data);
      String message = String((char *)(data));
      if (message.substring(0, 4).equals("data") == 1) {
        sensorData();
        Serial.printf("WebSocket client %u sent message %s \n", client->id(), a);
        client->text(a);
        break;
      } else {
        String c = "";
        Serial.printf("WebSocket client %u sent message %s \n", client->id(), c);
        client->text(c);
        break;
      }
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server,
                            AsyncWebSocketClient *client,
                            AwsEventType type,
                            void *arg,
                            uint8_t *data,
                            size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      cameraClientId = client->id();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      cameraClientId = 0;
      break;
    case WS_EVT_DATA:
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  if (psramFound()) {
    heap_caps_malloc_extmem_enable(20000);
    Serial.printf("PSRAM initialized. malloc to take memory from psram above this size");
  }
}

void sendCameraPicture() {
  if (cameraClientId == 0) {
    return;
  }
  unsigned long startTime1 = millis();
  //capture a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame buffer could not be acquired");
    return;
  }

  unsigned long startTime2 = millis();
  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);

  //Wait for message to be delivered
  while (true) {
    AsyncWebSocketClient *clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull())) {
      break;
    }
    delay(1);
  }

  unsigned long startTime3 = millis();
  Serial.printf("Time taken Total: %d|%d|%d\n", startTime3 - startTime1, startTime2 - startTime1, startTime3 - startTime2);
}


void setup(void) {
  //setUpPinModes();
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 14, 15);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started");

  setupCamera();

  //카메라가 상하반전되어 나올 경우 아래 코드 주석 해제
  sensor_t *s = esp_camera_sensor_get();
  s->set_hmirror(s, 1);
  s->set_vflip(s, 1);
  //카메라가 상하반전되어 나올 경우 위 코드 주석 해제
}



void loop() {
  //sensorData();
  wsCamera.cleanupClients();
  wsCarInput.cleanupClients();
  sendCameraPicture();

  String msg = WiFi.localIP().toString();
  Serial2.println(msg);
  delay(500);
}
