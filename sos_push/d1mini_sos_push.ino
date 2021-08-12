#include <time.h>
#include <Wire.h>
#include <RCSwitch.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#ifndef STASSID
#define STASSID "와이파이 SSID(이름)을 넣으세요."
#define STAPSK  "와이파이 비밀번호를 넣으세요."
#endif

RCSwitch mySwitch = RCSwitch();
Adafruit_BME280 bme;

WiFiClient espClient;
PubSubClient client(espClient);
StaticJsonDocument<200> data;

const int interval = 1000 * 600;
const char* ssid = STASSID;
const char* password = STAPSK;
const char* mqtt_server = "HA의 Mosquitto 주소를 넣으세요.";";
const int mqtt_port = 10001; // HA의 Mosquitto 포트 번호를 넣으세요.";
const char* mqtt_client = "HA의 Mosquitto 클라이언트 이름을 넣으세요.";";
const char* mqtt_id = "HA의 Mosquitto 아이디를 넣으세요.";
const char* mqtt_pw = "HA의 Mosquitto 패스워드를 넣으세요.";";
const char* mqtt_state = "ON"; // SOS 버튼을 누르면 ON을 전송할 예정입니다.
float temperature, humidity;
String jsondata = "";
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(74880);
  if (!bme.begin(0x76)) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
  }
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(NONE_SLEEP_T);
  WiFi.begin(ssid, password);
  Serial.println("");

  mySwitch.enableReceive(13);  // Receiver on interrupt 13 => that is DPin #7
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  client.setServer(mqtt_server, mqtt_port);
  
  // ------------------------
  ArduinoOTA.setPassword("OTA에 사용할 패스워드를 입력합니다.");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  // ------------------------
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client, mqtt_id, mqtt_pw)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("try again in 3 seconds");
        // Wait 3 seconds before retrying
        delay(3000);   
      }else {
        ESP.restart();
      }
    }
  }  
}

void loop() {
  ArduinoOTA.handle();
  
  if (mySwitch.available()) {
    if (!client.connected()) {
      reconnect();
    }
    int value = mySwitch.getReceivedValue();
    if (value == 13719715) { // 각 버튼의 고유 RF값을 사전에 파악해야 합니다.
      Serial.println("1번 SOS");
      client.publish("grandmom/sensor/button1", mqtt_state);
    }else if (value == 13461667) {
      Serial.println("2번 SOS");
      client.publish("grandmom/sensor/button2", mqtt_state);
    }else if (value == 9681056) {
      Serial.println("3번 SOS");
      client.publish("grandmom/sensor/button3", mqtt_state);
    }else {
      Serial.print("value : ");
      Serial.println(value); // 여기서 각 버튼의 고유 RF값을 파악할 수 있습니다.
    }
    mySwitch.resetAvailable();
  }
  
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    if (temperature > 0) {
      data["temp"] = temperature;
      data["hmdt"] = humidity;
      serializeJson(data, jsondata);
      Serial.println(jsondata);
      if (!client.connected()) {
        reconnect();
      }
      client.publish("grandmom/sensor", String(jsondata).c_str());
      jsondata = "";      
    }
  }   
}
