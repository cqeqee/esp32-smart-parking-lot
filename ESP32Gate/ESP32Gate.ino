#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include "rtc_wdt.h"
#include <LiquidCrystal_I2C.h>
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

#include <BlynkSimpleEsp32.h>

#include <WiFi.h>
#include <WiFiClient.h>
char ssid[] = "";
char pass[] = "";

int senState0, senState1, senState2, senState3, camState;
int lastState0, lastState1, lastState2, lastState3;
int door1 = 0, door2 = 0, door1Opened = 0, door2Opened = 0;

BLYNK_WRITE(V0) {
  senState0 = param.asInt();
}

BLYNK_WRITE(V1) {
  senState1 = param.asInt();
}

BLYNK_WRITE(V2) {
  senState2 = param.asInt();
}

BLYNK_WRITE(V3) {
  senState3 = param.asInt();
}

BLYNK_WRITE(V4) {
  camState = param.asInt();
}

#define SS_PIN 5
#define RST_PIN 0
#define PIN_SERVOENTER 17
#define PIN_SERVOEXIT 16
#define PIN_SENIN 26
#define PIN_SENOUT 25

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

#define EEPROM_SIZE 512       // 128 cards max
#define MAX_CARDS 50          // Maximum number of stored cards
#define CARD_SIZE 4           // UID size in bytes
#define MASTER_CARD_INDEX 0   // EEPROM index for the Master Card

byte masterCardUID[4] = {0x43, 0xC5, 0x03, 0x14};
// Card Management Mode
bool manageMode = false;  // True if in add/remove mode

Servo servoEnter, servoExit;
BlynkTimer timer;

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void syncSensor() {
  Blynk.syncVirtual(V0);
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V3);
}

void lcdMode(int mode) {
  if (mode == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Slot1:");
    lcd.setCursor(9, 0);
    lcd.print("Slot2:");
    lcd.setCursor(0, 1);
    lcd.print("Slot3:");
    lcd.setCursor(9, 1);
    lcd.print("Slot4:");
    lcd.setCursor(6, 0);
    senState0 == 1 ? lcd.print("F") : lcd.print("E");

    lcd.setCursor(15, 0);
    senState1 == 1 ? lcd.print("F") : lcd.print("E");

    lcd.setCursor(6, 1);
    senState2 == 1 ? lcd.print("F") : lcd.print("E");

    lcd.setCursor(15, 1);
    senState3 == 1 ? lcd.print("F") : lcd.print("E");
  }
  else if (mode == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Development Mode");
  }
}

BLYNK_CONNECTED() {
  lcdMode(0);
  Blynk.syncAll();
  lastState0 = lastState1 = lastState2 = lastState3 = 0;
}

void TaskDoor1() {
  while (1) {
    if (door1) {
      servoEnter.write(90);
      vTaskDelay(5000);
      servoEnter.write(0);
      camState = 0;
      Blynk.virtualWrite(V4, 0);
      door1Opened = 0;
      door1 = 0;
    }
    vTaskDelay(10);
  }
}

void TaskDoor2() {
  while (1) {
    if (door2) {
      servoExit.write(80);
      vTaskDelay(5000);
      servoExit.write(0);
      door2Opened = 0;
      door2 = 0;
    }
    vTaskDelay(10);
  }
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Smart  Parking");

  EEPROM.begin(EEPROM_SIZE);
  rfid.PCD_Init();

  servoEnter.setPeriodHertz(50);
  servoEnter.attach(PIN_SERVOENTER, 500, 2400);
  servoExit.setPeriodHertz(50);
  servoExit.attach(PIN_SERVOEXIT, 500, 2400);
  servoEnter.write(0);
  servoExit.write(0);
  pinMode(PIN_SENIN, INPUT);
  pinMode(PIN_SENOUT, INPUT);

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans multiple RFID cards."));
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, syncSensor);

  xTaskCreate((TaskFunction_t)&TaskDoor1, "Task1", 4000, NULL, 1, NULL);
  xTaskCreate((TaskFunction_t)&TaskDoor2, "Task2", 4000, NULL, 1, NULL);
  xTaskCreate((TaskFunction_t)&TaskMain, "Task3", 10000, NULL, 1, NULL);
}

void loop() {
}

bool readCard() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return false;
  }
  if (!rfid.PICC_ReadCardSerial()) {
    return false;
  }

  // Copy UID to buffer
  byte *uid = rfid.uid.uidByte;
  if (isMasterCard(uid)) {
    manageMode = !manageMode;  // Toggle management mode
    Serial.println(manageMode ? "Entered management mode." : "Exited management mode.");
    manageMode ? lcdMode(1) : lcdMode(0);
    delay(1000);
    return false;
  }

  if (manageMode) {
    // In management mode, add or remove the card
    if (isCardStored(uid)) {
      removeCard(uid);
    } else {
      addCard(uid);
    }
  } else {
    // In normal mode, check if the card is allowed
    if (isCardStored(uid)) {
      return true;
    } else {
      return false;
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return false;
}

void TaskMain() {
  while (1) {
    Blynk.run();
    timer.run();
    if ((readCard() || camState == 1) && !(senState0 & senState1 & senState2 & senState3) && !digitalRead(PIN_SENIN) && door1Opened == 0) {
      door1 = 1;
      door1Opened = 1;
    }

    if (!digitalRead(PIN_SENOUT) && door2Opened == 0) {
      door2 = 1;
      door2Opened = 1;
    }
    if (manageMode == false) {
      if (senState0 != lastState0) {
        lcd.setCursor(6, 0);
        senState0 == 1 ? lcd.print("F") : lcd.print("E");
        lastState0 = senState0;
      }

      if (senState1 != lastState1) {
        lcd.setCursor(15, 0);
        senState1 == 1 ? lcd.print("F") : lcd.print("E");
        lastState1 = senState1;
      }

      if (senState2 != lastState2) {
        lcd.setCursor(6, 1);
        senState2 == 1 ? lcd.print("F") : lcd.print("E");
        lastState2 = senState2;
      }

      if (senState3 != lastState3) {
        lcd.setCursor(15, 1);
        senState3 == 1 ? lcd.print("F") : lcd.print("E");
        lastState3 = senState3;
      }
    }
    vTaskDelay(10);
  }
}
bool isCardStored(byte *uid) {
  for (int i = 1; i <= MAX_CARDS; i++) { // Skip index 0 (Master Card)
    bool match = true;
    for (int j = 0; j < CARD_SIZE; j++) {
      if (EEPROM.read(i * CARD_SIZE + j) != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

// Function to add a new card to EEPROM
bool addCard(byte *uid) {
  for (int i = 1; i <= MAX_CARDS; i++) {
    bool emptySlot = true;
    for (int j = 0; j < CARD_SIZE; j++) {
      if (EEPROM.read(i * CARD_SIZE + j) != 0xFF) { // Check for empty slot
        emptySlot = false;
        break;
      }
    }
    if (emptySlot) {
      for (int j = 0; j < CARD_SIZE; j++) {
        EEPROM.write(i * CARD_SIZE + j, uid[j]);
      }
      EEPROM.commit();
      Serial.println(F("Card added successfully!"));
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Card Added");
      return true;
    }
  }
  Serial.println(F("No space to add a new card."));
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Not enough Space");
  return false;
}

// Function to remove a card from EEPROM
bool removeCard(byte *uid) {
  for (int i = 1; i <= MAX_CARDS; i++) {
    bool match = true;
    for (int j = 0; j < CARD_SIZE; j++) {
      if (EEPROM.read(i * CARD_SIZE + j) != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      for (int j = 0; j < CARD_SIZE; j++) {
        EEPROM.write(i * CARD_SIZE + j, 0xFF); // Clear slot
      }
      EEPROM.commit();
      Serial.println(F("Card removed successfully!"));
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Card Removed");
      return true;
    }
  }
  Serial.println(F("Card not found."));
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Card not found");
  return false;
}

// Function to check if the scanned card is the Master Card
bool isMasterCard(byte *uid) {
  for (int i = 0; i < CARD_SIZE; i++) {
    if (uid[i] != masterCardUID[i]) {
      return false;
    }
  }
  return true;
}
