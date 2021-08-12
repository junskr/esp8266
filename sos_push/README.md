# [ 배경 ]
요양병원에 계시던 연로하신 할머니께서 장기화되고 있는 코로나19로 인하여 얼마전 퇴소하셨습니다. <br />
(코로나19 이후 면회 자체가 불가능하게 되었고, 장기간 얼굴 조차 뵐 수 없었습니다...) <br />
\- 연로하신 할머니께서 집에 홀로 계시는 일이 잦아짐에 따라, 만일에 사태에 빠르게 대응하고자 IoT를 활용하였습니다. <br />
\- **병원의 SOS 버튼처럼 위급(응급) 상황 발생 시, 버튼 조작을 통해 저에게 연락이 왔으면 좋겠습니다.** <br />

# [ 환경 ]
1. 할머니댁은 **인터넷을 사용하지 않습니다.**
2. 그에 따라, 당연히 기존에 사용 중인 IoT 제품은 없습니다.
3. **저는 삽질을 좋아하는 가난한 학생입니다.**

# [ 방향 ]
1. LTE 라우터 + 저의 데이터 쉐어링 USIM을 활용한 무선네트워크 구성
2. 433MHz RF 통신을 이용한 게이트웨이와 다수의 푸쉬 버튼 구성
3. MQTT 프로토콜을 이용하여 저희집의 HA로 데이터 전송, 자동화 구현

**공부도 할 겸, 기존 완제품을 구매하는 것 보단 제 입맛에 맞추어 직접 제작해보기로 하였습니다.**

- - - 

# [ 구성품 ]
### 필수
1. WeMos D1 Mini (ESP 8266) : 서랍에 1년 넘게 방치된 녀석이 떠올랐습니다.
2. SRX882 센서 : 433MHz RF 수신을 위해 구매 (https://ko.aliexpress.com/item/32816634210.html)
3. SOS 비상 버튼 : 433MHz RF 프로토콜을 사용하는 버튼 구매 (https://ko.aliexpress.com/item/1005001873014997.html)
4. LTE 라우터 (E8372h-320) : 무선네트워크 구성을 위한 LTE 라우터 구매(https://ko.aliexpress.com/item/1005001889857671.html)

### 옵션
1. BME280 센서 : 온/습도 센서 모듈, 서랍에 굴러다니는 모듈이 있길래 조립하면서 함께 장착했습니다.
2. 다용도 수납 케이스 : 깔끔하게 정리하고자 다이소에서 3천원? 주고 산거같습니다.
3. 그 외 브레드보드(빵판), 점퍼 케이블, 5V2A 듀얼충전기, 멀티탭, 납땜을 위한 인두기 등

\# **LTE 라우터(5~6만원)을 제외한 다른 부품은 모두 만원 내외로 구매 가능한 저렴한 부품입니다 =D**

< 이미지 1>

# [ 기능 ]
### 단순합니다.
\- WeMos D1 Mini + SRX822 : RF 게이트웨이<br />
\- SOS 버튼 : RF 디바이스
 1. SOS 버튼을 누르면 D1이 신호를 수신하여 MQTT를 이용, 저희집의 HA로 데이터 전송<br />
 2. MQTT를 수신한 HA는 버튼을 식별한 후 Notify 수행
###### - 그 외 BME280 센서를 이용하여 10분 간격으로 온/습도 정보 HA로 송신<br />

<이미지 2>

# [ 소프트웨어 구현(中 주요 소스코드) ]
### ESP 8266 단독 사용이므로, Arduino IDE를 사용하여 프로그래밍 구현

1. MQTT를 이용하기 위한 접속 서버 설정
```
const char* mqtt_server = "HA의 Mosquitto 주소를 넣으세요.";";
const int mqtt_port = 10001; // HA의 Mosquitto 포트 번호를 넣으세요.";
const char* mqtt_client = "HA의 Mosquitto 클라이언트 이름을 넣으세요.";";
const char* mqtt_id = "HA의 Mosquitto 아이디를 넣으세요.";
const char* mqtt_pw = "HA의 Mosquitto 패스워드를 넣으세요.";";
const char* mqtt_state = "ON"; // SOS 버튼을 누르면 ON을 전송할 예정입니다.
```

2. BME280 센서 및 SRX822 센서 설정
```
  if (!bme.begin(0x76)) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
  }
...
  mySwitch.enableReceive(13);  // Receiver on interrupt 13 => that is DPin #7
```

3. SOS 버튼을 누르면 MQTT를 Push 합니다.
```
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
```

4. BME280 센서를 이용하여 수집한 온/습도를 MQTT를 Push 합니다.<br />
 \- delay 사용 시, 보드가 일시적으로 멈추게 되므로 millis 함수 사용
```
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
```

# [ 아두이노 IDE 셋팅 ]

<이미지 3>

# [ 하드웨어 구현 ]
1. SRX822 센서의 Data는 아두이노의 RCS와 연결해야 합니다.
### ※ 주의** 
**\- RCS가 가능한 GPIO: D1, D2, D5, D6, D7**<br />
**\- RCS가 불가능한 GPIO: D0, D3, D4, D8**<br />
**\- 저는 D7 pin, 즉 GPIO 13과 연결하였습니다.**<br />

<이미지 4>

2. BME280은 센서<br />
\- (5V 센서로 샀기때문에) BME280의 VIN은 D1 mini의 5V<br />
\- BME280의 GND는 D1 mini의 GND<br />
\- BME280의 SCL은 D1 mini의 GPIO5 (D1 Pin)<br />
\- BME280의 SDA은 D1 mini의 GPIO4 (D2 pin)<br />

<이미지 5>

3. SOS 버튼<br />
\- CR2032 3V 건전지 1개가 들어갑니다.<br />
\- 배터리가 포함되어 배송왔지만, 잔량이 많지 않아 교체하였습니다.<br />

<이미지 6>

# [ HA Config 정보 ]
1. binary_sensor <br />
\- MQTT를 이용하여 수신한 버튼 정보를 바이너리 센서의 on/off로 표현 합니다.<br />
```
  - platform: mqtt
    name: 'grandmom_sensor_button1'
    state_topic: "grandmom/sensor/button1"
    off_delay: 2
    qos: 1
    payload_on: "ON"
    
  - platform: mqtt
    name: 'grandmom_sensor_button2'
    state_topic: "grandmom/sensor/button2"
    off_delay: 2
    qos: 1
    payload_on: "ON"
    
  - platform: mqtt
    name: 'grandmom_sensor_button3'
    state_topic: "grandmom/sensor/button3"
    off_delay: 2
    qos: 1
    payload_on: "ON"  
```

2. sensor <br />
\- MQTT를 이용하여 수신한 온/습도 정보를 센서로 표현합니다.<br />
```
  - platform: mqtt    
    name: 'grandmom_sensor_temp'
    unit_of_measurement: '°C'
    state_topic: "grandmom/sensor"
    value_template: "{{ value_json.temp | round(0) }}"
    qos: 1
    force_update: true
  - platform: mqtt    
    name: 'grandmom_sensor_hmdt'
    unit_of_measurement: '%'
    state_topic: "grandmom/sensor"
    value_template: "{{ value_json.hmdt | round(0) }}"
    qos: 1
    force_update: true   
```

3. template sensor <br />
\- sensor의 온/습도 정보를 바탕으로 "불쾌지수" 센서를 만들어 함께 활용하고 있습니다.<br />
```
      grandmom_sensor_discomfort:  
        value_template: >-
          {% set temperature = states('sensor.grandmom_sensor_temp') | float %}
          {% set humidity = states('sensor.grandmom_sensor_hmdt') | float %}
          {% set discomfort = (1.8*temperature)-0.55*(1-humidity*0.01)*(1.8*temperature-26)+32 %}
          {{ discomfort | round(0) }}
        unit_of_measurement: '%'
        friendly_name_template: >-
          {% set temperature = states('sensor.grandmom_sensor_temp') | float %}
          {% set humidity = states('sensor.grandmom_sensor_hmdt') | float %}
          {% set discomfort = (1.8*temperature)-0.55*(1-humidity*0.01)*(1.8*temperature-26)+32 %}
          {% if discomfort < 68 %}
            쾌적
          {% elif 68 <= discomfort and discomfort < 75 %}
            보통
          {% elif 75 <= discomfort and discomfort < 80 %}
            불편
          {% else %}
            불쾌
          {% endif %}       
        icon_template: >-
          {% set temperature = states('sensor.grandmom_sensor_temp') | float %}
          {% set humidity = states('sensor.grandmom_sensor_hmdt') | float %}
          {% set discomfort = (1.8*temperature)-0.55*(1-humidity*0.01)*(1.8*temperature-26)+32 %}
          {% if discomfort < 68 %}
            mdi:emoticon-excited
          {% elif 68 <= discomfort and discomfort < 75 %}
            mdi:emoticon-neutral
          {% elif 75 <= discomfort and discomfort < 80 %}
            mdi:emoticon-sad
          {% else %}
            mdi:emoticon-angry
          {% endif %}
        friendly_name: "불쾌지수"     
```

4. automation <br />
\- 버튼을 누를 경우 자동으로 노티를 발생시킵니다.<br />
\- 현재 SOS 버튼을 누르면, 저에겐 텔레그램으로 노티가 오고 다른 가족들에겐 SMS가 전송되게끔 구현하였습니다.<br />
```
  - alias: "GrandMom Alarm"
    trigger:
    - platform: state
      from: 'off'
      to: 'on'
      entity_id:
        - binary_sensor.grandmom_sensor_button1
        - binary_sensor.grandmom_sensor_button2
        - binary_sensor.grandmom_sensor_button3
    action:
      - service: notify.telegram_****
        data:
          message: >
            {% if trigger.entity_id == 'binary_sensor.grandmom_sensor_button1' %}
              \[SOS] 할머니댁 안방 - 연락주세요!
            {% elif trigger.entity_id == 'binary_sensor.grandmom_sensor_button2' %}
              \[SOS] 할머니댁 화장실 - 연락주세요!
            {% elif trigger.entity_id == 'binary_sensor.grandmom_sensor_button3' %}
              \[SOS] 할머니댁 거실 - 연락주세요!
            {% else %}
              GrandMom Sensor Error
            {% endif %}
      - service: shell_command.notify_sms
        data:
          number: '010********'
          message: >
            {% if trigger.entity_id == 'binary_sensor.grandmom_sensor_button1' %}
              [SOS] 할머니댁 안방 - 연락주세요!
            {% elif trigger.entity_id == 'binary_sensor.grandmom_sensor_button2' %}
              [SOS] 할머니댁 화장실 - 연락주세요!
            {% elif trigger.entity_id == 'binary_sensor.grandmom_sensor_button3' %}
              [SOS] 할머니댁 거실 - 연락주세요!
            {% else %}
              GrandMom Sensor Error
            {% endif %}
```

# [ 완성품 ]
\- 다이소에서 3천원(?) 정도에 구매한 다용도 수납 케이스에 넣어보았습니다.<br />
\- 기본 RF안테나로 안방 - 화장실의 범위는 충분히 커버 가능했습니다.<br />

<이미지 7>

- - - 

친절하게 적으려고 하다보니, 내용이 많이 길어졌네요 ^^;; <br />
사실 시중에 판매되는 상용 제품을 구매하여 사용하면, 시간도 절약되고 안전성도 확보할 수 있을겁니다. <br />
하지만, 필요한 기능을 직접 하나하나 구현하다보니 공부도 되고 재미도 있고 좋네요 ^^ <br />


기록할 겸 메모해두었다가, 누군가에겐 도움이될 수 있겠다 싶어 올려봅니다 ! <br />
조언과 충고 감사한 마음으로 받습니다! <br />

**코딩 실력이 아직 많이 미숙하여 코드가 날라다닙니다..** <br />
소스코드에 대해 궁금하신 분은 댓글 남겨주시면 최대한 설명드리겠습니다 ! <br />
