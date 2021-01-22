#include <EEPROM.h>
#include <time.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "와이파이 SSID(이름)을 넣으세요."
#define STAPSK  "와이파이 비밀번호를 넣으세요."
#endif

ESP8266WebServer server(80); // 사용하실 포트 번호를 넣으세요.
const char* ssid = STASSID;
const char* password = STAPSK;

const int led1 = 14; // GPIO14
bool isFirstTime = true;
String L1Status;

unsigned long previousMillis = 0; // for Timer
const long interval = 1000;  // for Timer
struct tm * timeinfo; // for Alarm
bool isAlarm = false; // for Alarm

const char BTN1_ON[] PROGMEM = R"=====(
<button type="button" onclick="location.href='OFF1'" style=background-color:#fdfa3d; VALUE="OFF1"; name=submit>ON</button>
)=====";
 
const char BTN1_OFF[] PROGMEM = R"=====(
<button type="button" onclick="location.href='ON1'" VALUE="ON1"; name=submit>OFF</button>
)=====";

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="ko">
<head>
<meta name="viewport"content="width=device-width,initial-scale=1,user-scalable=no"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<link rel="icon" href="data:,">
<style>
body{text-align:center;font-family:verdana;}
button{border:0;border-radius:0.8rem;background-color:#1fa3ec;color:rgb(236, 93, 201);line-height:5.0rem;font-size:1.2rem;width:100%}
a{color: #566473;text-decoration: none;}
</style>
<TITLE>
WIFI Controler
</TITLE>
</head>
 
<BODY>
<div style="text-align:center;display:inline-block;min-width:260px;">
<CENTER>
<h2>IOT System</h2><br />
<p>중앙등</p>
<p>Auto Alarm: <a href="alarm1">@@ALARM@@</a></p>
<form method="post" action="/form1">
<p>@@L1@@</p>
</form>
</CENTER>
<p style=font-size:1px;>@@IP@@</p>
</BODY>
</HTML>
)=====";

void handleRoot() {
  String message = (const __FlashStringHelper *)MAIN_page;
  
  // 서버가 처음 실행되었을 때만 digitalRead로 ON/OFF 상태 초기화
  if(isFirstTime){
    if(digitalRead(led1)) L1Status = (const __FlashStringHelper *)BTN1_ON;
    else L1Status = (const __FlashStringHelper *)BTN1_OFF;
    isFirstTime = false;
  }
   
  // 스위치 상태에 맞게 HTML 코드 업데이트
  message.replace("@@L1@@", L1Status);
  message.replace("@@IP@@", WiFi.localIP().toString());
  message.replace("@@ALARM@@", isAlarm ? "Enable" : "Disable");
  
  server.send(200, "text/html", message);
}

void FunctionON1() {            
  L1Status=(const __FlashStringHelper *)BTN1_ON;   
  digitalWrite(led1, HIGH);         
  
  server.sendHeader("Location", "/");       // This Line Keeps It on Same Page
  server.send(302, "text/plain", "Updated-- Press Back Button"); 
}
void FunctionOFF1() {            
  L1Status=(const __FlashStringHelper *)BTN1_OFF;;   
  digitalWrite(led1, LOW);        
  
  server.sendHeader("Location", "/");       // This Line Keeps It on Same Page
  server.send(302, "text/plain", "Updated-- Press Back Button"); 
}

void FunctionAlarm1() {
  isAlarm ? isAlarm = false : isAlarm = true;
 
  server.sendHeader("Location", "/");       // This Line Keeps It on Same Page
  server.send(302, "text/plain", "Updated-- Press Back Button");   
  isAlarm ? EEPROM.write(1, 1) : EEPROM.write(1, 0); // EEPROM 1번지에 Alarm 설정값 저장. true:1, false:0
  EEPROM.commit();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void check_time(time_t now) {
  timeinfo = localtime(&now);
  //Serial.print(ctime(&now));
  
  // 초기값, 토요일, 일요일 예외처리
  if(timeinfo->tm_year+1900 < 2021 or timeinfo->tm_wday == 0 or timeinfo->tm_wday == 6 or isAlarm == false) return; 
  // 월~금 오전 7시 30분에 자동 점등
  if(timeinfo->tm_hour == 7 and timeinfo->tm_min == 30 and timeinfo->tm_sec == 0) {
    L1Status=(const __FlashStringHelper *)BTN1_ON;   
    digitalWrite(led1, HIGH);         
  }
}

void setup(void) {
  pinMode(led1, OUTPUT);
  digitalWrite(led1, LOW); // 초기 부팅시, 전등 꺼진 상태로 셋팅
  Serial.begin(115200);
  EEPROM.begin(512);
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(NONE_SLEEP_T); // 외부 전원을 사용할 예정이므로, Sleep 모드 사용 안함
  WiFi.begin(ssid, password);
  Serial.println("");

  // load config (alarm)
  EEPROM.read(1) ? isAlarm = true : isAlarm = false;
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // 외부 NTP서버와 시간 동기화 수행
  configTime(3600*9, 0, "maths.kaist.ac.kr", "time.bora.net");
  while (!time(nullptr)) { 
    delay(500);
    Serial.print("."); 
  }
  Serial.println("");

  // MDNS 사용. 현재 내부망에서 http://link.local 접속 가능 상태
  if (MDNS.begin("link")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/ON1", FunctionON1);
  server.on("/OFF1", FunctionOFF1);
  server.on("/alarm1", FunctionAlarm1);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  if(isAlarm) {
    // delay 사용 시, 보드가 일시적으로 멈추게 되므로 millis 함수 사용
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      time_t now = time(nullptr); 
      check_time(now);        
    }    
  }
}
