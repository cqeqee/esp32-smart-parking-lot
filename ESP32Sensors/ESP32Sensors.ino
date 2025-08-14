#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <BlynkSimpleEsp32.h>

#include <WiFi.h>
#include <WiFiClient.h>
char ssid[] = "";
char pass[] = "";

#define sensor0 23
#define sensor1 22
#define sensor2 21
#define sensor3 19
#define sensor5 18
#define sensor6 17
#define sensor7 16
#define sensor8 15

int state0, state1, state2, state3, state5, state6, state7, state8;
int vState0, vState1, vState2, vState3;
int lastvState0, lastvState1, lastvState2, lastvState3;

BLYNK_WRITE(V0) {
  lastvState0 = param.asInt();
}

BLYNK_WRITE(V1) {
  lastvState1 = param.asInt();
}

BLYNK_WRITE(V2) {
  lastvState2 = param.asInt();
}

BLYNK_WRITE(V3) {
  lastvState3 = param.asInt();
}

void setupSensor() {
  vState0, vState1, vState2, vState3 = 0;
  Blynk.syncVirtual(V0);
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V3);
}

void sendSensor() {
  state0 = digitalRead(sensor0);
  state1 = digitalRead(sensor1);
  state2 = digitalRead(sensor2);
  state3 = digitalRead(sensor3);
  state5 = digitalRead(sensor5);
  state6 = digitalRead(sensor6);
  state7 = digitalRead(sensor7);
  state8 = digitalRead(sensor8);

  if (!state0 & !state1)
    vState0 = 1;
  else
    vState0 = 0;
  if (vState0 != lastvState0) {
    Blynk.virtualWrite(V0, vState0);
    lastvState0 = vState0;
    if (vState0)
      Serial.println("Slot 1 full");
    else
      Serial.println("Slot 1 trong");
  }

  if (!state2 & !state3)
    vState1 = 1;
  else
    vState1 = 0;
  if (vState1 != lastvState1) {
    Blynk.virtualWrite(V1, vState1);
    lastvState1 = vState1;
    if (vState1)
      Serial.println("Slot 2 full");
    else
      Serial.println("Slot 2 trong");
  }

  if (!state5 & !state6)
    vState2 = 1;
  else
    vState2 = 0;
  if (vState2 != lastvState2) {
    Blynk.virtualWrite(V2, vState2);
    lastvState2 = vState2;
    if (vState2)
      Serial.println("Slot 3 full");
    else
      Serial.println("Slot 3 trong");
  }

  if (!state7 & !state8)
    vState3 = 1;
  else
    vState3 = 0;
  if (vState3 != lastvState3) {
    Blynk.virtualWrite(V3, vState3);
    lastvState3 = vState3;
    if (vState3)
      Serial.println("Slot 4 full");
    else
      Serial.println("Slot 4 trong");
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(sensor0, INPUT);
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor5, INPUT);
  pinMode(sensor6, INPUT);
  pinMode(sensor7, INPUT);
  pinMode(sensor8, INPUT);
  setupSensor();

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {

  Blynk.run();
  sendSensor();
}
