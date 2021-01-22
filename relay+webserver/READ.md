### [ 스토리 ]
회사 기숙사에 살고있는 친구가 갑자기 하소연을 합니다.   
" 아.. 아침에 자동으로 전등 탁! 켜졌으면 좋겠다.. "   
\- 겨울이라 아침에 일어나기 힘들다며, 정해진 시간에 자동으로 불이 켜졌으면 좋겠다고 합니다.   

### [ 조건 ]
1. 기숙사에 거주하고 있으므로, 퇴거시 기존 모습 그대로 복원을 해놓아야 한다.
2. 최대한 돈이 적게 들었으면 좋겠다.

### [ 환경 ]
1. 스마트폰을 제외한 IoT 제품 일체 없음
2. 오래된 건물로써, <b>전등 스위치로 중성선이 내려있지 않음 (2선식)</b>
3. 다행(?)히 WiFi를 위한 공유기는 설치된 상태
4. 다행(?)히 갤럭시 스마트폰을 사용중이므로, 간단하게 빅스비 루틴 사용 가능
5. 혼자 살기 때문에, 주렁주렁 외관은 아무런 상관 없다.

중성선이 없으므로 이너 릴레이도 사용하기 어렵습니다.   
방을 최대한 건들지 않기 위해 고민을 하다, 굴러다니는 아두이노같은 개발보드를 사용해보기로 합니다.   
__(개발보드는 별도로 전원을 공급해주면 되기에, 중성선이 없어도 충분할것으로 보입니다.)__

- - - 

### [ 구성품 ]
* WeMos D1 (ESP 8266) : 서랍에 1년 넘게 방치된 녀석이 떠올랐습니다.
* 1채널 릴레이(Relay) 모듈 : 아두이노 셋트에 박혀있던 녀석을 꺼냅니다.
* 그 외 점퍼 케이블, 5pin 케이블 등
* *저의 경우, 모두 방치된 부품들을 활용하다보니 제작 비용이 0원으로 만들었습니다.*

### [ 기능 ]
<이미지1>

1. 휴대폰으로 수동 형광등 ON / OFF 가능   
-> 빅스비 루틴, Tasker 어플을 이용한 자동화 구현 가능
2. ESP 8266 보드 자체 Timer를 이용한 Alarm 구현   
-> 월~금요일만 동작, 정해진 시간에 자동으로 형광등 ON / OFF 가능
3. 휴대폰으로 Alarm Enable / Disable 가능   
(Config 값이므로, 보드 Reboot 시에도 설정값 유지 필요)   
-> 공휴일, 휴가 등으로 Alarm이 불필요할 경우 Disable 설정 필요

### [ 소프트웨어 구현(中 주요 소스코드) ]
1. 형광등 ON / OFF, Alarm Enable / Disable 등 사용자의 Control을 위한 UI 구현   
-> ESP8266 WebServer 이용
```
#ifndef STASSID
#define STASSID "와이파이 SSID(이름)을 넣으세요."
#define STAPSK  "와이파이 비밀번호를 넣으세요."
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80); // 사용하실 포트 번호를 넣으세요.
```

2. NTP서버로 부터 현재 시간 동기화   
-> configTime 함수 활용
```
configTime(3600*9, 0, "maths.kaist.ac.kr", "time.bora.net")
```

3. Alarm Enable / Disable 설정값을 비휘발성 메모리인 EEPROM에 저장   
```
  isAlarm ? EEPROM.write(1, 1) : EEPROM.write(1, 0); // EEPROM 1번지에 Alram 설정값 저장. true:1, false:0
  EEPROM.commit();
```

4. delay 사용 시, 보드가 일시적으로 멈추게 되므로 millis 함수 사용
```
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    time_t now = time(nullptr); 
    check_time(now);     
```

5. 정해진 시간에 Alarm 작동
```
void check_time(time_t now) {
  timeinfo = localtime(&now);
  // 초기값, 토요일, 일요일 예외처리
  if(timeinfo->tm_year+1900 < 2021 or timeinfo->tm_wday == 0 or timeinfo->tm_wday == 6 or isAlarm == false) return; 
  // 월~금 오전 7시 30분에 자동 점등
  if(timeinfo->tm_hour == 7 and timeinfo->tm_min == 30 and timeinfo->tm_sec == 0) {
    L1Status=(const __FlashStringHelper *)BTN1_ON;   
    digitalWrite(led1, HIGH);         
  }
}
```

### [ 아두이노 IDE 셋팅 ]
<그림 2>


### [ 하드웨어 구현 ]
1. 벽 스위치와 릴레이 모듈을 병렬로 연결   
* 장점:
  - 벽스위치 및 릴레이 모두 사용 가능
  - 기존 구성을 최대한 건들지 않음
* 단점:
  - 스위치와 릴레이 중 하나라도 ON 일 경우: ON으로 작동
  - 예) 스위치ON, 릴레이OFF : ON   
    스위치OFF, 릴레이ON : ON
 - 즉, 모두 OFF일 경우에만 최종 OFF

<이미지3>   
*사진 출처 및 참고한 글: 렌비님의 인라인 스위치 관련 글 (https://cafe.naver.com/stsmarthome/29884)*   
   
2. 릴레이 모듈 연결
- 흔히 아두이노와 같은 개발보드에 사용하는 릴레이 모듈은 다음과 같은 입출력 단자로 구성됩니다.

<이미지4>
   
* 왼쪽
  - 제어 신호(SIG): 스위치 ON / OFF 제어를 위해 보드의 GPIO 핀에 연결합니다.
  - 입력전압(VCC): 릴레이모듈 작동을 위해 보드의 5V 전압과 연결합니다.
  - 입력전압(VCC): 릴레이모듈 작동을 위해 보드의 GND와 연결합니다.
* 오른쪽
  - NC: 평상시 항상 닫혀 있는(켜져) 있는 경우에 연결합니다. <이 경우 전원을 인가하면, 스위치가 열리게 됩니다>
  - COM: 공통 단자로써, 항상 연결되어 있는 단자를 의미합니다.
  - NO: 평상시 항상 열려 있는(꺼져) 있는 경우에 연결합니다. <이 경우 전원을 인가하면, 스위치가 닫히게 됩니다>  
**평소에는 불이 꺼져있다가 필요시에만 불이 켜지게 할 예정이므로, 아래 그림과 같이 NO를 사용합니다.**   
**(아래 그림은 예시일뿐입니다. 병렬 연결이 아닙니다 !)**   

<이미지5>   
   
* 실제 연결하면 다음과 같은 모습이 나옵니다.
  - (18650 리튬 이온 전지를 이용한 테스트 중)

<이미지6>   

### [ 웹페이지 구현 ]
1. 최대한 심플하게 구현하였습니다.

<이미지6>   

2. 파란색 / 노란색 버튼을 클릭함으로써 릴레이 스위치 컨트롤 가능
3. Auto Alarm의 Enable / Disable을 클릭함으로써 알람 설정값 변경 가능
4. GET /ON1, GET /OFF1으로 구현하여 삼성 루틴 기능으로 가볍게 ON / OFF 가능


### [ 시현 영상 ]
1. Web을 이용한 릴레이 제어

<동영상1>   

2. 릴레이를 통한 LED 제어 (건전지 저전압으로 인해, LED 불빛이 약하네요)

<동영상2>   

- - - 

기록할 겸 작성했다가, 누군가에겐 도움이될 수 있겠다 싶어 올려봅니다 !   
조언과 충고는 감사한 마음으로 받지만, 비난은 정중히 죄송합니다 ㅠㅠ..   
   
코딩 실력이 아직 많이 미숙하여 코드가 날라다닙니다..   
소스코드에 대해 궁금하신 분은 댓글 남겨주시면 최대한 설명드리겠습니다 !   
